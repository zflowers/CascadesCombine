#include "SampleTool.h"

SampleTool::SampleTool(){

  string pathPrefix = "root://cmseos.fnal.gov//store/user/lpcsusylep/NTUPLES_Cascades_v3/";

  MasterDict["ttbar"] = {
    pathPrefix + "Summer23BPix_130X/TTto2L2Nu-2Jets_TuneCP5_13p6TeV_amcatnloFXFX-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/TTtoLminusNu2Q-2Jets_TuneCP5_13p6TeV_amcatnloFXFX-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/TTtoLplusNu2Q-2Jets_TuneCP5_13p6TeV_amcatnloFXFX-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/TTto4Q_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/TTTT_TuneCP5_13p6TeV_amcatnlo-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/TTWW_TuneCP5_13p6TeV_madgraph-madspin-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/TTZ-ZtoQQ-1Jets_TuneCP5_13p6TeV_amcatnloFXFX-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/TZQB-ZtoLL-TtoL-CPV_TuneCP5_13p6TeV_madgraph-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/TTHto2B_M-125_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/TTHtoNon2B_M-125_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root"
  };

  MasterDict["ST"] = {
    pathPrefix + "Summer23BPix_130X/TBbartoLplusNuBbar-s-channel-4FS_TuneCP5_13p6TeV_amcatnlo-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/TQbarto2Q-t-channel_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/TQbartoLNu-t-channel_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/TbarBtoLminusNuB-s-channel-4FS_TuneCP5_13p6TeV_amcatnlo-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/TbarQto2Q-t-channel_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/TbarQtoLNu-t-channel_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/TbarWplusto2L2Nu_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/TbarWplusto4Q_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/TbarWplustoLNu2Q_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/TWminusto2L2Nu_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/TWminusto4Q_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/TWminustoLNu2Q_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root"
  };

  MasterDict["DY"] = {
    pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-120_HT-100to400_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-120_HT-1500to2500_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-120_HT-2500_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-120_HT-400to800_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-120_HT-40to70_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-120_HT-70to100_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-120_HT-800to1500_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-4to50_HT-100to400_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-4to50_HT-1500to2500_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-4to50_HT-2500_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-4to50_HT-400to800_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-4to50_HT-40to70_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-4to50_HT-70to100_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-4to50_HT-800to1500_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-50to120_HT-100to400_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-50to120_HT-1500to2500_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-50to120_HT-2500_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-50to120_HT-400to800_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-50to120_HT-40to70_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-50to120_HT-70to100_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-50to120_HT-800to1500_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root"
  };

  MasterDict["ZInv"] = {
    pathPrefix + "Summer23BPix_130X/Zto2Nu-4Jets_HT-100to200_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/Zto2Nu-4Jets_HT-1500to2500_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/Zto2Nu-4Jets_HT-200to400_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/Zto2Nu-4Jets_HT-2500_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/Zto2Nu-4Jets_HT-400to800_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/Zto2Nu-4Jets_HT-800to1500_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root"
  };

  MasterDict["DBTB"] = {
    pathPrefix + "Summer23BPix_130X/WWto2L2Nu_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/WWto4Q_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/WWtoLNu2Q_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/WZto2L2Q_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/WZto3LNu-1Jets-4FS_TuneCP5_13p6TeV_amcatnloFXFX-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/WZtoL3Nu-1Jets-4FS_TuneCP5_13p6TeV_amcatnloFXFX-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/WZtoLNu2Q-1Jets-4FS_TuneCP5_13p6TeV_amcatnloFXFX-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/ZZto2L2Nu_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/ZZto2L2Q_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/ZZto2Nu2Q_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/ZZto4L_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/WWW_4F_TuneCP5_13p6TeV_amcatnlo-madspin-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/WWZ_4F_TuneCP5_13p6TeV_amcatnlo-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/WZGtoLNuZG_TuneCP5_13p6TeV_amcatnlo-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/WZZ_TuneCP5_13p6TeV_amcatnlo-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/ZZZ_TuneCP5_13p6TeV_amcatnlo-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/VH_HtoNonbb_M-125_TuneCP5_13p6TeV_amcatnloFXFX-madspin-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/GluGluHToTauTau_M-125_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/GluGluHto2WtoLNu2Q_M-125_TuneCP5_13p6TeV_powheg-JHUGenV752-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/GluGluHto2Wto2L2Nu_M-125_TuneCP5_13p6TeV_powheg-jhugen752-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/GluGluHto2Zto2L2Q_M-125_TuneCP5_13p6TeV_powheg-jhugenv7520-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/GluGluHtoZZto4L_M-125_TuneCP5_13p6TeV_powheg-jhugen-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/WminusH_Hto2B_WtoLNu_M-125_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/WplusH_Hto2B_WtoLNu_M-125_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/ZH_Hto2B_Zto2L_M-125_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/ZH_Hto2B_Zto2Nu_M-125_TuneCP5_13p6TeV_powheg-minlo-pythia8_Summer23BPix_130X.root"
  };
 
  MasterDict["QCD"] = {
    pathPrefix + "Summer23BPix_130X/QCD-4Jets_HT-100to200_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/QCD-4Jets_HT-200to400_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/QCD-4Jets_HT-400to600_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/QCD-4Jets_HT-600to800_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/QCD-4Jets_HT-800to1000_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/QCD-4Jets_HT-1000to1200_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/QCD-4Jets_HT-1200to1500_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/QCD-4Jets_HT-1500to2000_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/QCD-4Jets_HT-2000_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root"
  };

  MasterDict["Wjets"] = {
    pathPrefix + "Summer23BPix_130X/WtoLNu-4Jets_MLNu-0to120_HT-100to400_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/WtoLNu-4Jets_MLNu-0to120_HT-1500to2500_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/WtoLNu-4Jets_MLNu-0to120_HT-2500_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/WtoLNu-4Jets_MLNu-0to120_HT-400to800_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/WtoLNu-4Jets_MLNu-0to120_HT-40to100_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/WtoLNu-4Jets_MLNu-0to120_HT-800to1500_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/WtoLNu-4Jets_MLNu-120_HT-100to400_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/WtoLNu-4Jets_MLNu-120_HT-1500to2500_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/WtoLNu-4Jets_MLNu-120_HT-2500_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/WtoLNu-4Jets_MLNu-120_HT-400to800_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/WtoLNu-4Jets_MLNu-120_HT-40to100_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    //pathPrefix + "Summer23BPix_130X/WtoLNu-4Jets_MLNu-120_HT-800to1500_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root"
  };

  MasterDict["Cascades"] = {
    pathPrefix + "Summer23BPix_130X_Cascades/SlepSnuCascade_MN1-220_MN2-260_MC1-240_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X_Cascades/SlepSnuCascade_MN1-260_MN2-280_MC1-270_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X_Cascades/SlepSnuCascade_MN1-270_MN2-280_MC1-275_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer22_130X_Cascades/SlepSnuCascade_220-209_200-190-180_2022_NANO_JustinPrivateMC_Summer22_130X_Cascades_Summer22_130X.root"
  };

  MasterDict["SMS_Gluinos"] = {
    pathPrefix + "Fall17_102X_SMS/SMS-T1bbbb_TuneCP2_13TeV-madgraphMLM-pythia8_Fall17_102X.root"
  };

}
void SampleTool::LoadBkgs( stringlist& bkglist ){
	for( long unsigned int i=0; i<bkglist.size(); i++){
		//check if background exists
		if( MasterDict.count(bkglist[i]) == 0 ){
			std::cout<<"Bkg: "<<bkglist[i]<<" not found ... skipping ...\n";
			continue;
		} 
		BkgDict[bkglist[i]] = MasterDict[bkglist[i]];		
	}
}
void SampleTool::LoadSigs( stringlist& siglist ){

	for( long unsigned int i=0; i<siglist.size(); i++){
		if( MasterDict.count(siglist[i]) == 0 ){
			std::cout<<"Sig: "<<siglist[i]<<" not found ... skipping ...\n";
			continue;
		}
		SigDict[siglist[i]] = MasterDict[siglist[i]];
	}
	//build signal keys
	stringlist s_strings{};
	for(long unsigned int i=0; i<siglist.size(); i++){
		std::vector< std::string > keylist{};
		s_strings = SigDict[siglist[i]];
		for( long unsigned int j=0; j< s_strings.size(); j++){
                        if(s_strings[j].find("X_Cascades") != std::string::npos) 
			  SignalKeys.push_back( BFTool::GetSignalTokensCascades( s_strings[j] ) );
                        else if(s_strings[j].find("X_SMS") != std::string::npos){
                          stringlist sms_temp = BFTool::GetSignalTokensSMS( s_strings[j] );
                          for (const auto& sms_entry : sms_temp)
			    SignalKeys.push_back( sms_entry );
                        }
                        else
			  SignalKeys.push_back( BFTool::GetSignalTokens( s_strings[j] ) );
		}
	}
}
void SampleTool::PrintDict( map<string,stringlist>& d ){
	for(auto it = d.cbegin(); it != d.cend(); ++it){
 	std::cout << "key:"<< it->first << ":\n";
 	stringlist str = it->second;
 	for (std::vector<string>::iterator it2 = str.begin(); it2 != str.end(); ++it2) {
  std::cout << *it2 << " \n";
 	}
 	
	}
	std::cout<<"\n";
	
}

void SampleTool::PrintKeys( stringlist sl ){
	
	for( long unsigned int i = 0; i<sl.size(); i++){
		std::cout<<sl[i]<<"\n";
	}

}



