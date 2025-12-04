#include <iostream>
#include <filesystem>
#include <vector>
#include <string>

#include "JSONFactory.h"
#include "BuildFit.h"

namespace fs = std::filesystem;

int main(int argc, char** argv) {
    // --- Default values ---
    std::string input_json = "./json/test_cascades.json";
    std::string datacard_dir = "datacards_cascades";
    std::string signal = "";   // leave empty to parse all signals

    // --- Override defaults from CLI ---
    if (argc > 1) input_json = argv[1];
    if (argc > 2) datacard_dir = argv[2];
    if (argc > 3) signal = argv[3];

    std::cout << "Using input JSON: " << input_json << "\n";
    std::cout << "Using datacard directory: " << datacard_dir << "\n";

    // --- Load JSON ---
    JSONFactory* j = new JSONFactory(input_json);

    // --- Get list of signals or control tags ---
    std::vector<std::string> targets;
    if (signal.empty())
        targets = j->GetSigProcs();
    else
        targets.push_back(signal);

    // --- Recreate datacard output directory ---
    fs::path dir_path = datacard_dir;
    fs::remove_all(dir_path);

    // --- Loop over each signal (or control tag) ---
    for (size_t i = 0; i < targets.size(); ++i) {
        const std::string &tag = targets[i];
        std::cout << "Running BF for: " << tag << std::endl;
        BuildFit* BF = new BuildFit();
        fs::create_directories(datacard_dir + "/" + tag);

        BF->BuildFitSkeleton(j, tag, datacard_dir);

        delete BF;
        std::cout << "Wrote datacard to: " << datacard_dir << "/" << tag << ".txt\n";
    }

    delete j;
    return 0;
}
