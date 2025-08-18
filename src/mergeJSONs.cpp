#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <array>
#include <cmath>

#include "SampleTool.h"
#include "BuildFitTools.h" // stringlist typedef

#include "nlohmann/json.hpp"
using json = nlohmann::json;
using stringlist = std::vector<std::string>;

// Helper: pick canonical group name using SampleTool
static std::string resolveGroup(SampleTool &ST, const std::string &key, const std::string &repFile) {
    for (const auto &kv : ST.BkgDict) {
        const auto &group = kv.first;
        for (const auto &entry : kv.second) {
            if (entry.find(key) != std::string::npos) return group;
        }
    }
    for (const auto &kv : ST.SigDict) {
        const auto &group = kv.first;
        for (const auto &entry : kv.second) {
            if (entry.find(key) != std::string::npos) return group;
        }
    }
    return key; // fallback
}

bool mergeJSONsFlattenedWithFileBreakdown(const std::vector<std::string> &inputFiles,
                                          const std::string &outMergedFile,
                                          const std::string &outFilesFile)
{
    SampleTool ST;
    stringlist bkglist = {"ttbar","ST","DY","ZInv","DBTB","QCD","Wjets"};
    stringlist siglist = {"Cascades"};
    ST.LoadBkgs(bkglist);
    ST.LoadSigs(siglist);

    // bin -> sample -> [count,sumW,err^2]
    std::map<std::string, std::map<std::string,std::array<double,3>>> merged;

    // bin -> sample -> file -> [count,sumW,err^2]
    std::map<std::string, std::map<std::string,std::map<std::string,std::array<double,3>>>> filesBreakdown;

    for (const auto &fname : inputFiles) {
        std::ifstream ifs(fname);
        if (!ifs.is_open()) {
            std::cerr << "[mergeJSONs] Cannot open " << fname << "\n";
            return false;
        }
        json j;
        ifs >> j;

        for (auto &binItem : j.items()) {
            const std::string &binName = binItem.key();
            const json &binContent = binItem.value();

            auto &binMap = merged[binName];
            auto &filesBinMap = filesBreakdown[binName];

            for (auto &sampleItem : binContent.items()) {
                std::string origKey = sampleItem.key();
                const json &sampleObj = sampleItem.value();

                std::string repFile;
                if (sampleObj.contains("files") && !sampleObj["files"].empty()) {
                    repFile = sampleObj["files"].begin().key();
                }

                std::string group = resolveGroup(ST, origKey, repFile);

                double cnt  = sampleObj["totals"][0].get<double>();
                double sumW = sampleObj["totals"][1].get<double>();
                double err  = sampleObj["totals"][2].get<double>();

                // merge totals
                auto &arr = binMap[group];
                arr[0] += cnt;
                arr[1] += sumW;
                arr[2] += err*err;

                // store per-file breakdown
                if (sampleObj.contains("files")) {
                    auto &fileMap = filesBinMap[group];
                    for (auto &fkv : sampleObj["files"].items()) {
                        std::string fname = fkv.key();
                        double fcnt = fkv.value()[0].get<double>();
                        double fsumW = fkv.value()[1].get<double>();
                        double ferr = fkv.value()[2].get<double>();
                        auto &fileArr = fileMap[fname];
                        fileArr[0] += fcnt;
                        fileArr[1] += fsumW;
                        fileArr[2] += ferr*ferr;
                    }
                }
            }
        }
    }

    // finalize errors
    for (auto &binPair : merged) {
        for (auto &samplePair : binPair.second) {
            samplePair.second[2] = std::sqrt(samplePair.second[2]);
        }
    }
    for (auto &binPair : filesBreakdown) {
        for (auto &samplePair : binPair.second) {
            for (auto &filePair : samplePair.second) {
                filePair.second[2] = std::sqrt(filePair.second[2]);
            }
        }
    }

    // write merged totals
    json outMerged;
    for (const auto &binPair : merged) {
        const std::string &binName = binPair.first;
        for (const auto &samplePair : binPair.second) {
            outMerged[binName][samplePair.first] = {samplePair.second[0],
                                                    samplePair.second[1],
                                                    samplePair.second[2]};
        }
    }
    std::ofstream ofs1(outMergedFile);
    if (!ofs1.is_open()) return false;
    ofs1 << outMerged.dump(4) << "\n";

    // write per-file breakdown
    json outFiles;
    for (const auto &binPair : filesBreakdown) {
        const std::string &binName = binPair.first;
        for (const auto &samplePair : binPair.second) {
            for (const auto &filePair : samplePair.second) {
                outFiles[binName][samplePair.first]["files"][filePair.first] = {filePair.second[0],
                                                                                filePair.second[1],
                                                                                filePair.second[2]};
            }
        }
    }
    std::ofstream ofs2(outFilesFile);
    if (!ofs2.is_open()) return false;
    ofs2 << outFiles.dump(4) << "\n";

    return true;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " merged input1.json [input2.json ...]\n";
        return 1;
    }
    std::string outFile = argv[1];
    std::vector<std::string> inputs;
    for (int i=2;i<argc;++i) inputs.push_back(argv[i]);

    if (!mergeJSONsFlattenedWithFileBreakdown(inputs, outFile+".json", outFile+"_files.json")) return 2;
    std::cout << "[mergeJSONs] Merged " << inputs.size() << " JSONs to " << outFile << "\n";
    return 0;
}

