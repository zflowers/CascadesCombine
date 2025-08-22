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
                 "[--predefined-cuts NAME1;NAME2;...] [--hist] [--hist-yaml HISTS.yaml] [--json]\n\n";
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

static bool buildCutsForBin(BuildFitInput* bfi,
                            const std::vector<std::string>& normalCuts,
                            const std::vector<std::string>& lepCuts,
                            const std::vector<std::string>& predefinedCuts,
                            std::vector<std::string>& outCuts) 
{
    if (!bfi) return false;
    outCuts.clear();
    outCuts = normalCuts;
    for (const auto &lepCut : lepCuts) {
        std::string builtCut = bfi->BuildLeptonCut(lepCut);
        if (!builtCut.empty()) outCuts.push_back(builtCut);
    }
    for (const auto &pcut : predefinedCuts) {
        if (pcut == "Cleaning") outCuts.push_back(bfi->GetCleaningCut());
        else if (pcut == "Zstar") outCuts.push_back(bfi->GetZstarCut());
        else if (pcut == "noZstar") outCuts.push_back(bfi->GetnoZstarCut());
        else std::cerr << "[BFI_condor] Unknown predefined cut: " << pcut << "\n";
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
    std::vector<std::string> cuts, lepCuts, predefCuts;
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
            for (auto &pc : tmp) {
                if (pc == "Cleaning") h.predefCuts.push_back(BFI->GetCleaningCut());
                else if (pc == "Zstar") h.predefCuts.push_back(BFI->GetZstarCut());
                else if (pc == "noZstar") h.predefCuts.push_back(BFI->GetnoZstarCut());
            }
        }
        hists.push_back(h);
    }
    return hists;
}

// ----------------------
// Main
// ----------------------
int main(int argc, char** argv) {
    std::string binName, cutsStr, lepCutsStr, predefCutsStr, rootFilePath, outputJsonPath, sampleName, histOutputPath;
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
        {"signal", no_argument, 0, 's'},
        {"sig-type", required_argument, 0, 't'},
        {"lumi", required_argument, 0, 'u'},
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
            case 's': isSignal=true; break;
            case 't': sigType=optarg; isSignal=true; break;
            case 'u': Lumi=atof(optarg); break;
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
        auto df_scaled = df.Define("weight_scaled",[Lumi](double w){return w*Lumi;},{"weight"})
                           .Define("weight_sq_scaled",[Lumi](double w){return w*w*Lumi*Lumi;},{"weight"});
        auto df_with_lep = BFI->DefineLeptonPairCounts(df_scaled,"");
        df_with_lep = BFI->DefineLeptonPairCounts(df_with_lep,"A");
        df_with_lep = BFI->DefineLeptonPairCounts(df_with_lep,"B");
        df_with_lep = BFI->DefinePairKinematics(df_with_lep,"");
        df_with_lep = BFI->DefinePairKinematics(df_with_lep,"A");
        df_with_lep = BFI->DefinePairKinematics(df_with_lep,"B");

        ROOT::RDF::RNode node=df_with_lep;
        for(const auto &c: finalCutsExpanded) node=node.Filter(c);

        if(doHist && !histYamlPath.empty()){
            auto histDefs = loadHistogramsYAML(histYamlPath,BFI);
            for(auto &h: histDefs){
                ROOT::RDF::RNode hnode = node;
                for(const auto &c:h.cuts) hnode=hnode.Filter(BFI->ExpandMacros(c));
                for(const auto &c:h.lepCuts) hnode=hnode.Filter(BFI->ExpandMacros(c));
                for(const auto &c:h.predefCuts) hnode=hnode.Filter(BFI->ExpandMacros(c));
        
                std::string hname = binName + "__" + processName + "__" + h.name;
                if(h.type=="1D"){
                    auto hist = hnode.Histo1D({hname.c_str(),hname.c_str(),h.nbins,h.xmin,h.xmax},h.expr,"weight_scaled");
                    if(hist->Write() == 0) std::cerr << "error writing: " << hname << std::endl; // <-- write to ROOT file
                    std::cout<<"[BFI_condor] Filled histogram: "<<h.name<<"\n";
                }else if(h.type=="2D"){
                    auto hist = hnode.Histo2D({hname.c_str(),hname.c_str(),h.nbins,h.xmin,h.xmax,h.nybins,h.ymin,h.ymax},
                                              h.expr,h.yexpr,"weight_scaled");
                    if(hist->Write() == 0) std::cerr << "error writing: " << hname << std::endl; // <-- write to ROOT file
                    std::cout<<"[BFI_condor] Filled 2D histogram: "<<h.name<<"\n";
                }
            }
        }

        if(doJSON){
            auto cnt = node.Count();
            auto sumW = node.Sum<double>("weight_scaled");
            auto sumW2 = node.Sum<double>("weight_sq_scaled");
            unsigned long long n_entries = cnt.GetValue();
            double sW = sumW.GetValue();
            double sW2 = sumW2.GetValue();
            double err = (sW2>=0)?std::sqrt(sW2):0.0;
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

