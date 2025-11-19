#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <string>
#include <cmath>
#include <memory>
#include <filesystem>
#include <optional>
#include <unordered_set>
#include <unordered_map>
#include "SampleTool.h"
#include "DefineUserHists.h"

namespace fs = std::filesystem;

// Small POD to keep nominal or variation yield info: {entries, sum, sumErr (stddev)}
using YieldTriple = std::array<double,3>;

// Systematic yields (each side stores a YieldTriple: entries, sum, err)
struct SystYields {
    YieldTriple up  = {0.0, 0.0, 0.0};
    YieldTriple down= {0.0, 0.0, 0.0};
};

// Per-file result: nominal + per-syst up/down
struct FileYields {
    YieldTriple nominal = {0.0, 0.0, 0.0};
    std::map<std::string, SystYields> systs; // keyed by SystInfo.tag
};

// Per-process totals: nominal + per-syst up/down
struct ProcTotals {
    YieldTriple nominal = {0.0, 0.0, 0.0};
    std::map<std::string, SystYields> systs;
};

SampleTool ST;
inline std::string GetSampleNameFromKey(const std::string& keyOrPath) {
    size_t lastSlash = keyOrPath.find_last_of("/\\");
    std::string name = (lastSlash == std::string::npos) ? keyOrPath : keyOrPath.substr(lastSlash + 1);
    size_t dot = name.rfind(".root");
    if (dot != std::string::npos) name = name.substr(0, dot);
    return name;
}

inline std::string GetProcessNameFromKey(const std::string &keyOrPath,
                                         const std::vector<std::string> &preferredGroups = {})
{
    // --- Helper: get prefix before first underscore ---
    auto getPrefix = [](const std::string &fname) -> std::string {
        std::string base = fs::path(fname).filename().string();
        auto pos = base.find("_");
        return (pos == std::string::npos) ? base : base.substr(0, pos);
    };

    // --- Recursive group expansion (same as in mergeJSONs.cpp) ---
    std::function<void(const std::string&, std::unordered_set<std::string>&, std::unordered_set<std::string>&)> expandGroupRec;
    expandGroupRec = [&expandGroupRec](const std::string &group,
                                            std::unordered_set<std::string> &out,
                                            std::unordered_set<std::string> &visited) -> void {
        if (visited.find(group) != visited.end()) return;
        visited.insert(group);

        auto it = ST.MasterDict.find(group);
        if (it == ST.MasterDict.end()) return;

        for (const auto &entry : it->second) {
            if (ST.MasterDict.find(entry) != ST.MasterDict.end()) {
                expandGroupRec(entry, out, visited);
            } else {
                out.insert(entry);
            }
        }
    };

    // --- Cached expansion ---
    std::unordered_map<std::string, std::vector<std::string>> expandedCache;
    auto getExpandedEntries = [&expandedCache, &expandGroupRec](const std::string &group) -> const std::vector<std::string>& {
        auto cit = expandedCache.find(group);
        if (cit != expandedCache.end()) return cit->second;

        std::unordered_set<std::string> expandedSet, visited;
        expandGroupRec(group, expandedSet, visited);
        std::vector<std::string> vec(expandedSet.begin(), expandedSet.end());
        auto ret = expandedCache.emplace(group, std::move(vec));
        return ret.first->second;
    };

    // --- Resolution logic ---
    auto resolveGroup = [&preferredGroups, &getExpandedEntries, &getPrefix](const std::string &jsonKey) -> std::string {
        std::string keyBase = fs::path(jsonKey).filename().string();

        // 1) Try preferred groups (YAML order)
        for (const auto &pref : preferredGroups) {
            const auto &entries = getExpandedEntries(pref);
            for (const auto &entry : entries) {
                std::string p = getPrefix(entry);
                if (!p.empty() && keyBase.rfind(p, 0) == 0) {
                    return pref;
                }
            }
        }

        // 2) Fallback: search all groups
        for (const auto &kv : ST.MasterDict) {
            const auto &group = kv.first;
            const auto &entries = getExpandedEntries(group);
            for (const auto &entry : entries) {
                std::string p = getPrefix(entry);
                if (!p.empty() && keyBase.rfind(p, 0) == 0) {
                    return group;
                }
            }
        }

        // 3) No match -> fallback to filename
        return keyBase;
    };

    return resolveGroup(keyOrPath);
}

inline bool SampleIsData(const std::string& keyOrPath){
    std::string sample = GetSampleNameFromKey(keyOrPath);
    std::transform(sample.begin(), sample.end(), sample.begin(),
               [](unsigned char c){ return std::tolower(c); });
    if (sample.find("data") != std::string::npos) return true;
    return false;
}

inline double GetLumiFromKey(const std::string& keyOrPath) {
    SampleTool ST;
    ST.LoadAllFromMaster();
    // Helper: strip known suffixes
    auto stripSuffixes = [](std::string s) -> std::string {
        const std::vector<std::string> suffixes = {"_SMS", "_Cascades"};
        for (const auto& suf : suffixes) {
            if (s.size() >= suf.size() &&
                s.compare(s.size() - suf.size(), suf.size(), suf) == 0) {
                s.erase(s.size() - suf.size());
            }
        }
        return s;
    };

    // Use stripSuffixes later when not mixing and matching signals across eras
    // Helper: search for the best (longest) LumiDict key appearing as a path-segment prefix
    auto findLumiKeyInString = [&ST, &stripSuffixes](const std::string &s) -> std::optional<std::string> {
        std::string best;
        for (const auto &kv : ST.LumiDict) {
            std::string tag = kv.first; // stripSuffixes(kv.first);
            std::size_t pos = 0;
            while (pos < s.size()) {
                std::size_t next = s.find('/', pos);
                std::string seg = (next == std::string::npos) ? s.substr(pos) : s.substr(pos, next - pos);
                //seg = stripSuffixes(seg);

                if (seg == tag ||
                    (seg.rfind(tag, 0) == 0 &&
                     (seg.size() == tag.size() || seg[tag.size()] == '_' || seg[tag.size()] == '-'))) {
                    if (tag.size() > best.size()) best = tag;
                }

                if (next == std::string::npos) break;
                pos = next + 1;
            }
        }
        if (!best.empty()) return best;
        return std::nullopt;
    };

    // 1) Try direct search in the provided path/string
    if (auto k = findLumiKeyInString(keyOrPath); k.has_value()) {
        return ST.LumiDict[*k];
    }

    // 2) Fallback: try to match by filename against MasterDict entries
    std::string keyBase = fs::path(keyOrPath).filename().string();
    for (const auto &kv : ST.MasterDict) {
        for (const auto &entry : kv.second) {
            std::string entryBase = fs::path(entry).filename().string();
            if (keyBase == entryBase) {
                if (auto k = findLumiKeyInString(entry); k.has_value()) {
                    return ST.LumiDict[*k];
                }
            }
        }
    }

    // 3) Give up
    std::cerr << "Warning: GetLumiFromKey could not determine lumi for '" << keyOrPath << "'. Returning 1.0\n";
    return 1.0;
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

static void writeSamplesJSON(
    std::ostream& os,
    const std::map<std::string, std::map<std::string,std::array<double,3>>>& files_for_write,
    const std::map<std::string, std::array<double,3>>& totals_for_write,
    const std::string& indent = "  "
)
{
    bool firstSample = true;
    for (const auto& kv : totals_for_write) {
        if (!firstSample) os << ",\n";
        firstSample = false;

        const std::string& sname = kv.first;
        const auto& totalVals   = kv.second;
        std::string sampleId    = GetSampleNameFromKey(sname);

        os << indent << "  \"" << sampleId << "\": {\n";
        os << indent << "    \"files\": {\n";

        bool firstFile = true;
        auto itFiles = files_for_write.find(sname);
        if (itFiles != files_for_write.end()) {
            for (const auto& fkv : itFiles->second) {
                if (!firstFile) os << ",\n";
                firstFile = false;

                os << indent << "      \"" << fkv.first << "\": ["
                   << (long long)fkv.second[0] << ", "
                   << fkv.second[1] << ", "
                   << fkv.second[2] << "]";
            }
        }

        os << "\n" << indent << "    },\n";
        os << indent << "    \"totals\": ["
           << (long long)totalVals[0] << ", "
           << totalVals[1] << ", "
           << totalVals[2] << "]\n";

        os << indent << "  }";
    }
}

static bool writePartialJSON(
    const std::string& outPath,
    const std::string& binname,
    const std::map<std::string, std::map<std::string, std::array<double,3>>>& fileResults,
    const std::map<std::string, std::array<double,3>>& totals)
{
    std::ofstream ofs(outPath);
    if (!ofs) return false;

    ofs << "{\n";
    ofs << "  \"" << binname << "\": {\n";

    writeSamplesJSON(ofs, fileResults, totals, "  ");

    ofs << "\n  }\n";
    ofs << "}\n";
    return true;
}

struct SystInfo {
    std::string tag;       // "METtrig"
    std::string nominal;   // "MetTrigSFweight"
    std::string up;        // "MetTrigSFweight_up"
    std::string down;      // "MetTrigSFweight_down"
};

static const std::vector<SystInfo> kDefaultSystematics = {
    {"METtrig", "MetTrigSFweight", "MetTrigSFweight_up", "MetTrigSFweight_down"},
    {"PU",      "PUweight",        "PUweight_up",        "PUweight_down"}
};

ROOT::RDF::RNode MultiSystWeights(ROOT::RDF::RNode node,
                                  bool is_data,
                                  int year,
                                  const std::vector<SystInfo>& systs = kDefaultSystematics)
{
    if (is_data) {
        return node
            .Define("weight_scaled",    [](){ return 1.0; })
            .Define("weight_sq_scaled", [](){ return 1.0; });
    }

    // --- Nominal product ---
    node = node.Define(
        "syst_nomin_product",
        [year](double met, double pu) {
            double metEff = (year > 2018 ? 1.0 : met); // hack since trigSF in Run3 samples set to 0
            //return metEff * pu;
            return 1.;
        },
        {"MetTrigSFweight", "PUweight"}
    );

    // --- Multiply base weight by nominal product ---
    node = node.Define("weight_scaled",
        [](double base, double sysNom) { return base * sysNom; },
        {"weight_scaled_raw", "syst_nomin_product"}
    );

    node = node.Define("weight_sq_scaled",
        [](double baseSq, double sysNom) { return baseSq * sysNom * sysNom; },
        {"weight_sq_scaled_raw", "syst_nomin_product"}
    );

    // --- Up/Down variations ---
    for (const auto& s : systs)
    {
        std::string colUp   = "weight_scaled_" + s.tag + "Up";
        std::string colDown = "weight_scaled_" + s.tag + "Down";

        node = node.Define(
            colUp,
            [=](double base, double nomProd, double nomVal, double upVal, double){
                return base * nomProd * (upVal / nomVal);
            },
            {"weight_scaled_raw", "syst_nomin_product", s.nominal, s.up, s.down}
        );

        node = node.Define(
            colDown,
            [=](double base, double nomProd, double nomVal, double, double downVal){
                return base * nomProd * (downVal / nomVal);
            },
            {"weight_scaled_raw", "syst_nomin_product", s.nominal, s.up, s.down}
        );
    }

    return node;
}

// Keeps RDataFrame alive while returning a convenient RNode
struct BaseNodeHandle {
    std::shared_ptr<ROOT::RDataFrame> df; // owns the RDataFrame lifetime
    ROOT::RDF::RNode node;                // node built from *df
};

// Small functor for computing per-event lumi
struct EventLumi {
    bool is_data;
    int year;
    double fullLumi;
    double hemLumi;
    bool is2018;

    EventLumi(bool data, int y, double fLumi, double hLumi)
        : is_data(data), year(y), fullLumi(fLumi), hemLumi(hLumi), is2018(y==2018) {}

    double operator()(int runnum, bool hem_veto) const {
        if (is_data) return 1.0;
        return (is2018 && hem_veto) ? hemLumi : fullLumi;
    }
};

// Small functor for computing pass_HEM
struct PassHEM {
    bool is_data;
    int year;
    bool is2018;

    PassHEM(bool data, int y) : is_data(data), year(y), is2018(y==2018) {}

    bool operator()(int runnum, bool hem_veto) const {
        if (is_data) {
            if (is2018 && runnum >= 319077 && hem_veto) return false;
            else return true;
        }
        return true; // MC always passes
    }
};

// Small functor for weight scaling using event_lumi
struct WeightScaled {
    bool is_data;
    WeightScaled(bool data) : is_data(data) {}
    double operator()(double w, double event_lumi) const {
        return is_data ? 1.0 : w * event_lumi;
    }
};

// Build the common base node (weights + lepton counts + pair kinematics).
// Caller is responsible for calling ROOT::EnableImplicitMT(...) to set MT state
// BEFORE calling this helper so the returned RNode is constructed under the
// desired implicit-MT configuration.
static BaseNodeHandle MakeBaseNode(const std::string &tree_name,
                                   const std::string &rootFilePath,
                                   BuildFitInput *BFI,
                                   double Lumi,
                                   bool is_data,
                                   int year)
{
    auto df = std::make_shared<ROOT::RDataFrame>(tree_name, rootFilePath);

    const double fullLumi = Lumi;
    const double hemLumi  = ST.LumiDict.at("HEM_LUMI"); // assume key exists

    // Instantiate functors
    EventLumi eventLumi(is_data, year, fullLumi, hemLumi);
    PassHEM passHEM(is_data, year);
    WeightScaled weightScaled(is_data);

    ROOT::RDF::RNode node = (*df)
        // event-by-event lumi and HEM pass flag
        .Define("event_lumi", eventLumi, {"runnum", "HEM_Veto"})
        .Define("pass_HEM",   passHEM,   {"runnum", "HEM_Veto"})

        // scaled weights now include event_lumi directly
        .Define("weight_scaled_raw",  weightScaled, {"weight", "event_lumi"})
        .Define("weight_sq_scaled_raw",
            [weightScaled](double w2, double el){ return weightScaled(w2, el) * el; },
            {"weight2", "event_lumi"})
    ;

    node = MultiSystWeights(node, is_data, year);

    // lepton counts / kinematics
    node = BFI->DefineLeptonPairCounts(node, "");
    node = BFI->DefineLeptonPairCounts(node, "A");
    node = BFI->DefineLeptonPairCounts(node, "B");
    node = BFI->DefinePairKinematics(node, "");
    node = BFI->DefinePairKinematics(node, "A");
    node = BFI->DefinePairKinematics(node, "B");

    return {df, node};
}
