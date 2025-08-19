#include <iostream>
#include <filesystem>
#include <vector>
#include <string>

#include "JSONFactory.h"
#include "BuildFit.h"

namespace fs = std::filesystem;

int main(int argc, char** argv) {
    // Default values
    std::string input_json = "./json/test_cascades.json";
    std::string datacard_dir = "datacards_cascades";

    // Override defaults if arguments are provided
    if (argc > 1) input_json = argv[1];
    if (argc > 2) datacard_dir = argv[2];

    std::cout << "Using input JSON: " << input_json << "\n";
    std::cout << "Using datacard directory: " << datacard_dir << "\n";

    JSONFactory* j = new JSONFactory(input_json);

    std::vector<std::string> signals = j->GetSigProcs();

    // regenerate datacard directories
    fs::path dir_path = datacard_dir;
    fs::remove_all(dir_path);

    for (long unsigned int i = 0; i < signals.size(); i++) {
        BuildFit* BF = new BuildFit();
        fs::create_directories(datacard_dir + "/" + signals[i]);
        BF->BuildAsimovFit(j, signals[i], datacard_dir);
	delete BF;
    }

    delete j; // clean up
    return 0;
}

