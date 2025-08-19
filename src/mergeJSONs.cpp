#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <array>
#include <cmath>
#include <filesystem>
#include <unordered_map>

#include "SampleTool.h"
#include "BuildFitTools.h" // stringlist typedef

#include "nlohmann/json.hpp"
using json = nlohmann::json;
using stringlist = std::vector<std::string>;
namespace fs = std::filesystem;

// Helper: add totals into an array, storing squared errors for accumulation
static inline void addTotalsAcc(std::array<double,3> &dst, double cnt, double sumW, double err) {
    dst[0] += cnt;
    dst[1] += sumW;
    dst[2] += err*err; // accumulate variance
}

bool mergeJSONsFlattenedWithFileBreakdown(const std::vector<std::string> &inputFiles,
                                          const std::string &outMergedFile,
                                          const std::string &outFilesFile = "")
{
    SampleTool ST;
    ST.LoadAllFromMaster();

    // --- Build a fast key -> canonical group map (unordered for speed) ---
    std::unordered_map<std::string,std::string> keyToGroup;
    keyToGroup.reserve(ST.BkgDict.size() + ST.SigDict.size());
    for (const auto &kv : ST.BkgDict) {
        const std::string &group = kv.first;
        for (const auto &entry : kv.second) keyToGroup[entry] = group;
    }
    for (const auto &kv : ST.SigDict) {
        const std::string &group = kv.first;
        for (const auto &entry : kv.second) keyToGroup[entry] = group;
    }

    auto resolveGroupFast = [&keyToGroup](const std::string &key) -> std::string {
        auto it = keyToGroup.find(key);
        if (it != keyToGroup.end()) return it->second;
        return key; // fallback
    };

    // merged[bin][group] -> { count, sumW, var }
    std::map<std::string, std::map<std::string,std::array<double,3>>> merged;

    // filesBreakdown[bin][group][file] -> { count, sumW, var }
    std::map<std::string, std::map<std::string,std::map<std::string,std::array<double,3>>>> filesBreakdown;

    // Read all input JSONs
    for (const auto &fname : inputFiles) {
        std::ifstream ifs(fname);
        if (!ifs.is_open()) {
            std::cerr << "[mergeJSONs] Cannot open " << fname << "\n";
            return false;
        }

        json j;
        try {
            ifs >> j;
        } catch (const std::exception &e) {
            std::cerr << "[mergeJSONs] Failed to parse " << fname << ": " << e.what() << "\n";
            return false;
        }

        for (auto &binItem : j.items()) {
            const std::string binName = binItem.key();
            const json &binContent = binItem.value();

            auto &mergedBinMap = merged[binName];
            auto &filesBinMap  = filesBreakdown[binName];

            for (auto &sampleItem : binContent.items()) {
                const std::string origKey = sampleItem.key();
                const json &sampleObj = sampleItem.value();

                const std::string group = resolveGroupFast(origKey);

                // read totals defensively (fallback to 0.0)
                double cnt  = 0.0;
                double sumW = 0.0;
                double err  = 0.0;
                if (sampleObj.contains("totals") && sampleObj["totals"].is_array() && sampleObj["totals"].size() >= 3) {
                    cnt  = sampleObj["totals"][0].get<double>();
                    sumW = sampleObj["totals"][1].get<double>();
                    err  = sampleObj["totals"][2].get<double>();
                }

                // merge totals (accumulate variance)
                addTotalsAcc(mergedBinMap[group], cnt, sumW, err);

                // store per-file breakdown only if requested and present
                if (!outFilesFile.empty() && sampleObj.contains("files") && sampleObj["files"].is_object()) {
                    for (auto &fkv : sampleObj["files"].items()) {
                        const std::string fileName = fkv.key();
                        const json &fileTotals = fkv.value();
                        if (!fileTotals.is_array() || fileTotals.size() < 3) continue;

                        double fcnt  = fileTotals[0].get<double>();
                        double fsumW = fileTotals[1].get<double>();
                        double ferr  = fileTotals[2].get<double>();

                        addTotalsAcc(filesBinMap[group][fileName], fcnt, fsumW, ferr);
                    }
                }
            }
        }
    }

    // finalize errors (take sqrt of accumulated variance)
    for (auto &binPair : merged) {
        for (auto &samplePair : binPair.second) {
            samplePair.second[2] = std::sqrt(samplePair.second[2]);
        }
    }
    if (!outFilesFile.empty()) {
        for (auto &binPair : filesBreakdown) {
            for (auto &samplePair : binPair.second) {
                for (auto &filePair : samplePair.second) {
                    filePair.second[2] = std::sqrt(filePair.second[2]);
                }
            }
        }
    }

    // --- write merged totals ---
    json outMerged = json::object();
    for (const auto &binPair : merged) {
        const std::string &binName = binPair.first;
        for (const auto &samplePair : binPair.second) {
            const std::string &group = samplePair.first;
            const auto &arr = samplePair.second;
            outMerged[binName][group]["totals"] = { arr[0], arr[1], arr[2] };
        }
    }

    std::ofstream ofs1(outMergedFile);
    if (!ofs1.is_open()) {
        std::cerr << "[mergeJSONs] Cannot write to " << outMergedFile << "\n";
        return false;
    }
    ofs1 << outMerged.dump(4) << "\n";

    // --- write per-file breakdown if requested ---
    if (!outFilesFile.empty()) {
        json outFiles = json::object();
        for (const auto &binPair : filesBreakdown) {
            const std::string &binName = binPair.first;
            for (const auto &samplePair : binPair.second) {
                const std::string &group = samplePair.first;
                for (const auto &filePair : samplePair.second) {
                    const std::string &fileName = filePair.first;
                    const auto &arr = filePair.second;
                    outFiles[binName][group]["files"][fileName] = { arr[0], arr[1], arr[2] };
                }
            }
        }

        std::ofstream ofs2(outFilesFile);
        if (!ofs2.is_open()) {
            std::cerr << "[mergeJSONs] Cannot write to " << outFilesFile << "\n";
            return false;
        }
        ofs2 << outFiles.dump(4) << "\n";
    }

    return true;
}

int main(int argc, char **argv) {
    if (argc < 3 || argc > 4) {
        std::cerr << "Usage: " << argv[0] << " merged output_directory [--per_file]\n";
        return 1;
    }

    std::string outFile = argv[1];
    std::string jsonDir = argv[2];
    bool per_file = false;

    if (argc == 4) {
        std::string flag = argv[3];
        if (flag == "--per_file") {
            per_file = true;
        } else {
            std::cerr << "Unknown flag: " << flag << "\n";
            return 1;
        }
    }

    std::vector<std::string> inputs;
    for (const auto &entry : fs::directory_iterator(jsonDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            inputs.push_back(entry.path().string());
        }
    }

    if (inputs.empty()) {
        std::cerr << "[mergeJSONs] No JSON files found in " << jsonDir << "\n";
        return 2;
    }

    bool success = false;
    if (per_file) {
        success = mergeJSONsFlattenedWithFileBreakdown(inputs, outFile + ".json", outFile + "_files.json");
    } else {
        success = mergeJSONsFlattenedWithFileBreakdown(inputs, outFile + ".json", "");
    }

    if (!success) return 3;

    std::cout << "[mergeJSONs] Merged " << inputs.size() << " JSONs to " << outFile << "\n";
    if (per_file) std::cout << "[mergeJSONs] Per-file breakdown written to " << outFile << "_files.json\n";

    return 0;
}

