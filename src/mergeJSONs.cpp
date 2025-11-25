// mergeJSONs.cpp
#include <iostream>
#include <fstream>
#include <filesystem>
#include <map>
#include <array>
#include <string>
#include <vector>
#include <unordered_set>
#include <cmath>
#include <set>
#include <nlohmann/json.hpp>
#include "SampleTool.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

/*
 * Merge flattened JSONs that use the 3-element totals layout:
 * [ count, sumW, err = sqrt(sumW2) ]
 *
 * Internally store index 2 = sumW2 (variance) while merging,
 * and output the final third element as sqrt(sumW2).
 */

bool mergeJSONsFlattenedWithFileBreakdown(
    SampleTool& ST,
    const std::vector<std::string> &inputFiles,
    const std::string &outMergedFile,
    const std::string &outFilesFile = "",
    const std::vector<std::string> &preferredGroups = {}
)
{
    // helper to get prefix (text before first underscore) from filename
    auto getPrefix = [](const std::string &fname) -> std::string {
        std::string base = fs::path(fname).filename().string();
        auto pos = base.find("_");
        return (pos == std::string::npos) ? base : base.substr(0, pos);
    };

    // Recursive expansion of a group into concrete entries (avoid cycles)
    std::function<void(const std::string&, std::unordered_set<std::string>&, std::unordered_set<std::string>&)> expandGroupRec;
    expandGroupRec = [&ST, &expandGroupRec](const std::string &group,
                                           std::unordered_set<std::string> &out,
                                           std::unordered_set<std::string> &visited) -> void {
        if (visited.find(group) != visited.end()) return; // avoid cycles
        visited.insert(group);

        auto it = ST.MasterDict.find(group);
        if (it == ST.MasterDict.end()) return;

        for (const auto &entry : it->second) {
            // If the entry is itself a group key, recurse; otherwise treat it as a file path
            if (ST.MasterDict.find(entry) != ST.MasterDict.end()) {
                expandGroupRec(entry, out, visited);
            } else {
                out.insert(entry);
            }
        }
    };

    // Cache expanded entries per group to avoid repeated recursion
    std::unordered_map<std::string, std::vector<std::string>> expandedCache;
    auto getExpandedEntries = [&expandedCache, &expandGroupRec](const std::string &group) -> const std::vector<std::string>& {
        auto cit = expandedCache.find(group);
        if (cit != expandedCache.end()) return cit->second;

        std::unordered_set<std::string> expandedSet;
        std::unordered_set<std::string> visited;
        expandGroupRec(group, expandedSet, visited);

        std::vector<std::string> vec;
        vec.reserve(expandedSet.size());
        for (const auto &s : expandedSet) vec.push_back(s);

        // cache and return
        auto ret = expandedCache.emplace(group, std::move(vec));
        return ret.first->second;
    };

    // The resolveGroup logic: check YAML preferredGroups first (in order),
    // then fall back to checking all groups.
    auto resolveGroup = [&ST, &preferredGroups, &getExpandedEntries, &getPrefix](const std::string &jsonKey) -> std::string {
        std::string keyBase = fs::path(jsonKey).filename().string();

        // 1) Preferred groups first (YAML order)
        for (const auto &pref : preferredGroups) {
            const auto &entries = getExpandedEntries(pref);
            for (const auto &entry : entries) {
                std::string p = getPrefix(entry);
                if (!p.empty() && keyBase.rfind(p, 0) == 0) {
                    return pref;
                }
            }
        }

        // 2) Fallback: scan all groups
        for (const auto &kv : ST.MasterDict) {
            const std::string &group = kv.first;
            const auto &entries = getExpandedEntries(group);
            for (const auto &entry : entries) {
                std::string p = getPrefix(entry);
                if (!p.empty() && keyBase.rfind(p, 0) == 0) {
                    return group;
                }
            }
        }

        // 3) No match -> return original key (unchanged)
        return jsonKey;
    };

    // Types for clarity:
    using Triple = std::array<double,3>; // [count, sumW, sumW2]
    // fileContribs[bin][group][filePath] -> Triple (sumW2 stored)
    std::map<std::string, std::map<std::string, std::map<std::string, Triple>>> fileContribs;

    // fileSystContribs[bin][group][syst][direction]["filePath"] -> Triple
    std::map<std::string, std::map<std::string, std::map<std::string, std::map<std::string, std::map<std::string, Triple>>>>> fileSystContribs;

    // Read all input JSONs and populate fileContribs and fileSystContribs
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
            const std::string binName = binItem.key();
            const json &binContent = binItem.value();

            for (auto &sampleItem : binContent.items()) {
                const std::string origKey = sampleItem.key();
                const json &sampleObj = sampleItem.value();
                
                std::size_t pos_FAKE = origKey.find("_FAKES_");
                std::string group;
                
                if (pos_FAKE == std::string::npos) {
                    // normal (no fake suffix) -> resolve as before
                    group = resolveGroup(origKey);
                } else {
                    // base name = everything before _FAKES_
                    std::string baseName = origKey.substr(0, pos_FAKE);
                
                    // resolve where baseName should go (e.g. boson)
                    std::string baseGroup = resolveGroup(baseName);
                
                    // fake type = everything after "_FAKES_"
                    std::string fakeType = origKey.substr(pos_FAKE + 7); // 7 = length of "_FAKES_"
                
                    // final group becomes: <baseGroup>_FAKES_<type>
                    group = baseGroup + "_FAKES_" + fakeType;
                }

                // If the sample lists files, use them. Otherwise fall back to totals and
                // attribute to a synthetic key so totals are still captured.
                if (sampleObj.contains("files") && sampleObj["files"].is_object()) {
                    for (auto &fkv : sampleObj["files"].items()) {
                        const std::string filePath = fkv.key();
                        const json &fjson = fkv.value();

                        // --- read nominal ---
                        double fcnt = 0.0;
                        double fsumW = 0.0;
                        double fvar = 0.0; // store sumW2 internally

                        // support both legacy array-at-file-level or object-with-"nominal"
                        if (fjson.is_array()) {
                            if (fjson.size() > 0 && !fjson[0].is_null()) fcnt = fjson[0].get<double>();
                            if (fjson.size() > 1 && !fjson[1].is_null()) fsumW = fjson[1].get<double>();
                            if (fjson.size() > 2 && !fjson[2].is_null()) {
                                double ferr = fjson[2].get<double>();
                                fvar = ferr * ferr;
                            }
                        } else if (fjson.contains("nominal") && fjson["nominal"].is_array()) {
                            const auto &nom = fjson["nominal"];
                            if (nom.size() > 0 && !nom[0].is_null()) fcnt = nom[0].get<double>();
                            if (nom.size() > 1 && !nom[1].is_null()) fsumW = nom[1].get<double>();
                            if (nom.size() > 2 && !nom[2].is_null()) {
                                double ferr = nom[2].get<double>();
                                fvar = ferr * ferr;
                            }
                        } else {
                            // If neither format is present, leave zeros and warn minimally.
                            // (Don't abort; allow merging across partial outputs.)
                        }

                        auto &dst = fileContribs[binName][group][filePath];
                        dst[0] += fcnt;
                        dst[1] += fsumW;
                        dst[2] += fvar;

                        // --- read systematics (if present) ---
                        if (fjson.contains("systematics") && fjson["systematics"].is_object()) {
                            for (auto &systItem : fjson["systematics"].items()) {
                                const std::string systName = systItem.key();
                                const json &dirs = systItem.value();
                                if (!dirs.is_object()) continue;

                                for (auto &dirItem : dirs.items()) {
                                    const std::string direction = dirItem.key(); // "Up" or "Down" or other
                                    const json &arr = dirItem.value();

                                    double scnt = 0.0;
                                    double ssumW = 0.0;
                                    double svar = 0.0;

                                    if (arr.is_array()) {
                                        if (arr.size() > 0 && !arr[0].is_null()) scnt  = arr[0].get<double>();
                                        if (arr.size() > 1 && !arr[1].is_null()) ssumW = arr[1].get<double>();
                                        if (arr.size() > 2 && !arr[2].is_null()) {
                                            double serr = arr[2].get<double>();
                                            svar = serr * serr;
                                        }
                                    } else if (arr.is_object()) {
                                        // defensive: some producers might put a nested object; skip
                                        continue;
                                    }

                                    auto &sdst = fileSystContribs[binName][group][systName][direction][filePath];
                                    sdst[0] += scnt;
                                    sdst[1] += ssumW;
                                    sdst[2] += svar;
                                } // dirItem
                            } // systItem
                        } // has systematics
                    } // files loop
                } else {
                    std::cout << "[mergeJSONs] Need files key in JSON (bin=" << binName << ", sample=" << origKey << ")!\n";
                }
            } // end sampleItem
        } // end binItem
    } // end inputFiles loop

    // Build merged totals by summing file-level contributions.
    std::map<std::string, std::map<std::string, Triple>> merged; // merged[bin][group] -> arr
    for (const auto &binPair : fileContribs) {
        const std::string binName = binPair.first;
        for (const auto &groupPair : binPair.second) {
            const std::string group = groupPair.first;

            Triple acc = {0.,0.,0.}; // [count, sumW, sumW2]
            for (const auto &filePair : groupPair.second) {
                const Triple &farr = filePair.second;

                acc[0] += farr[0];
                acc[1] += farr[1];
                acc[2] += farr[2]; // sum variances (sum of sumW2)
            }

            merged[binName][group] = acc;
        }
    }

    // Build merged systematics by summing file-level syst contributions.
    // mergedSyst[bin][group][syst][direction] -> Triple
    std::map<std::string, std::map<std::string, std::map<std::string, std::map<std::string, Triple>>>> mergedSyst;
    for (const auto &binPair : fileSystContribs) {
        const std::string binName = binPair.first;
        for (const auto &groupPair : binPair.second) {
            const std::string group = groupPair.first;

            for (const auto &systPair : groupPair.second) {
                const std::string systName = systPair.first;

                for (const auto &dirPair : systPair.second) {
                    const std::string direction = dirPair.first;

                    Triple acc = {0.,0.,0.};
                    for (const auto &filePair : dirPair.second) {
                        const Triple &farr = filePair.second;
                        acc[0] += farr[0];
                        acc[1] += farr[1];
                        acc[2] += farr[2];
                    }
                    mergedSyst[binName][group][systName][direction] = acc;
                }
            }
        }
    }

    // Prepare filesBreakdown (just reformat fileContribs) if requested
    auto filesBreakdown = fileContribs; // copy (nominal only); we'll build a richer outFiles when writing

    // ----- NEW: write one merged JSON per bin (robust to arbitrary job-grouping) -----
    auto make_safe = [](const std::string &s) -> std::string {
        std::string out;
        out.reserve(s.size());
        for (char c : s) {
            if (c == '/' || c == '\\' || isspace((unsigned char)c)) out.push_back('_');
            else out.push_back(c);
        }
        return out;
    };

    // If the caller passed a single merged name (e.g. "/some/path/group.json"), strip trailing ".json"
    std::string outBase = outMergedFile;
    const std::string jsuf = ".json";
    if (outBase.size() >= jsuf.size() && outBase.substr(outBase.size() - jsuf.size()) == jsuf) {
        outBase = outBase.substr(0, outBase.size() - jsuf.size());
    }

    // For each bin produce a separate merged file named: <outBase>_<sanitizedBin>.json
    for (const auto &binPair : merged) {
        const std::string &binName = binPair.first;
        const auto &samples = binPair.second;

        std::string safeBin = make_safe(binName);
        std::string outPath = outBase + "_" + safeBin + ".json";

        json single;
        for (const auto &samplePair : samples) {
            const std::string sampleName = samplePair.first;
            const auto &arr = samplePair.second;
            double err = std::sqrt(arr[2]); // arr[2] is sumW2

            // main entry with nominal
            json entry;
            entry["nominal"] = json::array({ arr[0], arr[1], err });

            // attach systematics if available for this bin/sample
            auto binSystIt = mergedSyst.find(binName);
            if (binSystIt != mergedSyst.end()) {
                auto sampleSystIt = binSystIt->second.find(sampleName);
                if (sampleSystIt != binSystIt->second.end()) {
                    for (const auto &systPair : sampleSystIt->second) {
                        const std::string &systName = systPair.first;
                        for (const auto &dirPair : systPair.second) {
                            const std::string &direction = dirPair.first;
                            const Triple &a = dirPair.second;
                            double aerr = std::sqrt(a[2]);
                            entry["systematics"][systName][direction] = json::array({ a[0], a[1], aerr });
                        }
                    }
                }
            }

            single[binName][sampleName] = entry;
        }

        // atomic-ish write: write to tmp then rename
        std::string tmpPath = outPath + ".tmp";
        std::ofstream ofs(tmpPath);
        if (!ofs.is_open()) {
            std::cerr << "[mergeJSONs] Failed to open output " << tmpPath << "\n";
            return false;
        }
        ofs << single.dump(4) << "\n";
        ofs.close();
        std::error_code ec;
        fs::rename(tmpPath, outPath, ec);
        if (ec) {
            std::cerr << "[mergeJSONs] Failed to rename " << tmpPath << " -> " << outPath << " : " << ec.message() << "\n";
            return false;
        }
    }

    // write per-file breakdown per bin if requested
    if (!outFilesFile.empty()) {
        json outFiles;
        for (const auto &binPair : fileContribs) {
            const std::string &binName = binPair.first;
            const auto &sampleMap = binPair.second;

            for (const auto &samplePair : sampleMap) {
                const std::string &sampleName = samplePair.first;

                for (const auto &filePair : samplePair.second) {
                    const std::string &filePath = filePair.first;
                    const auto &farr = filePair.second;

                    double fvar = farr[2];
                    double ferr = 0.0;
                    if (fvar != 0.0) ferr = std::sqrt(fvar);

                    // build per-file entry including systematics if present
                    json fileEntry;
                    fileEntry["nominal"] = json::array({ farr[0], farr[1], ferr });

                    // check for systematics for this file
                    auto binSIt = fileSystContribs.find(binName);
                    if (binSIt != fileSystContribs.end()) {
                        auto sampSIt = binSIt->second.find(sampleName);
                        if (sampSIt != binSIt->second.end()) {
                            for (const auto &systPair : sampSIt->second) {
                                const std::string &systName = systPair.first;
                                for (const auto &dirPair : systPair.second) {
                                    const std::string &direction = dirPair.first;
                                    auto fileSIt = dirPair.second.find(filePath);
                                    if (fileSIt != dirPair.second.end()) {
                                        const Triple &ta = fileSIt->second;
                                        double terr = (ta[2] != 0.0) ? std::sqrt(ta[2]) : 0.0;
                                        fileEntry["systematics"][systName][direction] = json::array({ ta[0], ta[1], terr });
                                    }
                                }
                            }
                        }
                    }

                    outFiles[binName][sampleName][filePath] = fileEntry;
                }
            }
        }

        // atomic-ish write
        std::ofstream ofs(outFilesFile + ".tmp");
        if (!ofs.is_open()) {
            std::cerr << "[mergeJSONs] Failed to open per-file output file: " << outFilesFile << "\n";
            return false;
        }
        ofs << outFiles.dump(4) << "\n";
        ofs.close();

        std::error_code ec;
        fs::rename(outFilesFile + ".tmp", outFilesFile, ec);
        if (ec) {
            std::cerr << "[mergeJSONs] Failed to rename tmp file -> " << outFilesFile << " : " << ec.message() << "\n";
            return false;
        }
    }

    return true;
}

int main(int argc, char **argv) {
    // Usage: prog merged output_directory [--per_file] [--processes <yaml>]
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " merged output_directory [--per_file] [--processes <yaml>]\n";
        return 1;
    }

    std::string outFile = argv[1];
    std::string jsonDir = argv[2];
    bool per_file = false;
    std::string processesYaml;

    // parse optional flags (flexible order)
    for (int i = 3; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--per_file") {
            per_file = true;
        } else if (arg == "--processes") {
            if (i + 1 < argc) {
                processesYaml = argv[++i];
            } else {
                std::cerr << "[mergeJSONs] --processes requires a YAML file path\n";
                return 1;
            }
        } else {
            std::cerr << "[mergeJSONs] Unknown option: " << arg << "\n";
            std::cerr << "Usage: " << argv[0] << " merged output_directory [--per_file] [--processes <yaml>]\n";
            return 1;
        }
    }

    // Normalize outFile: if user passed "something.json" strip the .json to avoid producing "something.json.json"
    std::string baseOut = outFile;
    const std::string suffix = ".json";
    if (baseOut.size() >= suffix.size()) {
        if (baseOut.substr(baseOut.size() - suffix.size()) == suffix) {
            baseOut = baseOut.substr(0, baseOut.size() - suffix.size());
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

    SampleTool ST;
    ST.LoadAllFromMaster();
    std::vector<std::string> preferredGroups;
    if (!processesYaml.empty()) {
        preferredGroups = ST.loadPreferredGroupsFromYaml(processesYaml);
        if (preferredGroups.empty())
            std::cerr << "[mergeJSONs] Warning: no preferred groups loaded from '" << processesYaml << "'\n";
    }

    std::string mergedName = baseOut + ".json";
    std::string filesName  = baseOut + "_files.json";

    bool success = per_file ?
        mergeJSONsFlattenedWithFileBreakdown(ST, inputs, mergedName, filesName, preferredGroups) :
        mergeJSONsFlattenedWithFileBreakdown(ST, inputs, mergedName, "", preferredGroups);

    if (!success) return 3;

    std::cout << "[mergeJSONs] Merged " << inputs.size() << " JSONs to " << mergedName << "\n";
    if (per_file) std::cout << "[mergeJSONs] Per-file breakdown written to " << filesName << "\n";

    return 0;
}
