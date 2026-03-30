#include "BuildFitInput.h"
#include "BuildFitTools.h"
#include <cmath>
#include <TMath.h>

// --- Cleaning cut ---
std::string BuildFitInput::GetCleaningCut() {
    return "(PTCM <= 200.) && "
           "( (PTCM <= -500.*sqrt( ((-2.777*pow(fabs(dphiCMI),2) + 1.388*fabs(dphiCMI) + 0.8264) > 0 ? "
           "(-2.777*pow(fabs(dphiCMI),2) + 1.388*fabs(dphiCMI) + 0.8264) : 0) ) + 575.) || "
           "(-2.777*pow(fabs(dphiCMI),2) + 1.388*fabs(dphiCMI) + 0.8264 <= 0.) ) && "
           "( (PTCM <= -500.*sqrt( ((-1.5625*pow(fabs(dphiCMI),2) + 7.8125*fabs(dphiCMI) - 8.766) > 0 ? "
           "(-1.5625*pow(fabs(dphiCMI),2) + 7.8125*fabs(dphiCMI) - 8.766) : 0) ) + 600.) || "
           "(-1.5625*pow(fabs(dphiCMI),2) + 7.8125*fabs(dphiCMI) - 8.766 <= 0.) )";
}
REGISTER_CUT(BuildFitInput, GetCleaningCut, "Cleaning");
std::string BuildFitInput::GetCleaningLEPCut() {
    return "(PTCM_LEP <= 200.) && "
           "( (PTCM_LEP <= -500.*sqrt( ((-2.777*pow(fabs(dphiCMI_LEP),2) + 1.388*fabs(dphiCMI_LEP) + 0.8264) > 0 ? "
           "(-2.777*pow(fabs(dphiCMI_LEP),2) + 1.388*fabs(dphiCMI_LEP) + 0.8264) : 0) ) + 575.) || "
           "(-2.777*pow(fabs(dphiCMI_LEP),2) + 1.388*fabs(dphiCMI_LEP) + 0.8264 <= 0.) ) && "
           "( (PTCM_LEP <= -500.*sqrt( ((-1.5625*pow(fabs(dphiCMI_LEP),2) + 7.8125*fabs(dphiCMI_LEP) - 8.766) > 0 ? "
           "(-1.5625*pow(fabs(dphiCMI_LEP),2) + 7.8125*fabs(dphiCMI_LEP) - 8.766) : 0) ) + 600.) || "
           "(-1.5625*pow(fabs(dphiCMI_LEP),2) + 7.8125*fabs(dphiCMI_LEP) - 8.766 <= 0.) )";
}
REGISTER_CUT(BuildFitInput, GetCleaningLEPCut, "Cleaning_LEP");

// --- Zstar cut ---
std::string BuildFitInput::GetZstarCut() {
    return "((" + BuildLeptonCut(">=1OSSF","a") + " || " +
           BuildLeptonCut(">=1OSSF","b") + ") || "
           + "(Nlep==2 && " +
           BuildLeptonCut(">=1OSSF") + "))";
}
REGISTER_CUT(BuildFitInput, GetZstarCut, "Zstar");

// --- noZstar cut ---
std::string BuildFitInput::GetnoZstarCut() {
    return "!" + GetZstarCut();
}
REGISTER_CUT(BuildFitInput, GetnoZstarCut, "noZstar");

// --- dphiMETV cut ---
std::string BuildFitInput::GetdphiMETVCut() {
    return "fabs(dphiMET_V)<TMath::Pi()/2.0";
}
REGISTER_CUT(BuildFitInput, GetdphiMETVCut, "dphiMETV");
std::string BuildFitInput::GetdphiMETVLEPCut() {
    return "fabs(dphiMET_V_LEP)<TMath::Pi()/2.0";
}
REGISTER_CUT(BuildFitInput, GetdphiMETVLEPCut, "dphiMETV_LEP");

// --- min mass cut ---
REGISTER_CUT(BuildFitInput, GetMinLEPMassCut, "MinLEPMassCut");
std::string BuildFitInput::GetMinLEPMassCut() {
   return "((Nlep >= 4 && ((Nlep_b_LEP < 2 || MVb_LEP > 1.5) && (Nlep_b_LEP != 1 || MVa_LEP > 1.5)))"
          "|| (Nlep == 3 && (MVa_LEP > 1.5))"
          "|| (Nlep == 2 && (MVa_LEP > 1.5))"
          ")";
}

//REGISTER_CUT(BuildFitInput, GetMinLEPMassCut, "EleTrigCut");
//std::string BuildFitInput::GetEleTrigCut() {
//    return "SingleElectrontrigger==1"
//           "|| DoubleElectrontrigger==1"
//           "|| TripleElectrontrigger==1"
//           ;
//}
