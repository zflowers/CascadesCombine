#include "SampleTool.h"

SampleTool::SampleTool(){

  LumiDict["Summer20UL16_106X"] = 19.5;
  LumiDict["Summer20UL16APV_106X"] = 16.8;
  LumiDict["Summer20UL17_106X"] = 41.479680529;
  LumiDict["Summer20UL18_106X"] = 21.077794578 + 0.95*38.662770624;
  LumiDict["Summer22_130X"] = 7.9804;
  LumiDict["Summer22EE_130X"] = 26.6717;
  LumiDict["Summer23_130X"] = 17.794;
  LumiDict["Summer23BPix_130X"] = 9.451;
  LumiDict["Summer24_130X"] = 109.;
  LumiDict["Summer25_130X"] = 60.;
  LumiDict["Summer26_130X"] = 30.;

  // Override lumis since not all samples available yet
  //LumiDict["Summer23BPix_130X"] = 400.;
  //LumiDict["Summer22_130X_SMS"] = 400.;
  //LumiDict["Summer22_130X_Cascades"] = 400.;
  //LumiDict["Summer23BPix_130X_Cascades"] = 400.;
  // Once 2018 is ready use \/
  LumiDict["Summer23BPix_130X"] = 285.;
  LumiDict["Summer22_130X_SMS"] = 285.;
  LumiDict["Summer22_130X_Cascades"] = 285. + 138.;
  LumiDict["Summer23BPix_130X_Cascades"] = 285. + 138.;
  LumiDict["Summer20UL18_106X"] = 138.;
  LumiDict["Summer20UL18_106X_SMS"] = 138.;

  string pathPrefix = "root://cmseos.fnal.gov//store/user/lpcsusylep/NTUPLES_Cascades_v6/";

  MasterDict["ttbar"] = {
    pathPrefix + "Summer23BPix_130X/TTto2L2Nu-2Jets_TuneCP5_13p6TeV_amcatnloFXFX-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/TTtoLminusNu2Q-2Jets_TuneCP5_13p6TeV_amcatnloFXFX-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/TTtoLplusNu2Q-2Jets_TuneCP5_13p6TeV_amcatnloFXFX-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/TTto4Q_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/TTTT_TuneCP5_13p6TeV_amcatnlo-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/TTWW_TuneCP5_13p6TeV_madgraph-madspin-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/TTZ-ZtoQQ-1Jets_TuneCP5_13p6TeV_amcatnloFXFX-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/TZQB-ZtoLL-TtoL-CPV_TuneCP5_13p6TeV_madgraph-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/TTLL_MLL-4to50_TuneCP5_13p6TeV_amcatnlo-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/TTLL_MLL-50_TuneCP5_13p6TeV_amcatnlo-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/TTLNu-1Jets_TuneCP5_13p6TeV_amcatnloFXFX-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/TTHto2B_M-125_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/TTHtoNon2B_M-125_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer20UL18_106X/TGJets_TuneCP5_13TeV-amcatnlo-madspin-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/TTGJets_TuneCP5_13TeV-amcatnloFXFX-madspin-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/TTJets_DiLept_TuneCP5_13TeV-madgraphMLM-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/TTJets_SingleLeptFromT_TuneCP5_13TeV-madgraphMLM-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/TTJets_SingleLeptFromTbar_TuneCP5_13TeV-madgraphMLM-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/TTTT_TuneCP5_13TeV-amcatnlo-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/TTWJetsToLNu_TuneCP5_13TeV-amcatnloFXFX-madspin-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/TTWJetsToQQ_TuneCP5_13TeV-amcatnloFXFX-madspin-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/TTWW_TuneCP5_13TeV-madgraph-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/TTZToLLNuNu_M-10_TuneCP5_13TeV-amcatnlo-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/TTZToQQ_TuneCP5_13TeV-amcatnlo-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/tZq_ll_4f_ckm_NLO_TuneCP5_13TeV-amcatnlo-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/ttHToNonbb_M125_TuneCP5_13TeV-powheg-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/ttHTobb_M125_TuneCP5_13TeV-powheg-pythia8_Summer20UL18_106X.root",
  };

  MasterDict["ST"] = {
    pathPrefix + "Summer23BPix_130X/TBbartoLplusNuBbar-s-channel-4FS_TuneCP5_13p6TeV_amcatnlo-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/TQbarto2Q-t-channel_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/TQbartoLNu-t-channel_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/TBbarQ_t-channel_4FS_TuneCP5_13p6TeV_powheg-madspin-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/TbarBQ_t-channel_4FS_TuneCP5_13p6TeV_powheg-madspin-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/TbarBtoLminusNuB-s-channel-4FS_TuneCP5_13p6TeV_amcatnlo-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/TbarQto2Q-t-channel_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/TbarQtoLNu-t-channel_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/TbarWplusto2L2Nu_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/TbarWplusto4Q_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/TbarWplustoLNu2Q_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/TWminusto2L2Nu_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/TWminusto4Q_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/TWminustoLNu2Q_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer20UL18_106X/ST_s-channel_4f_leptonDecays_TuneCP5_13TeV-amcatnlo-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/ST_t-channel_antitop_4f_InclusiveDecays_TuneCP5_13TeV-powheg-madspin-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/ST_t-channel_top_4f_InclusiveDecays_TuneCP5_13TeV-powheg-madspin-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/ST_tW_antitop_5f_NoFullyHadronicDecays_TuneCP5_13TeV-powheg-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/ST_tW_top_5f_NoFullyHadronicDecays_TuneCP5_13TeV-powheg-pythia8_Summer20UL18_106X.root",
  };

  MasterDict["DY"] = {
    pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-120_HT-100to400_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-120_HT-1500to2500_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-120_HT-2500_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-120_HT-400to800_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-120_HT-40to70_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-120_HT-70to100_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-120_HT-800to1500_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-4to50_HT-100to400_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-4to50_HT-1500to2500_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-4to50_HT-2500_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-4to50_HT-400to800_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-4to50_HT-40to70_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-4to50_HT-70to100_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-4to50_HT-800to1500_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-50to120_HT-100to400_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-50to120_HT-1500to2500_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-50to120_HT-2500_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-50to120_HT-400to800_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-50to120_HT-40to70_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-50to120_HT-70to100_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/DYto2L-4Jets_MLL-50to120_HT-800to1500_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer20UL18_106X/DYJetsToLL_M-50_HT-100to200_TuneCP5_PSweights_13TeV-madgraphMLM-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/DYJetsToLL_M-50_HT-1200to2500_TuneCP5_PSweights_13TeV-madgraphMLM-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/DYJetsToLL_M-50_HT-200to400_TuneCP5_PSweights_13TeV-madgraphMLM-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/DYJetsToLL_M-50_HT-2500toInf_TuneCP5_PSweights_13TeV-madgraphMLM-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/DYJetsToLL_M-50_HT-400to600_TuneCP5_PSweights_13TeV-madgraphMLM-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/DYJetsToLL_M-50_HT-600to800_TuneCP5_PSweights_13TeV-madgraphMLM-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/DYJetsToLL_M-50_HT-70to100_TuneCP5_PSweights_13TeV-madgraphMLM-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/DYJetsToLL_M-50_HT-800to1200_TuneCP5_PSweights_13TeV-madgraphMLM-pythia8_Summer20UL18_106X.root",
  };

  MasterDict["ZInv"] = {
    pathPrefix + "Summer23BPix_130X/Zto2Nu-4Jets_HT-100to200_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/Zto2Nu-4Jets_HT-1500to2500_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/Zto2Nu-4Jets_HT-200to400_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/Zto2Nu-4Jets_HT-2500_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/Zto2Nu-4Jets_HT-400to800_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/Zto2Nu-4Jets_HT-800to1500_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer20UL18_106X/ZJetsToNuNu_HT-100To200_TuneCP5_13TeV-madgraphMLM-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/ZJetsToNuNu_HT-1200To2500_TuneCP5_13TeV-madgraphMLM-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/ZJetsToNuNu_HT-200To400_TuneCP5_13TeV-madgraphMLM-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/ZJetsToNuNu_HT-2500ToInf_TuneCP5_13TeV-madgraphMLM-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/ZJetsToNuNu_HT-400To600_TuneCP5_13TeV-madgraphMLM-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/ZJetsToNuNu_HT-600To800_TuneCP5_13TeV-madgraphMLM-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/ZJetsToNuNu_HT-800To1200_TuneCP5_13TeV-madgraphMLM-pythia8_Summer20UL18_106X.root",
  };

  MasterDict["DBTB"] = {
    pathPrefix + "Summer23BPix_130X/WWto2L2Nu_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/WWto4Q_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/WWtoLNu2Q_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/WZto2L2Q_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/WZto3LNu-1Jets-4FS_TuneCP5_13p6TeV_amcatnloFXFX-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/WZtoL3Nu-1Jets-4FS_TuneCP5_13p6TeV_amcatnloFXFX-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/WZtoLNu2Q-1Jets-4FS_TuneCP5_13p6TeV_amcatnloFXFX-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/ZZto2L2Nu_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/ZZto2L2Q_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/ZZto2Nu2Q_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/ZZto4L_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/ZG2JtoG2L2J_EWK_MLL-50_MJJ-120_TuneCP5_13p6TeV_madgraph-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/WWW_4F_TuneCP5_13p6TeV_amcatnlo-madspin-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/WWZ_4F_TuneCP5_13p6TeV_amcatnlo-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/WZGtoLNuZG_TuneCP5_13p6TeV_amcatnlo-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/WZZ_TuneCP5_13p6TeV_amcatnlo-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/ZZZ_TuneCP5_13p6TeV_amcatnlo-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/VH_HtoNonbb_M-125_TuneCP5_13p6TeV_amcatnloFXFX-madspin-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/GluGluHToTauTau_M-125_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/GluGluHto2WtoLNu2Q_M-125_TuneCP5_13p6TeV_powheg-JHUGenV752-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/GluGluHto2Wto2L2Nu_M-125_TuneCP5_13p6TeV_powheg-jhugen752-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/GluGluHto2Zto2L2Q_M-125_TuneCP5_13p6TeV_powheg-jhugenv7520-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/GluGluHtoZZto4L_M-125_TuneCP5_13p6TeV_powheg-jhugen-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/WminusH_Hto2B_WtoLNu_M-125_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/WplusH_Hto2B_WtoLNu_M-125_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/ZH_Hto2B_Zto2L_M-125_TuneCP5_13p6TeV_powheg-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/ZH_Hto2B_Zto2Nu_M-125_TuneCP5_13p6TeV_powheg-minlo-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer20UL18_106X/GluGluHToTauTau_M-125_TuneCP5_13TeV-amcatnloFXFX-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/GluGluHToWWTo2L2Nu_M-125_TuneCP5_13TeV-powheg-jhugen727-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/GluGluHToWWToLNuQQ_M-125_TuneCP5_13TeV_powheg_jhugen751_pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/GluGluHToZZTo2L2Nu_M125_TuneCP5_13TeV_powheg2_JHUGenV735_pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/GluGluHToZZTo4L_M125_TuneCP5_13TeV_powheg2_minloHJJ_JHUGenV7011_pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/VHToNonbb_M125_TuneCP5_13TeV-amcatnloFXFX_madspin_pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/WGG_5f_TuneCP5_13TeV_amcatnlo-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/WWG_TuneCP5_13TeV-amcatnlo-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/WWTo1L1Nu2Q_4f_TuneCP5_13TeV-amcatnloFXFX-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/WWTo2L2Nu_TuneCP5_13TeV-powheg-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/WWTo4Q_4f_TuneCP5_13TeV-amcatnloFXFX-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/WWW_4F_TuneCP5_13TeV-amcatnlo-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/WWZ_4F_TuneCP5_13TeV-amcatnlo-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/WZG_TuneCP5_13TeV-amcatnlo-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/WZTo1L1Nu2Q_4f_TuneCP5_13TeV-amcatnloFXFX-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/WZTo1L3Nu_4f_TuneCP5_13TeV-amcatnloFXFX-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/WZTo2Q2L_mllmin4p0_TuneCP5_13TeV-amcatnloFXFX-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/WZTo3LNu_TuneCP5_13TeV-amcatnloFXFX-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/WZZ_TuneCP5_13TeV-amcatnlo-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/WminusH_HToBB_WToLNu_M-125_TuneCP5_13TeV-powheg-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/WplusH_HToBB_WToLNu_M-125_TuneCP5_13TeV-powheg-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/ZH_HToBB_ZToLL_M-125_TuneCP5_13TeV-powheg-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/ZH_HToBB_ZToNuNu_M-125_TuneCP5_13TeV-powheg-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/ZZTo2L2Nu_TuneCP5_13TeV_powheg_pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/ZZTo2Q2L_mllmin4p0_TuneCP5_13TeV-amcatnloFXFX-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/ZZTo2Q2Nu_TuneCP5_13TeV-amcatnloFXFX-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/ZZTo4L_TuneCP5_13TeV_powheg_pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/ZZZ_TuneCP5_13TeV-amcatnlo-pythia8_Summer20UL18_106X.root",
  };
 
  MasterDict["QCD"] = {
    pathPrefix + "Summer23BPix_130X/QCD-4Jets_HT-100to200_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/QCD-4Jets_HT-200to400_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/QCD-4Jets_HT-400to600_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/QCD-4Jets_HT-600to800_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/QCD-4Jets_HT-800to1000_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/QCD-4Jets_HT-1000to1200_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/QCD-4Jets_HT-1200to1500_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/QCD-4Jets_HT-1500to2000_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/QCD-4Jets_HT-2000_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer20UL18_106X/QCD_HT1000to1500_TuneCP5_13TeV-madgraphMLM-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/QCD_HT100to200_TuneCP5_13TeV-madgraphMLM-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/QCD_HT1500to2000_TuneCP5_13TeV-madgraphMLM-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/QCD_HT2000toInf_TuneCP5_13TeV-madgraphMLM-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/QCD_HT200to300_TuneCP5_13TeV-madgraphMLM-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/QCD_HT300to500_TuneCP5_13TeV-madgraphMLM-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/QCD_HT500to700_TuneCP5_13TeV-madgraphMLM-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/QCD_HT50to100_TuneCP5_13TeV-madgraphMLM-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/QCD_HT700to1000_TuneCP5_13TeV-madgraphMLM-pythia8_Summer20UL18_106X.root"
  };

  MasterDict["Wjets"] = {
    pathPrefix + "Summer23BPix_130X/WtoLNu-4Jets_MLNu-0to120_HT-100to400_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/WtoLNu-4Jets_MLNu-0to120_HT-1500to2500_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/WtoLNu-4Jets_MLNu-0to120_HT-2500_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/WtoLNu-4Jets_MLNu-0to120_HT-400to800_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/WtoLNu-4Jets_MLNu-0to120_HT-40to100_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/WtoLNu-4Jets_MLNu-0to120_HT-800to1500_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/WtoLNu-4Jets_MLNu-120_HT-100to400_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/WtoLNu-4Jets_MLNu-120_HT-1500to2500_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/WtoLNu-4Jets_MLNu-120_HT-2500_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/WtoLNu-4Jets_MLNu-120_HT-400to800_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/WtoLNu-4Jets_MLNu-120_HT-40to100_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/WtoLNu-4Jets_MLNu-120_HT-800to1500_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer20UL18_106X/WJetsToLNu_HT-100To200_TuneCP5_13TeV-madgraphMLM-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/WJetsToLNu_HT-1200To2500_TuneCP5_13TeV-madgraphMLM-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/WJetsToLNu_HT-200To400_TuneCP5_13TeV-madgraphMLM-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/WJetsToLNu_HT-2500ToInf_TuneCP5_13TeV-madgraphMLM-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/WJetsToLNu_HT-400To600_TuneCP5_13TeV-madgraphMLM-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/WJetsToLNu_HT-600To800_TuneCP5_13TeV-madgraphMLM-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/WJetsToLNu_HT-70To100_TuneCP5_13TeV-madgraphMLM-pythia8_Summer20UL18_106X.root",
    pathPrefix + "Summer20UL18_106X/WJetsToLNu_HT-800To1200_TuneCP5_13TeV-madgraphMLM-pythia8_Summer20UL18_106X.root"
  };

  MasterDict["Gjets"] = {
    pathPrefix + "Summer23BPix_130X/GJ-4Jets_HT-40to70_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/GJ-4Jets_HT-70to100_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/GJ-4Jets_HT-100to200_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/GJ-4Jets_HT-200to400_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/GJ-4Jets_HT-400to600_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X/GJ-4Jets_HT-600_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root"
  };

  MasterDict["Cascades"] = {
    pathPrefix + "Summer23BPix_130X_Cascades/SlepSnuCascade_MN1-220_MN2-260_MC1-240_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X_Cascades/SlepSnuCascade_MN1-260_MN2-280_MC1-270_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer23BPix_130X_Cascades/SlepSnuCascade_MN1-270_MN2-280_MC1-275_TuneCP5_13p6TeV_madgraphMLM-pythia8_Summer23BPix_130X.root",
    pathPrefix + "Summer22_130X_Cascades/SlepSnuCascade_220-209_200-190-180_2022_NANO_JustinPrivateMC_Summer22_130X_Cascades_Summer22_130X.root"
  };

  MasterDict["SMS_Gluinos"] = {
    //pathPrefix + "Fall17_102X_SMS/SMS-T1qqqq-compressedGluino_TuneCP2_13TeV-madgraphMLM-pythia8_Fall17_102X.root",
    //pathPrefix + "Fall17_102X_SMS/SMS-T1qqqqL_TuneCP2_13TeV-madgraphMLM-pythia8_Fall17_102X.root",
    pathPrefix + "Fall17_102X_SMS/SMS-T1qqqq_TuneCP2_13TeV-madgraphMLM-pythia8_Fall17_102X.root"
  };

  MasterDict["SMS_TChiWZ"] = {
    pathPrefix + "Summer22_130X_SMS/SMS-TChiWZ_mC1-300_mN2-300_mN1-290_NanoAODv12_JustinPrivateMC_Summer22_130X_SMS_Summer22_130X.root",
    pathPrefix + "Summer20UL18_106X_SMS/SMS-TChiWZ_ZToLL_mZMin-0p1_TuneCP5_13TeV-madgraphMLM-pythia8_Summer20UL18_106X.root"
  };

  MasterDict["SMS_TChiWZ_Sandwich"] = {
    pathPrefix + "Summer22_130X_SMS/SMS-TChiWZ_mC1-295_mN2-300_mN1-290_NanoAODv12_JustinPrivateMC_Summer22_130X_SMS_Summer22_130X.root"
  };

}

void SampleTool::LoadBkgs( const stringlist& bkglist ){
    for( long unsigned int i=0; i<bkglist.size(); i++){
        //check if background exists
        if( MasterDict.count(bkglist[i]) == 0 ){
            std::cout<<"Bkg: "<<bkglist[i]<<" not found ... skipping ...\n";
            continue;
        } 
        BkgDict[bkglist[i]] = MasterDict[bkglist[i]];        
    }
}
void SampleTool::LoadSigs( const stringlist& siglist ){
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
                stringlist sms_filters_tmp;
                for (const auto& sms_entry : sms_temp){
                    SignalKeys.push_back( sms_entry );
                    sms_filters_tmp.push_back( sms_entry );
                }
                if (BFTool::filterSignalsSMS.empty())
                    BFTool::SetFilterSignalsSMS(sms_filters_tmp);
            }
        }
    }
}

void SampleTool::LoadAllBkgs() {
    std::vector<std::string> allBkgs = {"ttbar","ST","DY","ZInv","DBTB","QCD","Wjets"};
    LoadBkgs(allBkgs);
}

void SampleTool::LoadAllSigs() {
    std::vector<std::string> allSigs = {"Cascades","SMS_Gluinos","SMS_TChiWZ"};
    LoadSigs(allSigs);
}

void SampleTool::LoadAllFromMaster() {
    for (const auto &kv : MasterDict) {
        const std::string &group = kv.first;
        const stringlist &files = kv.second;
        // Rule: treat "Cascades" or anything containing "SMS" as signal
        if (group == "Cascades" || group.find("SMS") != std::string::npos) {
            SigDict[group] = files;
        } else {
            BkgDict[group] = files;
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
    for( long unsigned int i = 0; i<sl.size(); i++)
        std::cout<<sl[i]<<"\n";
}
