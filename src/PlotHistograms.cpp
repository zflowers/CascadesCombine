// PlotHistograms.cpp
#include "PlottingTools.h"

// ----------------------
// Main
// ----------------------
int main(int argc, char* argv[]) {
    string inputFile, histCfg, processCfg, binsCfg;
    for(int i=1;i<argc;++i){
        string arg=argv[i];
        if(arg=="-i"||arg=="--input"){ if(i+1<argc) inputFile=argv[++i]; else{ cerr<<"[ERROR] Missing "<<arg<<endl; return 1;} }
        else if(arg=="-h"||arg=="--hist"){ if(i+1<argc) histCfg=argv[++i]; else{ cerr<<"[ERROR] Missing "<<arg<<endl; return 1;} }
        else if(arg=="-d"||arg=="--process"){ if(i+1<argc) processCfg=argv[++i]; else{ cerr<<"[ERROR] Missing "<<arg<<endl; return 1;} }
        else if(arg=="-b"||arg=="--bins"){ if(i+1<argc) binsCfg=argv[++i]; else{ cerr<<"[ERROR] Missing "<<arg<<endl; return 1;} }
        // lumi used when filling histograms upstream; doesn't rescale, just need for labels
        else if(arg=="-l"||arg=="--lumi"){ if(i+1<argc) lumi=std::stoi(argv[++i]); else{ lumi = 1.;} }
        else if(arg=="--help"){ cout<<"[PlotHistograms] Usage: "<<argv[0]<<" [options]\n -i <file.root>\n -h <hist.yaml>\n -d <process.yaml>\n -b <bins.yaml>\n"; return 0; }
        else{ cerr<<"[ERROR] Unknown arg "<<arg<<endl; return 1;}
    }
    if(inputFile.empty()){ cerr<<"[ERROR] No input ROOT file provided.\n"; return 1; }

    TString inputFileName=inputFile;
    TFile* inFile=TFile::Open(inputFileName,"READ");
    if(!inFile||inFile->IsZombie()){ cerr<<"Error: cannot open input "<<inputFileName<<endl; return 1; }
    gStyle->SetOptStat(0); gStyle->SetOptTitle(0);
    loadFormatMaps();
    tool.LoadAllFromMaster();

    map<string,map<string,TH1*>> groups;
    vector<TH1*> all_clones;
    set<string> uniqueBinNames;

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
            if(!id.bin.empty() && !id.var.empty()) groupKey = id.bin + "__" + id.var;
            else groupKey = id.var.empty() ? string(clone->GetName()) : id.var;
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

    gSystem->mkdir(outputDir.c_str(), kTRUE);
    gSystem->mkdir((outputDir+"pdfs").c_str(), kTRUE);
    for(const auto& bin : uniqueBinNames)
        gSystem->mkdir((outputDir+"pdfs/"+bin).c_str(), kTRUE);
    copyConfigsToOutput(outputDir,histCfg,processCfg,binsCfg);

    TString baseName=gSystem->BaseName(inputFileName); baseName.ReplaceAll(".root","");
    TString outRootName=Form("%soutput_%s.root",outputDir.c_str(),baseName.Data());
    outFile=new TFile(outRootName,"RECREATE");

    // maps for TEff inputs
    map<HistId, TH1*> numHists, denHists;
    // Main plotting loop
    for(auto &gpair : groups){
        string groupKey = gpair.first;
        auto &procmap = gpair.second;
        if(procmap.empty()) continue;

        bool isCutFlow = (groupKey.find("__CutFlow") != string::npos);
        
        // Separate histograms
        vector<TH1*> bkgHists, sigHists;
        vector<string> bkgProcs, sigProcs;
        TH1* dataHist = nullptr;
        
        for(auto &pp : procmap){
            TH1* h = pp.second;
            if(!h) continue;
            const string& proc = pp.first;
        
            if(proc=="data"||proc=="Data") { dataHist = h; continue; }
            if(tool.BkgDict.count(proc)) {
                bkgHists.push_back(h); 
                bkgProcs.push_back(proc); 
            }
            else if(find(tool.SignalKeys.begin(), tool.SignalKeys.end(), proc) != tool.SignalKeys.end() 
                || proc.find("SMS") != std::string::npos || proc.find("Cascades") != std::string::npos) {
              sigHists.push_back(h); sigProcs.push_back(proc); 
            }
        }
        
        SortByYield(bkgHists, bkgProcs);
        
        if(!isCutFlow){
            // Individual plots for 1D/2D histograms
            for(auto &pp : procmap){
                TH1* h = pp.second; if(!h) continue;
                if(string(h->GetName()).find("num__")!=string::npos) continue;
                if(string(h->GetName()).find("den__")!=string::npos) continue;
                if(h->InheritsFrom(TH2::Class())) Plot_Hist2D(dynamic_cast<TH2*>(h));
                else Plot_Hist1D(h);
            }
        
            // TEfficiency
            for(const auto &pp : procmap){
                TH1* h = pp.second; if(!h) continue;
                std::string hname = h->GetName();
                HistId hId = ParseHistName(hname);
                if(string(pp.second->GetName()).find("num__")!=string::npos) numHists[hId]=pp.second;
                else if(string(pp.second->GetName()).find("den__")!=string::npos) denHists[hId]=pp.second;
            }
        
            // Plot stack
            if(!bkgHists.empty() || !sigHists.empty() || dataHist){
                if(groupKey.find("num__")!=string::npos || groupKey.find("den__")!=string::npos) continue; // don't stack efficiency inputs
                if(bkgHists[0]->InheritsFrom(TH2::Class())) continue; // don't stack TH2s
                Plot_Stack(groupKey, bkgHists, sigHists, dataHist, 1.0);
            }
        
        } else {
            // Sort backgrounds by last-bin before calling Plot_CutFlow
            SortCutFlowsByLastBin(bkgHists, bkgProcs);
            if(!bkgHists.empty() || !sigHists.empty() || dataHist)
                Plot_CutFlow(groupKey, bkgHists, sigHists, dataHist, 1.0);
        }
    }

    // Maps to store TEffs grouped by bin and by process
    std::map<std::string, std::vector<TEfficiency*>> effsByBin;
    std::map<std::string, std::vector<TEfficiency*>> effsByProcess;
    
    for(const auto &nume : numHists){
        string denName = nume.second->GetName();
        size_t start_pos = 0;
        while ((start_pos = denName.find("__num__", start_pos)) != std::string::npos) {
            denName.replace(start_pos, 7, "__den__");
            start_pos += 7;
        }
    
        HistId denID = ParseHistName(denName);
        if(denHists.count(denID)){
            TEfficiency* eff = nullptr;
            if (HistsCompatible(nume.second, denHists[denID])) {
                gErrorIgnoreLevel = 1001; // ignore ROOT warnings temporarily
                eff = new TEfficiency(*nume.second, *denHists[denID]);
                gErrorIgnoreLevel = 0;
            }
            else continue;
    
            // Clean the name
            string effName = denName;
            start_pos = 0;
            while ((start_pos = effName.find("den__", start_pos)) != std::string::npos) {
                effName.replace(start_pos, 5, "");
            }
            eff->SetName(effName.c_str());
    
            // Store in the maps
            effsByBin[denID.bin].push_back(eff);
            effsByProcess[denID.proc].push_back(eff);
    
            Plot_Eff(eff);
        }
    }

    for(const auto& pair : effsByBin)
        Plot_Eff_Multi(pair.first, pair.second, "Bin");
    for(const auto& pair : effsByProcess)
        Plot_Eff_Multi(pair.first, pair.second, "Process");

    // Load up 2D CutFlow
    std::map<std::string, std::map<std::string, TH1*>> cutflowMap;
    for (const auto &gpair : groups) {
        const std::string &groupKey = gpair.first;
        if (groupKey.find("__CutFlow") == std::string::npos) continue;
        // extract the bin name (strip __CutFlow)
        std::string binName = groupKey.substr(0, groupKey.find("__CutFlow"));
        for (const auto &pp : gpair.second) {
            const std::string proc = pp.first;
            TH1* h = pp.second;
            if(!h) continue;
            cutflowMap[binName][proc] = h;
        }
    }
    
    // build global 2D cutflows from cutflowMap
    MakeAndPlotCutflow2D(cutflowMap, "GlobalCutflow", "yield", 1.0);
    MakeAndPlotCutflow2D(cutflowMap, "GlobalCutflow", "SoB",   1.0);
    MakeAndPlotCutflow2D(cutflowMap, "GlobalCutflow", "SoverSqrtB", 1.0);
    MakeAndPlotCutflow2D(cutflowMap, "GlobalCutflow", "Zbi", 10.0); // 10% systematic

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
