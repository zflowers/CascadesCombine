#include "SampleTool.h"
#include "BuildFitInput.h"
#include "JSONFactory.h"
#include <chrono> // for timer

int main() {
 	auto start = std::chrono::high_resolution_clock::now();
	double Lumi= 400.;
	SampleTool* ST = new SampleTool();
	
	stringlist bkglist = {"ttbar", "ST", "DY", "ZInv", "DBTB", "QCD", "Wjets"};
	stringlist siglist = {"Cascades"};
	//stringlist siglist = {"SMS_Gluinos"};
        // only process SMS signals with given mass point(s)
        //BFTool::filterSignalsSMS = {"SMS_1000_900"};
	
	ST->LoadBkgs( bkglist );
	ST->LoadSigs( siglist );
	
	ST->PrintDict(ST->BkgDict);
	ST->PrintDict(ST->SigDict);
	ST->PrintKeys(ST->SignalKeys);
	
	BuildFitInput* BFI = new BuildFitInput();
	BFI->LoadBkg_byMap(ST->BkgDict, Lumi);
	BFI->LoadSig_byMap(ST->SigDict, Lumi);
        // Register custom macros if needed
	BFI->RegisterMacro("AVG", "ROOT::VecOps::Mean");
	
        std::string met="MET>=150";
        std::string RISR="RISR>=0.85";
        std::string RISR_Upper="RISR<=1";
        std::string PTISR="PTISR>=250";
        std::string BVeto="Nbjet==0";
        std::string jet="Njet>0";
        std::string Mperp="Mperp<15";
        std::string maxSIP3D = "MAX(SIP3D_lep)<3";
        std::string CleaningCut = BFI->GetCleaningCut();
        std::string ZstarCut = BFI->GetZstarCut();
        std::string e2mu = "Nmu==2";
        std::string lep="Nlep>=2";
        std::string e2Pos = BFI->BuildLeptonCut("=2Pos");
        std::string e0Bronze = BFI->BuildLeptonCut("=0Bronze");
        std::string e2Gold = BFI->BuildLeptonCut("=2Gold");
        std::string ge1OSSF = BFI->BuildLeptonCut(">=1OSSF");
        std::string e1SSOF = BFI->BuildLeptonCut("=1SSOF");
        std::string ge1Elec = BFI->BuildLeptonCut(">=1Elec");
        std::string l1SSSF = BFI->BuildLeptonCut("<1SSSF");
        std::string ge1Elec_a = BFI->BuildLeptonCut(">=1Elec_a");
        std::string g1Pos_b = BFI->BuildLeptonCut(">=1Pos_b");
        std::string ge1OSSF_a = BFI->BuildLeptonCut(">=1OSSF_a"); 
	
	// Initial region
	BFI->CreateBin("TEST", {lep, met, RISR, RISR_Upper, PTISR, BVeto, jet, Mperp, maxSIP3D, CleaningCut});
	BFI->CreateBin("TEST_Zstar", {lep, met, RISR, RISR_Upper, PTISR, BVeto, jet, Mperp, maxSIP3D, CleaningCut, ZstarCut});
        std::cout << "Created Bins \n";


std::cout << "\n===== BEGIN CLOSURE TEST =====\n";

// ----------------------- Closure sweep with side-diagnostics -----------------------
std::vector<std::string> shorthands = {
    "=2Pos",
    "=0Bronze",
    "=2Gold",
    ">=1OSSF",
    "=1SSOF",
    ">=2Mu",
    ">=1Elec",
    "<1SSSF",
    ">=1Muon"
};
std::vector<std::string> sides = {
"",
"a",
"b"
};

std::cout << "\n[TEST] Building shorthand x side test bins (with diagnostics)...\n";
for (const auto &side : sides) {
    for (const auto &sh : shorthands) {
        // build expression
        std::string expr = BFI->BuildLeptonCut(sh, side);
        if (expr.empty()) {
            std::cout << "[SKIP] shorthand=\"" << sh << "\" side=\"" << (side.empty() ? "(none)" : side)
                      << "\" -> BuildLeptonCut returned empty (invalid shorthand)\n";
            continue;
        }

        std::cout << "[BIN-DEF] shorthand=\"" << sh
                  << " -> expr: " << expr << "\n";

        // create a unique name and the combined cut
        std::string filterName = "TEST_" + sh + (side.empty() ? "_All" : "_" + side);
        std::string filterCut  = lep + " && " + expr;

        BFI->CreateBin(filterName, {filterCut});
    }
}

std::cout << "\n[TEST] Finished creating shorthand test bins. Running RDF booking & event loop...\n";

// Book operations
countmap countResults   = BFI->CountRegions(BFI->bkg_filtered_dataframes);
countmap countResults_S = BFI->CountRegions(BFI->sig_filtered_dataframes);

summap sumResults   = BFI->SumRegions("weight_scaled",    BFI->bkg_filtered_dataframes);
summap sumResults_S = BFI->SumRegions("weight_scaled",    BFI->sig_filtered_dataframes);

summap sumW2Results   = BFI->SumRegions("weight_sq_scaled", BFI->bkg_filtered_dataframes);
summap sumW2Results_S = BFI->SumRegions("weight_sq_scaled", BFI->sig_filtered_dataframes);

// Trigger the event loop (computes RResultPtr values)
BFI->ReportRegions(0);

// Compute stat errors
errormap errorResults   = BFI->ComputeStatErrorFromSumW2(sumW2Results);
errormap errorResults_S = BFI->ComputeStatErrorFromSumW2(sumW2Results_S);

// Build in-memory objects
BFI->ConstructBkgBinObjects( countResults, sumResults, errorResults );
BFI->AddSigToBinObjects(    countResults_S, sumResults_S, errorResults_S, BFI->analysisbins );

// Compact closure report for backgrounds
std::cout << "\n[CLOSURE REPORT - BACKGROUNDS]\n";
for (auto &kv : countResults) {
    const proc_cut_pair key = kv.first;     // (proc, bin)
    const std::string procName = key.first;
    const std::string binName  = key.second;

    // kv.second is RResultPtr<unsigned long long> (non-const iteration so GetValue() is callable)
    unsigned long long count = kv.second.GetValue();

    double sum = 0.0, sumw2 = 0.0, err = 0.0;
    try {
        sum   = sumResults.at(key).GetValue();
        sumw2 = sumW2Results.at(key).GetValue();
        err   = errorResults.at(key);
    } catch (const std::out_of_range&) {
        std::cout << "[WARN] Missing sum/sumW2/error for " << procName << " / " << binName << "\n";
    }

    std::cout << "[BKG] Proc=\"" << procName << "\" Bin=\"" << binName
              << "\" Count=" << count
              << " Sum=" << sum << " +/- " << err
              << " (sumW2=" << sumw2 << ")\n";
}

// Compact closure report for signals
std::cout << "\n[CLOSURE REPORT - SIGNALS]\n";
for (auto &kv : countResults_S) {
    const proc_cut_pair key = kv.first;
    const std::string procName = key.first;
    const std::string binName  = key.second;

    unsigned long long count = kv.second.GetValue();

    double sum = 0.0, sumw2 = 0.0, err = 0.0;
    try {
        sum   = sumResults_S.at(key).GetValue();
        sumw2 = sumW2Results_S.at(key).GetValue();
        err   = errorResults_S.at(key);
    } catch (const std::out_of_range&) {
        std::cout << "[WARN] Missing signal sum/sumW2/error for " << procName << " / " << binName << "\n";
    }

    std::cout << "[SIG] Proc=\"" << procName << "\" Bin=\"" << binName
              << "\" Count=" << count
              << " Sum=" << sum << " +/- " << err
              << " (sumW2=" << sumw2 << ")\n";
}

// Print your usual detailed bin summary
BFI->PrintBins(1);

std::cout << "[TEST] Closure sweep complete.\n";
// -------------------------------------------------------------------------------

std::cout << "\n===== END CLOSURE TEST =====\n";



/*
	// Book operations
	countmap countResults   = BFI->CountRegions(BFI->bkg_filtered_dataframes);
	countmap countResults_S = BFI->CountRegions(BFI->sig_filtered_dataframes);
	
	// Book weighted sums
	summap sumResults   = BFI->SumRegions("weight_scaled",    BFI->bkg_filtered_dataframes);
	summap sumResults_S = BFI->SumRegions("weight_scaled",    BFI->sig_filtered_dataframes);
	
	// Book sum of squared weights for stat error
	summap sumW2Results   = BFI->SumRegions("weight_sq_scaled", BFI->bkg_filtered_dataframes);
	summap sumW2Results_S = BFI->SumRegions("weight_sq_scaled", BFI->sig_filtered_dataframes);
	
	// Trigger the event loop
	BFI->ReportRegions(0);
	
	errormap errorResults   = BFI->ComputeStatErrorFromSumW2(sumW2Results);
	errormap errorResults_S = BFI->ComputeStatErrorFromSumW2(sumW2Results_S);
	//BFI->FullReport( countResults, sumResults, errorResults );
	
	//aggregate maps into more easily useable classes
	BFI->ConstructBkgBinObjects( countResults, sumResults, errorResults );
	BFI->AddSigToBinObjects( countResults_S, sumResults_S, errorResults_S, BFI->analysisbins);
	BFI->PrintBins(1);
*/

	
        std::cout << "Making json... \n";
	JSONFactory* json = new JSONFactory(BFI->analysisbins);
	json->WriteJSON("./json/test_cascades.json");
	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = end - start;
	std::cout << "Took " << elapsed.count() << " seconds to produce BFI" << std::endl;
	return 0;
}
