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
#include <yaml-cpp/yaml.h>

namespace fs = std::filesystem;
using json = nlohmann::json;

/*
 * Merge flattened JSONs that use the 6-element totals layout:
 * [ count, sumW, err = sqrt(sumW2), sumG, sumG2, var = sum(err^2) ]
 *
 * Behavior:
 * - Accumulate contributions at the per-file level so that files present in
 *   multiple input JSONs aren't double-counted.
 * - sumG and sumG2 (generator-level sums) are added only once per (group, file).
 * - If a file-level entry provides only err (index 2) but not var (index 5),
 *   var = err^2 is computed. If only var is provided, err = sqrt(var) is computed
 *   when writing per-file output.
 * - Silent on expected missing bins/processes (this is the chosen silent mode).
 */

// Load preferred groups from YAML file (reads processes.bkg and processes.sig in order)
static std::vector<std::string> loadPreferredGroupsFromYaml(const std::string &yamlPath) {
    std::vector<std::string> out;
    try {
        YAML::Node root = YAML::LoadFile(yamlPath);
        if (!root) return out;

        if (root["processes"]) {
            YAML::Node procs = root["processes"];
            if (procs["bkg"] && procs["bkg"].IsSequence()) {
                for (const auto &n : procs["bkg"]) {
                    if (n.IsScalar()) out.push_back(n.as<std::string>());
                }
            }
            if (procs["sig"] && procs["sig"].IsSequence()) {
                for (const auto &n : procs["sig"]) {
                    if (n.IsScalar()) out.push_back(n.as<std::string>());
                }
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "[loadPreferredGroupsFromYaml] Failed to read YAML '" << yamlPath << "': " << e.what() << "\n";
    }
    return out;
}

bool mergeJSONsFlattenedWithFileBreakdown(const std::vector<std::string> &inputFiles,
                                          const std::string &outMergedFile,
                                          const std::string &outFilesFile = "",
                                          const std::vector<std::string> &preferredGroups = {})
{
    SampleTool ST;
    ST.LoadAllFromMaster();

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
    
        // If expansion returned nothing (maybe entries were raw paths stored directly) leave it empty
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

    // fileContribs[bin][group][filePath] -> array{count, sumW, err (unused here), sumG, sumG2, var}
    std::map<std::string, std::map<std::string, std::map<std::string, std::array<double,6>>>> fileContribs;

    // Read all input JSONs and populate fileContribs
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
                const std::string group = resolveGroup(origKey);

                // If the sample lists files, use them. Otherwise fall back to totals and
                // attribute to a synthetic key so totals are still captured.
                if (sampleObj.contains("files") && sampleObj["files"].is_object()) {
                    for (auto &fkv : sampleObj["files"].items()) {
                        const std::string filePath = fkv.key();
                        const json &fjson = fkv.value();

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
                        }

                        // Recover missing variance/err if one is present
                        if (fvar == 0.0 && ferr != 0.0) fvar = ferr * ferr;
                        if (ferr == 0.0 && fvar != 0.0) ferr = std::sqrt(fvar);

                        auto &dst = fileContribs[binName][group][filePath];
                        dst[0] += fcnt;
                        dst[1] += fsumW;
                        // store raw variance in index 5 for merging; index 2 will be computed when needed
                        dst[5] += fvar;
                        dst[3] += fsumG;
                        dst[4] += fsumG2;
                    }
                } else {
                    // fallback: attribute totals to a synthetic file key unique to this input JSON + sample key
                    const json totalsJson = sampleObj.contains("totals") ? sampleObj["totals"] : json();

                    double tcnt = 0.0;
                    double tsumW = 0.0;
                    double terr = 0.0;
                    double tsumG = 0.0;
                    double tsumG2 = 0.0;
                    double tvar = 0.0;

                    if (totalsJson.is_array()) {
                        if (totalsJson.size() > 0 && !totalsJson[0].is_null()) tcnt = totalsJson[0].get<double>();
                        if (totalsJson.size() > 1 && !totalsJson[1].is_null()) tsumW = totalsJson[1].get<double>();
                        if (totalsJson.size() > 2 && !totalsJson[2].is_null()) terr = totalsJson[2].get<double>();
                        if (totalsJson.size() > 3 && !totalsJson[3].is_null()) tsumG = totalsJson[3].get<double>();
                        if (totalsJson.size() > 4 && !totalsJson[4].is_null()) tsumG2 = totalsJson[4].get<double>();
                        if (totalsJson.size() > 5 && !totalsJson[5].is_null()) tvar = totalsJson[5].get<double>();
                    }

                    if (tvar == 0.0 && terr != 0.0) tvar = terr * terr;
                    if (terr == 0.0 && tvar != 0.0) terr = std::sqrt(tvar);

                    std::string syntheticFileKey = std::string("__src__:") + fs::path(fname).filename().string() + ":" + origKey;
                    auto &dst = fileContribs[binName][group][syntheticFileKey];
                    dst[0] += tcnt;
                    dst[1] += tsumW;
                    dst[5] += tvar;
                    dst[3] += tsumG;
                    dst[4] += tsumG2;
                }
            } // end sampleItem
        } // end binItem
    } // end inputFiles loop

    // Build merged totals by summing file-level contributions.
    // Ensure sumG/sumG2 are counted only once per (group, filePath).
    std::map<std::string, std::map<std::string, std::array<double,6>>> merged; // merged[bin][group] -> arr
    std::map<std::string, std::map<std::string, std::unordered_set<std::string>>> seenRawPerBinGroup;

    for (const auto &binPair : fileContribs) {
        const std::string binName = binPair.first;
        for (const auto &groupPair : binPair.second) {
            const std::string group = groupPair.first;

            std::array<double,6> acc = {0.,0.,0.,0.,0.,0.};
            auto &seenSet = seenRawPerBinGroup[binName][group];

            for (const auto &filePair : groupPair.second) {
                const std::string filePath = filePair.first;
                const std::array<double,6> &farr = filePair.second;

                // Always accumulate per-bin quantities (count, sumW, var)
                acc[0] += farr[0];
                acc[1] += farr[1];
                acc[5] += farr[5];

                // Only add raw generator sums once per (group, file)
                if (seenSet.find(filePath) == seenSet.end()) {
                    acc[3] += farr[3];
                    acc[4] += farr[4];
                    seenSet.insert(filePath);
                }
            } // end per-file loop

            // compute final err = sqrt(var)
            acc[2] = std::sqrt(acc[5]);

            merged[binName][group] = acc;
        }
    }

    // Prepare filesBreakdown (just reformat fileContribs) if requested
    auto filesBreakdown = fileContribs; // copy

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
            single[binName][samplePair.first] = {
                samplePair.second[0], // count
                samplePair.second[1], // sumW
                samplePair.second[2], // sqrt(var)
                samplePair.second[3], // sumG
                samplePair.second[4], // sumG2
                samplePair.second[5]  // var
            };
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
    
                    // Ensure err is sqrt(var) if missing
                    double ferr = farr[2];
                    double fvar = farr[5];
                    if (ferr == 0.0 && fvar != 0.0) ferr = std::sqrt(fvar);
    
                    outFiles[binName][sampleName][filePath] = {
                        farr[0],   // count
                        farr[1],   // sumW
                        ferr,      // err
                        farr[3],   // sumG
                        farr[4],   // sumG2
                        farr[5]    // var
                    };
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

    std::vector<std::string> preferredGroups;
    if (!processesYaml.empty()) {
        preferredGroups = loadPreferredGroupsFromYaml(processesYaml);
        if (preferredGroups.empty())
            std::cerr << "[mergeJSONs] Warning: no preferred groups loaded from '" << processesYaml << "'\n";
    }

    std::string mergedName = baseOut + ".json";
    std::string filesName  = baseOut + "_files.json";

    bool success = per_file ?
        mergeJSONsFlattenedWithFileBreakdown(inputs, mergedName, filesName, preferredGroups) :
        mergeJSONsFlattenedWithFileBreakdown(inputs, mergedName, "", preferredGroups);

    if (!success) return 3;

    std::cout << "[mergeJSONs] Merged " << inputs.size() << " JSONs to " << mergedName << "\n";
    if (per_file) std::cout << "[mergeJSONs] Per-file breakdown written to " << filesName << "\n";

    return 0;
}

