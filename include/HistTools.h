#include "TH1D.h"
#include "TH2D.h"

#include "BuildFitInput.h"

struct UserCutInfo {
    std::string name;                 // name of the user cut
    std::string expr;                 // expanded expression (from allUserCuts)
    std::vector<std::string> columns; // derived columns required by the cut
    bool found = false;               // found in allUserCuts?
    bool applied = false;             // did validation apply this cut (so fill pass should replay it)?
};

struct HistFilterPlan {
    std::vector<std::string> baseFilters;     // expanded cuts, lepCuts, predefCuts
    std::vector<UserCutInfo> userCuts;        // metadata for each requested user cut (in order)
    std::vector<UserCutInfo> appliedUserCuts; // subset (in order) of userCuts that were applied during validation
};

struct HistDef {
    std::string name, expr, yexpr, type, x_title, y_title;
    int nbins=0, nybins=0;
    double xmin=0, xmax=0, ymin=0, ymax=0;
    std::vector<std::string> cuts, lepCuts, predefCuts, userCuts;
};

// Helper to split strings by ';' or ','
static std::vector<std::string> splitTopLevel(const std::string& s) {
    auto trim = [](std::string str) -> std::string {
        const char* ws = " \t\n\r\f\v";
        size_t start = str.find_first_not_of(ws);
        if (start == std::string::npos) return "";
        size_t end = str.find_last_not_of(ws);
        return str.substr(start, end - start + 1);
    };
    std::vector<std::string> elems;
    if (s.empty()) return elems;
    char delim = (s.find(';') != std::string::npos) ? ';' : ',';
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        std::string t = trim(item);
        if (!t.empty()) elems.push_back(t);
    }
    return elems;
}

// Parse YAML histograms
static std::vector<HistDef> loadHistogramsYAML(const std::string &yamlPath, BuildFitInput* BFI) {
    std::vector<HistDef> hists;
    YAML::Node root = YAML::LoadFile(yamlPath);
    for (const auto &hnode : root["histograms"]) {
        HistDef h;
        h.name = hnode["name"].as<std::string>();
        h.expr = hnode["expr"].as<std::string>();
        h.type = hnode["type"].as<std::string>();
        h.nbins = hnode["nbins"].as<int>();
        h.xmin = hnode["xmin"].as<double>();
        h.xmax = hnode["xmax"].as<double>();
        h.x_title = hnode["x_title"].as<std::string>();
        if (h.type == "2D") {
            h.y_title = hnode["y_title"].as<std::string>();
            h.yexpr = hnode["yexpr"].as<std::string>();
            h.nybins = hnode["nybins"].as<int>();
            h.ymin = hnode["ymin"].as<double>();
            h.ymax = hnode["ymax"].as<double>();
        }
        if (hnode["cuts"]) h.cuts = splitTopLevel(hnode["cuts"].as<std::string>());
        if (hnode["lep-cuts"]) {
            auto tmp = splitTopLevel(hnode["lep-cuts"].as<std::string>());
            for (auto &lc : tmp) { std::string b = BFI->BuildLeptonCut(lc); if (!b.empty()) h.lepCuts.push_back(b); }
        }
        if (hnode["predefined-cuts"]) {
            auto tmp = splitTopLevel(hnode["predefined-cuts"].as<std::string>());
            for (const auto &pcut : tmp) {
                std::string cut;
                if (BFI->GetCutByName(pcut, cut))
                  h.predefCuts.push_back(cut);
                else
                  std::cerr << "[BFI_condor] Unknown predefined cut: " << pcut << "\n";
            }
        }
        if (hnode["user-cuts"]) h.userCuts = splitTopLevel(hnode["user-cuts"].as<std::string>());
        hists.push_back(h);
    }
    return hists;
}

// Build the initial plan: expand macros for base filters and record user-cut metadata (no application).
HistFilterPlan BuildHistFilterPlan(const HistDef &h,
                                   BuildFitInput *BFI,
                                   const std::map<std::string, CutDef> &allUserCuts) {
    HistFilterPlan plan;
    // base filters
    for (const auto &c : h.cuts)       plan.baseFilters.push_back(BFI->ExpandMacros(c));
    for (const auto &c : h.lepCuts)    plan.baseFilters.push_back(BFI->ExpandMacros(c));
    for (const auto &c : h.predefCuts) plan.baseFilters.push_back(BFI->ExpandMacros(c));

    // user-cuts metadata
    for (const auto &cname : h.userCuts) {
        UserCutInfo uci;
        uci.name = cname;
        auto it = allUserCuts.find(cname);
        if (it == allUserCuts.end()) {
            uci.found = false;
            std::cerr << "[BFI_condor] WARNING: User cut '" << cname
                      << "' not found when building hist plan for '" << h.name << "'\n";
        } else {
            uci.found = true;
            uci.expr = BFI->ExpandMacros(it->second.expression);
            uci.columns = it->second.columns; // assume Cut has member 'columns'
        }
        plan.userCuts.push_back(std::move(uci));
    }

    return plan;
}

// Fill a histogram from a validated plan. Assumes MT is enabled for speed.
// Recreates node under MT-on context, applies base filters + plan.appliedUserCuts, and fills.
void FillHistFromPlan(const ROOT::RDF::RNode &node,
                      const HistFilterPlan &plan,
                      const HistDef &h,
                      const std::string &hname) {
    ROOT::RDF::RNode hnode = node; // create fresh node (inherits current MT state)
    for (const auto &f : plan.baseFilters) hnode = hnode.Filter(f);
    for (const auto &uci : plan.appliedUserCuts) hnode = hnode.Filter(uci.expr);

    if (h.type == "1D") {
        auto hist = hnode.Histo1D({hname.c_str(), hname.c_str(), h.nbins, h.xmin, h.xmax}, h.expr, "weight_scaled");
        hist->GetXaxis()->SetTitle(h.x_title.c_str());
        hist->GetYaxis()->SetTitle(h.y_title.empty() ? "Events" : h.y_title.c_str());
        if (hist->Write() == 0) std::cerr << "error writing: " << hname << std::endl;
    } else if (h.type == "2D") {
        auto hist = hnode.Histo2D({hname.c_str(), hname.c_str(), h.nbins, h.xmin, h.xmax, h.nybins, h.ymin, h.ymax},
                                  h.expr, h.yexpr, "weight_scaled");
        hist->GetXaxis()->SetTitle(h.x_title.c_str());
        hist->GetYaxis()->SetTitle(h.y_title.empty() ? "Events" : h.y_title.c_str());
        if (hist->Write() == 0) std::cerr << "error writing: " << hname << std::endl;
    }
}

static bool ValidateDerivedVarNode(ROOT::RDF::RNode node, const DerivedVar &dv, unsigned nCheck = 50) {
    ROOT::RDF::RNode tmp_node = node;
    bool ok = ValidateDerivedVar(tmp_node, dv, nCheck);
    return ok;
}

// Validate & apply user-cuts for a single histogram plan.
// - hnode should be created from `node` before calling this.
// - On success, plan.appliedUserCuts will contain the sequence of UserCutInfo that were applied (in order).
// - Returns true if histogram should be kept (valid), false to skip it.
bool ValidateAndRecordAppliedUserCuts(ROOT::RDF::RNode hnode,
                                     HistFilterPlan &plan,
                                     const HistDef &h,
                                     BuildFitInput *BFI) {
    if (plan.userCuts.empty()) return true;
    // apply base filters first
    for (const auto &f : plan.baseFilters) hnode = hnode.Filter(f);

    // iterate user-cuts in order; validate columns for each before applying
    for (auto &uci : plan.userCuts) {
        if (!uci.found) {
            // missing user cut skip histogram
            return false;
        }

        bool ok = true;
        for (const auto &col : uci.columns) {
            DerivedVar dv;
            dv.name = col;
            dv.expr = col; // we assume derived var is already defined on the RNode context
            if (!ValidateDerivedVarNode(hnode, dv)) {
                std::cerr << "[BFI_condor] WARNING: For histogram '" << h.name
                          << "' user cut '" << uci.name
                          << "' failed validation for derived variable '" << col << "'\n";
                ok = false;
                break;
            }
        }
        if (!ok) return false; // if a required column for this user-cut fails -> drop the histogram
        // apply the validated user-cut and record that we applied it
        hnode = hnode.Filter(uci.expr);
        uci.applied = true;
        plan.appliedUserCuts.push_back(uci);
    }

    // After user-cuts applied, validate axis derived variables on final hnode
    if (h.type == "1D") {
        DerivedVar dvx{h.expr, h.expr};
        if (!ValidateDerivedVarNode(hnode, dvx)) {
            std::cerr << "[BFI_condor] WARNING: Skipping 1D histogram '" << h.name
                      << "' due to invalid axis expression '" << h.expr << "'\n";
            return false;
        }
    } else if (h.type == "2D") {
        DerivedVar dvx{h.expr, h.expr}, dvy{h.yexpr, h.yexpr};
        if (!ValidateDerivedVarNode(hnode, dvx) || !ValidateDerivedVarNode(hnode, dvy)) {
            std::cerr << "[BFI_condor] WARNING: Skipping 2D histogram '" << h.name
                      << "' due to invalid axis expressions.\n";
            return false;
        }
    }

    return true;
}
