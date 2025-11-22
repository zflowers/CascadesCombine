// flattenJSONs.cpp
// Scans one or more input directories for per-file JSONs (the analyzer output),
// merges them into a single flattened JSON, and writes the result.
// Format: each sample has:
//
//   {
//      "nominal": [cnt, sumW, err],
//      "systematics": {
//         "SYSTNAME": {
//            "Up":   [cnt, sumW, err],
//            "Down": [cnt, sumW, err]
//         }
//      }
//   }

#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <cmath>
#include <string>
#include <map>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

// Helper: read a 3-number array from JSON (cnt,sumW,err)
static inline void readTriple(const json &arr, double &cnt, double &sumW, double &var)
{
    cnt = sumW = var = 0.0;
    if (!arr.is_array()) return;

    if (arr.size() > 0 && !arr[0].is_null()) cnt = arr[0].get<double>();
    if (arr.size() > 1 && !arr[1].is_null()) sumW = arr[1].get<double>();

    if (arr.size() > 2 && !arr[2].is_null()) {
        double err = arr[2].get<double>();
        var = err * err;
    }
}

int main(int argc, char** argv)
{
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0]
                  << " <input_dir1> [<input_dir2> ...] <output_file>\n";
        return 1;
    }

    fs::path outputFile = argv[argc - 1];

    // Final result structure:
    // merged[bin][sample]["nominal"]                = array
    // merged[bin][sample]["systematics"][syst][dir] = array
    json merged;

    // Variance accumulation stores sumW2
    // nominalVar[bin][sample] = sumW2
    // systVar[bin][sample][syst][dir] = sumW2
    std::map<std::string,
        std::map<std::string, double>> nominalVar;

    std::map<std::string,
        std::map<std::string,
            std::map<std::string,
                std::map<std::string, double>>>> systVar;

    // ---------------------------
    // Loop over input directories
    // ---------------------------
    for (int argi = 1; argi < argc - 1; ++argi) {
        fs::path inputDir = argv[argi];

        if (!fs::exists(inputDir) || !fs::is_directory(inputDir)) {
            std::cerr << "Input path is not a directory: " << inputDir << "\n";
            continue;
        }

        for (const auto &entry : fs::directory_iterator(inputDir)) {
            if (!entry.is_regular_file()) continue;
            if (entry.path().extension() != ".json") continue;

            std::ifstream in(entry.path());
            if (!in.is_open()) {
                std::cerr << "Failed to open JSON file: " << entry.path() << "\n";
                continue;
            }

            json j;
            try {
                in >> j;
            }
            catch (json::parse_error &e) {
                std::cerr << "JSON parse error in file "
                          << entry.path() << ": " << e.what() << "\n";
                continue;
            }

            // ---------------------------
            // Merge per-bin, per-sample
            // ---------------------------
            for (auto &binPair : j.items()) {
                const std::string binName = binPair.key();
                const json &binObj = binPair.value();

                for (auto &samplePair : binObj.items()) {
                    const std::string sampleName = samplePair.key();
                    const json &entryObj = samplePair.value();

                    // Ensure sample object exists
                    if (!merged.contains(binName))
                        merged[binName] = json::object();
                    if (!merged[binName].contains(sampleName))
                        merged[binName][sampleName] = json::object();

                    json &outSample = merged[binName][sampleName];

                    // ---------------------------------------
                    // NOMINAL
                    // ---------------------------------------
                    if (entryObj.contains("nominal")) {
                        double cnt, sumW, var;
                        readTriple(entryObj["nominal"], cnt, sumW, var);

                        // Initialize if first time
                        if (!outSample.contains("nominal"))
                            outSample["nominal"] = json::array({0.0, 0.0, 0.0});

                        // Accumulation
                        outSample["nominal"][0] =
                            outSample["nominal"][0].get<double>() + cnt;

                        outSample["nominal"][1] =
                            outSample["nominal"][1].get<double>() + sumW;

                        nominalVar[binName][sampleName] += var;
                    }

                    // ---------------------------------------
                    // SYSTEMATICS
                    // ---------------------------------------
                    if (entryObj.contains("systematics")) {
                        const json &systs = entryObj["systematics"];

                        for (auto &systPair : systs.items()) {
                            const std::string &systName = systPair.key();
                            const json &dirs = systPair.value();

                            for (auto &dirPair : dirs.items()) {
                                const std::string &direction = dirPair.key();
                                const json &arr = dirPair.value();

                                double cnt, sumW, var;
                                readTriple(arr, cnt, sumW, var);

                                // Ensure structure exists
                                if (!outSample.contains("systematics"))
                                    outSample["systematics"] = json::object();
                                if (!outSample["systematics"].contains(systName))
                                    outSample["systematics"][systName] = json::object();
                                if (!outSample["systematics"][systName].contains(direction))
                                    outSample["systematics"][systName][direction] =
                                        json::array({0.0, 0.0, 0.0});

                                json &outArr = outSample["systematics"][systName][direction];

                                outArr[0] = outArr[0].get<double>() + cnt;
                                outArr[1] = outArr[1].get<double>() + sumW;

                                systVar[binName][sampleName][systName][direction] += var;
                            }
                        }
                    }
                }
            }
        }
    }

    // -----------------------------------------------------
    // After merging all files, fill final sqrt(var) values
    // -----------------------------------------------------
    for (auto &binPair : merged.items()) {
        const std::string &bin = binPair.key();
        for (auto &samplePair : binPair.value().items()) {
            const std::string &sample = samplePair.key();
            json &obj = merged[bin][sample];

            // nominal
            if (obj.contains("nominal")) {
                double v = nominalVar[bin][sample];
                obj["nominal"][2] = std::sqrt(v);
            }

            // systematics
            if (obj.contains("systematics")) {
                for (auto &systPair : obj["systematics"].items()) {
                    const std::string &systName = systPair.key();
                    for (auto &dirPair : systPair.value().items()) {
                        const std::string &direction = dirPair.key();
                        double v = systVar[bin][sample][systName][direction];
                        obj["systematics"][systName][direction][2] = std::sqrt(v);
                    }
                }
            }
        }
    }

    // -----------------------------------------------------
    // Write merged output file
    // -----------------------------------------------------
    std::ofstream out(outputFile);
    if (!out.is_open()) {
        std::cerr << "Failed to write output file: " << outputFile << "\n";
        return 2;
    }

    out << merged.dump(4);
    std::cout << "Merged flattened JSON written to " << outputFile << "\n";
    return 0;
}

