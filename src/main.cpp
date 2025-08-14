#include "SampleTool.h"
#include "BuildFitInput.h"
#include "JSONFactory.h"

int main() {
	double Lumi= 400.;
	SampleTool* ST = new SampleTool();
	
	stringlist bkglist = {"ttbar", "ST", "DY", "ZInv", "DBTB", "QCD", "Wjets"};
	stringlist siglist = {"Cascades"};
	
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
        std::string RISR= "&& (RISR>0.7)";
        std::string PTISR="&& (PTISR>250)";
        std::string BVeto="&& (Nbjet_ISR==0 && Nbjet_S==0)";
        std::string jet="&& (Njet>0)";
	
	BFI->FilterRegions("TEST", lep+met+RISR+PTISR+BVeto+jet);
        BFI->CreateBin("TEST");

	//book operations
	countmap countResults = BFI->CountRegions(BFI->bkg_filtered_dataframes);
	countmap countResults_S = BFI->CountRegions(BFI->sig_filtered_dataframes); 
	
	summap sumResults = BFI->SumRegions("weight",BFI->bkg_filtered_dataframes, Lumi);
	summap sumResults_S = BFI->SumRegions("weight",BFI->sig_filtered_dataframes, Lumi);
	
	//initiate action
	BFI->ReportRegions(0);

	//compute errors and report bins
        errormap errorResults = BFI->ComputeStatError(BFI->bkg_filtered_dataframes, Lumi);
        std::cout << "Computed stat error for bkg \n";
        errormap errorResults_S = BFI->ComputeStatError(BFI->sig_filtered_dataframes, Lumi);
        std::cout << "Computed stat error for sig \n";
	//BFI->FullReport( countResults, sumResults, errorResults );
	
	//aggregate maps into more easily useable classes
	BFI->ConstructBkgBinObjects( countResults, sumResults, errorResults );
	BFI->AddSigToBinObjects( countResults_S, sumResults_S, errorResults_S, BFI->analysisbins);
	BFI->PrintBins(1);
	
        std::cout << "Making json... \n";
	JSONFactory* json = new JSONFactory(BFI->analysisbins);
	json->WriteJSON("./json/test_eos.json");
}
