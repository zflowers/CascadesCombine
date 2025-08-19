#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <input_directory> <output_file>\n";
        return 1;
    }

    fs::path inputDir = argv[1];
    fs::path outputFile = argv[2];

    json mergedFlattened;

    // loop over .json files in directory
    for (const auto &entry : fs::directory_iterator(inputDir)) {
        if (entry.path().extension() == ".json") {
            std::ifstream in(entry.path());
            if (!in) {
                std::cerr << "Failed to open " << entry.path() << "\n";
                continue;
            }
            json j;
            in >> j;

            // merge into flattened structure
            for (auto &binPair : j.items()) {
                const std::string &binName = binPair.key();
                for (auto &samplePair : binPair.value().items()) {
                    const std::string &sampleName = samplePair.key();
                    const auto &arr = samplePair.value();
                    for (size_t i = 0; i < 3; ++i) {
                        // accumulate if same bin+sample appears in multiple files
                        if (mergedFlattened[binName].contains(sampleName)) {
			    if (i == 2) { // error: combine in quadrature
			        double oldErr = mergedFlattened[binName][sampleName][i].get<double>();
			        double newErr = arr[i].get<double>();
			        mergedFlattened[binName][sampleName][i] = std::sqrt(oldErr*oldErr + newErr*newErr);
			    } else {
			        mergedFlattened[binName][sampleName][i] = 
			            mergedFlattened[binName][sampleName][i].get<double>() + arr[i].get<double>();
			    }
                        } else {
                            mergedFlattened[binName][sampleName][i] = arr[i];
                        }
                    }
                }
            }
        }
    }

    // write merged json
    std::ofstream out(outputFile);
    out << mergedFlattened.dump(4);
    std::cout << "Merged flattened JSON written to " << outputFile << "\n";
    return 0;
}

