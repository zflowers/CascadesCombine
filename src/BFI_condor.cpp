// src/BFI_condor.cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <string>
#include <getopt.h>
#include "TFile.h"
#include "BuildFitInput.h"
#include "SampleTool.h"
#include "BuildFitTools.h"

// ----------------------
// Helpers
// ----------------------

static void usage(const char* me) {
    std::cerr << "Usage: " << me 
              << " --bin BINNAME --file ROOTFILE --output OUT.json "
                 "[--cuts CUT1,CUT2,...] [--lep-cuts LEPCUT1,LEPCUT2,...] "
                 "[--predefined-cuts NAME1,NAME2,...]\n\n";

    std::cerr << "Required arguments:\n";
    std::cerr << "  --bin        Name of the bin to process (e.g. TEST)\n";
    std::cerr << "  --file       Path to one ROOT file to process\n";
    std::cerr << "  --output     Path to write partial JSON output\n\n";

    std::cerr << "Optional arguments:\n";
    std::cerr << "  --cuts               Comma-separated list of normal tree cuts "
                 "(e.g. MET>=150,PTISR>=250)\n";
    std::cerr << "  --lep-cuts           Comma-separated list of cuts that should be "
                 "passed to BuildLeptonCut (e.g. >=1OSSF,=2Pos)\n";
    std::cerr << "  --predefined-cuts    Comma-separated list of predefined cuts by name "
                 "(e.g. Cleaning,ZStar)\n";
    std::cerr << "  --help               Display this help message\n";
}

static bool buildCutsForBin(BuildFitInput* bfi,
                            const std::vector<std::string>& normalCuts,
                            const std::vector<std::string>& lepCuts,
                            const std::vector<std::string>& predefinedCuts,
                            std::vector<std::string>& outCuts) {
    if (!bfi) return false;

    outCuts.clear();

    // 1) Add normal tree cuts
    outCuts = normalCuts;

    // 2) Add lepton cuts processed via BuildLeptonCut
    for (const auto &lepCut : lepCuts) {
        std::string builtCut = bfi->BuildLeptonCut(lepCut);
        if (!builtCut.empty()) outCuts.push_back(builtCut);
    }

    // 3) Map predefined cut names to BFI functions
    for (const auto &pcut : predefinedCuts) {
        if (pcut == "Cleaning") {
            std::string c = bfi->GetCleaningCut();
            if (!c.empty()) outCuts.push_back(c);
        } else if (pcut == "ZStar") {
            std::string z = bfi->GetZstarCut();
            if (!z.empty()) outCuts.push_back(z);
        } else {
            std::cerr << "[BFI_condor] Unknown predefined cut: " << pcut << "\n";
        }
    }

    return true;
}

// Minimal JSON writer for the shape you showed:
// { "BINNAME": { "Sample": [count, weighted_sum, error], ... } }
// This writes a pretty-printed JSON file.
static bool writePartialJSON(const std::string& outPath,
                             const std::string& binname,
                             const std::map<std::string, std::array<double,3>>& results) {
    std::ofstream ofs(outPath);
    if (!ofs) return false;

    ofs << "{\n";
    ofs << "  \"" << binname << "\": {\n";

    bool firstSample = true;
    for (const auto &kv : results) {
        if (!firstSample) ofs << ",\n";
        firstSample = false;
        const std::string &sname = kv.first;
        const auto &vals = kv.second; // [count, weighted_sum, error]
        // count may be integer; but we'll print as number (int or double)
        // We keep formatting similar to your example.
        ofs << "    \"" << sname << "\": [\n";
        ofs << "      " << (long long)vals[0] << ",\n";              // count as integer
        ofs << "      " << vals[1] << ",\n";                       // weighted sum
        ofs << "      " << vals[2] << "\n";                        // error
        ofs << "    ]";
    }
    ofs << "\n  }\n}\n";
    ofs.close();
    return true;
}

// ----------------------
// Main
// ----------------------
int main(int argc, char** argv) {

    std::string binName;
    std::string cutsStr;
    std::string lepCutsStr;
    std::string predefCutsStr;
    std::string rootFilePath;
    std::string outputJsonPath;
    
    static struct option long_options[] = {
        {"bin", required_argument, 0, 'b'},
        {"file", required_argument, 0, 'f'},
        {"output", required_argument, 0, 'o'},
        {"cuts", required_argument, 0, 'c'},
        {"lep-cuts", required_argument, 0, 'l'},
        {"predefined-cuts", required_argument, 0, 'p'},
        {"help", no_argument, 0, 'h'},
        {0,0,0,0}
    };
    
    int opt;
    int opt_index = 0;
    while ((opt = getopt_long(argc, argv, "b:f:o:c:l:p:h", long_options, &opt_index)) != -1) {
        switch (opt) {
            case 'b': binName = optarg; break;
            case 'f': rootFilePath = optarg; break;
            case 'o': outputJsonPath = optarg; break;
            case 'c': cutsStr = optarg; break;
            case 'l': lepCutsStr = optarg; break;
            case 'p': predefCutsStr = optarg; break;
            case 'h':
            default:
                usage(argv[0]);
                return 1;
        }
    }
    
    // Verify required arguments
    if (binName.empty() || rootFilePath.empty() || outputJsonPath.empty()) {
        usage(argv[0]);
        return 1;
    }
    
    // -------------------------
    // Split comma-separated strings into vectors
    // -------------------------
    auto splitString = [](const std::string& s, char delim) -> std::vector<std::string> {
        std::vector<std::string> elems;
        std::stringstream ss(s);
        std::string item;
        while (std::getline(ss, item, delim)) {
            if (!item.empty()) elems.push_back(item);
        }
        return elems;
    };
    
    std::vector<std::string> cutsVec    = splitString(cutsStr, ',');
    std::vector<std::string> lepCutsVec = splitString(lepCutsStr, ',');
    std::vector<std::string> predefCutsVec = splitString(predefCutsStr, ',');


    // -------------------------
    // Initialize BFI and SampleTool
    // -------------------------
    BuildFitInput* BFI = nullptr;
    SampleTool* ST = nullptr;
    
    try {
        // Construct BFI
        BFI = new BuildFitInput();
        
        // Construct SampleTool
        ST = new SampleTool();
    
        // Lists of backgrounds and signals
        stringlist bkglist = {"ttbar", "ST", "DY", "ZInv", "DBTB", "QCD", "Wjets"};
        stringlist siglist = {"Cascades"}; // or other SMS signals as needed
    
        // Load samples
        ST->LoadBkgs(bkglist);
        ST->LoadSigs(siglist);
    
        // Optional: print dictionaries for debugging
        ST->PrintDict(ST->BkgDict);
        ST->PrintDict(ST->SigDict);
        ST->PrintKeys(ST->SignalKeys);
    
    } catch (...) {
        std::cerr << "[BFI_condor] Failed to initialize BFI or SampleTool\n";
        if (BFI) delete BFI;
        if (ST) delete ST;
        return 2;
    }

    // Build cuts for this bin
    std::vector<std::string> finalCuts;
    if (!buildCutsForBin(BFI, cutsVec, lepCutsVec, predefCutsVec, finalCuts)) {
        std::cerr << "[BFI_condor] Failed to build cuts for bin " << binName << "\n";
        delete BFI;
        return 3;
    }

    // Open ROOT file
    TFile* infile = TFile::Open(rootFilePath.c_str(), "READ");
    if (!infile || infile->IsZombie()) {
        std::cerr << "[BFI_condor] ERROR opening ROOT file: " << rootFilePath << "\n";
        if (infile) infile->Close();
        delete BFI;
        return 4;
    }

    // -------------------------
    // Process the file
    // -------------------------
    // We'll collect results per-sample here. map: sampleName -> [count, weighted_sum, error]
    std::map<std::string, std::array<double,3>> results;

    // --- SAMPLE LOOP ---
    // You need to adapt the below to your SampleTool API.
    // Example assumed API:
    //   auto sampleNames = sampleTool.GetSampleNames();   // vector<string>
    //   sampleTool knows how samples map to files/trees, etc.
    //
    // For each sample you will:
    //   - ensure this ROOT file corresponds to that sample (or skip if not)
    //   - run the selection/cuts
    //   - compute: count, weighted_sum, error (or sumW, sumW2 -> error = sqrt(sumW2))
    //
    // If you already have a function in BuildFitInput that processes a file for a given
    // sample and bin, call that here. Example:
    //    auto out = BFI->ProcessFileForSample(rootFilePath, sampleName, cuts);
    //    // where 'out' returns struct { long long count; double sumW; double err; }
    //
    // Below are placeholders that you should replace with your concrete calls.

    // Placeholder: retrieve list of samples (replace with your API)
    //std::vector<std::string> sampleNames = sampleTool.GetAllSampleNames(); // <-- adapt or implement

    //for (const auto &sname : sampleNames) {
        // Optionally: check if this ROOT file belongs to this sample (depends on your workflow)
        // If each file in your farming is one file from one sample, you might map file->sample
        // and skip samples that don't match.

        // TODO: replace the following stub with the real per-sample processing call:
        // Example (pseudocode):
        // auto stats = BFI->ProcessRootFileForSample(infile, sname, cuts);
        // results[sname] = { (double)stats.count, stats.sumW, stats.err };
        //
        // For now, populate zeros so the JSON structure is produced.
        //results[sname] = { 0.0, 0.0, 0.0 };
    //}

    // Close file
    infile->Close();

    // -------------------------
    // Write JSON
    // -------------------------
    if (!writePartialJSON(outputJsonPath, binName, results)) {
        std::cerr << "[BFI_condor] ERROR: could not write output JSON to " << outputJsonPath << "\n";
        delete BFI;
        return 5;
    }

    // Cleanup
    delete BFI;
    return 0;
}

