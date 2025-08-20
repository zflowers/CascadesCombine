#include "BuildFitInput.h"

BuildFitInput::BuildFitInput(){
	ROOT::EnableImplicitMT();
	std::cout<<"Enabled MT \n";
}

void BuildFitInput::LoadBkg_KeyValue(const std::string& key, const stringlist& bkglist, const double& Lumi) {
    for (unsigned int i = 0; i < bkglist.size(); i++) {
        std::string subkey = key + "_" + std::to_string(i);

        ROOT::RDataFrame df("KUAnalysis", bkglist[i]);

        // Define scaled weight (w * Lumi) and squared weight
        auto df_scaled = df
            .Define("weight_scaled", [Lumi](double w){ return w * Lumi; }, {"weight"})
            .Define("weight_sq_scaled", [Lumi](double w){ return (w*Lumi)*(w*Lumi); }, {"weight"});
            //.Define("weight_sq_scaled", [Lumi](double w2){ return w2 * Lumi * Lumi; }, {"weight2"});


        // Define lepton pair counts for all sides
        //auto df_with_lep = df_scaled;  // debug temp
        auto df_with_lep = DefineLeptonPairCounts(df_scaled, "");  // all
        df_with_lep = DefineLeptonPairCounts(df_with_lep, "A");    // side A
        df_with_lep = DefineLeptonPairCounts(df_with_lep, "B");    // side B

        df_with_lep = DefinePairKinematics(df_with_lep, "");
        df_with_lep = DefinePairKinematics(df_with_lep, "A");
        df_with_lep = DefinePairKinematics(df_with_lep, "B");

        _base_rdf_BkgDict[subkey] = std::make_unique<RNode>(df_with_lep);
        rdf_BkgDict[subkey]       = std::make_unique<RNode>(df_with_lep);
    }
}

void BuildFitInput::LoadSig_KeyValue(const std::string& key, const stringlist& siglist, const double& Lumi) {
    for (unsigned int i = 0; i < siglist.size(); i++) {
        std::string tree_name = "KUAnalysis";
        stringlist subkeys;
        bool isSMS = false;

        if (siglist[i].find("X_SMS") != std::string::npos) {
            isSMS = true;
            subkeys = BFTool::GetSignalTokensSMS(siglist[i]);
        } else {
            subkeys.push_back(BFTool::GetSignalTokensCascades(siglist[i]));
        }

        for (const auto& subkey : subkeys) {
            if (isSMS) tree_name = subkey;

            ROOT::RDataFrame df(tree_name, siglist[i]);

            // Define scaled weights
            auto df_scaled = df
                .Define("weight_scaled", [Lumi](double w){ return w * Lumi; }, {"weight"})
                .Define("weight_sq_scaled", [Lumi](double w){ return (w*Lumi)*(w*Lumi); }, {"weight"});

            // Define lepton pair counts for all sides
            //auto df_with_lep = df_scaled;  // debug temp
            auto df_with_lep = DefineLeptonPairCounts(df_scaled, "");  // all
            df_with_lep = DefineLeptonPairCounts(df_with_lep, "A");    // side A
            df_with_lep = DefineLeptonPairCounts(df_with_lep, "B");    // side B

            df_with_lep = DefinePairKinematics(df_with_lep, "");
            df_with_lep = DefinePairKinematics(df_with_lep, "A");
            df_with_lep = DefinePairKinematics(df_with_lep, "B");

            _base_rdf_SigDict[subkey] = std::make_unique<RNode>(df_with_lep);
            rdf_SigDict[subkey]       = std::make_unique<RNode>(df_with_lep);
        }
    }
}

void BuildFitInput::LoadBkg_byMap( map< std::string, stringlist>& BkgDict, const double& Lumi){
	
	for (const auto& pair : BkgDict) {
		std::cout<<"Loading RDataFrame for: "<<pair.first<<"\n";
		LoadBkg_KeyValue( pair.first, pair.second, Lumi);
	}

}
void BuildFitInput::LoadSig_byMap( map< std::string, stringlist>& SigDict, const double& Lumi){
	
	for (const auto& pair : SigDict) {
		std::cout<<"Loading RDataFrame for: "<<pair.first<<"\n";
		LoadSig_KeyValue( pair.first, pair.second, Lumi);
	}
	
}

inline bool ColumnExists(ROOT::RDF::RNode rdf, const std::string& name) {
    auto cols = rdf.GetColumnNames(); // OK now, rdf is non-const
    return std::find(cols.begin(), cols.end(), name) != cols.end();
}

// -----------------------------------------------------------------------------
// DefinePairKinematics: create side-specific kinematic vectors and per-pair
// Mass_... and DeltaR_... RVecs for each pair-vector produced by DefineLeptonPairCounts.
// -----------------------------------------------------------------------------
ROOT::RDF::RNode BuildFitInput::DefinePairKinematics(ROOT::RDF::RNode rdf, const std::string& side) {
    // Determine side-specific index branch
    std::string indexBranch;
    if (side == "A")      indexBranch = "index_lep_a";
    else if (side == "B") indexBranch = "index_lep_b";

    std::string sideSuffix = "";
    std::string pairPrefix = "";
    if (side == "A")      { sideSuffix = "_A"; pairPrefix = "A_"; }
    else if (side == "B") { sideSuffix = "_B"; pairPrefix = "B_"; }
    else                  { sideSuffix = "";    pairPrefix = "All_"; }

    // 1) Define kinematic arrays
    if (!indexBranch.empty()) {
        if (!ColumnExists(rdf, "PT_lep" + sideSuffix))
            rdf = rdf.Define("PT_lep" + sideSuffix,
                             [=](const std::vector<double>& pt_all, const std::vector<int>& idx) {
                                 ROOT::RVec<double> out(idx.size());
                                 for (size_t i = 0; i < idx.size(); ++i) out[i] = pt_all[idx[i]];
                                 return out;
                             }, {"PT_lep", indexBranch});

        if (!ColumnExists(rdf, "Eta_lep" + sideSuffix))
            rdf = rdf.Define("Eta_lep" + sideSuffix,
                             [=](const std::vector<double>& eta_all, const std::vector<int>& idx) {
                                 ROOT::RVec<double> out(idx.size());
                                 for (size_t i = 0; i < idx.size(); ++i) out[i] = eta_all[idx[i]];
                                 return out;
                             }, {"Eta_lep", indexBranch});

        if (!ColumnExists(rdf, "Phi_lep" + sideSuffix))
            rdf = rdf.Define("Phi_lep" + sideSuffix,
                             [=](const std::vector<double>& phi_all, const std::vector<int>& idx) {
                                 ROOT::RVec<double> out(idx.size());
                                 for (size_t i = 0; i < idx.size(); ++i) out[i] = phi_all[idx[i]];
                                 return out;
                             }, {"Phi_lep", indexBranch});

        if (!ColumnExists(rdf, "M_lep" + sideSuffix))
            rdf = rdf.Define("M_lep" + sideSuffix,
                             [=](const std::vector<double>& m_all, const std::vector<int>& idx) {
                                 ROOT::RVec<double> out(idx.size());
                                 for (size_t i = 0; i < idx.size(); ++i) out[i] = m_all[idx[i]];
                                 return out;
                             }, {"M_lep", indexBranch});
    } else {
        // Only define _All branches if they don't exist yet
        if (!ColumnExists(rdf, "PT_lep_All"))
            rdf = rdf.Define("PT_lep_All",
                             [](const std::vector<double>& v) { return ROOT::RVec<double>(v.begin(), v.end()); },
                             {"PT_lep"});

        if (!ColumnExists(rdf, "Eta_lep_All"))
            rdf = rdf.Define("Eta_lep_All",
                             [](const std::vector<double>& v) { return ROOT::RVec<double>(v.begin(), v.end()); },
                             {"Eta_lep"});

        if (!ColumnExists(rdf, "Phi_lep_All"))
            rdf = rdf.Define("Phi_lep_All",
                             [](const std::vector<double>& v) { return ROOT::RVec<double>(v.begin(), v.end()); },
                             {"Phi_lep"});

        if (!ColumnExists(rdf, "M_lep_All"))
            rdf = rdf.Define("M_lep_All",
                             [](const std::vector<double>& v) { return ROOT::RVec<double>(v.begin(), v.end()); },
                             {"M_lep"});
    }

    // 2) For each pair-vector, define Mass and DeltaR
    auto makeMassDeltaDefs = [](ROOT::RDF::RNode r,
                                const std::string& pairVar,
                                const std::string& ptVar,
                                const std::string& etaVar,
                                const std::string& phiVar,
                                const std::string& mVar) -> ROOT::RDF::RNode {
        if (!ColumnExists(r, "Mass_" + pairVar))
            r = r.Define("Mass_" + pairVar,
                         [=](const ROOT::RVec<std::pair<int,int>>& pairs,
                             const ROOT::RVec<double>& pt,
                             const ROOT::RVec<double>& eta,
                             const ROOT::RVec<double>& phi,
                             const ROOT::RVec<double>& m) {
                             ROOT::RVec<double> out; out.reserve(pairs.size());
                             for (size_t k = 0; k < pairs.size(); ++k) {
                                 size_t i = pairs[k].first;
                                 size_t j = pairs[k].second;
                                 if (i >= pt.size() || j >= pt.size()) { out.push_back(0.0); continue; }
                                 double px_i = pt[i]*std::cos(phi[i]);
                                 double py_i = pt[i]*std::sin(phi[i]);
                                 double pz_i = pt[i]*std::sinh(eta[i]);
                                 double Ei   = std::sqrt(px_i*px_i + py_i*py_i + pz_i*pz_i + m[i]*m[i]);
                                 double px_j = pt[j]*std::cos(phi[j]);
                                 double py_j = pt[j]*std::sin(phi[j]);
                                 double pz_j = pt[j]*std::sinh(eta[j]);
                                 double Ej   = std::sqrt(px_j*px_j + py_j*py_j + pz_j*pz_j + m[j]*m[j]);
                                 double E = Ei + Ej, px = px_i + px_j, py = py_i + py_j, pz = pz_i + pz_j;
                                 double mass2 = E*E - (px*px + py*py + pz*pz);
                                 out.push_back(mass2>0 ? std::sqrt(mass2) : 0.0);
                             }
                             return out;
                         }, {pairVar, ptVar, etaVar, phiVar, mVar});

        if (!ColumnExists(r, "DeltaR_" + pairVar))
            r = r.Define("DeltaR_" + pairVar,
                         [=](const ROOT::RVec<std::pair<int,int>>& pairs,
                             const ROOT::RVec<double>& eta,
                             const ROOT::RVec<double>& phi) {
                             ROOT::RVec<double> out; out.reserve(pairs.size());
                             for (size_t k = 0; k < pairs.size(); ++k) {
                                 size_t i = pairs[k].first;
                                 size_t j = pairs[k].second;
                                 if (i >= eta.size() || j >= eta.size()) { out.push_back(0.0); continue; }
                                 double deta = eta[i] - eta[j];
                                 double dphi = phi[i] - phi[j];
                                 while (dphi > M_PI) dphi -= 2.0*M_PI;
                                 while (dphi <= -M_PI) dphi += 2.0*M_PI;
                                 out.push_back(std::sqrt(deta*deta + dphi*dphi));
                             }
                             return out;
                         }, {pairVar, etaVar, phiVar});

        return r;
    };

    std::vector<std::string> pairTypes = {"OSSFPairs", "OSOFPairs", "SSSFPairs", "SSOFPairs"};
    for (const auto &ptype : pairTypes) {
        std::string pairVar = pairPrefix + ptype;
        std::string ptVar   = "PT_lep" + sideSuffix;
        std::string etaVar  = "Eta_lep" + sideSuffix;
        std::string phiVar  = "Phi_lep" + sideSuffix;
        std::string mVar    = "M_lep" + sideSuffix;
        rdf = makeMassDeltaDefs(rdf, pairVar, ptVar, etaVar, phiVar, mVar);
    }

    return rdf;
}

// Helper trim
static inline std::string trim_copy(const std::string &s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace((unsigned char)s[a])) ++a;
    while (b > a && std::isspace((unsigned char)s[b-1])) --b;
    return s.substr(a, b-a);
}

std::string BuildFitInput::BuildLeptonCut(const std::string& shorthand_in, const std::string& side) {
    // maps
    std::map<std::string,int> qualMap   = {{"Gold",0}, {"Silver",1}, {"Bronze",2}};
    std::map<std::string,int> chargeMap = {{"Pos",1}, {"Neg",0}};

    // make a mutable copy
    std::string shorthand = shorthand_in;

    // split first token (count/type) from extraCuts using | instead of comma
    std::vector<std::string> tokens;
    {
        size_t sep = shorthand.find('|');
        std::string first = (sep == std::string::npos) ? shorthand : shorthand.substr(0, sep);
        std::string extras = (sep == std::string::npos) ? "" : shorthand.substr(sep+1);
    
        tokens.push_back(trim_copy(first));
    
        if (!extras.empty()) {
            std::stringstream ss(extras);
            std::string tok;
            while (std::getline(ss, tok, '|')) tokens.push_back(trim_copy(tok));
        }
    }

    std::string first = tokens[0];
    std::vector<std::string> extraCuts;
    if (tokens.size() > 1) extraCuts.assign(tokens.begin()+1, tokens.end());

    // determine effective side:
    // - if caller provided 'side' use that
    // - otherwise if first token ends with _a or _b (immediately) use that and strip it
    std::string effectiveSide = side; // caller-provided side
    std::regex sideRgx(R"((.*)(_([ab])$))", std::regex::icase);
    std::smatch sm;
    if (std::regex_match(first, sm, sideRgx)) {
        first = trim_copy((std::string)sm[1]);          // strip the _a/_b suffix
        if (effectiveSide.empty()) {
            effectiveSide = std::string((std::string)sm[3]); // use suffix if side not given
        }
    }
    
    // side-dependent naming for later use
    std::string sideSuffix = "";
    std::string pairPrefix = "";
    if (effectiveSide == "a" || effectiveSide == "A") {
        sideSuffix = "_A";
        pairPrefix = "A_";
        effectiveSide = "a";
    } else if (effectiveSide == "b" || effectiveSide == "B") {
        sideSuffix = "_B";
        pairPrefix = "B_";
        effectiveSide = "b";
    } else {
        sideSuffix = "";
        pairPrefix = "All_";
        effectiveSide = "";
    }

    // regexes
    std::regex singleRgx(R"(^\s*(>=|<=|=|<|>)(\d+)(Gold|Silver|Bronze)\s*$)", std::regex::icase);
    std::regex chargeRgx(R"(^\s*(>=|<=|=|<|>)(\d+)(Pos|Neg)\s*$)", std::regex::icase);
    std::regex pairRgx(R"(^\s*(>=|<=|=|<|>)(\d+)(OSSF|OSOF|SSSF|SSOF)\s*$)", std::regex::icase);
    std::regex flavorRgx(R"(^\s*(>=|<=|=|<|>)(\d+)(Elec|Muon|Mu)\s*$)", std::regex::icase);
    std::smatch match;

    // 1) single-lepton quality, e.g. "=0Bronze" or ">=1Gold"
    if (std::regex_match(first, match, singleRgx)) {
        std::string op = match[1]; if (op == "=") op = "==";
        int n = std::stoi(match[2]);
        std::string prop = match[3];
        int val = qualMap[prop];
        // LepQual_lep[_A/_B/_All] exist (we standardize on LepQual_lep_X names)
        std::string branch = "LepQual_lep" + sideSuffix;
        return "(SUM(" + branch + "==" + std::to_string(val) + ")" + op + std::to_string(n) + ")";
    }

    // 2) single-lepton charge, e.g. "=2Pos"
    if (std::regex_match(first, match, chargeRgx)) {
        std::string op = match[1]; if (op == "=") op = "==";
        int n = std::stoi(match[2]);
        std::string prop = match[3];
        int val = chargeMap[prop];
        std::string branch = "Charge_lep" + sideSuffix;
        return "(SUM(" + branch + "==" + std::to_string(val) + ")" + op + std::to_string(n) + ")";
    }

    // 3) Handle "AllSS" (All Same-Sign Leptons)
    if (first == "AllSS") {
        std::string branch = "Charge_lep" + sideSuffix;
        return "((SUM(" + branch + "==1) == Nlep) || (SUM(" + branch + "==0) == Nlep))";
    }

    // 4) Handle "AllSF" (All Same-Flavor Leptons)
    if (first == "AllSF") {
        std::string branch = "Flavor_lep" + sideSuffix;
        return "((SUM(" + branch + "==0) == Nlep) || (SUM(" + branch + "==1) == Nlep))";
    }

    // 5) pair counts (possibly with extra pair-level cuts)
    // Accept forms: ">=1OSSF" or ">=1OSSF_a" (the latter already stripped if found)
    {
        // For pair parsing we also want to capture if first token included a suffix (we already handled suffix earlier).
        std::regex pairFullRgx(R"(^\s*(>=|<=|=|<|>)(\d+)(OSSF|OSOF|SSSF|SSOF)\s*$)", std::regex::icase);
        if (std::regex_match(first, match, pairFullRgx)) {
            std::string op = match[1]; if (op == "=") op = "==";
            int n = std::stoi(match[2]);
            std::string pairType = match[3]; // OSSF, OSOF, ...
            // pair vector variable (RVec<pair<int,int>>), created earlier: e.g. "All_OSSFPairs", "A_OSSFPairs"
            std::string pairIndexVar = pairPrefix + pairType + "Pairs";    // e.g. All_OSSFPairs
            std::string pairCountVar = pairPrefix + "Num" + pairType + "Pairs"; // e.g. All_NumOSSFPairs

            // no extraCuts: use the count variable
            if (extraCuts.empty()) {
                return "(" + pairCountVar + " " + op + " " + std::to_string(n) + ")";
            }

            // with extraCuts: build pair-level boolean expression on Mass_... and DeltaR_...
            // mass comparator: mass<val, mass<=val, mass>val, mass>=val, or mass![a,b] (veto)
            // DeltaR comparator: DeltaR<val etc.
            std::vector<std::string> conds;
            for (auto c : extraCuts) {
                c = trim_copy(c);
                if (c.empty()) continue;

                // mass range veto: mass![a,b]
                std::regex massVeto(R"(^mass!\s*\[\s*([0-9.eE+\-]+)\s*,\s*([0-9.eE+\-]+)\s*\]\s*$)", std::regex::icase);
                std::smatch m2;
                if (std::regex_match(c, m2, massVeto)) {
                    std::string a = m2[1], b = m2[2];
                    conds.push_back("!(Mass_" + pairIndexVar + " >= " + a + " && Mass_" + pairIndexVar + " <= " + b + ")");
                    continue;
                }
                // mass comparator: mass<val etc.
                std::regex massCmp(R"(^mass\s*(<=|>=|<|>)\s*([0-9.eE+\-]+)\s*$)", std::regex::icase);
                if (std::regex_match(c, m2, massCmp)) {
                    std::string op2 = m2[1];
                    std::string val = m2[2];
                    conds.push_back("(Mass_" + pairIndexVar + " " + op2 + " " + val + ")");
                    continue;
                }
                // DeltaR comparator
                std::regex drCmp(R"(^DeltaR\s*(<=|>=|<|>)\s*([0-9.eE+\-]+)\s*$)", std::regex::icase);
                if (std::regex_match(c, m2, drCmp)) {
                    std::string op2 = m2[1];
                    std::string val = m2[2];
                    conds.push_back("(DeltaR_" + pairIndexVar + " " + op2 + " " + val + ")");
                    continue;
                }
                // unrecognized: emit warning and skip
                std::cerr << "[BuildLeptonCut] Unrecognized pair-level predicate: '" << c << "'\n";
            }

            // combine conds into a single elementwise boolean expression
            std::string combined;
            if (conds.empty()) {
                combined = "true"; // trivial
            } else {
                combined = "(" + conds[0] + ")";
                for (size_t i = 1; i < conds.size(); ++i) combined += " && (" + conds[i] + ")";
            }

            // We want to sum the per-pair boolean mask (RVec<bool>) numeric, then compare.
            // Use SUM(...) macro that ExpandMacros will convert to ROOT::VecOps::Sum.
            // Example final: (SUM((Mass_All_OSSFPairs < 65) && (DeltaR_All_OSSFPairs > 0.4)) >= 1)
            std::string sumExpr = "SUM(" + combined + ")";
            return "(" + sumExpr + " " + op + " " + std::to_string(n) + ")";
        }
    }

    // 6) flavor counts (single-lepton flavor), e.g. ">=1Elec" or ">=2Muon"
    if (std::regex_match(first, match, flavorRgx)) {
        std::string op = match[1]; if (op == "=") op = "==";
        int n = std::stoi(match[2]);
        std::string flavor = match[3]; // Elec or Muon
        if (!effectiveSide.empty()) {
            // side-specific flavor vector: Flavor_lep_A / Flavor_lep_B (coded 0=elec,1=muon)
            int code = (flavor == "Elec" || flavor == "elec") ? 0 : 1;
            std::string branch = "Flavor_lep" + sideSuffix;
            return "(SUM(" + branch + "==" + std::to_string(code) + ")" + op + std::to_string(n) + ")";
        } else {
            // "All" — use absolute PDGID
            int pdg = (flavor == "Elec" || flavor == "elec") ? 11 : 13;
            return "(SUM(abs(PDGID_lep)==" + std::to_string(pdg) + ")" + op + std::to_string(n) + ")";
        }
    }

    // fallback: pass back to the old simple logic (maybe single-token not matched)
    // If you have an older simple BuildLeptonCut that handles more cases, call it here;
    // otherwise print invalid shorthand.
    std::cerr << "[BuildLeptonCut] Invalid shorthand: " << shorthand_in << std::endl;
    return "";
}

ROOT::RDF::RNode BuildFitInput::DefineLeptonPairCounts(ROOT::RDF::RNode rdf, const std::string& side) {
    std::string indexBranch;
    if (side == "A")      indexBranch = "index_lep_a";
    else if (side == "B") indexBranch = "index_lep_b";
    // else "" => all leptons

    // --- define side-specific flattened vectors (flavour, charge, quality)
    if (!indexBranch.empty()) {
        rdf = rdf.Define("Flavor_lep_" + side, [=](const std::vector<int>& pdgids, const std::vector<int>& idx){
            ROOT::RVec<int> flavor(idx.size());
            for (size_t ii = 0; ii < idx.size(); ++ii) flavor[ii] = (std::abs(pdgids[idx[ii]]) == 11 ? 0 : 1);
            return flavor;
        }, {"PDGID_lep", indexBranch});

        rdf = rdf.Define("Charge_lep_" + side, [=](const std::vector<int>& charges, const std::vector<int>& idx){
            ROOT::RVec<int> charge(idx.size());
            for (size_t ii = 0; ii < idx.size(); ++ii) charge[ii] = charges[idx[ii]];
            return charge;
        }, {"Charge_lep", indexBranch});

        rdf = rdf.Define("LepQual_lep_" + side, [=](const std::vector<int>& quals, const std::vector<int>& idx){
            ROOT::RVec<int> qual(idx.size());
            for (size_t ii = 0; ii < idx.size(); ++ii) qual[ii] = quals[idx[ii]];
            return qual;
        }, {"LepQual_lep", indexBranch});

        // also expose kinematics for the side (needed to compute pair masses / ΔR)
        rdf = rdf.Define("PT_lep_" + side, [=](const std::vector<double>& PT, const std::vector<int>& idx){
            ROOT::RVec<double> v(idx.size());
            for (size_t ii = 0; ii < idx.size(); ++ii) v[ii] = PT[idx[ii]];
            return v;
        }, {"PT_lep", indexBranch});

        rdf = rdf.Define("Eta_lep_" + side, [=](const std::vector<double>& Eta, const std::vector<int>& idx){
            ROOT::RVec<double> v(idx.size());
            for (size_t ii = 0; ii < idx.size(); ++ii) v[ii] = Eta[idx[ii]];
            return v;
        }, {"Eta_lep", indexBranch});

        rdf = rdf.Define("Phi_lep_" + side, [=](const std::vector<double>& Phi, const std::vector<int>& idx){
            ROOT::RVec<double> v(idx.size());
            for (size_t ii = 0; ii < idx.size(); ++ii) v[ii] = Phi[idx[ii]];
            return v;
        }, {"Phi_lep", indexBranch});

        rdf = rdf.Define("M_lep_" + side, [=](const std::vector<double>& M, const std::vector<int>& idx){
            ROOT::RVec<double> v(idx.size());
            for (size_t ii = 0; ii < idx.size(); ++ii) v[ii] = M[idx[ii]];
            return v;
        }, {"M_lep", indexBranch});
    }
    else {
        // alias existing branches to RVec types for consistency
        rdf = rdf.Define("Flavor_lep_All", [](const std::vector<int>& pdgids){
            ROOT::RVec<int> flavor(pdgids.size());
            for (size_t i=0;i<pdgids.size();++i) flavor[i] = (std::abs(pdgids[i])==11 ? 0 : 1);
            return flavor;
        }, {"PDGID_lep"});

        rdf = rdf.Define("Charge_lep_All", [](const std::vector<int>& charges){
            return ROOT::RVec<int>(charges.begin(), charges.end());
        }, {"Charge_lep"});

        rdf = rdf.Define("LepQual_lep_All", [](const std::vector<int>& quals){
            return ROOT::RVec<int>(quals.begin(), quals.end());
        }, {"LepQual_lep"});

        rdf = rdf.Define("PT_lep_All", [](const std::vector<double>& v){ return ROOT::RVec<double>(v.begin(), v.end()); }, {"PT_lep"});
        rdf = rdf.Define("Eta_lep_All", [](const std::vector<double>& v){ return ROOT::RVec<double>(v.begin(), v.end()); }, {"Eta_lep"});
        rdf = rdf.Define("Phi_lep_All", [](const std::vector<double>& v){ return ROOT::RVec<double>(v.begin(), v.end()); }, {"Phi_lep"});
        rdf = rdf.Define("M_lep_All", [](const std::vector<double>& v){ return ROOT::RVec<double>(v.begin(), v.end()); }, {"M_lep"});
    }

    // --- helper: define pairs, pair masses and pair dR
    auto definePairs = [&](ROOT::RDF::RNode rdf,
                           const std::string& flavorVar,
                           const std::string& chargeVar,
                           const std::string& ptVar,
                           const std::string& etaVar,
                           const std::string& phiVar,
                           const std::string& mVar,
                           const std::string& prefix) {

        // returns vector of index pairs (i,j) (i<j) satisfying predicate
        auto pairLambda = [](const ROOT::RVec<int>& f, const ROOT::RVec<int>& c, auto pred){
            ROOT::RVec<std::pair<int,int>> pairs;
            if (f.size() < 2) return pairs;
            for (size_t i=0;i<f.size();++i)
                for (size_t j=i+1;j<f.size();++j)
                    if (pred(f[i], f[j], c[i], c[j]))
                        pairs.emplace_back((int)i,(int)j);
            return pairs;
        };

        // define the index-pairs columns
        rdf = rdf.Define(prefix + "OSSFPairs", [=](const ROOT::RVec<int>& f, const ROOT::RVec<int>& c){
            return pairLambda(f,c,[](int fi,int fj,int ci,int cj){ return fi==fj && ci!=cj; });
        }, {flavorVar, chargeVar});

        rdf = rdf.Define(prefix + "OSOFPairs", [=](const ROOT::RVec<int>& f, const ROOT::RVec<int>& c){
            return pairLambda(f,c,[](int fi,int fj,int ci,int cj){ return fi!=fj && ci!=cj; });
        }, {flavorVar, chargeVar});

        rdf = rdf.Define(prefix + "SSSFPairs", [=](const ROOT::RVec<int>& f, const ROOT::RVec<int>& c){
            return pairLambda(f,c,[](int fi,int fj,int ci,int cj){ return fi==fj && ci==cj; });
        }, {flavorVar, chargeVar});

        rdf = rdf.Define(prefix + "SSOFPairs", [=](const ROOT::RVec<int>& f, const ROOT::RVec<int>& c){
            return pairLambda(f,c,[](int fi,int fj,int ci,int cj){ return fi!=fj && ci==cj; });
        }, {flavorVar, chargeVar});

        // keep counts for backward compatibility
        rdf = rdf.Define(prefix + "NumOSSFPairs", [=](const ROOT::RVec<std::pair<int,int>>& pairs){ return (int)pairs.size(); }, {prefix + "OSSFPairs"});
        rdf = rdf.Define(prefix + "NumOSOFPairs", [=](const ROOT::RVec<std::pair<int,int>>& pairs){ return (int)pairs.size(); }, {prefix + "OSOFPairs"});
        rdf = rdf.Define(prefix + "NumSSSFPairs", [=](const ROOT::RVec<std::pair<int,int>>& pairs){ return (int)pairs.size(); }, {prefix + "SSSFPairs"});
        rdf = rdf.Define(prefix + "NumSSOFPairs", [=](const ROOT::RVec<std::pair<int,int>>& pairs){ return (int)pairs.size(); }, {prefix + "SSOFPairs"});

        // --- define pair masses and deltaR for each pair index list
        // Mass calculation using (E,p) from (pt,eta,phi,m)
        rdf = rdf.Define(prefix + "OSSFPairMasses", [=](const ROOT::RVec<std::pair<int,int>>& pairs,
                                                       const ROOT::RVec<double>& pt,
                                                       const ROOT::RVec<double>& eta,
                                                       const ROOT::RVec<double>& phi,
                                                       const ROOT::RVec<double>& mass) {
            ROOT::RVec<double> masses; masses.reserve(pairs.size());
            for (const auto &pr : pairs) {
                int i = pr.first, j = pr.second;
                // build 4-vectors (E, px, py, pz)
                auto e = [&](double pt_, double eta_, double m_){
                    double pz = pt_ * std::sinh(eta_);
                    double p = std::sqrt(pt_*pt_ + pz*pz);
                    return std::sqrt(p*p + m_*m_);
                };
                double Ei = e(pt[i], eta[i], mass[i]);
                double Ej = e(pt[j], eta[j], mass[j]);
                double pix = pt[i]*std::cos(phi[i]);
                double piy = pt[i]*std::sin(phi[i]);
                double piz = pt[i]*std::sinh(eta[i]);
                double pjx = pt[j]*std::cos(phi[j]);
                double pjy = pt[j]*std::sin(phi[j]);
                double pjz = pt[j]*std::sinh(eta[j]);
                double px = pix + pjx;
                double py = piy + pjy;
                double pz = piz + pjz;
                double E = Ei + Ej;
                double invm2 = E*E - (px*px + py*py + pz*pz);
                masses.push_back(invm2 > 0. ? std::sqrt(invm2) : 0.);
            }
            return masses;
        }, {prefix + "OSSFPairs", ptVar, etaVar, phiVar, mVar});

        // deltaR
        rdf = rdf.Define(prefix + "OSSFPairDR", [=](const ROOT::RVec<std::pair<int,int>>& pairs,
                                                   const ROOT::RVec<double>& eta,
                                                   const ROOT::RVec<double>& phi){
            ROOT::RVec<double> drs; drs.reserve(pairs.size());
            for (const auto &pr : pairs) {
                int i=pr.first, j=pr.second;
                double dphi = std::abs(phi[i]-phi[j]);
                if (dphi > M_PI) dphi = 2*M_PI - dphi;
                double deta = eta[i] - eta[j];
                drs.push_back(std::sqrt(deta*deta + dphi*dphi));
            }
            return drs;
        }, {prefix + "OSSFPairs", etaVar, phiVar});

        rdf = rdf.Define(prefix + "OSOFPairMasses", [=](const ROOT::RVec<std::pair<int,int>>& pairs,
                                                       const ROOT::RVec<double>& pt,
                                                       const ROOT::RVec<double>& eta,
                                                       const ROOT::RVec<double>& phi,
                                                       const ROOT::RVec<double>& mass) {
            ROOT::RVec<double> masses; masses.reserve(pairs.size());
            for (const auto &pr : pairs) {
                int i = pr.first, j = pr.second;
                // build 4-vectors (E, px, py, pz)
                auto e = [&](double pt_, double eta_, double m_){
                    double pz = pt_ * std::sinh(eta_);
                    double p = std::sqrt(pt_*pt_ + pz*pz);
                    return std::sqrt(p*p + m_*m_);
                };
                double Ei = e(pt[i], eta[i], mass[i]);
                double Ej = e(pt[j], eta[j], mass[j]);
                double pix = pt[i]*std::cos(phi[i]);
                double piy = pt[i]*std::sin(phi[i]);
                double piz = pt[i]*std::sinh(eta[i]);
                double pjx = pt[j]*std::cos(phi[j]);
                double pjy = pt[j]*std::sin(phi[j]);
                double pjz = pt[j]*std::sinh(eta[j]);
                double px = pix + pjx;
                double py = piy + pjy;
                double pz = piz + pjz;
                double E = Ei + Ej;
                double invm2 = E*E - (px*px + py*py + pz*pz);
                masses.push_back(invm2 > 0. ? std::sqrt(invm2) : 0.);
            }
            return masses;
        }, {prefix + "OSOFPairs", ptVar, etaVar, phiVar, mVar});

        // deltaR
        rdf = rdf.Define(prefix + "OSOFPairDR", [=](const ROOT::RVec<std::pair<int,int>>& pairs,
                                                   const ROOT::RVec<double>& eta,
                                                   const ROOT::RVec<double>& phi){
            ROOT::RVec<double> drs; drs.reserve(pairs.size());
            for (const auto &pr : pairs) {
                int i=pr.first, j=pr.second;
                double dphi = std::abs(phi[i]-phi[j]);
                if (dphi > M_PI) dphi = 2*M_PI - dphi;
                double deta = eta[i] - eta[j];
                drs.push_back(std::sqrt(deta*deta + dphi*dphi));
            }
            return drs;
        }, {prefix + "OSOFPairs", etaVar, phiVar});

        rdf = rdf.Define(prefix + "SSOFPairMasses", [=](const ROOT::RVec<std::pair<int,int>>& pairs,
                                                       const ROOT::RVec<double>& pt,
                                                       const ROOT::RVec<double>& eta,
                                                       const ROOT::RVec<double>& phi,
                                                       const ROOT::RVec<double>& mass) {
            ROOT::RVec<double> masses; masses.reserve(pairs.size());
            for (const auto &pr : pairs) {
                int i = pr.first, j = pr.second;
                // build 4-vectors (E, px, py, pz)
                auto e = [&](double pt_, double eta_, double m_){
                    double pz = pt_ * std::sinh(eta_);
                    double p = std::sqrt(pt_*pt_ + pz*pz);
                    return std::sqrt(p*p + m_*m_);
                };
                double Ei = e(pt[i], eta[i], mass[i]);
                double Ej = e(pt[j], eta[j], mass[j]);
                double pix = pt[i]*std::cos(phi[i]);
                double piy = pt[i]*std::sin(phi[i]);
                double piz = pt[i]*std::sinh(eta[i]);
                double pjx = pt[j]*std::cos(phi[j]);
                double pjy = pt[j]*std::sin(phi[j]);
                double pjz = pt[j]*std::sinh(eta[j]);
                double px = pix + pjx;
                double py = piy + pjy;
                double pz = piz + pjz;
                double E = Ei + Ej;
                double invm2 = E*E - (px*px + py*py + pz*pz);
                masses.push_back(invm2 > 0. ? std::sqrt(invm2) : 0.);
            }
            return masses;
        }, {prefix + "SSOFPairs", ptVar, etaVar, phiVar, mVar});

        // deltaR
        rdf = rdf.Define(prefix + "SSOFPairDR", [=](const ROOT::RVec<std::pair<int,int>>& pairs,
                                                   const ROOT::RVec<double>& eta,
                                                   const ROOT::RVec<double>& phi){
            ROOT::RVec<double> drs; drs.reserve(pairs.size());
            for (const auto &pr : pairs) {
                int i=pr.first, j=pr.second;
                double dphi = std::abs(phi[i]-phi[j]);
                if (dphi > M_PI) dphi = 2*M_PI - dphi;
                double deta = eta[i] - eta[j];
                drs.push_back(std::sqrt(deta*deta + dphi*dphi));
            }
            return drs;
        }, {prefix + "SSOFPairs", etaVar, phiVar});

        rdf = rdf.Define(prefix + "SSSFPairMasses", [=](const ROOT::RVec<std::pair<int,int>>& pairs,
                                                       const ROOT::RVec<double>& pt,
                                                       const ROOT::RVec<double>& eta,
                                                       const ROOT::RVec<double>& phi,
                                                       const ROOT::RVec<double>& mass) {
            ROOT::RVec<double> masses; masses.reserve(pairs.size());
            for (const auto &pr : pairs) {
                int i = pr.first, j = pr.second;
                // build 4-vectors (E, px, py, pz)
                auto e = [&](double pt_, double eta_, double m_){
                    double pz = pt_ * std::sinh(eta_);
                    double p = std::sqrt(pt_*pt_ + pz*pz);
                    return std::sqrt(p*p + m_*m_);
                };
                double Ei = e(pt[i], eta[i], mass[i]);
                double Ej = e(pt[j], eta[j], mass[j]);
                double pix = pt[i]*std::cos(phi[i]);
                double piy = pt[i]*std::sin(phi[i]);
                double piz = pt[i]*std::sinh(eta[i]);
                double pjx = pt[j]*std::cos(phi[j]);
                double pjy = pt[j]*std::sin(phi[j]);
                double pjz = pt[j]*std::sinh(eta[j]);
                double px = pix + pjx;
                double py = piy + pjy;
                double pz = piz + pjz;
                double E = Ei + Ej;
                double invm2 = E*E - (px*px + py*py + pz*pz);
                masses.push_back(invm2 > 0. ? std::sqrt(invm2) : 0.);
            }
            return masses;
        }, {prefix + "SSSFPairs", ptVar, etaVar, phiVar, mVar});

        // deltaR
        rdf = rdf.Define(prefix + "SSSFPairDR", [=](const ROOT::RVec<std::pair<int,int>>& pairs,
                                                   const ROOT::RVec<double>& eta,
                                                   const ROOT::RVec<double>& phi){
            ROOT::RVec<double> drs; drs.reserve(pairs.size());
            for (const auto &pr : pairs) {
                int i=pr.first, j=pr.second;
                double dphi = std::abs(phi[i]-phi[j]);
                if (dphi > M_PI) dphi = 2*M_PI - dphi;
                double deta = eta[i] - eta[j];
                drs.push_back(std::sqrt(deta*deta + dphi*dphi));
            }
            return drs;
        }, {prefix + "SSSFPairs", etaVar, phiVar});

        return rdf;
    };

    // invoke definePairs for either side-specific or All_
    if (!indexBranch.empty()) {
        rdf = definePairs(rdf,
                          std::string("Flavor_lep_") + side,
                          std::string("Charge_lep_") + side,
                          std::string("PT_lep_") + side,
                          std::string("Eta_lep_") + side,
                          std::string("Phi_lep_") + side,
                          std::string("M_lep_") + side,
                          std::string(side + std::string("_")));
    } else {
        rdf = definePairs(rdf,
                          "Flavor_lep_All",
                          "Charge_lep_All",
                          "PT_lep_All",
                          "Eta_lep_All",
                          "Phi_lep_All",
                          "M_lep_All",
                          "All_");
    }

    return rdf;
}

void BuildFitInput::FilterRegions(const std::string& filterName, const stringlist& filterCuts) {
    std::string combinedCuts;
    for (size_t i = 0; i < filterCuts.size(); ++i) {
        if (i > 0) combinedCuts += " && ";
        combinedCuts += filterCuts[i];
    }
    std::string fullCuts = ExpandMacros(combinedCuts);

    for (const auto& it : rdf_BkgDict) {
        bkg_filtered_dataframes[ std::make_pair(it.first, filterName) ] =
            std::make_unique<RN>(it.second->Filter(fullCuts, filterName));
    }

    for (const auto& it : rdf_SigDict) {
        sig_filtered_dataframes[ std::make_pair(it.first, filterName) ] =
            std::make_unique<RN>(it.second->Filter(fullCuts, filterName));
    }
}

void BuildFitInput::RegisterMacro(const std::string& name, const std::string& expansion) {
    userMacros[name] = expansion;
}

std::string BuildFitInput::ExpandMacros(const std::string& expr) {
    std::string expanded = expr;
    for (const auto& kv : userMacros) {
        std::regex rgx(kv.first + R"(\(([^)]+)\))"); 
        expanded = std::regex_replace(expanded, rgx, kv.second + "($1)");
    }
    return expanded;
}

void BuildFitInput::ReportRegions(int verbosity,
                                  countmap &countResults,
                                  summap &sumResults,
                                  errormap &errorResults,
				  bool DoSig)
{

    countResults.clear();
    sumResults.clear();
    errorResults.clear();

    auto processNodes = [&](auto& nodes){
        for (const auto& it : nodes){
            RNode& node = *(it.second);
    
            // Compute counts, sums, and sum-of-squares
            ROOT::RDF::RResultPtr<long long unsigned int> count_r = node.Count();
            ROOT::RDF::RResultPtr<double> sum_r   = node.Sum("weight_scaled");
            ROOT::RDF::RResultPtr<double> sumw2_r = node.Sum("weight_sq_scaled");
    
            // Extract values
            double count_val = static_cast<double>(*count_r);
            double sum_val   = sum_r.GetValue();
            double error_val = std::sqrt(sumw2_r.GetValue());
    
            // Convert string key to proc_cut_pair
            std::string strkey = it.first;
            size_t last_underscore = strkey.rfind('_');
            std::string procname = strkey.substr(0, last_underscore);
            std::string binname  = strkey.substr(last_underscore + 1);
            proc_cut_pair key{procname, binname};
    
            // Fill maps
            countResults[key] = count_val;
            sumResults[key]   = sum_val;
            errorResults[key] = error_val;
    
            if (verbosity > 0){
                std::cout << strkey << ":\n"
                          << "Count: " << count_val
                          << ", Sum: " << sum_val
                          << ", Error: " << error_val << "\n\n";
            }
        }
    };


    if(DoSig){
      std::cout << "Processing Sig nodes...\n";
      processNodes(_base_rdf_SigDict);
    } else {
      std::cout << "Processing Bkg nodes...\n";
      processNodes(_base_rdf_BkgDict);
    }
}

void BuildFitInput::ReportRegions(int verbosity){
	std::cout<<"Reporting bkg nodes ...  \n";
	for (const auto& it : _base_rdf_BkgDict){
		if( verbosity > 0 ){
			std::cout<<it.first<<": \n";
			(it.second)->Report()->Print();
			std::cout<<"\n";
		}else{
			(it.second)->Report();
		}
	}
	std::cout<<"Reporting sig nodes ... \n";
	for (const auto& it : _base_rdf_SigDict){
		if( verbosity > 0 ){
			std::cout<<it.first<<": \n";
			(it.second)->Report()->Print();
			std::cout<<"\n";
		}else{
			(it.second)->Report();
		}
	}
}

void BuildFitInput::PrintCountReports(const countmap& countResults) {
    std::cout << "Reporting counts ... \n";
    for (const auto& it : countResults) {
        std::cout << it.first.first << " " << it.first.second
                  << " " << it.second << "\n";
    }
    std::cout << "\n";
}

void BuildFitInput::PrintSumReports(const summap& sumResults) {
    std::cout << "Reporting sums ... \n";
    for (const auto& it : sumResults) {
        std::cout << it.first.first << " " << it.first.second
                  << " " << it.second << "\n";
    }
    std::cout << "\n";
}

void BuildFitInput::FullReport(const countmap& countResults,
                               const summap& sumResults,
                               const errormap& errorResults) {

    std::cout << "BkgKey RawEvt WtEvt Err\n";

    for (const auto& it : countResults) {
        const proc_cut_pair& key = it.first;
        // Values are already double, no RResultPtr
        double count = it.second;          // countResults now stores double
        double sum   = sumResults.at(key); // sumResults stores double
        double err   = errorResults.at(key);

        std::cout << key.first << " " << key.second
                  << " " << count << " " << sum << " " << err
                  << std::endl;
    }
}

std::map<std::string, Process*> BuildFitInput::CombineBkgs( std::map<std::string, Process*>& bkgProcs ){
	//build the set of backgrounds
	std::map<std::string, Process*> combinedBkgProcs{};
	for( const auto& it : bkgProcs){
		std::string procname = it.first;
		
		//check for key, if it doesn't exist create it
		procname = BFTool::SplitString( procname, "_")[0] ;
		if( combinedBkgProcs.find(procname) == combinedBkgProcs.end()){
			//key was not found, make a new proc and put it in the map
			Process* newproc = new Process(procname, 0, 0 ,0);
			combinedBkgProcs[procname] = newproc;
		}
		
		//if it is in there already, add up the  pieces with the existing components
		combinedBkgProcs[procname]->Add(bkgProcs[it.first]);
		
	}
	for( const auto& it: combinedBkgProcs){//loop back through and sqrt the errors
		combinedBkgProcs[it.first]->FixError();
	}
	return combinedBkgProcs;
}

void BuildFitInput::CreateBin(const std::string& binname){
	Bin* bin = new Bin();
	bin->binname = binname;
	analysisbins[binname] = bin;
}

// Overload: takes vector of cut strings
void BuildFitInput::CreateBin(const std::string& binName, const std::vector<std::string>& cuts) {
    FilterRegions(binName, cuts);    // automatically filter
    CreateBin(binName);              // then create the bin
}

void BuildFitInput::ConstructBkgBinObjects(countmap countResults,
                                           summap sumResults,
                                           errormap errorResults)
{
    for (const auto& it : countResults) {
        proc_cut_pair cutpairkey = it.first;
        std::string procname = cutpairkey.first;
        std::string binname  = cutpairkey.second;

        Process* thisproc = new Process(procname,
                                        countResults[cutpairkey],
                                        sumResults[cutpairkey],
                                        errorResults[cutpairkey]);
        analysisbins[binname]->bkgProcs.insert({procname, thisproc});
    }

    for (const auto& it : analysisbins){
        it.second->combinedProcs = CombineBkgs(it.second->bkgProcs);
    }
}

void BuildFitInput::AddSigToBinObjects(countmap countResults,
                                       summap sumResults,
                                       errormap errorResults,
                                       std::map<std::string, Bin*>& analysisbins)
{
    for (const auto& it : analysisbins) {
        std::string binname = it.first;
        for (const auto& it2 : countResults) {
            proc_cut_pair cutpairkey = it2.first;
            if (binname != cutpairkey.second) continue;

            std::string procname = cutpairkey.first;
            Process* thisproc = new Process(procname,
                                            countResults[cutpairkey],
                                            sumResults[cutpairkey],
                                            errorResults[cutpairkey]);
            analysisbins[binname]->signals.insert({procname, thisproc});
        }
    }
}

void BuildFitInput::PrintBins(int verbosity){
	for(const auto& it: analysisbins){
		std::cout<<"Bin: "<< it.second->binname << "\n";
		//loop over raw bkg procs
		if(verbosity >= 3){
			for(const auto& it2: it.second->bkgProcs ){
				std::cout<<"   "<<it2.second->procname<<" "<< it2.second->nevents <<" "<<it2.second->wnevents<<" "<<it2.second->staterror<<"\n";
			}
		}
		if(verbosity > 0){
			for(const auto& it2: it.second->combinedProcs){
				std::cout<<"   "<< it2.second->procname<<" "<<it2.second->nevents <<" "<<it2.second->wnevents<<" "<<it2.second->staterror<<"\n";
			}	
		}
		if(verbosity >= 1){
			for(const auto& it2: it.second->signals){
				std::cout<<"   "<< it2.second->procname<<" "<<it2.second->nevents <<" "<<it2.second->wnevents<<" "<<it2.second->staterror<<"\n";
			}	
		}
	}
}

std::string BuildFitInput::GetCleaningCut(){
        return "(PTCM <= 200.) && "
	"( (PTCM <= -500.*sqrt( ((-2.777*pow(fabs(dphiCMI),2) + 1.388*fabs(dphiCMI) + 0.8264) > 0 ? "
	"(-2.777*pow(fabs(dphiCMI),2) + 1.388*fabs(dphiCMI) + 0.8264) : 0) ) + 575.) || "
	"(-2.777*pow(fabs(dphiCMI),2) + 1.388*fabs(dphiCMI) + 0.8264 <= 0.) ) && "
	"( (PTCM <= -500.*sqrt( ((-1.5625*pow(fabs(dphiCMI),2) + 7.8125*fabs(dphiCMI) - 8.766) > 0 ? "
	"(-1.5625*pow(fabs(dphiCMI),2) + 7.8125*fabs(dphiCMI) - 8.766) : 0) ) + 600.) || "
	"(-1.5625*pow(fabs(dphiCMI),2) + 7.8125*fabs(dphiCMI) - 8.766 <= 0.) )";
}
std::string BuildFitInput::GetZstarCut(){
        return "(" + BuildLeptonCut(">=1OSSF","a") + " || " +
               BuildLeptonCut(">=1OSSF","b") + ") || "
               + "(Nlep==2 && " +
               BuildLeptonCut(">=1OSSF") + ")";
}
std::string BuildFitInput::GetnoZstarCut(){
        return "!"+GetZstarCut();
}
