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

	// Declare maps for bkg and signal
	countmap countResults, countResults_S;
	summap sumResults, sumResults_S;
	errormap errorResults, errorResults_S;
	
	// Compute counts, sums, errors for background and signal
	BFI->ReportRegions(0, countResults, sumResults, errorResults, false);
	BFI->ReportRegions(0, countResults_S, sumResults_S, errorResults_S, true);
	
	// Construct bins
	BFI->ConstructBkgBinObjects(countResults, sumResults, errorResults);
	BFI->AddSigToBinObjects(countResults_S, sumResults_S, errorResults_S, BFI->analysisbins);

	//BFI->FullReport( countResults, sumResults, errorResults );
	BFI->PrintBins(1);
	
        std::cout << "Making json... \n";
	JSONFactory* json = new JSONFactory(BFI->analysisbins);
	json->WriteJSON("./json/test_cascades.json");
	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = end - start;
	std::cout << "Took " << elapsed.count() << " seconds to produce BFI" << std::endl;
	return 0;
}
