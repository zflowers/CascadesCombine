#include <map>
#include <utility>
#include <cmath>
#include <iostream>
#include <string>

#include "TFile.h"
#include "TMatrixDSym.h"
#include "RooFitResult.h"
#include "RooArgList.h"

using namespace std;

void FitDiagCov(){
        std::map<double,std::pair<int,int>> rho_map;
        std::string fit_diag_file = "runs/run_Cascades_CRFit_Impacts_FD_234L_Run2Run3_v419_June30_2026_1144_run_July08_2026_1220/datacards/SMS_TChiWZ_SMS_500_450/fitDiagnostics.Test.root";
        TFile* f = new TFile(fit_diag_file.c_str());
	RooFitResult* r = (RooFitResult*) f->Get("fit_b");
	TMatrixDSym m = r->covarianceMatrix();
	RooArgList l = r->floatParsFinal();
	int n=m.GetNrows();
	double x = 999;
	double x1 = 999;
	double x2 = 999;
	double rho =-1;
	for(int i=0; i<n-1; i++){
		for(int j=i+1; j<n; j++){
			x = m(i,j);
			x1 = sqrt(m(i,i));
			x2 = sqrt(m(j,j));
			rho = x/(x1*x2);
			if( fabs(rho) > 0.1){
				rho_map.insert(std::make_pair(fabs(rho),std::make_pair(i,j)));

			}
		}		
	}
        for(std::map<double,std::pair<int,int>>::iterator iter = rho_map.begin(); iter != rho_map.end(); iter++)
	{
		int i = iter->second.first;
		int j = iter->second.second;
		x = m(i,j);
		x1 = sqrt(m(i,i));
		x2 = sqrt(m(j,j));
		rho = x/(x1*x2);
		std::cout<<"("<<i<<","<<j<<") Cov: "<< x <<" CorrCoeff: "<<rho<<"\n";
		l[i].Print();
		l[j].Print();
		std::cout<<"\n";
	}
}
