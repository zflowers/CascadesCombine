#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <memory>
#include <regex>
#include <string>
#include <vector>

#include "TCanvas.h"
#include "TFile.h"
#include "TH2D.h"
#include "TKey.h"
#include "TLeaf.h"
#include "TList.h"
#include "TROOT.h"
#include "TTree.h"

namespace {

constexpr int kMassQuantumPerGeV = 100;

float UnquantizeMass(int massInt)
{
    return static_cast<float>(massInt) / kMassQuantumPerGeV;
}

struct TreeResult {
    int xMass;
    int yMass;
    std::vector<double> means;
};

bool IsNumericLeaf(const TLeaf* leaf)
{
    const std::string type = leaf->GetTypeName();
    return type == "Char_t" || type == "UChar_t" || type == "Short_t" ||
           type == "UShort_t" || type == "Int_t" || type == "UInt_t" ||
           type == "Long_t" || type == "ULong_t" || type == "Long64_t" ||
           type == "ULong64_t" || type == "Float_t" || type == "Double_t";
}

double TreeMean(TTree* tree, const std::string& branchName)
{
    TLeaf* leaf = tree->GetLeaf(branchName.c_str());
    if (leaf == nullptr || leaf->GetLeafCount() != nullptr || !IsNumericLeaf(leaf)) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    double sum = 0.0;
    Long64_t count = 0;
    const Long64_t entries = tree->GetEntries();
    for (Long64_t entry = 0; entry < entries; ++entry) {
        tree->GetEntry(entry);
        const double value = leaf->GetValue();
        if (std::isfinite(value)) {
            sum += value;
            ++count;
        }
    }
    return count == 0 ? std::numeric_limits<double>::quiet_NaN() : sum / count;
}

std::string OutputStem(const std::string& inputName)
{
    const std::size_t slashPosition = inputName.find_last_of('/');
    std::string stem = slashPosition == std::string::npos
                           ? inputName
                           : inputName.substr(slashPosition + 1);
    const std::size_t extensionPosition = stem.rfind(".root");
    if (extensionPosition != std::string::npos) {
        stem.erase(extensionPosition);
    }
    return stem + "_sms_mass_map";
}

} // namespace

int main(int argc, char** argv)
{
    gROOT->SetBatch(kTRUE);

    if (argc != 1) {
        std::cerr << "Usage: " << argv[0] << std::endl;
        return EXIT_FAILURE;
    }

    const std::vector<std::string> inputFiles = {
        "root://cmseos.fnal.gov//store/user/lpcsusylep/NTUPLES_Cascades_v10/Summer24_130X_SMS/SMS-TChiWZ_Bin-genHT-50-genMET-80_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer24_130X.root",
    };
    const std::vector<std::string> plotBranches = {"RISR_LEP", "Mperp_LEP"};

    const std::regex nominalTreePattern(R"(^SMS_([0-9]+)_([0-9]+)$)");
    for (const std::string& inputName : inputFiles) {
        std::unique_ptr<TFile> inputFile(TFile::Open(inputName.c_str(), "READ"));
        if (inputFile == nullptr || inputFile->IsZombie()) {
            std::cerr << "Failed to open input file: " << inputName << std::endl;
            continue;
        }
        std::cout << "Processing input file: " << inputName << std::endl;

        std::vector<TreeResult> results;
        std::vector<int> xMasses;
        std::vector<int> yMasses;

        TIter keyIterator(inputFile->GetListOfKeys());
        while (TObject* object = keyIterator()) {
            auto* key = dynamic_cast<TKey*>(object);
            if (key == nullptr || std::string(key->GetClassName()) != "TTree") {
                continue;
            }

            std::smatch match;
            const std::string treeName = key->GetName();
            if (!std::regex_match(treeName, match, nominalTreePattern)) {
                continue;
            }
            std::cout << "Found nominal SMS tree: " << treeName << std::endl;

            auto* tree = dynamic_cast<TTree*>(key->ReadObj());
            if (tree == nullptr) {
                continue;
            }

            TreeResult result;
            result.xMass = std::stoi(match[1]);
            result.yMass = std::stoi(match[2]);
            for (const std::string& branchName : plotBranches) {
                result.means.push_back(TreeMean(tree, branchName));
            }
            results.push_back(result);
            xMasses.push_back(result.xMass);
            yMasses.push_back(result.yMass);
            delete tree;
        }

        if (results.empty()) {
            std::cerr << "No nominal SMS_<mass>_<mass> trees found in " << inputName << std::endl;
            continue;
        }

        std::sort(xMasses.begin(), xMasses.end());
        xMasses.erase(std::unique(xMasses.begin(), xMasses.end()), xMasses.end());
        std::sort(yMasses.begin(), yMasses.end());
        yMasses.erase(std::unique(yMasses.begin(), yMasses.end()), yMasses.end());

        const std::string outputStem = OutputStem(inputName);
        const std::string outputName = outputStem + ".root";
        TFile outputFile(outputName.c_str(), "RECREATE");
        for (std::size_t branchIndex = 0; branchIndex < plotBranches.size(); ++branchIndex) {
            const std::string& branchName = plotBranches[branchIndex];
            const std::string histogramName = "sms_mass_map_" + branchName;
            std::cout << "Creating histogram for branch: " << branchName << std::endl;
            TH2D map(histogramName.c_str(), (branchName + " mean;first mass [GeV];second mass [GeV]").c_str(),
                     xMasses.size(), 0.0, static_cast<double>(xMasses.size()),
                     yMasses.size(), 0.0, static_cast<double>(yMasses.size()));
            for (std::size_t index = 0; index < xMasses.size(); ++index) {
                map.GetXaxis()->SetBinLabel(index + 1, std::to_string(UnquantizeMass(xMasses[index])).c_str());
            }
            for (std::size_t index = 0; index < yMasses.size(); ++index) {
                map.GetYaxis()->SetBinLabel(index + 1, std::to_string(UnquantizeMass(yMasses[index])).c_str());
            }

            for (const TreeResult& result : results) {
                const auto xBin = std::find(xMasses.begin(), xMasses.end(), result.xMass) - xMasses.begin() + 1;
                const auto yBin = std::find(yMasses.begin(), yMasses.end(), result.yMass) - yMasses.begin() + 1;
                if (std::isfinite(result.means[branchIndex])) {
                    map.SetBinContent(xBin, yBin, result.means[branchIndex]);
                }
            }

            map.Write();
            TCanvas canvas(("canvas_" + branchName).c_str(), branchName.c_str(), 1000, 800);
            map.Draw("COLZ TEXT");
            canvas.Write();
            canvas.SaveAs((outputStem + "_" + branchName + ".png").c_str());
        }
        outputFile.Close();
        std::cout << "Wrote " << results.size() << " nominal trees to " << outputName << std::endl;
    }
    std::cout << "Finished processing all input files." << std::endl;
    return EXIT_SUCCESS;
}