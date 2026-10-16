#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <map>
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
#include "TStyle.h"

namespace {

constexpr int kMassQuantumPerGeV = 100;
constexpr int kYBinWidthGeV = 10;
constexpr int kMinimumParentMass = 100 * kMassQuantumPerGeV;
constexpr double kWeightScale = 100.0;

float UnquantizeMass(int massInt)
{
    return static_cast<float>(massInt) / kMassQuantumPerGeV;
}

int YBinLowerMass(int yMass)
{
    const int yBinWidth = kYBinWidthGeV * kMassQuantumPerGeV;
    return (yMass / yBinWidth) * yBinWidth;
}

double RoundToSignificantFigures(double value, int significantFigures)
{
    if (value == 0.0 || !std::isfinite(value)) {
        return value;
    }
    const double decimalPlaces = significantFigures - 1 - std::floor(std::log10(std::abs(value)));
    const double scale = std::pow(10.0, decimalPlaces);
    return std::round(value * scale) / scale;
}

struct TreeResult {
    int xMass;
    int yMass;
    double eventCount;
    double scaledWeightSum;
    std::vector<double> means;
};

using ZAxisRanges = std::map<std::string, std::pair<double, double>>;

bool IsNumericLeaf(const TLeaf* leaf)
{
    const std::string type = leaf->GetTypeName();
    return type == "Char_t" || type == "UChar_t" || type == "Short_t" ||
           type == "UShort_t" || type == "Int_t" || type == "UInt_t" ||
           type == "Long_t" || type == "ULong_t" || type == "Long64_t" ||
           type == "ULong64_t" || type == "Float_t" || type == "Double_t";
}

std::string BranchNameToTitle(const std::string& branchName)
{
    std::string text = branchName;
    size_t pos = text.find("_LEP");
    if (pos != std::string::npos) {
        // Syntax: replace(starting_index, length_to_erase, new_string)
        text.replace(pos, 4, "");
    }
    return text;
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

double TreeSum(TTree* tree, const std::string& branchName)
{
    TLeaf* leaf = tree->GetLeaf(branchName.c_str());
    if (leaf == nullptr || leaf->GetLeafCount() != nullptr || !IsNumericLeaf(leaf)) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    double sum = 0.0;
    Long64_t finiteValues = 0;
    const Long64_t entries = tree->GetEntries();
    for (Long64_t entry = 0; entry < entries; ++entry) {
        tree->GetEntry(entry);
        const double value = leaf->GetValue();
        if (std::isfinite(value)) {
            sum += value;
            ++finiteValues;
        }
    }
    return finiteValues == 0 && entries > 0
               ? std::numeric_limits<double>::quiet_NaN()
               : sum;
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

std::string InputNameToTitle(const std::string& inputName)
{
    const std::size_t slashPosition = inputName.find_last_of('/');
    std::string title = slashPosition == std::string::npos
                            ? inputName
                            : inputName.substr(slashPosition + 1);
    const std::size_t summerPosition = title.find("_Summer");
    if (summerPosition != std::string::npos) {
        const std::size_t campaignStart = summerPosition + 1;
        const std::size_t campaignEnd = title.find('_', campaignStart);
        const std::string campaign = title.substr(
            campaignStart, campaignEnd == std::string::npos
                               ? std::string::npos
                               : campaignEnd - campaignStart);
        title = campaign + " " + title.substr(0, summerPosition);
    }
    return title;
}

} // namespace

int main(int argc, char** argv)
{
    gROOT->SetBatch(kTRUE);
    gStyle->SetOptStat(0);
    gStyle->SetTitleFontSize(0.05);

    if (argc != 1) {
        std::cerr << "Usage: " << argv[0] << std::endl;
        return EXIT_FAILURE;
    }

    const std::vector<std::string> inputFiles = {
        "root://cmseos.fnal.gov//store/user/lpcsusylep/NTUPLES_Cascades_v10/Summer24_130X_SMS/SMS-TChiWZ_Bin-genHT-50-genMET-80_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer24_130X.root",
        "root://cmseos.fnal.gov//store/user/lpcsusylep/NTUPLES_Cascades_v10/Summer24_130X_SMS/SMS-TChiWW_Bin-genHT-50-genMET-80_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer24_130X.root",
        "root://cmseos.fnal.gov//store/user/lpcsusylep/NTUPLES_Cascades_v10/Summer24_130X_SMS/SMS-TSlepSlep_Bin-genHT-50-genMET-80_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer24_130X.root",
    };
    const std::vector<std::string> plotBranches = {"RISR_LEP", "Mperp_LEP"};
    const ZAxisRanges zAxisRanges = {
        {"RISR_LEP", {0.7, 1.0}},
        {"Mperp_LEP", {0.0, 100.0}},
    };

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
        std::vector<int> deltaMasses;

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
            if (result.xMass < kMinimumParentMass || result.xMass > 65000 ||
                (result.xMass / 100. - result.yMass / 100.) > 135) { // remove low parent masses and weird DMs
                delete tree;
                continue;
            }
            result.eventCount = static_cast<double>(tree->GetEntries());
            result.scaledWeightSum = TreeSum(tree, "weight") * kWeightScale;
            for (const std::string& branchName : plotBranches) {
                result.means.push_back(TreeMean(tree, branchName));
            }
            results.push_back(result);
            xMasses.push_back(result.xMass);
            yMasses.push_back(result.yMass);
            deltaMasses.push_back(result.xMass - result.yMass);
            delete tree;
        }

        if (results.empty()) {
            std::cerr << "No nominal SMS_<mass>_<mass> trees found in " << inputName << std::endl;
            continue;
        }

        std::sort(xMasses.begin(), xMasses.end());
        xMasses.erase(std::unique(xMasses.begin(), xMasses.end()), xMasses.end());
        for (int& yMass : yMasses) {
            yMass = YBinLowerMass(yMass);
        }
        std::sort(yMasses.begin(), yMasses.end());
        yMasses.erase(std::unique(yMasses.begin(), yMasses.end()), yMasses.end());
        for (int& deltaMass : deltaMasses) {
            deltaMass = YBinLowerMass(deltaMass);
        }
        std::sort(deltaMasses.begin(), deltaMasses.end());
        deltaMasses.erase(std::unique(deltaMasses.begin(), deltaMasses.end()), deltaMasses.end());

        const std::string outputStem = OutputStem(inputName);
        const std::string inputTitle = InputNameToTitle(inputName);
        const std::string outputName = outputStem + ".root";
        TFile outputFile(outputName.c_str(), "RECREATE");
        const auto makePlot = [&](const std::string& plotName,
                                  const std::string& zTitle,
                                  const std::vector<int>& yBins,
                                  const std::string& yTitle,
                                  const bool useDeltaMass,
                                  const bool sumAcrossTrees,
                                  const auto& valueForTree) {
            const auto drawMap = [&](const std::string& suffix,
                                     const std::vector<int>& yMassBins,
                                     const std::string& yAxisTitle,
                                     const bool useDeltaMassBins) {
                const std::string histogramName = "sms_mass_map_" + plotName + suffix;
                std::cout << "Creating histogram for branch: " << plotName
                          << (useDeltaMassBins ? " (DM)" : "") << std::endl;
                TH2D map(histogramName.c_str(), (inputTitle + ";Parent mass [GeV];" + yAxisTitle).c_str(),
                         xMasses.size(), 0.0, static_cast<double>(xMasses.size()),
                         yMassBins.size(), 0.0, static_cast<double>(yMassBins.size()));
                map.GetZaxis()->SetTitle(zTitle.c_str());
                for (std::size_t index = 0; index < xMasses.size(); ++index) {
                    map.GetXaxis()->SetBinLabel(index + 1, std::to_string(int(UnquantizeMass(xMasses[index]))).c_str());
                }
                for (std::size_t index = 0; index < yMassBins.size(); ++index) {
                    const int lowerEdge = static_cast<int>(UnquantizeMass(yMassBins[index]));
                    const std::string label = std::to_string(lowerEdge) + "-" +
                                               std::to_string(lowerEdge + kYBinWidthGeV);
                    map.GetYaxis()->SetBinLabel(index + 1, label.c_str());
                }

                std::map<std::pair<int, int>, std::pair<double, int>> binSums;
                for (const TreeResult& result : results) {
                    const auto xBin = std::find(xMasses.begin(), xMasses.end(), result.xMass) - xMasses.begin() + 1;
                    const int yMass = useDeltaMassBins ? result.xMass - result.yMass : result.yMass;
                    const int yBinMass = YBinLowerMass(yMass);
                    const auto yBin = std::find(yMassBins.begin(), yMassBins.end(), yBinMass) - yMassBins.begin() + 1;
                    const double value = valueForTree(result);
                    if (std::isfinite(value)) {
                        auto& aggregate = binSums[{xBin, yBin}];
                        aggregate.first += value;
                        ++aggregate.second;
                    }
                }
                for (const auto& [bin, aggregate] : binSums) {
                    map.SetBinContent(bin.first, bin.second,
                                      RoundToSignificantFigures(
                                          sumAcrossTrees ? aggregate.first : aggregate.first / aggregate.second, 3));
                }

                const auto zRange = zAxisRanges.find(plotName);
                if (zRange != zAxisRanges.end()) {
                    map.SetMinimum(zRange->second.first);
                    map.SetMaximum(zRange->second.second);
                }
                map.Write();
                TCanvas canvas(("canvas_" + plotName + suffix).c_str(), plotName.c_str(), 1000, 800);
                canvas.SetRightMargin(0.15);
                canvas.SetLeftMargin(0.15);
                map.GetXaxis()->CenterTitle();
                map.GetYaxis()->CenterTitle();
                map.GetZaxis()->CenterTitle();
                map.GetZaxis()->SetTitleOffset(1.15);
                map.Draw("COLZ");
                canvas.Write();
                canvas.SaveAs((outputStem + "_" + plotName + suffix + ".png").c_str());
            };

            drawMap("", yBins, yTitle, useDeltaMass);
            drawMap("_DM", deltaMasses, "Parent mass - LSP mass [GeV]", true);
        };

        for (std::size_t branchIndex = 0; branchIndex < plotBranches.size(); ++branchIndex) {
            const std::string& branchName = plotBranches[branchIndex];
            makePlot(branchName, BranchNameToTitle(branchName) + " mean", yMasses,
                     "LSP mass [GeV]", false, false,
                     [branchIndex](const TreeResult& result) { return result.means[branchIndex]; });
        }
        makePlot("event_count", "Events per mass point", yMasses, "LSP mass [GeV]", false, true,
                 [](const TreeResult& result) { return result.eventCount; });
        makePlot("weight_sum_Lumi", "Sum of weight x Lumi (100 fb^-1)", yMasses, "LSP mass [GeV]", false, true,
                 [](const TreeResult& result) { return result.scaledWeightSum; });
        outputFile.Close();
        std::cout << "Wrote " << results.size() << " nominal trees to " << outputName << std::endl;
    }
    std::cout << "Finished processing all input files." << std::endl;
    return EXIT_SUCCESS;
}