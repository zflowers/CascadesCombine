// src/BFI_condor.cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <string>
#include <getopt.h>
#include <cmath>
#include <memory>
#include <filesystem>
#include "TFile.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TROOT.h"
#include "TLorentzVector.h"
#include "yaml-cpp/yaml.h"
#include "BuildFitInput.h"
#include "BuildFitTools.h"
#include "SampleTool.h"
namespace fs = std::filesystem;

// ----------------------
// Helpers
// ----------------------

static void usage(const char* me) {
    std::cerr << "Usage: " << me
              << " --bin BINNAME --file ROOTFILE [--json-output OUT.json] "
                 "[--root-output OUT.root] [--cuts CUT1;CUT2;...] [--lep-cuts LEPCUT1;LEPCUT2;...] "
                 "[--predefined-cuts NAME1;NAME2;...] [--user-cuts NAME1;NAME2;...] [--hist] [--hist-yaml HISTS.yaml] [--json]\n\n";
    std::cerr << "Required arguments:\n";
    std::cerr << "  --bin           Name of the bin to process (e.g. TEST)\n";
    std::cerr << "  --file          Path to one ROOT file to process\n\n";
    std::cerr << "Optional arguments:\n";
    std::cerr << "  --json-output      Path to write partial JSON output\n";
    std::cerr << "  --root-output      Path to write ROOT/histogram output\n";
    std::cerr << "  --cuts             Semicolon-separated list of normal tree cuts "
                 "(e.g. MET>=150;PTISR>=250)\n";
    std::cerr << "  --lep-cuts         Semicolon-separated list of lepton cuts for BuildLeptonCut\n";
    std::cerr << "  --predefined-cuts  Semicolon-separated list of predefined cuts\n";
    std::cerr << "  --user-cuts        Semicolon-separated list of user cuts\n";
    std::cerr << "  --hist             Fill histograms\n";
    std::cerr << "  --hist-yaml        YAML file defining histogram expressions\n";
    std::cerr << "  --json             Write JSON yields\n";
    std::cerr << "  --signal           Mark this dataset as signal\n";
    std::cerr << "  --sig-type TYPE    Specify signal type (sets --signal automatically)\n";
    std::cerr << "  --lumi VALUE       Integrated luminosity to scale yields\n";
    std::cerr << "  --sample-name NAME Optional name of the sample\n";
    std::cerr << "  --sms-filters LIST Comma-separated list of SMS filters\n";
    std::cerr << "  --help             Display this help message\n";
}

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

// Split strings by ';' or ',' (for backward compatibility)
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

inline std::string GetSampleNameFromKey(const std::string& keyOrPath) {
    size_t lastSlash = keyOrPath.find_last_of("/\\");
    std::string name = (lastSlash == std::string::npos) ? keyOrPath : keyOrPath.substr(lastSlash + 1);
    size_t dot = name.rfind(".root");
    if (dot != std::string::npos) name = name.substr(0, dot);
    return name;
}

inline std::string GetProcessNameFromKey(const std::string& keyOrPath) {
    SampleTool ST;
    ST.LoadAllFromMaster();

    auto resolveGroup = [&ST](const std::string &Key) -> std::string {
        std::string keyBase = fs::path(Key).filename().string();    
        for (const auto &kv : ST.MasterDict) {       // kv.first = canonical group
            for (const auto &entry : kv.second) {    // entry = full path
                std::string entryBase = fs::path(entry).filename().string();
                // Use starts-with match instead of find anywhere
                if (keyBase.rfind(entryBase.substr(0, entryBase.find("_")), 0) == 0)
                    return kv.first;
            }
        }
        return keyBase; // fallback
    };

    return resolveGroup(keyOrPath);
}

static bool buildCutsForBin(BuildFitInput* BFI,
                            const std::vector<std::string>& normalCuts,
                            const std::vector<std::string>& lepCuts,
                            const std::vector<std::string>& predefinedCuts,
                            std::vector<std::string>& outCuts) 
{
    if (!BFI) return false;
    outCuts.clear();
    outCuts = normalCuts;
    for (const auto &lepCut : lepCuts) {
        std::string builtCut = BFI->BuildLeptonCut(lepCut);
        if (!builtCut.empty()) outCuts.push_back(builtCut);
    }
    for (const auto &pcut : predefinedCuts) {
        std::string cut;
        if (BFI->GetCutByName(pcut, cut))
          outCuts.push_back(cut);
        else
          std::cerr << "[BFI_condor] Unknown predefined cut: " << pcut << "\n";
    }
    return true;
}

static bool writePartialJSON(const std::string& outPath,
                             const std::string& binname,
                             const std::map<std::string, std::map<std::string, std::array<double,3>>>& fileResults,
                             const std::map<std::string, std::array<double,3>>& totals) 
{
    std::ofstream ofs(outPath);
    if (!ofs) return false;
    ofs << "{\n  \"" << binname << "\": {\n";
    bool firstSample = true;
    for (const auto &kv : totals) {
        if (!firstSample) ofs << ",\n";
        firstSample = false;
        const std::string &sname = kv.first;
        std::string sampleId = GetSampleNameFromKey(sname); 
        const auto &totalVals = kv.second;
        ofs << "    \"" << sampleId << "\": {\n      \"files\": {\n";
        bool firstFile = true;
        auto itFiles = fileResults.find(sname);
        if (itFiles != fileResults.end()) {
            for (const auto &fkv : itFiles->second) {
                if (!firstFile) ofs << ",\n";
                firstFile = false;
                ofs << "        \"" << fkv.first << "\": ["
                    << (long long)fkv.second[0] << ", "
                    << fkv.second[1] << ", "
                    << fkv.second[2] << "]";
            }
        }
        ofs << "\n      },\n";
        ofs << "      \"totals\": ["
            << (long long)totalVals[0] << ", "
            << totalVals[1] << ", "
            << totalVals[2] << "]\n";
        ofs << "    }";
    }
    ofs << "\n  }\n}\n";
    ofs.close();
    return true;
}

// Parse YAML histograms
struct HistDef {
    std::string name, expr, yexpr, type;
    int nbins=0, nybins=0;
    double xmin=0, xmax=0, ymin=0, ymax=0;
    std::vector<std::string> cuts, lepCuts, predefCuts, userCuts;
};

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
        if (h.type == "2D") {
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
        if (hnode["user-cuts"]) h.cuts = splitTopLevel(hnode["user-cuts"].as<std::string>());
        hists.push_back(h);
    }
    return hists;
}

// Match your existing HistDef type (same fields used by YAML loader)
static std::vector<HistDef> loadHistogramsUser(ROOT::RDF::RNode &node) {
    std::vector<HistDef> hdefs;
    // ---------------------------------------------------------------------
    // Step 1: Build TLorentzVectors for the leading leptons from vector branches
    // ---------------------------------------------------------------------
    // We assume the tree stores lepton kinematics as vectors:
    //   vector<double>  *PT_lep, *Eta_lep, *Phi_lep, *M_lep
    //
    // For safety we check vector sizes before accessing indices.
    // My_p4_lep0 and My_p4_lep1 are TLorentzVector objects for the leading and
    // sub-leading leptons respectively (by ordering already present in the trees).
    node = node
        .Define("My_p4_lep0", [](const std::vector<double> &pt,
                              const std::vector<double> &eta,
                              const std::vector<double> &phi,
                              const std::vector<double> &mass) {
                // Construct TLV for lepton 0 if present; otherwise returns a zero TLV.
                TLorentzVector v;
                if (!pt.empty() && pt.size() == eta.size() && eta.size() == phi.size() && phi.size() == mass.size()) {
                    // safe access to [0]
                    v.SetPtEtaPhiM(pt[0], eta[0], phi[0], mass[0]);
                }
                return v;
            }, {"PT_lep","Eta_lep","Phi_lep","M_lep"})
        .Define("My_p4_lep1", [](const std::vector<double> &pt,
                              const std::vector<double> &eta,
                              const std::vector<double> &phi,
                              const std::vector<double> &mass) {
                // Construct TLV for lepton 1 if present; otherwise returns a zero TLV.
                TLorentzVector v;
                if (pt.size() > 1 && pt.size() == eta.size() && eta.size() == phi.size() && phi.size() == mass.size()) {
                    // safe access to [1]
                    v.SetPtEtaPhiM(pt[1], eta[1], phi[1], mass[1]);
                }
                return v;
            }, {"PT_lep","Eta_lep","Phi_lep","M_lep"});

    // ---------------------------------------------------------------------
    // Step 2: Calculate invariant mass of the leading two leptons (M_ll)
    // ---------------------------------------------------------------------
    // This defines a column "M_ll" computed from the two TLVs. If either TLV
    // is zero (insufficient leptons) the TLV addition yields a zero mass (0).
    node = node.Define("M_ll", [](const TLorentzVector &l0, const TLorentzVector &l1){
                return (l0 + l1).M();
            }, {"My_p4_lep0","My_p4_lep1"});

    // ---------------------------------------------------------------------
    // Step 3: Extract charges and flavors for the first two leptons (convenience)
    // ---------------------------------------------------------------------
    // These are convenience columns for users who want to filter on charge/flavor.
    // They are defensive: if the vector doesn't have enough entries we return 0.
    node = node
        .Define("Q_lep0", [](const std::vector<int> &charge){
                return charge.empty() ? 0 : charge[0];
            }, {"Charge_lep"})
        .Define("Q_lep1", [](const std::vector<int> &charge){
                return (charge.size() > 1) ? charge[1] : 0;
            }, {"Charge_lep"})
        // Flavor encoding function: 1 = electron, 2 = muon, 0 = other/unknown.
        // We keep flavors simple and explicit so it's straightforward to filter on.
        .Define("F_lep0", [](const std::vector<int> &pdgid){
                if (pdgid.empty()) return 0;
                int id = std::abs(pdgid[0]);
                return (id == 11 ? 1 : (id == 13 ? 2 : 0));
            }, {"PDGID_lep"})
        .Define("F_lep1", [](const std::vector<int> &pdgid){
                if (pdgid.size() < 2) return 0;
                int id = std::abs(pdgid[1]);
                return (id == 11 ? 1 : (id == 13 ? 2 : 0));
            }, {"PDGID_lep"});

    // ---------------------------------------------------------------------
    // Step 4: Define OSSF boolean robustly (explicitly check vector sizes)
    // ---------------------------------------------------------------------
    // IMPORTANT: instead of relying on the convenience Q/F columns above,
    // we compute OSSF_pair directly from the vectors to be *explicitly*
    // robust for events with <2 leptons.
    //
    // OSSF_pair = true when:
    //   - there are at least two leptons (vectors have size >= 2)
    //   - the first two leptons are opposite sign (q0 * q1 < 0)
    //   - the first two leptons have the same flavor (both electrons or both muons)
    node = node.Define("OSSF_pair",
            [](const std::vector<int> &charge, const std::vector<int> &pdgid) -> bool {
                // must have at least two leptons
                if (charge.size() < 2 || pdgid.size() < 2) return false;

                int q0 = charge[0], q1 = charge[1];
                int id0 = std::abs(pdgid[0]), id1 = std::abs(pdgid[1]);

                // require electron-electron or muon-muon
                bool sameFlavor = (id0 == 11 && id1 == 11) || (id0 == 13 && id1 == 13);
                bool oppositeSign = (q0 * q1 < 0);

                return (sameFlavor && oppositeSign);
            }, {"Charge_lep","PDGID_lep"});

    // ---------------------------------------------------------------------
    // Step 5: Define HT_eta24 / MET ratio safely
    // ---------------------------------------------------------------------
    // Add a derived column "HTeta24_over_MET". Protect against MET==0.
    node = node.Define("HTeta24_over_MET", [](double HT_eta24, double MET) {
                if (MET == 0.0) return 0.0; // avoid divide-by-zero; choose safe default
                return HT_eta24 / MET;
            }, {"HT_eta24","MET"});

    // ---------------------------------------------------------------------
    // Step 6: Build HistDefs to return (do NOT fill/write here)
    // ---------------------------------------------------------------------
    // 1) 1D histogram for M_ll but restricted to OSSF pairs. We express that
    //    restriction by adding "OSSF_pair" to the histogram-specific cuts;
    //    the main histogram loop will apply this cut as node.Filter(BFI->ExpandMacros(...)).
    HistDef h1;
    h1.name = "M_ll_lead2_OSSF";
    h1.type = "1D";
    h1.expr = "M_ll";          // histogram the M_ll column (computed above)
    h1.nbins = 50;
    h1.xmin = 0;
    h1.xmax = 200;
    // The main loop will interpret these strings as filters, so use the column name:
    // it will call node = node.Filter(BFI->ExpandMacros("OSSF_pair"));
    h1.cuts = {"OSSF_pair"};
    h1.lepCuts = {};
    h1.predefCuts = {};
    h1.userCuts = {};
    hdefs.push_back(h1);

    // 2) 2D histogram: M_ll (x) vs HTeta24_over_MET (y)
    //    This uses the derived ratio defined above; main loop will validate axes.
    HistDef h2;
    h2.name = "M_ll_lead2_vs_HTeta24overMET";
    h2.type = "2D";
    h2.expr = "M_ll";               // X-axis: invariant mass of leading two leptons
    h2.yexpr = "HTeta24_over_MET";   // Y-axis: derived HT/MET ratio
    h2.nbins = 50;
    h2.xmin = 0;
    h2.xmax = 500;
    h2.nybins = 50;
    h2.ymin = 0;
    h2.ymax = 5;
    h2.cuts = {};       // no extra event-level cuts here (hist loop may also apply global cuts)
    h2.lepCuts = {};
    h2.predefCuts = {};
    h2.userCuts = {};
    hdefs.push_back(h2);

    // Return the user-provided histogram definitions. The main loop will:
    //   - apply h.cuts / h.lepCuts / h.predefCuts (via BFI->ExpandMacros) / h.userCuts
    //   - perform derived-variable validation for 2D axes
    //   - call Histo1D/Histo2D to fill and write histograms
    return hdefs;
}

// ----------------------
// Derived variables
// ----------------------
struct DerivedVar {
    std::string name; // variable name to be created in the RDataFrame
    std::string expr; // expression to define it (ROOT/C++ expression)
};

static std::vector<DerivedVar> loadDerivedVariablesYAML(const std::string &yamlPath) {
    std::vector<DerivedVar> vars;
    YAML::Node root = YAML::LoadFile(yamlPath);
    if(!root["derived_variables"]) return vars;

    for(const auto &vnode : root["derived_variables"]) {
        DerivedVar dv;
        dv.name = vnode["name"].as<std::string>();
        dv.expr = vnode["expr"].as<std::string>();
        vars.push_back(dv);
    }
    return vars;
}

// --------------------------------------------------
// Helper to validate a single derived variable
// --------------------------------------------------
static bool ValidateDerivedVar(ROOT::RDF::RNode& node, const DerivedVar &dv,
                               unsigned nCheck = 50) {

    try {
        // Define a temporary column
        ROOT::RDF::RNode tmpNode = node.Define(dv.name + "_test", dv.expr);

        // Take first nCheck entries
        auto subset = tmpNode.Range(0, static_cast<long>(nCheck));

        // --- Try double path first ---
        try {
            auto valsD = subset.Take<double>(dv.name + "_test").GetValue();
            if (!valsD.empty()) {
                if (std::any_of(valsD.begin(), valsD.end(), [](double v){ return std::isfinite(v); })) {
                    return true; // Passes validation
                }
                std::cerr << "[BFI_condor] WARNING: Derived variable '" << dv.name
                          << "' produced no finite doubles in first " << nCheck << " entries.\n";
                return false;
            } else {
                std::cerr << "[BFI_condor] WARNING: Derived variable '" << dv.name
                          << "' produced empty double vector.\n";
                return false;
            }
        } catch (const std::exception &e) {
            std::string exmsg = e.what();

            // --- Try int path if double fails ---
            try {
                auto valsI = subset.Take<int>(dv.name + "_test").GetValue();
                if (!valsI.empty()) return true;
                std::cerr << "[BFI_condor] WARNING: Derived variable '" << dv.name
                          << "' evaluated as int but returned empty vector.\n";
                return false;
            } catch (const std::exception &e2) {
                std::cerr << "[BFI_condor] ERROR validating '" << dv.name << "'\n"
                          << "  double-path exception: " << exmsg << "\n"
                          << "  int-path exception:    " << e2.what() << "\n";
                if (dv.expr.find("/") != std::string::npos &&
                    dv.expr.find("SafeDiv") == std::string::npos) {
                    std::cerr << "  HINT: Expression contains '/', consider using SafeDiv(num, den, def)\n";
                }
                if (dv.expr.find("[") != std::string::npos &&
                    dv.expr.find("SafeIndex") == std::string::npos) {
                    std::cerr << "  HINT: Expression uses indexing '[]', consider using SafeIndex(vec, idx, defaultVal)\n";
                }
                return false;
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "[BFI_condor] WARNING: Exception validating '" << dv.name
                  << "': " << e.what() << "\n";
        return false;
    } catch (...) {
        std::cerr << "[BFI_condor] WARNING: Unknown exception validating '" << dv.name << "'\n";
        return false;
    }
}

static bool ValidateDerivedVarNode(const ROOT::RDF::RNode& node, const DerivedVar &dv, unsigned nCheck = 50) {
    ROOT::RDF::RNode tmp_node = node;
    bool ok = ValidateDerivedVar(tmp_node, dv, nCheck);
    return ok;
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
        if (!ok) return false;            // if a required column for this user-cut fails → drop the histogram
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
        if (hist->Write() == 0) std::cerr << "error writing: " << hname << std::endl;
    } else if (h.type == "2D") {
        auto hist = hnode.Histo2D({hname.c_str(), hname.c_str(), h.nbins, h.xmin, h.xmax, h.nybins, h.ymin, h.ymax},
                                  h.expr, h.yexpr, "weight_scaled");
        if (hist->Write() == 0) std::cerr << "error writing: " << hname << std::endl;
    }
}
// ----------------------
// Main
// ----------------------
int main(int argc, char** argv) {
    RegisterSafeHelpers();
    std::string binName, cutsStr, lepCutsStr, predefCutsStr, userCutsStr, rootFilePath, outputJsonPath, sampleName, histOutputPath;
    std::vector<std::string> smsFilters;
    bool isSignal=false, doHist=false, doJSON=false;
    std::string sigType, histYamlPath;
    double Lumi=1.0;

    static struct option long_options[] = {
        {"bin", required_argument, 0, 'b'},
        {"file", required_argument, 0, 'f'},
        {"json-output", required_argument, 0, 'o'},
        {"cuts", required_argument, 0, 'c'},
        {"lep-cuts", required_argument, 0, 'l'},
        {"predefined-cuts", required_argument, 0, 'p'},
        {"user-cuts", required_argument, 0, 'u'},
        {"signal", no_argument, 0, 's'},
        {"sig-type", required_argument, 0, 't'},
        {"lumi", required_argument, 0, 'L'},
        {"sample-name", required_argument, 0, 'n'},
        {"sms-filters", required_argument, 0, 'm'},
        {"hist", no_argument, 0, 'H'},
        {"hist-yaml", required_argument, 0, 'y'},
        {"json", no_argument, 0, 'J'},
        {"root-output", required_argument, 0, 'O'},
        {"help", no_argument, 0, 'h'},
        {0,0,0,0}
    };

    int opt, opt_index=0;
    while ((opt = getopt_long(argc, argv, "b:f:o:c:l:p:st:u:n:m:Hy:J", long_options, &opt_index)) != -1) {
        switch(opt){
            case 'b': binName=optarg; break;
            case 'f': rootFilePath=optarg; break;
            case 'o': outputJsonPath=optarg; break;
            case 'c': cutsStr=optarg; break;
            case 'l': lepCutsStr=optarg; break;
            case 'p': predefCutsStr=optarg; break;
            case 'u': userCutsStr=optarg; break;
            case 's': isSignal=true; break;
            case 't': sigType=optarg; isSignal=true; break;
            case 'L': Lumi=atof(optarg); break;
            case 'n': sampleName=optarg; break;
            case 'm': smsFilters=BFTool::SplitString(optarg,","); break;
            case 'H': doHist=true; break;
            case 'y': histYamlPath=optarg; break;
            case 'J': doJSON=true; break;
            case 'O': histOutputPath = optarg; break;
            case 'h':
            default: usage(argv[0]); return 1;
        }
    }

    if (sampleName.empty()) sampleName = GetSampleNameFromKey(rootFilePath);
    if (outputJsonPath.empty()) outputJsonPath = binName + "_" + sampleName + ".json";
    if (binName.empty() || rootFilePath.empty() || (!doHist && !doJSON)) { usage(argv[0]); return 1; }

    BuildFitInput* BFI=nullptr;
    try{BFI=new BuildFitInput();}catch(...){std::cerr<<"[BFI_condor] Failed to construct BuildFitInput\n";return 3;}

    std::vector<std::string> cutsVec=splitTopLevel(cutsStr);
    std::vector<std::string> lepCutsVec=splitTopLevel(lepCutsStr);
    std::vector<std::string> predefCutsVec=splitTopLevel(predefCutsStr);
    std::vector<std::string> userCutsVec=splitTopLevel(userCutsStr);
    std::vector<std::string> finalCuts;
    if(!buildCutsForBin(BFI,cutsVec,lepCutsVec,predefCutsVec,finalCuts)){std::cerr<<"[BFI_condor] Failed to build final cuts\n"; delete BFI; return 2;}
    std::vector<std::string> finalCutsExpanded;
    for(const auto &c : finalCuts) finalCutsExpanded.push_back(BFI->ExpandMacros(c));

    std::unique_ptr<TFile> histFile;
    if(doHist && !histOutputPath.empty()){
        histFile.reset(TFile::Open(histOutputPath.c_str(),"RECREATE"));
        if(!histFile || histFile->IsZombie()){
            std::cerr << "[BFI_condor] ERROR opening hist output file: " << histOutputPath << "\n";
            delete BFI;
            return 6;
        }
    }
    if(!smsFilters.empty()) BFTool::filterSignalsSMS=smsFilters;

    if(isSignal && sigType.empty())
        sigType=(rootFilePath.find("SMS")!=std::string::npos)?"sms":"cascades";

    std::map<std::string, std::map<std::string,std::array<double,3>>> fileResults;
    std::map<std::string,std::array<double,3>> totals;
    std::string processName = "";
    if(isSignal && sigType == "cascades")
        processName = BFTool::GetSignalTokensCascades(rootFilePath);
    else if(isSignal && sigType == "sms")
        processName = GetProcessNameFromKey(rootFilePath) + "_" + BFTool::GetFilterSignalsSMS()[0];
    else
        processName = GetProcessNameFromKey(rootFilePath);

    auto processTree=[&](const std::string &tree_name, const std::string &key){
        histFile->cd();
        ROOT::RDataFrame df(tree_name, rootFilePath);
    
        // Scale weights
        auto df_scaled = df.Define("weight_scaled",[Lumi](double w){return w*Lumi;},{"weight"})
                           .Define("weight_sq_scaled",[Lumi](double w){return w*w*Lumi*Lumi;},{"weight"});
    
        // Lepton counts / kinematics
        auto df_with_lep = BFI->DefineLeptonPairCounts(df_scaled,"");
        df_with_lep = BFI->DefineLeptonPairCounts(df_with_lep,"A");
        df_with_lep = BFI->DefineLeptonPairCounts(df_with_lep,"B");
        df_with_lep = BFI->DefinePairKinematics(df_with_lep,"");
        df_with_lep = BFI->DefinePairKinematics(df_with_lep,"A");
        df_with_lep = BFI->DefinePairKinematics(df_with_lep,"B");

        std::vector<DerivedVar> derivedVars;
        if(doHist && !histYamlPath.empty()){
            derivedVars = loadDerivedVariablesYAML(histYamlPath);
        }
    
        for (const auto &dv : derivedVars) {
            ValidateDerivedVarNode(df_with_lep, dv);
        }
    
        // --- Define derived variables after checking ---
        for(const auto &dv : derivedVars){
            try{
                df_with_lep = df_with_lep.Define(dv.name, dv.expr);
            }catch(const std::exception &e){
                std::cerr << "[BFI_condor] WARNING: Failed to define derived variable '"
                          << dv.name << "' Expression: " << dv.expr
                          << " Exception: " << e.what() << "\n";
            }
        }
    
        // --- Apply final event selection cuts ---
        ROOT::RDF::RNode node=df_with_lep;
        for(const auto &c: finalCutsExpanded) node=node.Filter(c);

        // --- Load all user-defined cuts ---
        auto allUserCuts = BuildFitInput::loadCutsUser(node);

        // --- Apply selected user cuts with validation ---
        for (const auto &cutName : userCutsVec) {
            auto it = allUserCuts.find(cutName);
            if (it != allUserCuts.end()) {
                const auto &cut = it->second;
                bool valid = true;
                // Validate any derived columns required by this cut
                for (const auto &col : cut.columns) {
                    DerivedVar dv;
                    dv.name = col;
                    dv.expr = col; // assumes the column is already defined in the RNode
                    if (!ValidateDerivedVarNode(node, dv)) {
                        std::cerr << "[BFI_condor] WARNING: Skipping user cut '" 
                                  << cutName << "' due to invalid derived variable '" 
                                  << col << "'\n";
                        valid = false;
                        break;
                    }
                }
                // Apply the cut if all derived variables are valid
                if (valid) {
                    node = node.Filter(BFI->ExpandMacros(cut.expression));
                }
            } else {
                std::cerr << "[BFI_condor] WARNING: User cut '" << cutName 
                          << "' not found in BuildFitInput::loadCutsUser\n";
            }
        }
    
        // --- Histograms ---
        if(doHist && !histYamlPath.empty()){
            // --- prepare histDefs as before ---
            std::cout << "Loading histogramsUser \n";
            auto userHists = loadHistogramsUser(node);
            std::cout << "Loaded histogramsUser \n";
            auto histDefs = loadHistogramsYAML(histYamlPath, BFI);
            histDefs.insert(histDefs.end(), userHists.begin(), userHists.end());
            
            size_t N = histDefs.size();
            std::vector<HistFilterPlan> plans(N);
            std::vector<char> keep(N, 0);
            
            // --- VALIDATION PASS (MT OFF) ---
            ROOT::EnableImplicitMT(0);
            
            for (size_t i = 0; i < N; ++i) {
                const auto &h = histDefs[i];
                plans[i] = BuildHistFilterPlan(h, BFI, allUserCuts);
            
                // create a hnode copy for validation context (node must have been created with MT OFF)
                ROOT::RDF::RNode hnode = node;
            
                bool ok = ValidateAndRecordAppliedUserCuts(hnode, plans[i], h, BFI);
                keep[i] = ok ? 1 : 0;
            }
            
            // --- FILL PASS (MT ON) ---
            ROOT::EnableImplicitMT(); // turn on multi-threading once
            
            for (size_t i = 0; i < N; ++i) {
                if (!keep[i]) continue;
                const auto &h = histDefs[i];
                std::string hname = binName + "__" + processName + "__" + h.name;
            
                // Use the recorded plan — appliedUserCuts were stored in validation
                FillHistFromPlan(node, plans[i], h, hname);
                std::cout << "[BFI_condor] Filled histogram: " << h.name << "\n";
            }
        }
    
        // --- JSON output ---
        if(doJSON){
	    ROOT::EnableImplicitMT();
            auto json_node = node;
            auto cnt = json_node.Count();
            auto sumW = json_node.Sum<double>("weight_scaled");
            auto sumW2 = json_node.Sum<double>("weight_sq_scaled");
            unsigned long long n_entries = cnt.GetValue();
            double sW = sumW.GetValue();
            double sW2Val = sumW2.GetValue();
            double err = (sW2Val>=0)?std::sqrt(sW2Val):0.0;
            fileResults[key][rootFilePath] = {(double)n_entries,sW,err};
            auto &tot=totals[key];
            tot[0]+= (double)n_entries;
            tot[1]+= sW;
            tot[2]+= err*err;
        }
    };

    if(!isSignal) processTree("KUAnalysis",sampleName);
    else if(sigType=="cascades") processTree("KUAnalysis",BFTool::GetSignalTokensCascades(rootFilePath));
    else if(sigType=="sms"){
        for(const auto &tree_name:BFTool::GetSignalTokensSMS(rootFilePath))
            processTree(tree_name,tree_name);
    }else{std::cerr<<"[BFI_condor] Unknown sig-type: "<<sigType<<"\n"; delete BFI; return 4;}

    for(auto &kv: totals) kv.second[2]=std::sqrt(kv.second[2]);

    if(doJSON && !writePartialJSON(outputJsonPath,binName,fileResults,totals)){
        std::cerr<<"[BFI_condor] ERROR writing JSON to "<<outputJsonPath<<"\n"; delete BFI; return 5;
    }
    if(histFile) histFile->Close();

    delete BFI;
    return 0;
}

