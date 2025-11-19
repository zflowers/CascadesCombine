// src/BFI_condor.cpp
#include <getopt.h>
#include "TFile.h"
#include "TROOT.h"

#include "BFICondorTools.h"

// ----------------------
// Helpers
// ----------------------

static void usage(const char* me) {
    std::cerr << "Usage: " << me
              << " --bin BINNAME[;BINNAME2;...] --file ROOTFILE [--json-output OUT.json] "
                 "[--root-output OUT.root] [--cuts CUT1;CUT2;...] [--lep-cuts LEPCUT1;LEPCUT2;...] "
                 "[--predefined-cuts NAME1;NAME2;...] [--user-cuts NAME1;NAME2;...] [--hist] [--hist-yaml HISTS.yaml] [--json] [--proc-yaml PROCS.yaml]\n\n";
    std::cerr << "Required arguments:\n";
    std::cerr << "  --bin           Name (or semicolon-separated list) of the bin(s) to process (e.g. TEST or BIN1;BIN2)\n";
    std::cerr << "  --file          Path to one ROOT file to process\n\n";
    std::cerr << "Optional arguments:\n";
    std::cerr << "  --json-output      Path to write partial JSON output (use {bin} to place bin name)\n";
    std::cerr << "  --root-output      Path to write ROOT/histogram output\n";
    std::cerr << "  --cuts             Semicolon-separated list of normal tree cuts "
                 "(e.g. MET>=150;PTISR>=250)\n";
    std::cerr << "  --lep-cuts         Semicolon-separated list of lepton cuts for BuildLeptonCut\n";
    std::cerr << "  --predefined-cuts  Semicolon-separated list of predefined cuts\n";
    std::cerr << "  --user-cuts        Semicolon-separated list of user cuts\n";
    std::cerr << "  --hist             Fill histograms\n";
    std::cerr << "  --hist-yaml        YAML file defining histogram expressions\n";
    std::cerr << "  --proc-yaml        YAML file containg processes inputted for ST lookup\n";
    std::cerr << "  --json             Write JSON yields\n";
    std::cerr << "  --signal           Mark this process as signal\n";
    std::cerr << "  --sig-type TYPE    Specify signal type (sets --signal automatically)\n";
    std::cerr << "  --lumi VALUE       Integrated luminosity to scale yields\n";
    std::cerr << "  --sample-name NAME Optional name of the sample\n";
    std::cerr << "  --sms-filters LIST Comma-separated list of SMS filters\n";
    std::cerr << "  --systematics      Semicolon- or comma-separated list of systematic names\n";
    std::cerr << "  --help             Display this help message\n";
}

static std::string makePathForBin(const std::string &base, const std::string &bin, const std::string &sampleName) {
    // If base is empty, default to <bin>_<sampleName>.json
    if (base.empty()) {
        return bin + "_" + sampleName + ".json";
    }
    // If placeholder present, replace it
    size_t pos = base.find("{bin}");
    if (pos != std::string::npos) {
        std::string s = base;
        s.replace(pos, 5, bin);
        return s;
    }
    // Insert _<bin> before final extension
    size_t dot = base.find_last_of('.');
    if (dot == std::string::npos) {
        return base + "_" + bin;
    }
    return base.substr(0, dot) + "_" + bin + base.substr(dot);
}

// ----------------------
// Main
// ----------------------
int main(int argc, char** argv) {
    RegisterSafeHelpers();
    std::string binArg, cutsStr, lepCutsStr, predefCutsStr, userCutsStr, rootFilePath, outputJsonPathBase, sampleName, histOutputPath, cutsMultiStr, lepCutsMultiStr, predefCutsMultiStr, userCutsMultiStr, binsCfgName;
    std::vector<std::string> smsFilters;
    std::vector<std::string> systematicNames;
    bool isSignal=false, doHist=false, doJSON=false;
    std::string sigType, histYamlPath, procYamlPath;
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
        {"lumi", required_argument, 0, 'S'},
        {"sample-name", required_argument, 0, 'n'},
        {"sms-filters", required_argument, 0, 'm'},
        {"hist", no_argument, 0, 'H'},
        {"hist-yaml", required_argument, 0, 'y'},
        {"proc-yaml", required_argument, 0, 'd'},
        {"json", no_argument, 0, 'J'},
        {"root-output", required_argument, 0, 'O'},
        {"cuts-multi", required_argument, 0, 'M'},
        {"lep-cuts-multi", required_argument, 0, 'L'},
        {"predefined-cuts-multi", required_argument, 0, 'P'},
        {"user-cuts-multi", required_argument, 0, 'U'},
        {"bins-cfg", required_argument, 0, 'B'},
        {"systematics", required_argument, 0, 'R'},
        {"help", no_argument, 0, 'h'},
        {0,0,0,0}
    };

    int opt, opt_index=0;
    while ((opt = getopt_long(argc, argv, "b:f:o:c:l:p:st:u:n:m:Hy:J", long_options, &opt_index)) != -1) {
        switch(opt){
            case 'b': binArg=optarg; break;
            case 'f': rootFilePath=optarg; break;
            case 'o': outputJsonPathBase=optarg; break;
            case 'c': cutsStr=optarg; break;
            case 'l': lepCutsStr=optarg; break;
            case 'p': predefCutsStr=optarg; break;
            case 'u': userCutsStr=optarg; break;
            case 's': isSignal=true; break;
            case 't': sigType=optarg; isSignal=true; break;
            case 'S': Lumi=atof(optarg); break;
            case 'n': sampleName=optarg; break;
            case 'm': smsFilters=BFTool::SplitString(optarg,","); break;
            case 'H': doHist=true; break;
            case 'y': histYamlPath=optarg; break;
            case 'd': procYamlPath=optarg; break;
            case 'J': doJSON=true; break;
            case 'O': histOutputPath = optarg; break;
            case 'M': cutsMultiStr = optarg; break;
            case 'L': lepCutsMultiStr = optarg; break;
            case 'P': predefCutsMultiStr = optarg; break;
            case 'U': userCutsMultiStr = optarg; break;
            case 'B': binsCfgName = optarg; break;
            case 'R': systematicNames = splitTopLevel(optarg); break;
            case 'h':
            default: usage(argv[0]); return 1;
        }
    }

    ST.LoadAllFromMaster();
    bool IsData = SampleIsData(rootFilePath);
    int year = 0;
    if (rootFilePath.find("Summer20UL16_") != std::string::npos) year = 2016;
    if (rootFilePath.find("Summer20UL16APV_") != std::string::npos) year = 2016;
    if (rootFilePath.find("Summer20UL17_") != std::string::npos) year = 2017;
    if (rootFilePath.find("Summer20UL18_") != std::string::npos) year = 2018;
    if (rootFilePath.find("Summer22_") != std::string::npos) year = 2022;
    if (rootFilePath.find("Summer22EE_") != std::string::npos) year = 2022;
    if (rootFilePath.find("Summer23_") != std::string::npos) year = 2023;
    if (rootFilePath.find("Summer23BPix_") != std::string::npos) year = 2023;
    if (Lumi <= 0.) Lumi = GetLumiFromKey(rootFilePath); // get lumi from ST for file
    if (sampleName.empty()) sampleName = GetSampleNameFromKey(rootFilePath);
    if (binArg.empty() || rootFilePath.empty() || (!doHist && !doJSON)) { usage(argv[0]); return 1; }

    // split semicolon-separated bin list
    std::vector<std::string> binNames = splitTopLevel(binArg);
    if (binNames.empty()) { std::cerr << "[BFI_condor] No bins parsed from --bin\n"; return 1; }

    BuildFitInput* BFI=nullptr;
    try{BFI=new BuildFitInput();}catch(...){std::cerr<<"[BFI_condor] Failed to construct BuildFitInput\n";return 3;}

    // parse common cut lists (these are strings from CLI)
    std::vector<std::string> cutsVec=splitTopLevel(cutsStr);
    std::vector<std::string> lepCutsVec=splitTopLevel(lepCutsStr);
    std::vector<std::string> predefCutsVec=splitTopLevel(predefCutsStr);
    std::vector<std::string> userCutsVec=splitTopLevel(userCutsStr);

    // Build map bin -> finalCutsExpanded (vector<string>), honoring cuts-multi if provided
    std::map<std::string, std::vector<std::string>> finalCutsExpandedMap;
    // Also keep per-bin user-cuts (names referring to allUserCuts map) so we can expand them later
    std::map<std::string, std::vector<std::string>> userCutsPerBin;
    
    auto split_on_delim = [](const std::string &s, const std::string &delim) {
        std::vector<std::string> out;
        size_t pos = 0;
        while (pos < s.size()) {
            size_t found = s.find(delim, pos);
            if (found == std::string::npos) {
                out.push_back(s.substr(pos));
                break;
            } else {
                out.push_back(s.substr(pos, found - pos));
                pos = found + delim.size();
            }
        }
        return out;
    };
    
    // Helper to split semicolon-delimited (reuse existing splitTopLevel semantics)
    auto splitSemicolon = [&](const std::string &s) {
        return splitTopLevel(s);
    };
    
    if (!cutsMultiStr.empty() || !lepCutsMultiStr.empty() || !predefCutsMultiStr.empty() || !userCutsMultiStr.empty()) {
        // split each multi-string on |||
        std::vector<std::string> cutsParts = split_on_delim(cutsMultiStr, "|||");
        std::vector<std::string> lepCutsParts = split_on_delim(lepCutsMultiStr, "|||");
        std::vector<std::string> predefParts = split_on_delim(predefCutsMultiStr, "|||");
        std::vector<std::string> userParts = split_on_delim(userCutsMultiStr, "|||");
    
        for (size_t i = 0; i < binNames.size(); ++i) {
            const std::string bin = binNames[i];
    
            // pick the corresponding part or empty string
            std::string cutsForBin = (i < cutsParts.size()) ? cutsParts[i] : "";
            std::string lepForBin = (i < lepCutsParts.size()) ? lepCutsParts[i] : "";
            std::string predefForBin = (i < predefParts.size()) ? predefParts[i] : "";
            std::string userForBin = (i < userParts.size()) ? userParts[i] : "";
    
            // split into vectors that match buildCutsForBin signature
            std::vector<std::string> normalCuts = splitSemicolon(cutsForBin);
            std::vector<std::string> lepCuts = splitSemicolon(lepForBin);
            std::vector<std::string> predefCuts = splitSemicolon(predefForBin);
    
            // call your existing helper which will call BFI->BuildLeptonCut(...) for each lepCut
            std::vector<std::string> outCuts;
            if (!buildCutsForBin(BFI, normalCuts, lepCuts, predefCuts, outCuts)) {
                std::cerr << "[BFI_condor] Failed to build final cuts for bin (multi): " << bin << "\n";
                delete BFI;
                return 2;
            }
    
            // Expand macros on the resulting cuts and store per-bin
            std::vector<std::string> expanded;
            expanded.reserve(outCuts.size());
            for (const auto &c : outCuts) expanded.push_back(BFI->ExpandMacros(c));
            finalCutsExpandedMap[bin] = std::move(expanded);
    
            // user-cuts: store per-bin list of user-cut *names* (they will be looked up in allUserCuts later)
            userCutsPerBin[bin] = splitSemicolon(userForBin);
        }
    } else {
        // Fallback: original logic (compute finalCutsExpandedMap by calling buildCutsForBin)
        for (const auto &b : binNames) {
            // global vectors built earlier (cutsVec, lepCutsVec, predefCutsVec) reflect CLI single-valued inputs
            std::vector<std::string> finalCuts;
            if(!buildCutsForBin(BFI, cutsVec, lepCutsVec, predefCutsVec, finalCuts)){
                std::cerr<<"[BFI_condor] Failed to build final cuts for bin: "<<b<<"\n"; delete BFI; return 2;
            }
            std::vector<std::string> finalCutsExpanded;
            for(const auto &c : finalCuts) finalCutsExpanded.push_back(BFI->ExpandMacros(c));
            finalCutsExpandedMap[b] = std::move(finalCutsExpanded);
    
            // preserve original global userCutsVec for each bin
            userCutsPerBin[b] = userCutsVec;
        }
    }

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

    //std::map<std::string, std::map<std::string, std::map<std::string, FileYields>>> fileResultsByBin;
    //std::map<std::string, std::map<std::string, ProcTotals>> totalsByBin;
    std::map<std::string, std::map<std::string,std::map<std::string,std::array<double,3>>>> fileResultsByBin;
    std::map<std::string, std::map<std::string, std::array<double,3>>> totalsByBin;

    std::string processName = "";
    auto preferredGroups = ST.loadPreferredGroupsFromYaml(procYamlPath);
    if(isSignal && sigType == "cascades")
        processName = BFTool::GetSignalTokensCascades(rootFilePath);
    else if(isSignal && sigType == "sms")
        processName = GetProcessNameFromKey(rootFilePath, preferredGroups) + "_" + BFTool::GetFilterSignalsSMS()[0];
    else
        processName = GetProcessNameFromKey(rootFilePath, preferredGroups);

    // processTree will now compute per-bin results by reusing a single df_with_lep
    auto processTree=[&](const std::string &tree_name, const std::string &key){
        if(doHist) histFile->cd();

        // --- Define any other derived variables from YAML (these are independent of bin) ---
        std::vector<DerivedVar> derivedVars;
        if(doHist && !histYamlPath.empty()){
            derivedVars = loadDerivedVariablesYAML(histYamlPath);
        }

        // --- Base node to be copied per-bin ---
        if(ROOT::IsImplicitMTEnabled()) ROOT::DisableImplicitMT(); // disable MT for validation
        BaseNodeHandle valHandle = MakeBaseNode(tree_name, rootFilePath, BFI, Lumi, IsData, year);
        ROOT::RDF::RNode base_node_val = valHandle.node; // RNode constructed under IMT OFF

        // --- Validate derived variables (on base node) ---
        std::vector<DerivedVar> validatedDerivedVars;
        for (const auto &dv : derivedVars) {
            if(ValidateDerivedVarNode(base_node_val, dv))
                validatedDerivedVars.push_back(dv);
        }

        // --- Load all user cuts with base_node_val ---
        std::map<std::string, CutDef> allUserCuts;
        base_node_val = BuildFitInput::loadCutsUser(base_node_val, allUserCuts, true);

        for (const auto &bin : binNames) {
            if(ROOT::IsImplicitMTEnabled()) ROOT::DisableImplicitMT(); // disable MT for validation
            BaseNodeHandle bin_valHandle = MakeBaseNode(tree_name, rootFilePath, BFI, Lumi, IsData, year);
            ROOT::RDF::RNode bin_base_node_val = bin_valHandle.node; // RNode constructed under IMT OFF

            // --- Define validated derived variables on base node ---
            for(const auto &dv : validatedDerivedVars){
                try{
                    bin_base_node_val = bin_base_node_val.Define(dv.name, dv.expr);
                }catch(const std::exception &e){
                    std::cerr << "[BFI_condor] WARNING: Failed to define derived variable '"
                              << dv.name << "' Expression: " << dv.expr
                              << " Exception: " << e.what() << "\n";
                }
            }

            base_node_val = BuildFitInput::loadCutsUser(base_node_val, allUserCuts, false);

            // Retrieve the already-expanded bin-specific cuts
            const auto &finalCutsExpanded = finalCutsExpandedMap.at(bin);
        
            // Build validUserCuts for this bin (use per-bin userCutsPerBin mapping)
            std::vector<DerivedVar> validUserCuts;
            auto itUserList = userCutsPerBin.find(bin);
            const std::vector<std::string> &userListForThisBin = (itUserList != userCutsPerBin.end()) ? itUserList->second : userCutsVec;
            
            for (const auto &cutName : userListForThisBin) {
                if (cutName.empty()) continue;
                auto it = allUserCuts.find(cutName);
                if (it == allUserCuts.end()) {
                    std::cerr << "[BFI_condor] Requested cut not found: " << cutName << " (bin " << bin << ")\n";
                    continue;
                }
                const auto &cut = it->second;
                std::string expanded = BFI->ExpandMacros(cut.expression);
                if (!expanded.empty())
                    validUserCuts.push_back({cutName, expanded});
            }
 
            ROOT::RDF::RNode node = bin_base_node_val;
       
            // --- Histograms / CutFlow per-bin ---
            if(doHist){
                if(histFile) histFile->cd();

                // --- Build ordered cuts list (bin-specific final cuts + user cuts)
                // Ensure every cutsOrdered entry has a corresponding human-readable label
                std::vector<std::string> cutsOrdered;
                std::vector<std::string> cutLabels;
                
                // helper to create a readable, length-limited label from an expression
                auto make_readable_label = [&](const std::string &expr) -> std::string {
                    // basic sanitization: remove excessive whitespace and enclosing parentheses
                    std::string s = expr;
                    // trim
                    auto l = s.find_first_not_of(" \t\n\r");
                    auto r = s.find_last_not_of(" \t\n\r");
                    if (l == std::string::npos) s = "";
                    else s = s.substr(l, r - l + 1);
                
                    // remove outer parentheses if present
                    if (s.size() > 2 && s.front() == '(' && s.back() == ')') {
                        s = s.substr(1, s.size() - 2);
                        // trim again
                        l = s.find_first_not_of(" \t\n\r");
                        r = s.find_last_not_of(" \t\n\r");
                        if (l == std::string::npos) s = "";
                        else s = s.substr(l, r - l + 1);
                    }
                
                    // collapse multiple spaces
                    std::string out;
                    bool lastSpace = false;
                    for (char ch : s) {
                        if (isspace((unsigned char)ch)) {
                            if (!lastSpace) { out.push_back(' '); lastSpace = true; }
                        } else {
                            out.push_back(ch);
                            lastSpace = false;
                        }
                    }
                    s = out;
                
                    // truncate to reasonable length for axis label
                    const size_t maxLen = 52;
                    if (s.size() > maxLen) {
                        // smart truncation: keep beginning and end
                        std::string head = s.substr(0, 28);
                        std::string tail = s.substr(s.size() - 16);
                        s = head + "..." + tail;
                    }
                
                    if (s.empty()) s = std::string("Cut_") + std::to_string(cutLabels.size() + 1);
                    return s;
                };
                
                // --- Determine which CLI cut lists correspond to this bin ---
                // Default to global single-bin values
                size_t binIndex = std::distance(binNames.begin(), std::find(binNames.begin(), binNames.end(), bin));
                std::vector<std::string> cutsCLI = cutsVec;
                std::vector<std::string> lepCLI = lepCutsVec;
                std::vector<std::string> predefCLI = predefCutsVec;
                
                if (!cutsMultiStr.empty() || !lepCutsMultiStr.empty() || !predefCutsMultiStr.empty()) {
                    // Recompute per-bin CLI args using same logic as above
                    std::vector<std::string> cutsParts = split_on_delim(cutsMultiStr, "|||");
                    std::vector<std::string> lepParts = split_on_delim(lepCutsMultiStr, "|||");
                    std::vector<std::string> predefParts = split_on_delim(predefCutsMultiStr, "|||");
                
                    if (binIndex < cutsParts.size()) cutsCLI = splitTopLevel(cutsParts[binIndex]);
                    if (binIndex < lepParts.size())  lepCLI  = splitTopLevel(lepParts[binIndex]);
                    if (binIndex < predefParts.size()) predefCLI = splitTopLevel(predefParts[binIndex]);
                }
                
                // Add finalCutsExpanded with best-effort labels (prefer CLI names if supplied)
                for (size_t idx = 0; idx < finalCutsExpanded.size(); ++idx) {
                    const auto &c = finalCutsExpanded[idx];
                    if (c.empty()) continue;
                
                    cutsOrdered.push_back(c);
                
                    std::string label;
                    // try to use CLI-provided names in order
                    if (idx < cutsCLI.size() && !cutsCLI[idx].empty()) {
                        label = cutsCLI[idx];
                    }
                    else if (idx - cutsCLI.size() < lepCLI.size() && (idx >= cutsCLI.size()) && !lepCLI[idx - cutsCLI.size()].empty()) {
                        label = lepCLI[idx - cutsCLI.size()];
                    }
                    else if (idx - cutsCLI.size() - lepCLI.size() < predefCLI.size() && (idx >= cutsCLI.size() + lepCLI.size()) && !predefCLI[idx - cutsCLI.size() - lepCLI.size()].empty()) {
                        label = predefCLI[idx - cutsCLI.size() - lepCLI.size()];
                    }
                    else {
                        label = make_readable_label(c);
                    }
                
                    cutLabels.push_back(label);
                }
                
                // Append user cuts with their explicit names (they should be human-readable)
                for (const auto &uc : validUserCuts) {
                    cutsOrdered.push_back(uc.expr);
                    // use uc.name if non-empty, else fall back to trimmed expression
                    if (!uc.name.empty()) cutLabels.push_back(uc.name);
                    else cutLabels.push_back(make_readable_label(uc.expr));
                }
                
                const int Ncuts = static_cast<int>(cutsOrdered.size());

                // Book CutFlow specific to (bin, process)
                std::string cfName = bin + "__" + processName + "__CutFlow";
                auto hist_CutFlow = std::make_shared<TH1D>(cfName.c_str(), cfName.c_str(), Ncuts+1, 0.0, double(Ncuts+1));
                hist_CutFlow->Sumw2();

                // Total events from NTUPLES (no cuts)
                auto sumW_NoCuts = node.Sum<double>("weight_scaled");
                auto sumW2_NoCuts = node.Sum<double>("weight_sq_scaled");
                double sW_NoCuts = sumW_NoCuts.GetValue();
                double sW2_NoCuts = sumW2_NoCuts.GetValue();
                double err_NoCuts = (sW2_NoCuts>=0)?std::sqrt(sW2_NoCuts):0.0;
                hist_CutFlow->SetBinContent(0, sW_NoCuts);
                hist_CutFlow->SetBinError(0, err_NoCuts);
                hist_CutFlow->SetBinContent(1, sW_NoCuts);
                hist_CutFlow->SetBinError(1, err_NoCuts);
                hist_CutFlow->GetXaxis()->SetBinLabel(1, "NTUPLES");

                if (Ncuts > 1) {
                    auto make_pass_name = [&](int i){ return processName + std::string("_pass_") + std::to_string(i+1); };

                    ROOT::RDF::RNode defNode = node;
                    for (int i = 0; i < Ncuts; ++i) {
                        std::string expr = (i == 0) ? ("(" + cutsOrdered[0] + ")")
                                                    : (make_pass_name(i-1) + " && (" + cutsOrdered[i] + ")");
                        defNode = defNode.Define(make_pass_name(i), expr);
                    }

                    // npassed = sum(pass_i ? 1 : 0)
                    std::string npassedExpr;
                    for (int i = 0; i < Ncuts; ++i) {
                        if (i) npassedExpr += " + ";
                        npassedExpr += "(" + make_pass_name(i) + " ? 1 : 0)";
                    }
                    std::string npassed_col = processName + std::string("_npassed");
                    defNode = defNode.Define(npassed_col, npassedExpr);

                    // Fill Histo1D once (use .c_str())
                    std::string histNameTmp = processName + std::string("_npassed_tmp");
                    auto r_h_npassed = defNode.Histo1D(
                        { histNameTmp.c_str(), histNameTmp.c_str(), Ncuts, 0.0, double(Ncuts) },
                        npassed_col.c_str(),
                        "weight_scaled"
                    );

                    // Execute once and get the TH1D by value (copy)
                    auto h_npassed = r_h_npassed.GetValue();

                    // Fill classical CutFlow: bins 1..Ncuts = events surviving cut1..cutN
                    for (int i = 2; i <= Ncuts+1; ++i) {
                        double surv = 0.0;
                        double surv_err2 = 0.0;
                        for (int k = i-1; k <= Ncuts; ++k) {
                            // mapping: npassed == k is stored in histogram bin index (k + 1)
                            int rootBin = k + 1;
                            double c = h_npassed.GetBinContent(rootBin);
                            double e = h_npassed.GetBinError(rootBin);
                            surv += c;
                            surv_err2 += e * e;
                        }
                        hist_CutFlow->SetBinContent(i, surv);
                        hist_CutFlow->SetBinError(i, std::sqrt(surv_err2));

                        std::string lbl = (i - 2 < (int)cutLabels.size()) ? cutLabels[i - 2] : ("Cut_" + std::to_string(i-1));
                        hist_CutFlow->GetXaxis()->SetBinLabel(i, lbl.c_str());
                    }
                }
                // --- Write CutFlow for this bin ---
                hist_CutFlow->Write();

                // --- Prepare histogram definitions (reload per-bin to respect any bin-dependent expansions) ---
                auto userHists = loadHistogramsUser(node);
                auto histDefs = loadHistogramsYAML(histYamlPath, BFI);
                histDefs.insert(histDefs.end(), userHists.begin(), userHists.end());

                size_t N = histDefs.size();
                std::vector<HistFilterPlan> plans(N);
                std::vector<char> keep(N, 0);

                for (size_t i = 0; i < N; ++i) {
                    const auto &h = histDefs[i];
                    plans[i] = BuildHistFilterPlan(h, BFI, allUserCuts);

                    // create a hnode copy for validation context (node must have been created with MT OFF)
                    ROOT::RDF::RNode hnode = node;

                    bool ok = ValidateAndRecordAppliedUserCuts(hnode, plans[i], h, BFI);
                    keep[i] = ok ? 1 : 0;
                }

                // --- FILL PASS (MT ON) per-bin ---
                if(!ROOT::IsImplicitMTEnabled()) ROOT::EnableImplicitMT(); // turn on multi-threading for filling
                BaseNodeHandle fillHandle = MakeBaseNode(tree_name, rootFilePath, BFI, Lumi, IsData, year);
                ROOT::RDF::RNode base_node_fill = fillHandle.node; // RNode constructed under IMT ON
                base_node_fill = BuildFitInput::loadCutsUser(base_node_fill, allUserCuts, false);

                std::cout << "[BFI_condor] Filling histograms (bin=" << bin << ")\n";
                for (size_t i = 0; i < N; ++i) {
                    if (!keep[i]) continue;
                    const auto &h = histDefs[i];
                    std::string hname = bin + "__" + processName + "__" + h.name;

                    // Use the recorded plan; appliedUserCuts were stored in validation
                    FillHistFromPlan(base_node_fill, plans[i], h, hname);
                }
            } // end doHist

            // --- JSON output per-bin ---
            if(doJSON){
                std::cout << "[BFI_condor] Filling json (bin=" << bin << ")\n";
                if(!ROOT::IsImplicitMTEnabled()) ROOT::EnableImplicitMT(); 
                BaseNodeHandle jsonHandle = MakeBaseNode(tree_name, rootFilePath, BFI, Lumi, IsData, year);
                ROOT::RDF::RNode json_node = jsonHandle.node; // RNode constructed under IMT ON
                // --- Apply filters to node for JSON 
                json_node = BuildFitInput::loadCutsUser(json_node, allUserCuts, false);
                for (const auto &c : finalCutsExpanded) if (!c.empty()) json_node = json_node.Filter(c);
                for (const auto &vc : validUserCuts) json_node = json_node.Filter(vc.expr);
                auto cnt = json_node.Count();
                auto sumW = json_node.Sum<double>("weight_scaled");
                auto sumW2 = json_node.Sum<double>("weight_sq_scaled");
            
                unsigned long long n_entries = cnt.GetValue();
                double sW = sumW.GetValue();
                double sW2Val = sumW2.GetValue();
                double err = (sW2Val>=0)?std::sqrt(sW2Val):0.0;
            
                // --- NEW: accumulate totals per-process per-file only ONCE ---
                if (fileResultsByBin.find(bin) == fileResultsByBin.end() ||
                    fileResultsByBin[bin].find(key) == fileResultsByBin[bin].end() ||
                    fileResultsByBin[bin][key].find(rootFilePath) == fileResultsByBin[bin][key].end())
                {
                    // record per-bin, per-process, per-rootfile
                    fileResultsByBin[bin][key][rootFilePath] = { (double)n_entries, sW, err};
            
                    // Accumulate totals (do this only once per process per file)
                    auto &tot = totalsByBin[bin][key];
                    tot[0] += (double)n_entries;
                    tot[1] += sW;
                    tot[2] += err*err; // we accumulate err^2 and sqrt later
                }
            }
        } // end loop over bins
    }; // end processTree

    // Dispatch tree(s)
    if(!isSignal) processTree("KUAnalysis",sampleName);
    else if(sigType=="cascades") processTree("KUAnalysis",BFTool::GetSignalTokensCascades(rootFilePath));
    else if(sigType=="sms"){
        for(const auto &tree_name:BFTool::GetSignalTokensSMS(rootFilePath))
            processTree(tree_name,processName);
    }else{std::cerr<<"[BFI_condor] Unknown sig-type: "<<sigType<<"\n"; delete BFI; return 4;}

    // finalize totals: convert accumulated err^2 -> sqrt(err^2)
    for (auto &bin_kv : totalsByBin) {
        for (auto &proc_kv : bin_kv.second) {
            proc_kv.second[2] = std::sqrt(proc_kv.second[2]);
        }
    }

    // write JSON: if an explicit --json-output base was given, produce a single aggregated JSON
    // that contains one top-level key per bin. Otherwise, write per-bin JSON files
    if (doJSON) {
        if (!outputJsonPathBase.empty()) {
            // Write a single JSON file with one top-level key per bin
            std::ofstream ofs(outputJsonPathBase);
            if (!ofs) {
                std::cerr << "[BFI_condor] ERROR opening JSON for write: " << outputJsonPathBase << "\n";
                delete BFI;
                return 5;
            }

            ofs << "{\n";

            for (size_t ib = 0; ib < binNames.size(); ++ib) {
                const auto &bin = binNames[ib];
                if (ib) ofs << ",\n";
                ofs << "  \"" << bin << "\": {\n";

                // Ensure per-bin totals have stddev (sqrt of variance) for writeout.
                // Create a local copy so we do not mutate the on-disk accumulation semantics.
                std::map<std::string, std::array<double,3>> totals_for_write;
                auto itT = totalsByBin.find(bin);
                if (itT != totalsByBin.end()) {
                    totals_for_write = itT->second;
                    for (auto &proc_kv : totals_for_write) {
                        proc_kv.second[2] = (proc_kv.second[2] >= 0.0) ? std::sqrt(proc_kv.second[2]) : 0.0;
                    }
                }

                // fileResults for this bin
                std::map<std::string, std::map<std::string,std::array<double,3>>> files_for_write;
                auto itF = fileResultsByBin.find(bin);
                if (itF != fileResultsByBin.end()) {
                    files_for_write = itF->second;
                }

                // Iterate samples (keys in totals_for_write)
                writeSamplesJSON(ofs, files_for_write, totals_for_write, "  ");
                ofs << "\n  }"; // close this bin
            } // end bins loop

            ofs << "\n}\n";
            ofs.close();

            std::cout << "[BFI_condor] Wrote JSON output to: " << outputJsonPathBase << std::endl;
        } else {
            // No explicit --json-output given: preserve per-bin behavior (one file per bin)
            for (const auto &bin : binNames) {
                // Take sqrt of accumulated variance for this bin before writing (mutate the totalsByBin copy)
                for (auto &proc_kv : totalsByBin[bin]) {
                    proc_kv.second[2] = (proc_kv.second[2] >= 0.0) ? std::sqrt(proc_kv.second[2]) : 0.0;
                }

                std::string outPath = makePathForBin(outputJsonPathBase, bin, sampleName);
                if(!writePartialJSON(outPath, bin, fileResultsByBin[bin], totalsByBin[bin])){
                    std::cerr<<"[BFI_condor] ERROR writing JSON to "<<outPath<<"\n";
                    delete BFI;
                    return 5;
                } else {
                    std::cout << "[BFI_condor] Wrote JSON output to: " << outPath << std::endl;
                }
            }
        }
    }

    if(histFile) histFile->Close();
    if(doHist) std::cout <<"[BFI_condor] Wrote ROOT output to: " << histOutputPath << std::endl;

    delete BFI;
    return 0;
}

