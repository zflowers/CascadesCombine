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
    std::cerr << "  --signal           Mark this process as signal\n";
    std::cerr << "  --sig-type TYPE    Specify signal type (sets --signal automatically)\n";
    std::cerr << "  --lumi VALUE       Integrated luminosity to scale yields\n";
    std::cerr << "  --sample-name NAME Optional name of the sample\n";
    std::cerr << "  --sms-filters LIST Comma-separated list of SMS filters\n";
    std::cerr << "  --help             Display this help message\n";
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

        // --- Define node to apply final event selection cuts ---
        ROOT::RDF::RNode node = df_with_lep;
        
        // --- Define any other derived variables from YAML ---
        std::vector<DerivedVar> derivedVars;
        if(doHist && !histYamlPath.empty()){
            derivedVars = loadDerivedVariablesYAML(histYamlPath);
        }
        
        // --- Validate derived variables ---
        for (const auto &dv : derivedVars) {
            ValidateDerivedVarNode(node, dv);
        }
        
        // --- Define derived variables ---
        for(const auto &dv : derivedVars){
            try{
                node = node.Define(dv.name, dv.expr);
            }catch(const std::exception &e){
                std::cerr << "[BFI_condor] WARNING: Failed to define derived variable '"
                          << dv.name << "' Expression: " << dv.expr
                          << " Exception: " << e.what() << "\n";
            }
        }
        
        // --- Load all user cuts ---
        std::map<std::string, CutDef> allUserCuts;
        node = BuildFitInput::loadCutsUser(node, allUserCuts);
        // --- Select which user cuts to keep ---
        std::vector<DerivedVar> validUserCuts;
        for (const auto &cutName : userCutsVec) {
            auto it = allUserCuts.find(cutName);
            if (it == allUserCuts.end()) {
                std::cerr << "[BFI_condor] Requested cut not found: " << cutName << "\n";
                continue;
            }
            const auto &cut = it->second;
            std::string expanded = BFI->ExpandMacros(cut.expression);
            if (!expanded.empty())
                validUserCuts.push_back({cutName, expanded});
        }

        if(doHist){
            // --- Build ordered cuts list ---
            std::vector<std::string> cutsOrdered;
            std::vector<std::string> cutLabels;
            for (const auto &c : finalCutsExpanded) { if (!c.empty()) { cutsOrdered.push_back(c); cutLabels.push_back(c); } }
            for (const auto &uc : validUserCuts) { cutsOrdered.push_back(uc.expr); cutLabels.push_back(uc.name); }
            const int Ncuts = static_cast<int>(cutsOrdered.size());
            
            // --- Book single CutFlow histogram (Ncuts+1 bins: 0..Ncuts) ---
            std::string cfName = binName + "__" + processName + "__CutFlow";
            auto hist_CutFlow = std::make_shared<TH1D>(cfName.c_str(), cfName.c_str(), Ncuts + 1, 0.0, double(Ncuts + 1));
            //hist_CutFlow->Sumw2();
            
            // --- Dummy total events (bin 0) ---
            // Turn off to speed up processing
            // Count() returns an RResultPtr; force evaluation with GetValue()
            // double totalEvents = static_cast<double>(node.Count().GetValue());
            // hist_CutFlow->SetBinContent(0, totalEvents);
            // hist_CutFlow->SetBinError(0, std::sqrt(totalEvents));
            
            // --- Only build cumulative CutFlow if there are cuts ---
            if (Ncuts > 0) {
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
                    { histNameTmp.c_str(), histNameTmp.c_str(), Ncuts + 1, 0.0, double(Ncuts + 1) },
                    npassed_col.c_str(),
                    "weight_scaled"
                );
            
                // Execute once and get the TH1D by value (copy)
                auto h_npassed = r_h_npassed.GetValue(); // h_npassed is a TH1D (value copy)
            
                // Fill classical CutFlow: bins 1..Ncuts = events surviving cut1..cutN
                for (int i = 1; i <= Ncuts; ++i) {
                    double surv = 0.0;
                    double surv_err2 = 0.0;
                    for (int k = i; k <= Ncuts; ++k) {
                        // mapping: npassed == k is stored in histogram bin index (k + 1)
                        int rootBin = k + 1;
                        double c = h_npassed.GetBinContent(rootBin);
                        double e = h_npassed.GetBinError(rootBin);
                        surv += c;
                        surv_err2 += e * e;
                    }
                    hist_CutFlow->SetBinContent(i, surv);
                    hist_CutFlow->SetBinError(i, std::sqrt(surv_err2));
            
                    std::string lbl = (i - 1 < (int)cutLabels.size()) ? cutLabels[i - 1] : ("Cut_" + std::to_string(i));
                    hist_CutFlow->GetXaxis()->SetBinLabel(i, lbl.c_str());
                }
            }
            // --- Write CutFlow ---
            hist_CutFlow->Write();
        }

        // --- Apply filters to node ---
        for (const auto &c : finalCutsExpanded) if (!c.empty()) node = node.Filter(c);
        for (const auto &vc : validUserCuts) node = node.Filter(vc.expr);
    
        // --- Histograms ---
        if(doHist && !histYamlPath.empty()){
            // --- prepare histDefs as before ---
            auto userHists = loadHistogramsUser(node);
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
            
                // Use the recorded plan; appliedUserCuts were stored in validation
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

