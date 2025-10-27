// mergeJSONs.cpp
#include <iostream>
#include <fstream>
#include <filesystem>
#include <map>
#include <array>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "SampleTool.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

/*
 * Merge flattened JSONs that use the 6-element totals layout:
 * [ count, sumW, err = sqrt(sumW2), sumG, sumG2, var = sum(err^2) ]
 *
 * Produces merged totals with the same layout, where:
 * - totals[2] is set to sqrt(totals[5]) (absolute error)
 * - totals[5] is the accumulated variance (Σ err^2)
 *
 * If outFilesFile is provided, also writes a per-file breakdown with the same layout.
 */
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
                auto pos = entryBase.find("_");
                std::string prefix = (pos == std::string::npos) ? entryBase : entryBase.substr(0, pos);
                if (!prefix.empty() && keyBase.rfind(prefix, 0) == 0)
                    return kv.first;
            }
        }
        return jsonKey; // fallback
    };

    // merged[bin][group] -> { count, sumW, err (sqrt var), sumG, sumG2, var }
    std::map<std::string, std::map<std::string,std::array<double,6>>> merged;

    // filesBreakdown[bin][group][file] -> { count, sumW, err, sumG, sumG2, var }
    std::map<std::string, std::map<std::string,std::map<std::string,std::array<double,6>>>> filesBreakdown;

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
            std::cerr << "[mergeJSONs] JSON parse error in " << fname << ": " << e.what() << "\n";
            return false;
        }

        for (auto &binItem : j.items()) {
            const std::string &binName = binItem.key();
            const json &binContent = binItem.value();

            auto &binMap = merged[binName];
            auto &filesBinMap = filesBreakdown[binName];

            for (auto &sampleItem : binContent.items()) {
                std::string origKey = sampleItem.key();
                const json &sampleObj = sampleItem.value();

                std::string group = resolveGroup(origKey);

                // Expect sampleObj["totals"] to be at least 5 elements:
                // [cnt, sumW, err, sumG, sumG2] or the full 6-element layout.
                const json &totalsJson = sampleObj["totals"];

                double cnt   = 0.0;
                double sumW  = 0.0;
                double err   = 0.0; // sqrt(sumW2)
                double sumG  = 0.0;
                double sumG2 = 0.0;
                double var   = 0.0; // variance (we'll accumulate)

                if (totalsJson.is_array()) {
                    if (totalsJson.size() > 0 && !totalsJson[0].is_null()) cnt = totalsJson[0].get<double>();
                    if (totalsJson.size() > 1 && !totalsJson[1].is_null()) sumW = totalsJson[1].get<double>();
                    if (totalsJson.size() > 2 && !totalsJson[2].is_null()) err = totalsJson[2].get<double>();
                    if (totalsJson.size() > 3 && !totalsJson[3].is_null()) sumG = totalsJson[3].get<double>();
                    if (totalsJson.size() > 4 && !totalsJson[4].is_null()) sumG2 = totalsJson[4].get<double>();
                    if (totalsJson.size() > 5 && !totalsJson[5].is_null()) var = totalsJson[5].get<double>();
                    // If input only provides err (sqrt(sumW2)) and not var, set var = err^2
                    if (var == 0.0) var = err * err;
                } else {
                    std::cerr << "[mergeJSONs] Unexpected totals format for " << origKey << " in " << fname << "\n";
                    continue;
                }

                // merge totals
                auto &arr = binMap[group]; // default-initialized array<double,6> zeros
                arr[0] += cnt;
                arr[1] += sumW;
                arr[5] += var;   // accumulate raw variance
                arr[3] += sumG;
                arr[4] += sumG2;

                // store per-file breakdown only if requested
                if (!outFilesFile.empty() && sampleObj.contains("files")) {
                    auto &fileMap = filesBinMap[group];
                    for (auto &fkv : sampleObj["files"].items()) {
                        std::string filePath = fkv.key();
                        const json &fjson = fkv.value();
                        // same layout expected for file-level arrays
                        double fcnt = 0.0;
                        double fsumW = 0.0;
                        double ferr = 0.0;
                        double fsumG = 0.0;
                        double fsumG2 = 0.0;
                        double fvar = 0.0;

                        if (fjson.is_array()) {
                            if (fjson.size() > 0 && !fjson[0].is_null()) fcnt = fjson[0].get<double>();
                            if (fjson.size() > 1 && !fjson[1].is_null()) fsumW = fjson[1].get<double>();
                            if (fjson.size() > 2 && !fjson[2].is_null()) ferr = fjson[2].get<double>();
                            if (fjson.size() > 3 && !fjson[3].is_null()) fsumG = fjson[3].get<double>();
                            if (fjson.size() > 4 && !fjson[4].is_null()) fsumG2 = fjson[4].get<double>();
                            if (fjson.size() > 5 && !fjson[5].is_null()) fvar = fjson[5].get<double>();
                            if (fvar == 0.0) fvar = ferr * ferr;
                        }

                        auto &fileArr = fileMap[filePath];
                        fileArr[0] += fcnt;
                        fileArr[1] += fsumW;
                        fileArr[5] += fvar;
                        fileArr[3] += fsumG;
                        fileArr[4] += fsumG2;
                    }
                }
            }
        }
    }

    // finalize errors (compute absolute error = sqrt(variance))
    for (auto &binPair : merged) {
        for (auto &samplePair : binPair.second) {
            samplePair.second[2] = std::sqrt(samplePair.second[5]); // sqrt(var)
        }
    }
    if (!outFilesFile.empty()) {
        for (auto &binPair : filesBreakdown) {
            for (auto &samplePair : binPair.second) {
                for (auto &filePair : samplePair.second) {
                    filePair.second[2] = std::sqrt(filePair.second[5]);
                }
            }
        }
    }

    // write merged totals
    json outMerged;
    for (const auto &binPair : merged) {
        const std::string &binName = binPair.first;
        for (const auto &samplePair : binPair.second) {
            outMerged[binName][samplePair.first] = {
                samplePair.second[0], // count
                samplePair.second[1], // sumW
                samplePair.second[2], // sqrt(var)
                samplePair.second[3], // sumG
                samplePair.second[4], // sumG2
                samplePair.second[5]  // var
            };
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
                    outFiles[binName][samplePair.first][filePair.first] = {
                        filePair.second[0],
                        filePair.second[1],
                        filePair.second[2],
                        filePair.second[3],
                        filePair.second[4],
                        filePair.second[5]
                    };
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

    std::cout << "[mergeJSONs] Merged " << inputs.size() << " JSONs to " << outFile << ".json\n";
    if (per_file) std::cout << "[mergeJSONs] Per-file breakdown written to " << outFile << "_files.json\n";

    return 0;
}
