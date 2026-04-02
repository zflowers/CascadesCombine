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
    string patternFile;
    for(int i=1;i<argc;++i){
        string arg=argv[i];
        if(arg=="-i"||arg=="--input"){ if(i+1<argc) inputFile=argv[++i]; else{ cerr<<"[ERROR] Missing "<<arg<<endl; return 1;} }
        else if(arg=="-o"||arg=="--output"){ if(i+1<argc) outputDir=argv[++i]; else{ cerr<<"[ERROR] Missing "<<arg<<endl; return 1;} }
        // lumi used when filling histograms upstream; doesn't rescale, just need for labels
        else if(arg=="-l"||arg=="--lumi"){ if(i+1<argc) lumi=std::stoi(argv[++i]); else{ lumi = 1.;} }
        else if(arg=="--help"){ cout<<"[PlotYields] Usage: "<<argv[0]<<" [options]\n -i <file.root>\n -h <hist.yaml>\n -d <process.yaml>\n -b <bins.yaml>\n"; return 0; }
        else if(arg=="--config"){
            if(i+1<argc) patternFile = argv[++i];
            else { cerr<<"[ERROR] Missing --config\n"; return 1; }
        }
        else{ cerr<<"[ERROR] Unknown arg "<<arg<<endl; return 1;}
    }
    if(inputFile.empty()){ cerr<<"[ERROR] No input JSON file provided.\n"; return 1; }
    if(patternFile.empty()){ cerr << "[PlotYields] NEED TO SUPPLY CONFIG FILE FOR RULES\n"; return 1; }

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

    gStyle->SetOptStat(0); gStyle->SetOptTitle(0);
    loadFormatMaps();
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

    vector<string> allBinNames;
    for (auto &[bin, _] : j.items())
        allBinNames.push_back(bin);
    
    YamlConfig cfg = LoadYamlConfig(patternFile);
    vector<MergedBinGroup> groups =
        BuildMergedBinGroupsFromYaml(allBinNames, cfg);

    bool hasData = false;
    // Make map from JSON yields
    std::map<std::string, std::map<std::string, TH1*>> cutflowMap;
    std::map<std::string, std::map<std::string, TH1*>> cutflowMap_raw;
    for (auto& [binName, procMap] : j.items()) {
        // Loop over processes
        for (auto& [procName, values] : procMap.items()) {
            if (procName.find("data") != std::string::npos || procName.find("Data") != std::string::npos) hasData = true;

            // Expect: { "nominal":[...], "systematics":{...} }
            if (!values.contains("nominal")) {
                cerr << "[WARN] Skipping " << binName << " : " << procName << " (missing nominal)" << endl;
                continue;
            }
            
            const auto& nom = values["nominal"];
            if (!nom.is_array() || nom.size() < 3) {
                cerr << "[WARN] Bad nominal format for " << binName << " : " << procName << endl;
                continue;
            }
            
            double raw      = nom[0].get<double>();
            double weighted = nom[1].get<double>(); // sumW
            double err      = nom[2].get<double>(); // stat error

            // Create a 1-bin histogram
            TH1F* h = new TH1F(Form("%s__%s", binName.c_str(), procName.c_str()), Form("%s__%s", binName.c_str(), procName.c_str()), 1, 0, 1);
            h->SetBinContent(1, weighted);
            h->SetBinError(1, err);
            cutflowMap[binName][procName] = h;
            TH1F* h_raw = new TH1F(Form("%s__%s_raw", binName.c_str(), procName.c_str()), Form("%s__%s_raw", binName.c_str(), procName.c_str()), 1, 0, 1);
            h_raw->SetBinContent(1, raw);
            h_raw->SetBinError(1, err);
            cutflowMap_raw[binName][procName] = h_raw;
        }
    }
    
    auto mergedCutflows = BuildMergedJsonCutflow(cutflowMap, groups, cfg);
    auto mergedCutflows_raw = BuildMergedJsonCutflow(cutflowMap_raw, groups, cfg);
    for (const auto &pair : mergedCutflows) {
        const std::string &grpName = pair.first;
        const auto &groupCutflowMap = pair.second; // map<binName, map<proc,TH1*>>
    
        MakeAndPlotCutflow2D(groupCutflowMap, grpName, "yield", 1.0);
        MakeAndPlotCutflow2D(groupCutflowMap, grpName, "SoB",   1.0);
        MakeAndPlotCutflow2D(groupCutflowMap, grpName, "SoverSqrtB", 1.0);
        MakeAndPlotCutflow2D(groupCutflowMap, grpName, "effective", 1.0);
        if (!hasData)
            MakeAndPlotCutflow2D(groupCutflowMap, grpName, "Zbi", 3.0); // 3% systematic
    }
    for (const auto &pair : mergedCutflows_raw) {
        const std::string &grpName = pair.first;
        const auto &groupCutflowMap_raw = pair.second; // map<binName, map<proc,TH1*>>
        MakeAndPlotCutflow2D(groupCutflowMap_raw, grpName, "raw", 1.0);
    }

    outFile->Close();

    cout<<"[PlotYields] All plots saved to "<<outRootName.Data()<<" and "<<outputDir<<"pdfs/"<<endl;
    return 0;
}
