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
        BFTool::filterSignalsSMS = {"SMS_1000_900"};
	
	ST->LoadBkgs( bkglist );
	ST->LoadSigs( siglist );
	
	ST->PrintDict(ST->BkgDict);
	ST->PrintDict(ST->SigDict);
	ST->PrintKeys(ST->SignalKeys);
	
	BuildFitInput* BFI = new BuildFitInput();
	BFI->LoadBkg_byMap(ST->BkgDict, Lumi);
	BFI->LoadSig_byMap(ST->SigDict, Lumi);
	
        std::string lep= "(Nlep==3)";
        std::string met= "&& (MET>150)";
        std::string RISR= "&& (RISR>=0.7)";
        std::string RISR_Upper= "&& (RISR<=1)";
        std::string PTISR="&& (PTISR>250)";
        std::string BVeto="&& (Nbjet_ISR==0 && Nbjet_S==0)";
        std::string jet="true"; // "&& (Njet>0)"
	
	//BFI->FilterRegions("TEST", lep+met+RISR+RISR_Upper+PTISR+BVeto+jet);
	BFI->FilterRegions("TEST", jet);
        BFI->CreateBin("TEST");

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
	//BFI->FullReport( countResults, sumResults, errorResults );
	
	errormap errorResults   = BFI->ComputeStatErrorFromSumW2(sumW2Results);
	errormap errorResults_S = BFI->ComputeStatErrorFromSumW2(sumW2Results_S);
	
	//aggregate maps into more easily useable classes
	BFI->ConstructBkgBinObjects( countResults, sumResults, errorResults );
	BFI->AddSigToBinObjects( countResults_S, sumResults_S, errorResults_S, BFI->analysisbins);
	BFI->PrintBins(1);
	
        std::cout << "Making json... \n";
	JSONFactory* json = new JSONFactory(BFI->analysisbins);
	json->WriteJSON("./json/test_eos.json");
	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = end - start;
	std::cout << "Took " << elapsed.count() << " seconds to produce BFI" << std::endl;
	return 0;
}
