#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <input_dir1> [<input_dir2> ...] <output_file>\n";
        return 1;
    }

    json mergedFlattened;

    // last argument is output file
    fs::path outputFile = argv[argc-1];

    // loop over input directories
    for (int argi = 1; argi < argc-1; ++argi) {
        fs::path inputDir = argv[argi];
        if (!fs::exists(inputDir) || !fs::is_directory(inputDir)) {
            std::cerr << "Input path is not a directory: " << inputDir << "\n";
            continue;
        }

        for (const auto &entry : fs::directory_iterator(inputDir)) {
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

            // --- merge logic ---
            for (auto &binPair : j.items()) {
                const std::string &binName = binPair.key();
                for (auto &samplePair : binPair.value().items()) {
                    const std::string &sampleName = samplePair.key();
                    const auto &arr = samplePair.value();

                    for (size_t i = 0; i < 3; ++i) {
                        double oldVal = 0.0;
                        if (mergedFlattened.contains(binName) &&
                            mergedFlattened[binName].contains(sampleName) &&
                            !mergedFlattened[binName][sampleName][i].is_null())
                        {
                            oldVal = mergedFlattened[binName][sampleName][i].get<double>();
                        }

                        double newVal = arr[i].is_null() ? 0.0 : arr[i].get<double>();

                        if (!mergedFlattened.contains(binName))
                            mergedFlattened[binName] = json::object();
                        if (!mergedFlattened[binName].contains(sampleName))
                            mergedFlattened[binName][sampleName] = json::array({0.0, 0.0, 0.0});

                        if (i == 2)
                            mergedFlattened[binName][sampleName][i] = std::sqrt(oldVal*oldVal + newVal*newVal);
                        else
                            mergedFlattened[binName][sampleName][i] = oldVal + newVal;
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
