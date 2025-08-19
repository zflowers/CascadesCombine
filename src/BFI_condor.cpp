// src/BFI_condor.cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <string>
#include <getopt.h>
#include "TFile.h"
#include "BuildFitInput.h"
#include "SampleTool.h"
#include "BuildFitTools.h"

// ----------------------
// Helpers
// ----------------------

static void usage(const char* me) {
    std::cerr << "Usage: " << me 
              << " --bin BINNAME --file ROOTFILE --output OUT.json "
                 "[--cuts CUT1,CUT2,...] [--lep-cuts LEPCUT1,LEPCUT2,...] "
                 "[--predefined-cuts NAME1,NAME2,...]\n\n";

    std::cerr << "Required arguments:\n";
    std::cerr << "  --bin        Name of the bin to process (e.g. TEST)\n";
    std::cerr << "  --file       Path to one ROOT file to process\n";
    std::cerr << "  --output     Path to write partial JSON output\n\n";

    std::cerr << "Optional arguments:\n";
    std::cerr << "  --cuts               Comma-separated list of normal tree cuts "
                 "(e.g. MET>=150,PTISR>=250)\n";
    std::cerr << "  --lep-cuts           Comma-separated list of cuts that should be "
                 "passed to BuildLeptonCut (e.g. >=1OSSF,=2Pos)\n";
    std::cerr << "  --predefined-cuts    Comma-separated list of predefined cuts by name "
                 "(e.g. Cleaning,ZStar)\n";
    std::cerr << "  --help               Display this help message\n";
}

static bool buildCutsForBin(BuildFitInput* bfi,
                            const std::vector<std::string>& normalCuts,
                            const std::vector<std::string>& lepCuts,
                            const std::vector<std::string>& predefinedCuts,
                            std::vector<std::string>& outCuts) {
    if (!bfi) return false;

    outCuts.clear();

    // 1) Add normal tree cuts
    outCuts = normalCuts;

    // 2) Add lepton cuts processed via BuildLeptonCut
    for (const auto &lepCut : lepCuts) {
        std::string builtCut = bfi->BuildLeptonCut(lepCut);
        if (!builtCut.empty()) outCuts.push_back(builtCut);
    }

    // 3) Map predefined cut names to BFI functions
    for (const auto &pcut : predefinedCuts) {
        if (pcut == "Cleaning") {
            std::string c = bfi->GetCleaningCut();
            if (!c.empty()) outCuts.push_back(c);
        } else if (pcut == "ZStar") {
            std::string z = bfi->GetZstarCut();
            if (!z.empty()) outCuts.push_back(z);
        } else {
            std::cerr << "[BFI_condor] Unknown predefined cut: " << pcut << "\n";
        }
    }

    return true;
}

inline std::string GetSampleNameFromKey(const std::string& keyOrPath) {
    // If keyOrPath is a full path, strip directory and extension
    size_t lastSlash = keyOrPath.find_last_of("/\\");
    std::string name = (lastSlash == std::string::npos) ? keyOrPath : keyOrPath.substr(lastSlash + 1);

    // Remove ROOT extension if present
    size_t dot = name.rfind(".root");
    if (dot != std::string::npos) name = name.substr(0, dot);

    return name;
}

// Writes JSON like:
// { "BINNAME": { "Sample": { "files": { "file1.root": [...], "file2.root": [...] }, "totals": [...] } } }
static bool writePartialJSON(const std::string& outPath,
                             const std::string& binname,
                             const std::map<std::string, std::map<std::string, std::array<double,3>>>& fileResults,
                             const std::map<std::string, std::array<double,3>>& totals) 
{
    std::ofstream ofs(outPath);
    if (!ofs) return false;

    ofs << "{\n";
    ofs << "  \"" << binname << "\": {\n";

    bool firstSample = true;
    for (const auto &kv : totals) {
        if (!firstSample) ofs << ",\n";
        firstSample = false;

        const std::string &sname = kv.first;
        std::string sampleId = GetSampleNameFromKey(sname); 
        const auto &totalVals = kv.second;

        ofs << "    \"" << sampleId << "\": {\n";

        // per-file breakdown
        ofs << "      \"files\": {\n";
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

        // totals
        ofs << "      \"totals\": ["
            << (long long)totalVals[0] << ", "
            << totalVals[1] << ", "
            << totalVals[2] << "]\n";

        ofs << "    }"; // close sample object
    }

    ofs << "\n  }\n}\n";
    ofs.close();
    return true;
}

// ----------------------
// Main
// ----------------------
int main(int argc, char** argv) {
    // ----- parse args (extended) -----
    std::string binName;
    std::string cutsStr;
    std::string lepCutsStr;
    std::string predefCutsStr;
    std::string rootFilePath;
    std::string outputJsonPath;
    std::string sampleName;
    bool isSignal = false;
    std::string sigType; // "cascades" or "sms" (optional)
    double Lumi = 1.0;

    static struct option long_options[] = {
        {"bin", required_argument, 0, 'b'},
        {"file", required_argument, 0, 'f'},
        {"output", required_argument, 0, 'o'},
        {"cuts", required_argument, 0, 'c'},
        {"lep-cuts", required_argument, 0, 'l'},
        {"predefined-cuts", required_argument, 0, 'p'},
        {"signal", no_argument, 0, 's'},
        {"sig-type", required_argument, 0, 't'},
        {"lumi", required_argument, 0, 'u'},
        {"sample-name", required_argument, 0, 'n'},
        {"help", no_argument, 0, 'h'},
        {0,0,0,0}
    };

    int opt;
    int opt_index = 0;
    while ((opt = getopt_long(argc, argv, "b:f:o:c:l:p:st:u:n:h", long_options, &opt_index)) != -1) {
        switch (opt) {
            case 'b': binName = optarg; break;
            case 'f': rootFilePath = optarg; break;
            case 'o': outputJsonPath = optarg; break;
            case 'c': cutsStr = optarg; break;
            case 'l': lepCutsStr = optarg; break;
            case 'p': predefCutsStr = optarg; break;
            case 's': isSignal = true; break;
            case 't': sigType = optarg; isSignal = true; break;
            case 'u': Lumi = atof(optarg); break;
            case 'n': sampleName = optarg; break;
            case 'h':
            default:
                usage(argv[0]);
                return 1;
        }
    }

    // If sample name not provided, default to file stem
    if (sampleName.empty()) {
        std::string fname = rootFilePath;
        size_t lastSlash = fname.find_last_of("/\\");
        if (lastSlash != std::string::npos) fname = fname.substr(lastSlash + 1);
        size_t dot = fname.find_last_of('.');
        if (dot != std::string::npos) fname = fname.substr(0, dot);
        sampleName = GetSampleNameFromKey(fname);
    }

    if (outputJsonPath.empty()) {
        std::string sampleId = isSignal
                               ? (sigType == "cascades" ? BFTool::GetSignalTokensCascades(rootFilePath)
                                                        : "SMS")
                               : sampleName;

        outputJsonPath = binName + "_" + sampleId + ".json";
    }

    if (binName.empty() || rootFilePath.empty() || outputJsonPath.empty()) {
        usage(argv[0]);
        return 1;
    }

    // Construct BFI early because buildCutsForBin and later calls use it
    BuildFitInput* BFI = nullptr;
    try { BFI = new BuildFitInput(); }
    catch (...) { std::cerr << "[BFI_condor] Failed to construct BuildFitInput\n"; return 3; }

    // helper to split CSV -> vector<string>
    auto splitString = [](const std::string& s, char delim) -> std::vector<std::string> {
        std::vector<std::string> elems;
        if (s.empty()) return elems;
        std::stringstream ss(s);
        std::string item;
        while (std::getline(ss, item, delim)) {
            if (!item.empty()) elems.push_back(item);
        }
        return elems;
    };

    std::vector<std::string> cutsVec    = splitString(cutsStr, ',');
    std::vector<std::string> lepCutsVec = splitString(lepCutsStr, ',');
    std::vector<std::string> predefCutsVec = splitString(predefCutsStr, ',');

    // Build finalCuts using BFI (builds lepton cuts and maps predefined cuts)
    std::vector<std::string> finalCuts;
    if (!buildCutsForBin(BFI, cutsVec, lepCutsVec, predefCutsVec, finalCuts)) {
        std::cerr << "[BFI_condor] Failed to build final cuts\n";
        delete BFI;
        return 2;
    }

    // Auto-detect sig type if needed
    if (isSignal && sigType.empty()) {
        if (rootFilePath.find("SMS") != std::string::npos || rootFilePath.find("sms") != std::string::npos)
            sigType = "sms";
        else
            sigType = "cascades";
    }

    // Prepare results structures
    std::map<std::string, std::map<std::string, std::array<double,3>>> fileResults; // sample -> file -> [count, sum, err]
    std::map<std::string, std::array<double,3>> totals;                             // sample -> total [count, sum, err]
    
    // ---------- PROCESS BACKGROUND ----------
    if (!isSignal) {
        std::string tree_name = "KUAnalysis";
        ROOT::RDataFrame df(tree_name, rootFilePath);
    
        auto df_scaled = df
            .Define("weight_scaled", [Lumi](double w){ return w * Lumi; }, {"weight"})
            .Define("weight_sq_scaled", [Lumi](double w){ return (w*Lumi)*(w*Lumi); }, {"weight"});
    
        auto df_with_lep = BFI->DefineLeptonPairCounts(df_scaled, "");
        df_with_lep = BFI->DefineLeptonPairCounts(df_with_lep, "A");
        df_with_lep = BFI->DefineLeptonPairCounts(df_with_lep, "B");
    
        df_with_lep = BFI->DefinePairKinematics(df_with_lep, "");
        df_with_lep = BFI->DefinePairKinematics(df_with_lep, "A");
        df_with_lep = BFI->DefinePairKinematics(df_with_lep, "B");
    
        ROOT::RDF::RNode node = df_with_lep;
        for (const auto &c : finalCuts) node = node.Filter(c);
    
        auto cnt = node.Count();
        auto sumW = node.Sum<double>("weight_scaled");
        auto sumW2 = node.Sum<double>("weight_sq_scaled");
    
        unsigned long long n_entries = cnt.GetValue();
        double sW = sumW.GetValue();
        double sW2 = sumW2.GetValue();
        double err = (sW2 >= 0) ? std::sqrt(sW2) : 0.0;
    
        // store per-file
        fileResults[sampleName][rootFilePath] = { (double)n_entries, sW, err };
        // update totals (sum counts and weights, sum squared errors)
        auto &tot = totals[sampleName];
        tot[0] += (double)n_entries;
        tot[1] += sW;
        tot[2] += err*err;
    }
    
    // ---------- PROCESS SIGNAL ----------
    else {
        if (sigType == "cascades") {
            std::string subkey = BFTool::GetSignalTokensCascades(rootFilePath);
            std::string tree_name = "KUAnalysis";
            ROOT::RDataFrame df(tree_name, rootFilePath);
    
            auto df_scaled = df
                .Define("weight_scaled", [Lumi](double w){ return w * Lumi; }, {"weight"})
                .Define("weight_sq_scaled", [Lumi](double w){ return (w*Lumi)*(w*Lumi); }, {"weight"});
    
            auto df_with_lep = BFI->DefineLeptonPairCounts(df_scaled, "");
            df_with_lep = BFI->DefineLeptonPairCounts(df_with_lep, "A");
            df_with_lep = BFI->DefineLeptonPairCounts(df_with_lep, "B");
    
            df_with_lep = BFI->DefinePairKinematics(df_with_lep, "");
            df_with_lep = BFI->DefinePairKinematics(df_with_lep, "A");
            df_with_lep = BFI->DefinePairKinematics(df_with_lep, "B");
    
            ROOT::RDF::RNode node = df_with_lep;
            for (const auto &c : finalCuts) node = node.Filter(c);
    
            auto cnt = node.Count();
            auto sumW = node.Sum<double>("weight_scaled");
            auto sumW2 = node.Sum<double>("weight_sq_scaled");
    
            unsigned long long n_entries = cnt.GetValue();
            double sW = sumW.GetValue();
            double sW2 = sumW2.GetValue();
            double err = (sW2 >= 0) ? std::sqrt(sW2) : 0.0;
    
            std::string name = subkey.empty() ? sampleName : subkey;
            fileResults[name][rootFilePath] = { (double)n_entries, sW, err };
            auto &tot = totals[name];
            tot[0] += (double)n_entries;
            tot[1] += sW;
            tot[2] += err*err;
    
        } else if (sigType == "sms") {
            auto tree_names = BFTool::GetSignalTokensSMS(rootFilePath);
            for (const auto &tree_name : tree_names) {
                ROOT::RDataFrame df(tree_name, rootFilePath);
    
                auto df_scaled = df
                    .Define("weight_scaled", [Lumi](double w){ return w * Lumi; }, {"weight"})
                    .Define("weight_sq_scaled", [Lumi](double w){ return (w*Lumi)*(w*Lumi); }, {"weight"});
    
                auto df_with_lep = BFI->DefineLeptonPairCounts(df_scaled, "");
                df_with_lep = BFI->DefineLeptonPairCounts(df_with_lep, "A");
                df_with_lep = BFI->DefineLeptonPairCounts(df_with_lep, "B");
    
                df_with_lep = BFI->DefinePairKinematics(df_with_lep, "");
                df_with_lep = BFI->DefinePairKinematics(df_with_lep, "A");
                df_with_lep = BFI->DefinePairKinematics(df_with_lep, "B");
    
                ROOT::RDF::RNode node = df_with_lep;
                for (const auto &c : finalCuts) node = node.Filter(c);
    
                auto cnt = node.Count();
                auto sumW = node.Sum<double>("weight_scaled");
                auto sumW2 = node.Sum<double>("weight_sq_scaled");
    
                unsigned long long n_entries = cnt.GetValue();
                double sW = sumW.GetValue();
                double sW2 = sumW2.GetValue();
                double err = (sW2 >= 0) ? std::sqrt(sW2) : 0.0;
    
                fileResults[tree_name][rootFilePath] = { (double)n_entries, sW, err };
                auto &tot = totals[tree_name];
                tot[0] += (double)n_entries;
                tot[1] += sW;
                tot[2] += err*err;
            }
        } else {
            std::cerr << "[BFI_condor] Unknown sig-type: " << sigType << "\n";
            delete BFI;
            return 4;
        }
    }
    
    // Convert summed squared errors to sqrt
    for (auto &kv : totals) kv.second[2] = std::sqrt(kv.second[2]);
    
    // Write JSON
    if (!writePartialJSON(outputJsonPath, binName, fileResults, totals)) {
        std::cerr << "[BFI_condor] ERROR writing JSON to " << outputJsonPath << "\n";
        delete BFI;
        return 5;
    } else {
        std::cout << "Wrote output to: " << outputJsonPath << std::endl;
    }

    delete BFI;
    return 0;
}

