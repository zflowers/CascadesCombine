


#include "SampleTool.h"
#include "BuildFitInput.h"
#include "JSONFactory.h"

int main() {
	double Lumi= 400.;
	SampleTool* ST = new SampleTool();
	
	stringlist bkglist = {"Wjets", "Zjets", "Top", "Gjets"};
	stringlist siglist = {"gogoG"};
	
	ST->LoadBkgs( bkglist );
	ST->LoadSigs( siglist );
	
	ST->PrintDict(ST->BkgDict);
	ST->PrintDict(ST->SigDict);
	ST->PrintKeys(ST->SignalKeys);
	
	BuildFitInput* BFI = new BuildFitInput();
	BFI->LoadBkg_byMap(ST->BkgDict, Lumi);
	BFI->LoadSig_byMap(ST->SigDict, Lumi);
	BFI->BuildRVBranch();
	

	std::string pho1= "(nSelPhotons==1)";
	std::string pho2= "(nSelPhotons==2)";
	std::string MMT = "&& (rjrASMass[1] > 2750) && (rjrAX2NQSum[1] > 0.275) && (Rv > 0.3)";
	std::string LLL = "&& (rjrASMass[1] > 2000) && (rjrAX2NQSum[1] > 0.2) && (Rv > 0.0)"; 
	std::string jets12 ="&&( ( (rjrNJetsJa[1] == 1) && (rjrNJetsJb[1] >= 1 ) ) || ( (rjrNJetsJa[1] >=1 ) && (rjrNJetsJb[1] == 1) ) )" ;
	std::string jets22="&& (rjrNJetsJa[1] >= 2) && (rjrNJetsJb[1] >= 2)";
	
	
	BFI->FilterRegions( "1Photon MMT =1+1j", pho1+MMT+jets12);
	BFI->FilterRegions( "1Photon MMT >=2+2j", pho1+MMT+jets22);
	BFI->FilterRegions( "2Photon LLL", pho2+LLL);
	BFI->CreateBin("1Photon MMT =1+1j");
	BFI->CreateBin("1Photon MMT >=2+2j");
	BFI->CreateBin("2Photon LLL");

	//book operations
	countmap countResults = BFI->CountRegions(BFI->bkg_filtered_dataframes);
	countmap countResults_S = BFI->CountRegions(BFI->sig_filtered_dataframes); 
	
	summap sumResults = BFI->SumRegions("evtwt",BFI->bkg_filtered_dataframes );
	summap sumResults_S = BFI->SumRegions("evtwt",BFI->sig_filtered_dataframes);
	
	//initiate action
	BFI->ReportRegions(0);

	//compute errors and report bins
	errormap errorResults = BFI->ComputeStatError( countResults, BFI->bkg_evtwt );
	errormap errorResults_S = BFI->ComputeStatError( countResults_S, BFI->sig_evtwt);
	//BFI->FullReport( countResults, sumResults, errorResults );
	
	//aggregate maps into more easily useable classes
	BFI->ConstructBkgBinObjects( countResults, sumResults, errorResults );
	BFI->AddSigToBinObjects( countResults_S, sumResults_S, errorResults_S, BFI->analysisbins);
	BFI->PrintBins(1);
	
	JSONFactory* json = new JSONFactory(BFI->analysisbins);
	json->WriteJSON("test.json");
}
