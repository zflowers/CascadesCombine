// PlotYields.cpp
#include "PlottingTools.h"
#include <nlohmann/json.hpp>
#include <fstream>
using json = nlohmann::json;

// ----------------------
// Main
// ----------------------
int main(int argc, char* argv[]) {
    string inputFile;
    for(int i=1;i<argc;++i){
        string arg=argv[i];
        if(arg=="-i"||arg=="--input"){ if(i+1<argc) inputFile=argv[++i]; else{ cerr<<"[ERROR] Missing "<<arg<<endl; return 1;} }
        else if(arg=="-o"||arg=="--output"){ if(i+1<argc) outputDir=argv[++i]; else{ cerr<<"[ERROR] Missing "<<arg<<endl; return 1;} }
        // lumi used when filling histograms upstream; doesn't rescale, just need for labels
        else if(arg=="-l"||arg=="--lumi"){ if(i+1<argc) lumi=std::stoi(argv[++i]); else{ lumi = 1.;} }
        else if(arg=="--help"){ cout<<"[PlotYields] Usage: "<<argv[0]<<" [options]\n -i <file.root>\n -h <hist.yaml>\n -d <process.yaml>\n -b <bins.yaml>\n"; return 0; }
        else{ cerr<<"[ERROR] Unknown arg "<<arg<<endl; return 1;}
    }
    if(inputFile.empty()){ cerr<<"[ERROR] No input JSON file provided.\n"; return 1; }

    // Build outputDir safely
    if(outputDir.empty()){
        if(outputDir.empty()) outputDir = "output";
        outputDir += "/";
    } else {
        // if outputDir already set, ensure trailing slash
        if(outputDir.back() != '/') outputDir += '/';
    }

    gSystem->mkdir(outputDir.c_str(), kTRUE);
    gSystem->mkdir((outputDir+"pdfs").c_str(), kTRUE);

    TString outRootName=Form("%soutput_2DYields.root",outputDir.c_str());
    outFile=new TFile(outRootName,"RECREATE");

    // Extract Yields from JSON
    std::ifstream jsonFile(inputFile);
    if(!jsonFile.is_open()){ 
        cerr << "[ERROR] Could not open JSON file: " << inputFile << endl; 
        return 1; 
    }
    json j;
    jsonFile >> j;

    // Make map from JSON yields
    std::map<std::string, std::map<std::string, TH1*>> cutflowMap;
    for (auto& [binName, procMap] : j.items()) {
        // Loop over processes
        for (auto& [procName, values] : procMap.items()) {
            if (!values.is_array() || values.size() < 3) {
                cerr << "[WARN] Skipping " << binName << " : " << procName << " (bad format)" << endl;
                continue;
            }
            double weighted = values[1].get<double>(); // weighted events
            double err      = values[2].get<double>(); // stat error
            // Create a 1-bin histogram
            TH1F* h = new TH1F(Form("%s__%s", binName.c_str(), procName.c_str()), "", 1, 0, 1);
            h->SetBinContent(1, weighted);
            h->SetBinError(1, err);
            cutflowMap[binName][procName] = h;
        }
    }
    
    // build global 2D cutflows from cutflowMap
    MakeAndPlotCutflow2D(cutflowMap, "GlobalCutflow", "yield", 1.0);
    MakeAndPlotCutflow2D(cutflowMap, "GlobalCutflow", "SoB",   1.0);
    MakeAndPlotCutflow2D(cutflowMap, "GlobalCutflow", "SoverSqrtB", 1.0);
    MakeAndPlotCutflow2D(cutflowMap, "GlobalCutflow", "Zbi", 1.0); // 1% systematic

    outFile->Close();

    cout<<"[PlotYields] All plots saved to "<<outRootName.Data()<<" and "<<outputDir<<"pdfs/"<<endl;
    return 0;
}
