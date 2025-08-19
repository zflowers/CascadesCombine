#include "nlohmann/json.hpp"
#include <filesystem>
#include <unordered_map>
#include <cmath>
#include <fstream>
#include <iostream>
#include <array>
#include <map>
#include <vector>
#include <string>
#include "SampleTool.h"

using json = nlohmann::json;
namespace fs = std::filesystem;

bool mergeJSONsFlattenedWithFileBreakdown(const std::vector<std::string> &inputFiles,
                                          const std::string &outMergedFile,
                                          const std::string &outFilesFile = "")
{
    SampleTool ST;
    ST.LoadAllFromMaster();

    auto resolveGroup = [&ST](const std::string &jsonKey) -> std::string {
        std::string keyBase = fs::path(jsonKey).filename().string();
    
        for (const auto &kv : ST.MasterDict) {       // kv.first = canonical group
            for (const auto &entry : kv.second) {    // entry = full path
                std::string entryBase = fs::path(entry).filename().string();
    
                // Use starts-with match instead of find anywhere
                if (keyBase.rfind(entryBase.substr(0, entryBase.find("_")), 0) == 0)
                    return kv.first;
            }
        }
        return jsonKey; // fallback
    };

    // merged[bin][group] -> { count, sumW, var }
    std::map<std::string, std::map<std::string,std::array<double,3>>> merged;

    // filesBreakdown[bin][group][file] -> { count, sumW, var }
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

                std::string group = resolveGroup(origKey);

                double cnt  = sampleObj["totals"][0].get<double>();
                double sumW = sampleObj["totals"][1].get<double>();
                double err  = sampleObj["totals"][2].get<double>();

                // merge totals
                auto &arr = binMap[group];
                arr[0] += cnt;
                arr[1] += sumW;
                arr[2] += err*err;

                // store per-file breakdown only if requested
                if (!outFilesFile.empty() && sampleObj.contains("files")) {
                    auto &fileMap = filesBinMap[group];
                    for (auto &fkv : sampleObj["files"].items()) {
                        std::string fname = fkv.key();
                        double fcnt  = fkv.value()[0].get<double>();
                        double fsumW = fkv.value()[1].get<double>();
                        double ferr  = fkv.value()[2].get<double>();
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
    if (!outFilesFile.empty()) {
        for (auto &binPair : filesBreakdown) {
            for (auto &samplePair : binPair.second) {
                for (auto &filePair : samplePair.second) {
                    filePair.second[2] = std::sqrt(filePair.second[2]);
                }
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
    if (!outFilesFile.empty()) {
        json outFiles;
        for (const auto &binPair : filesBreakdown) {
            const std::string &binName = binPair.first;
            for (const auto &samplePair : binPair.second) {
                for (const auto &filePair : samplePair.second) {
                    outFiles[binName][samplePair.first][filePair.first] = {filePair.second[0],
                                                                           filePair.second[1],
                                                                           filePair.second[2]};
                }
            }
        }
        std::ofstream ofs2(outFilesFile);
        if (!ofs2.is_open()) return false;
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
    bool per_file = (argc == 4 && std::string(argv[3]) == "--per_file");

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

    bool success = per_file ?
        mergeJSONsFlattenedWithFileBreakdown(inputs, outFile + ".json", outFile + "_files.json") :
        mergeJSONsFlattenedWithFileBreakdown(inputs, outFile + ".json", "");

    if (!success) return 3;

    std::cout << "[mergeJSONs] Merged " << inputs.size() << " JSONs to " << outFile << "\n";
    if (per_file) std::cout << "[mergeJSONs] Per-file breakdown written to " << outFile << "_files.json\n";

    return 0;
}
