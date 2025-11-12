// flattenJSONs.cpp
// Scans one or more input directories for per-file JSONs (the analyzer output),
// merges them into a single flattened JSON, and writes the result.
// The flattened layout uses 6 elements per sample:
// [ count, sumW, err = sqrt(sumW²), sumG, sumG2, var = sum(err²) ]

#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <cmath>
#include <string>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <input_dir1> [<input_dir2> ...] <output_file>\n";
        return 1;
    }

    json mergedFlattened;
    fs::path outputFile = argv[argc - 1];

    // loop over input directories
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
            } catch (json::parse_error &e) {
                std::cerr << "JSON parse error in file " << entry.path() << ": " << e.what() << "\n";
                continue;
            }

            // --- merge logic: for each bin/sample add arrays into mergedFlattened ---
            for (auto &binPair : j.items()) {
                const std::string &binName = binPair.key();
                for (auto &samplePair : binPair.value().items()) {
                    const std::string &sampleName = samplePair.key();
                    const auto &arr = samplePair.value();

                    // Ensure structure exists and initialize to 6 zeros if needed
                    if (!mergedFlattened.contains(binName))
                        mergedFlattened[binName] = json::object();
                    if (!mergedFlattened[binName].contains(sampleName))
                        mergedFlattened[binName][sampleName] = json::array({0.0, 0.0, 0.0, 0.0, 0.0, 0.0});

                    // Read input array safely (support input arrays of size 3..6)
                    double cnt = 0.0;
                    double sumW = 0.0;
                    double err = 0.0;   // sqrt(sumW2)
                    double sumG = 0.0;
                    double sumG2 = 0.0;
                    double var = 0.0;

                    if (arr.is_array()) {
                        if (arr.size() > 0 && !arr[0].is_null()) cnt = arr[0].get<double>();
                        if (arr.size() > 1 && !arr[1].is_null()) sumW = arr[1].get<double>();
                        if (arr.size() > 2 && !arr[2].is_null()) err = arr[2].get<double>();
                        if (arr.size() > 3 && !arr[3].is_null()) sumG = arr[3].get<double>();
                        if (arr.size() > 4 && !arr[4].is_null()) sumG2 = arr[4].get<double>();
                        if (arr.size() > 5 && !arr[5].is_null()) var = arr[5].get<double>();
                    } else {
                        std::cerr << "Unexpected JSON format in " << entry.path() << " for " << sampleName << "\n";
                        continue;
                    }

                    // If input didn't provide var, recover it from err
                    if (var == 0.0) var = err * err;

                    // Accumulate into mergedFlattened
                    double oldCnt  = mergedFlattened[binName][sampleName][0].get<double>();
                    double oldSumW = mergedFlattened[binName][sampleName][1].get<double>();
                    double oldVar  = mergedFlattened[binName][sampleName][5].get<double>();
                    double oldSumG = mergedFlattened[binName][sampleName][3].get<double>();
                    double oldSumG2 = mergedFlattened[binName][sampleName][4].get<double>();

                    mergedFlattened[binName][sampleName][0] = oldCnt + cnt;
                    mergedFlattened[binName][sampleName][1] = oldSumW + sumW;
                    mergedFlattened[binName][sampleName][4] = oldSumG2 + sumG2;
                    mergedFlattened[binName][sampleName][3] = oldSumG + sumG;
                    mergedFlattened[binName][sampleName][5] = oldVar + var;

                    // update the sqrt(sumW2) slot to reflect the new variance
                    mergedFlattened[binName][sampleName][2] = std::sqrt(mergedFlattened[binName][sampleName][5].get<double>());
                }
            }
        }
    }

    // write merged flattened json
    std::ofstream out(outputFile);
    if (!out.is_open()) {
        std::cerr << "Failed to write output file: " << outputFile << "\n";
        return 2;
    }

    out << mergedFlattened.dump(4);
    std::cout << "Merged flattened JSON written to " << outputFile << "\n";
    return 0;
}
