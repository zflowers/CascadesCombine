// PlotHistograms.cpp
#include "PlottingTools.h"

// ----------------------
// Main
// ----------------------
int main(int argc, char* argv[]) {
    string inputFile;
    string ratiosYaml;
    string patternFile;
    for(int i=1;i<argc;++i){
        string arg=argv[i];
        if(arg=="-i"||arg=="--input"){ if(i+1<argc) inputFile=argv[++i]; else{ cerr<<"[ERROR] Missing "<<arg<<endl; return 1;} }
        else if(arg=="-o"||arg=="--output"){ if(i+1<argc) outputDir=argv[++i]; else{ cerr<<"[ERROR] Missing "<<arg<<endl; return 1;} }
        // lumi used when filling histograms upstream; doesn't rescale, just need for labels
        else if(arg=="-l"||arg=="--lumi"){ if(i+1<argc) lumi=std::stoi(argv[++i]); else{ lumi = 1.;} }
        else if(arg=="-r" || arg=="--ratios"){
            if(i+1<argc) ratiosYaml = argv[++i];
            else { cerr<<"[ERROR] Missing "<<arg<<endl; return 1; }
        }
        else if(arg=="--config"){
            if(i+1<argc) patternFile = argv[++i];
            else {
                cerr << "[ERROR] Missing " << arg << endl;
                return 1;
            }
        }
        else if(arg=="--help"){
            cout << "[PlotHistograms] Usage: " << argv[0] << " [options]\n"
                 << " -i <file.root>\n"
                 << " -r <ratios.yaml>\n"
                 << " --config <plotting.yaml>\n";
            return 0;
        }
        else{ cerr<<"[ERROR] Unknown arg "<<arg<<endl; return 1;}
    }
    if(inputFile.empty()){ cerr<<"[ERROR] No input ROOT file provided.\n"; return 1; }

    TString inputFileName=inputFile;
    TFile* inFile=TFile::Open(inputFileName,"READ");
    if(!inFile||inFile->IsZombie()){ cerr<<"Error: cannot open input "<<inputFileName<<endl; return 1; }
    gStyle->SetOptStat(0); gStyle->SetOptTitle(0);
    loadFormatMaps();
    tool.LoadAllFromMaster();
    YamlConfig cfg = LoadYamlConfig(patternFile);

    map<string,map<string,TH1*>> groups;
    vector<TH1*> all_clones;
    set<string> uniqueBinNames;
    map<string, map<string, TH2*>> h2ByVarProcToBin;

    TIter next(inFile->GetListOfKeys());
    TKey* key;
    while((key=(TKey*)next())){
        TObject* obj = key->ReadObj();
        if(!obj) continue;
        if(!obj->InheritsFrom(TH1::Class())) continue;

        TH1* hIn = (TH1*)obj;
        string hname = hIn->GetName();
        HistId id = ParseHistName(hname);

        // Clone once and store
        TH1* clone = (TH1*)hIn->Clone();
        if(!clone) continue;
        clone->SetDirectory(0);
        if (clone->InheritsFrom(TH2::Class()) && !id.var.empty() && !id.proc.empty() && !id.bin.empty()) {
            std::string key = id.var + "__" + id.proc;   // fixed var+proc, vary bin
            h2ByVarProcToBin[key][id.bin] = dynamic_cast<TH2*>(clone);
        }
        all_clones.push_back(clone);

        // Determine group key:
        // - For CutFlow histograms prefer grouping by bin: "<bin>__CutFlow"
        // - For other histograms follow existing "<bin>__<var>" or "<var>" behavior
        string groupKey;
        bool isCutFlow = id.var == "CutFlow";
        if(isCutFlow){
            if(!id.bin.empty()){
                groupKey = id.bin + "__CutFlow";
                uniqueBinNames.insert(id.bin);
            } else if(!id.var.empty()){
                // fallback if bin isn't present (rare) so it still groups sensibly
                groupKey = id.var + "__CutFlow";
            } else {
                // ultimate fallback to the histogram name to avoid collisions
                groupKey = string(clone->GetName()) + "__CutFlow";
            }
        } else {
            if(!id.var.empty()) {
                if(!id.bin.empty()) {
                    groupKey = id.bin + "__" + id.var;
                    uniqueBinNames.insert(id.bin);
                } else {
                    groupKey = id.var;
                }
            } else {
                // Only CutFlow or malformed names should land here
                groupKey = string(clone->GetName());
            }
            if(!id.bin.empty()) uniqueBinNames.insert(id.bin);
        }

        // use process name for keys if available
        string procKey = id.proc.empty() ? string(clone->GetName()) : id.proc;
        // store in groups for this bin/var or CutFlow
        groups[groupKey][procKey] = clone;
        // track unique bins
        if(!id.bin.empty()) uniqueBinNames.insert(id.bin);
    }

    // Build outputDir safely
    if(outputDir.empty()){
        // if no pre-set outputDir, create a sensible one from the unique bins
        bool first = true;
        for(const auto& bin : uniqueBinNames){
            if(!first) outputDir += "__";
            outputDir += bin;
            first = false;
        }
        if(outputDir.empty()) outputDir = "output";
        outputDir += "/";
    } else {
        // if outputDir already set, ensure trailing slash
        if(outputDir.back() != '/') outputDir += '/';
    }

    TString baseName=gSystem->BaseName(inputFileName); baseName.ReplaceAll(".root","");
    TString outRootName=Form("%soutput_%s.root",outputDir.c_str(),baseName.Data());
    outFile=new TFile(outRootName,"RECREATE");

    // Ratios 
    vector<RatioDef> ratioDefs;
    std::map<std::string, std::vector<TEfficiency*>> effsByBin;
    std::map<std::string, std::vector<TEfficiency*>> effsByProcess;
    if(!ratiosYaml.empty()){
        cout << "[PlotHistograms] Loading ratios from " << ratiosYaml << endl;
        ratioDefs = LoadRatioYAML(ratiosYaml);
        RunRatios(ratioDefs, groups, uniqueBinNames, outputDir, effsByBin, effsByProcess);
    }

    // Main plotting loop
    for(auto &gpair : groups){
        string groupKey = gpair.first;
        auto &procmap = gpair.second;
        if(procmap.empty()) continue;
        
        // Apply the same process-merging rules used by PlotFitDiagnostics.
        // This operates on the actual histograms for this group.
        map<string, TH1*> mergedProcmap;
        set<string> consumed;
        
        for(const auto &rule : cfg.process_merges){
        
            vector<string> matches;
        
            for(const auto &pp : procmap){
        
                const string &proc = pp.first;
        
                if(consumed.count(proc)) continue;
        
                bool match = false;
        
                for(const auto &inc : rule.include){
                    if(fnmatch(inc.c_str(), proc.c_str(), 0) == 0){
                        match = true;
                        break;
                    }
                }
        
                if(!match) continue;
        
                for(const auto &ex : rule.exclude){
                    if(fnmatch(ex.c_str(), proc.c_str(), 0) == 0){
                        match = false;
                        break;
                    }
                }
        
                if(match)
                    matches.push_back(proc);
            }
        
            if(matches.empty()) continue;
        
            // groupKey is "<bin>__<var>" or just "<var>"
            size_t sep = groupKey.find("__");
            std::string mergedName = (sep == std::string::npos)
                ? rule.name                       // no bin in this group
                : groupKey.substr(0, sep) + "__" + rule.name + "__" + groupKey.substr(sep + 2);
            
            TH1 *merged = dynamic_cast<TH1*>(procmap.at(matches.front())->Clone(mergedName.c_str()));
            merged->SetName(mergedName.c_str());
            merged->SetTitle(rule.name.c_str());   // keep title as the clean proc key for anything that wants it directly
        
            merged->SetDirectory(nullptr);
            all_clones.push_back(merged);
        
            // Start with zero and add all matching processes.
            merged->Reset();
        
            for(const auto &proc : matches){
                merged->Add(procmap.at(proc));
                consumed.insert(proc);
            }
        
            mergedProcmap[rule.name] = merged;
        }
        
        // Keep processes which were not consumed by any rule.
        for(const auto &pp : procmap){
            if(!consumed.count(pp.first))
                mergedProcmap[pp.first] = pp.second;
        }
        
        // Use merged map from here onward.
        procmap = mergedProcmap;
        bool isCutFlow = (groupKey.find("__CutFlow") != string::npos);
        
        // Separate histograms
        vector<TH1*> bkgHists, sigHists, smsHists, emptyHists, bkgHists_Run2, bkgHists_Run3, sigHists_Run2, sigHists_Run3;
        vector<string> bkgProcs, sigProcs, bkgProcs_Run2, bkgProcs_Run3, sigProcs_Run2, sigProcs_Run3;
        TH1* dataHist = nullptr;
        
        for(auto &pp : procmap){
            TH1* h = pp.second;
            if(!h) continue;
            const string& proc = pp.first;
            std::string proc_name_lower = proc;
            std::transform(proc_name_lower.begin(), proc_name_lower.end(), proc_name_lower.begin(), [](unsigned char c)
                { return std::tolower(c); });
            if(proc_name_lower.find("data") != std::string::npos) {
               if(dataHist){
                   cerr << "[WARNING] Multiple data hists in group " << groupKey
                        << " (" << dataHist->GetName() << " vs " << h->GetName()
                        << "); keeping the first." << endl;
               } else {
                   dataHist = h;
               }
               continue;
            }
            if(tool.BkgDict.count(proc) || isFakeProcess(proc)) {
                bkgHists.push_back(h); 
                bkgProcs.push_back(proc); 
                if(proc.find("Run2") != std::string::npos){
                    bkgHists_Run2.push_back(h); 
                    bkgProcs_Run2.push_back(proc); 
                }
                if(proc.find("Run3") != std::string::npos){
                    bkgHists_Run3.push_back(h); 
                    bkgProcs_Run3.push_back(proc); 
                }
            }
            else if(find(tool.SignalKeys.begin(), tool.SignalKeys.end(), proc) != tool.SignalKeys.end() 
                || proc.find("SMS") != std::string::npos || proc.find("Cascades") != std::string::npos) {
                sigHists.push_back(h);
                sigProcs.push_back(proc); 
                if(proc.find("SMS") != std::string::npos) smsHists.push_back(h);
                if(proc.find("Run2") != std::string::npos){
                    sigHists_Run2.push_back(h);
                    sigProcs_Run2.push_back(proc); 
                }
                if(proc.find("Run3") != std::string::npos){
                    sigHists_Run3.push_back(h);
                    sigProcs_Run3.push_back(proc); 
                }
            }
        }
        
        SortByYield(bkgHists, bkgProcs);
        SortByYield(bkgHists_Run2, bkgProcs_Run2);
        SortByYield(bkgHists_Run3, bkgProcs_Run3);

        gSystem->mkdir(outputDir.c_str(), kTRUE);
        gSystem->mkdir((outputDir+"pdfs").c_str(), kTRUE);
        for(const auto& bin : uniqueBinNames)
            gSystem->mkdir((outputDir + "pdfs/" + SanitizeString(bin)).c_str(), kTRUE);

        for (const auto& kv : h2ByVarProcToBin) {
            const auto& binMap = kv.second;
        
            std::vector<TH2*> bkg2D, sig2D, sms2D;
            for (const auto& bp : binMap) {
                TH2* h = bp.second;
                if (!h) continue;
        
                std::string proc = ExtractProcName(h->GetName());
                if (tool.BkgDict.count(proc)) {
                    bkg2D.push_back(h);
                } else if (find(tool.SignalKeys.begin(), tool.SignalKeys.end(), proc) != tool.SignalKeys.end()
                           || proc.find("SMS") != std::string::npos
                           || proc.find("Cascades") != std::string::npos) {
                    sig2D.push_back(h);
                }
                if (proc.find("SMS") != std::string::npos) sms2D.push_back(h);
            }
        
            //if (!bkg2D.empty() || !sig2D.empty()) {
            //    const std::string& varProcKey = kv.first;
            //    Plot_Hist2DScatter(varProcKey + "_byBin_combined", bkg2D, sig2D);
            //    Plot_Hist2DScatter(varProcKey + "_byBin_bkg", bkg2D, {});
            //    Plot_Hist2DScatter(varProcKey + "_byBin_sig", {}, sig2D);
            //    if(!sms2D.empty()) Plot_Hist2DScatter(varProcKey + "_byBin_sms", {}, sms2D);
            //    Plot_Hist2DContour(varProcKey + "_byBin_combined", bkg2D, sig2D);
            //    Plot_Hist2DContour(varProcKey + "_byBin_bkg", bkg2D, {});
            //    Plot_Hist2DContour(varProcKey + "_byBin_sig", {}, sig2D);
            //    if(!sms2D.empty()) Plot_Hist2DContour(varProcKey + "_byBin_sms", {}, sms2D);
            //}
        }
        
        if(!isCutFlow){
            // Individual plots for 1D/2D histograms
            for(auto &pp : procmap){
                TH1* h = pp.second; if(!h) continue;
                if(h->InheritsFrom(TH2::Class())) Plot_Hist2D(dynamic_cast<TH2*>(h));
                //else Plot_Hist1D(h);
            }
            // 2D scatter overlays for this bin+var group
            std::vector<TH2*> bkg2D, sig2D, sms2D;
            for (auto &pp : procmap) {
                TH1* h = pp.second;
                if (!h || !h->InheritsFrom(TH2::Class())) continue;
            
                if (tool.BkgDict.count(pp.first)) {
                    bkg2D.push_back(dynamic_cast<TH2*>(h));
                } else if (find(tool.SignalKeys.begin(), tool.SignalKeys.end(), pp.first) != tool.SignalKeys.end()
                           || pp.first.find("SMS") != std::string::npos
                           || pp.first.find("Cascades") != std::string::npos) {
                    sig2D.push_back(dynamic_cast<TH2*>(h));
                }
                if (pp.first.find("SMS") != std::string::npos) sms2D.push_back(dynamic_cast<TH2*>(h));
            }
            // keep these before Plot_Stack / Plot_Overlay, since those mutate hists
            //if (!bkg2D.empty() || !sig2D.empty()) {
            //    Plot_Hist2DScatter(groupKey + "_scatter_combined", bkg2D, sig2D);
            //    Plot_Hist2DScatter(groupKey + "_scatter_bkg", bkg2D, {});
            //    Plot_Hist2DScatter(groupKey + "_scatter_sig", {}, sig2D);
            //    if(!sms2D.empty()) Plot_Hist2DScatter(groupKey + "_scatter_sms", {}, sms2D);
            //    Plot_Hist2DContour(groupKey + "_contour_combined", bkg2D, sig2D);
            //    Plot_Hist2DContour(groupKey + "_contour_bkg", bkg2D, {});
            //    Plot_Hist2DContour(groupKey + "_contour_sig", {}, sig2D);
            //    if(!sms2D.empty()) Plot_Hist2DContour(groupKey + "_contour_sms", {}, sms2D);
            //}       
 
            // Plot stack
            if(!bkgHists.empty() || !sigHists.empty() || dataHist){
                if(bkgHists.size() > 0) { if(bkgHists[0]->InheritsFrom(TH2::Class())) continue; } // don't stack TH2s
                if(sigHists.size() > 0) { if(sigHists[0]->InheritsFrom(TH2::Class())) continue; } // don't stack TH2s
                Plot_Stack(groupKey, bkgHists, sigHists, dataHist, 1.0);
                Plot_Overlay(groupKey, bkgHists, sigHists, dataHist);
                Plot_Overlay(groupKey, bkgHists, sigHists, dataHist, true);
                // Only sigs or only bkgs
                Plot_Stack(groupKey+"_bkg", bkgHists, emptyHists, nullptr, 1.0);
                Plot_Overlay(groupKey+"_bkg", bkgHists, emptyHists, nullptr);
                Plot_Overlay(groupKey+"_bkg", bkgHists, emptyHists, nullptr, true);
                Plot_Overlay(groupKey+"_sig", emptyHists, sigHists, nullptr);
                Plot_Overlay(groupKey+"_sig", emptyHists, sigHists, nullptr, true);
                // Only sms
                Plot_Overlay(groupKey+"_sms", emptyHists, smsHists, nullptr);
                // Only Run2 bkg
                Plot_Stack(groupKey+"_bkgRun2", bkgHists_Run2, emptyHists, nullptr, 1.0);
                Plot_Overlay(groupKey+"_bkgRun2", bkgHists_Run2, emptyHists, nullptr);
                // Only Run3 bkg
                Plot_Stack(groupKey+"_bkgRun3", bkgHists_Run3, emptyHists, nullptr, 1.0);
                Plot_Overlay(groupKey+"_bkgRun3", bkgHists_Run3, emptyHists, nullptr);
                // Only Run2 sig
                Plot_Overlay(groupKey+"_sigRun2", emptyHists, sigHists_Run2, nullptr);
                // Only Run3 sig
                Plot_Overlay(groupKey+"_sigRun3", emptyHists, sigHists_Run3, nullptr);
            }
        
        } else {
            // Sort backgrounds by last-bin before calling Plot_CutFlow
            SortCutFlowsByLastBin(bkgHists, bkgProcs);
            if(!bkgHists.empty() || !sigHists.empty() || dataHist)
                Plot_CutFlow(groupKey, bkgHists, sigHists, dataHist, 1.0);
        }
    }

    for(const auto& pair : effsByBin)
        Plot_Eff_Multi(pair.first, pair.second, "Bin");
    for(const auto& pair : effsByProcess)
        Plot_Eff_Multi(pair.first, pair.second, "Process");

    outFile->Close();
    inFile->Close();
    for(TH1* p:all_clones) delete p;
    all_clones.clear(); groups.clear();

    // after making all multigraphs:
    std::set<TEfficiency*> uniqueEffs;
    for (auto &p : effsByBin)    for (auto *e : p.second) if(e) uniqueEffs.insert(e);
    for (auto &p : effsByProcess) for (auto *e : p.second) if(e) uniqueEffs.insert(e);
    
    // delete each unique TEff
    for (auto *e : uniqueEffs) delete e;
    uniqueEffs.clear();
    effsByBin.clear();
    effsByProcess.clear();

    cout<<"[PlotHistograms] All plots saved to "<<outRootName.Data()<<" and "<<outputDir<<"pdfs/"<<endl;
    return 0;
}
