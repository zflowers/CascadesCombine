// PlotTriggers.cpp
#include "PlottingTools.h"

// ----------------------
// Main
// ----------------------
int main(int argc, char* argv[]) {
    string inputFile;
    string ratiosYaml;
    for(int i=1;i<argc;++i){
        string arg=argv[i];
        if(arg=="-i"||arg=="--input"){ if(i+1<argc) inputFile=argv[++i]; else{ cerr<<"[ERROR] Missing "<<arg<<endl; return 1;} }
        else if(arg=="-o"||arg=="--output"){ if(i+1<argc) outputDir=argv[++i]; else{ cerr<<"[ERROR] Missing "<<arg<<endl; return 1;} }
        else if(arg=="-r" || arg=="--ratios"){
            if(i+1<argc) ratiosYaml = argv[++i];
            else { cerr<<"[ERROR] Missing "<<arg<<endl; return 1; }
        }
        else if(arg=="--help"){
            cout << "[PlotTriggers] Usage: " << argv[0] << " [options]\n"
                 << " -i <file.root>\n"
                 << " -r <ratios.yaml>\n";
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
            continue;
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

    gSystem->mkdir(outputDir.c_str(), kTRUE);
    gSystem->mkdir((outputDir+"pdfs").c_str(), kTRUE);
    for(const auto& bin : uniqueBinNames)
        gSystem->mkdir((outputDir+"pdfs/"+bin).c_str(), kTRUE);

    TString baseName=gSystem->BaseName(inputFileName); baseName.ReplaceAll(".root","");
    TString outRootName=Form("%soutput_triggers_%s.root",outputDir.c_str(),baseName.Data());
    outFile=new TFile(outRootName,"RECREATE");

    // Ratios 
    vector<RatioDef> ratioDefs;
    std::map<std::string, std::vector<TEfficiency*>> effsByBin;
    std::map<std::string, std::vector<TEfficiency*>> effsByProcess;
    if(!ratiosYaml.empty()){
        cout << "[PlotTriggers] Loading ratios from " << ratiosYaml << endl;
        ratioDefs = LoadRatioYAML(ratiosYaml);
        RunRatios(ratioDefs, groups, uniqueBinNames, outputDir, effsByBin, effsByProcess);
    }

    // Main plotting loop
    for(auto &gpair : groups){
        string groupKey = gpair.first;
        auto &procmap = gpair.second;
        if(procmap.empty()) continue;

        // Separate histograms
        vector<TH1*> bkgHists, sigHists, emptyHists;
        vector<string> bkgProcs, sigProcs;
        
        for(auto &pp : procmap){
            TH1* h = pp.second;
            if(!h) continue;
            const string& proc = pp.first;
        
            if(tool.BkgDict.count(proc)) {
                bkgHists.push_back(h); 
                bkgProcs.push_back(proc); 
            }
            else if(find(tool.SignalKeys.begin(), tool.SignalKeys.end(), proc) != tool.SignalKeys.end() 
                || proc.find("SMS") != std::string::npos || proc.find("Cascades") != std::string::npos) {
                sigHists.push_back(h);
                sigProcs.push_back(proc); 
            }
        }
    }

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

    cout<<"[PlotTriggers] All plots saved to "<<outRootName.Data()<<" and "<<outputDir<<"pdfs/"<<endl;
    return 0;
}
