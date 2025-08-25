#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <string>
#include <cmath>
#include <memory>
#include <filesystem>

#include "yaml-cpp/yaml.h"

#include "SampleTool.h"
#include "DefineUserHists.h"

namespace fs = std::filesystem;

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

static bool buildCutsForBin(BuildFitInput* BFI,
                            const std::vector<std::string>& normalCuts,
                            const std::vector<std::string>& lepCuts,
                            const std::vector<std::string>& predefinedCuts,
                            std::vector<std::string>& outCuts) 
{
    if (!BFI) return false;
    outCuts.clear();
    outCuts = normalCuts;
    for (const auto &lepCut : lepCuts) {
        std::string builtCut = BFI->BuildLeptonCut(lepCut);
        if (!builtCut.empty()) outCuts.push_back(builtCut);
    }
    for (const auto &pcut : predefinedCuts) {
        std::string cut;
        if (BFI->GetCutByName(pcut, cut))
          outCuts.push_back(cut);
        else
          std::cerr << "[BFI_condor] Unknown predefined cut: " << pcut << "\n";
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

static std::vector<DerivedVar> loadDerivedVariablesYAML(const std::string &yamlPath) {
    std::vector<DerivedVar> vars;
    YAML::Node root = YAML::LoadFile(yamlPath);
    if(!root["derived_variables"]) return vars;

    for(const auto &vnode : root["derived_variables"]) {
        DerivedVar dv;
        dv.name = vnode["name"].as<std::string>();
        dv.expr = vnode["expr"].as<std::string>();
        vars.push_back(dv);
    }
    return vars;
}
