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

    SampleTool tool; tool.LoadAllFromMaster();

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
        // track unique bins (already mostly correct)
        if(!id.bin.empty()) uniqueBinNames.insert(id.bin);
    }

    // Build outputDir safely (preserve behavior but avoid undefined back())
    if(outputDir.empty()){
        // if no pre-set outputDir, create a sensible one from the unique bins (same behavior as before)
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
    copyConfigsToOutput(outputDir,histCfg,processCfg,binsCfg);

    TString baseName=gSystem->BaseName(inputFileName); baseName.ReplaceAll(".root","");
    TString outRootName=Form("%soutput_%s.root",outputDir.c_str(),baseName.Data());
    outFile=new TFile(outRootName,"RECREATE");

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
            else if(find(tool.SignalKeys.begin(), tool.SignalKeys.end(), proc) != tool.SignalKeys.end()) { 
                sigHists.push_back(h); sigProcs.push_back(proc); 
            }
        }
        
        SortBackgroundsByYield(bkgHists, bkgProcs);
        
        if(!isCutFlow){
            // Individual plots for 1D/2D histograms
            for(auto &pp : procmap){
                TH1* h = pp.second; if(!h) continue;
                if(string(h->GetName()).find("num__")!=string::npos) continue;
                if(string(h->GetName()).find("den__")!=string::npos) continue;
                if(h->InheritsFrom(TH2::Class())) Plot_Hist2D(dynamic_cast<TH2*>(h));
                else Plot_Hist1D(h);
            }
        
            // Plot stack
            if(!bkgHists.empty() || !sigHists.empty() || dataHist){
                if(groupKey.find("num__")!=string::npos || groupKey.find("den__")!=string::npos) continue;
                Plot_Stack(groupKey, bkgHists, sigHists, dataHist, 1.0);
            }
        
            // TEfficiency
            map<string, TH1*> numHists, denHists;
            for(const auto &pp : procmap){
                if(pp.first.find("num__")!=string::npos) numHists[pp.first]=pp.second;
                else if(pp.first.find("den__")!=string::npos) denHists[pp.first]=pp.second;
            }
            for(const auto &nume : numHists){
                string procName = nume.first; procName.replace(0,5,"");
                string denKey = "den__"+procName;
                if(denHists.count(denKey)){
                    TEfficiency* eff = new TEfficiency(*nume.second,*denHists[denKey]);
                    Plot_Eff(eff);
                }
            }
        
        } else {
            // Sort backgrounds by last-bin before calling Plot_CutFlow
            SortCutFlowsByLastBin(bkgHists, bkgProcs);
            if(!bkgHists.empty() || !sigHists.empty() || dataHist)
                Plot_CutFlow(groupKey, bkgHists, sigHists, dataHist, 1.0);
        }
    }

    outFile->Close();
    inFile->Close();
    for(TH1* p:all_clones) delete p;
    all_clones.clear(); groups.clear();

    cout<<"[PlotHistograms] All plots saved to "<<outRootName.Data()<<" and "<<outputDir<<"pdfs/"<<endl;
    return 0;
}
