#include "BuildFitInput.h"
#include <iostream>
#include <TLorentzVector.h>

// -------------------------------------
// User-defined cuts loader (with examples)
// -------------------------------------
ROOT::RDF::RNode BuildFitInput::loadCutsUser(ROOT::RDF::RNode &node, std::map<std::string, CutDef>& ValidCuts){
    std::map<std::string, CutDef> cuts;

    CutDef cut_leadSjetPt30;
    cut_leadSjetPt30.name = "leadSjet_pt_lt30";
    cut_leadSjetPt30.columns = {"PT_jet", "index_jet_S", "Njet_S"};
    cut_leadSjetPt30.expression =
        "(Njet_S == 0) || (Njet_S > 0 && SafeIndex(PT_jet, SafeIndex(index_jet_S, 0, -1), -1.0) < 30)";
    cuts[cut_leadSjetPt30.name] = cut_leadSjetPt30;

    CutDef cut_leadSjetPt40;
    cut_leadSjetPt40.name = "leadSjet_pt_lt40";
    cut_leadSjetPt40.columns = {"PT_jet", "index_jet_S", "Njet_S"};
    cut_leadSjetPt40.expression =
        "(Njet_S == 0) || (Njet_S > 0 && SafeIndex(PT_jet, SafeIndex(index_jet_S, 0, -1), -1.0) < 40)";
    cuts[cut_leadSjetPt40.name] = cut_leadSjetPt40;

    CutDef cut_leadSjetPt45;
    cut_leadSjetPt45.name = "leadSjet_pt_lt45";
    cut_leadSjetPt45.columns = {"PT_jet", "index_jet_S", "Njet_S"};
    cut_leadSjetPt45.expression =
        "(Njet_S == 0) || (Njet_S > 0 && SafeIndex(PT_jet, SafeIndex(index_jet_S, 0, -1), -1.0) < 45)";
    cuts[cut_leadSjetPt45.name] = cut_leadSjetPt45;

    /*
    // Example 1: invariant mass of leading two jets -> M_jj (double)
    node = node.Define("M_jj", [](const std::vector<double> &pt,
                                  const std::vector<double> &eta,
                                  const std::vector<double> &phi,
                                  const std::vector<double> &mass) {
        // Return -1.0 if not enough jets
        if (pt.size() < 2 || eta.size() < 2 || phi.size() < 2 || mass.size() < 2) return -1.0;
        // compute using temporary TLorentzVector locally (we only return a double)
        TLorentzVector j0, j1;
        j0.SetPtEtaPhiM(pt[0], eta[0], phi[0], mass[0]);
        j1.SetPtEtaPhiM(pt[1], eta[1], phi[1], mass[1]);
        return (j0 + j1).M();
    }, {"PT_jet","Eta_jet","Phi_jet","M_jet"});

    CutDef cut1;
    cut1.name = "M_jj_gt_100";
    cut1.columns = {"M_jj"};
    cut1.expression = "M_jj > 100";
    cuts[cut1.name] = cut1;

    // -----------------------------------------------------------------
    // Example 2: HT_eta24 / MET ratio > 1.5 
    // -----------------------------------------------------------------
    node = node.Define("HT_eta24_over_MET", [](double HT_eta24, double MET) {
        if (MET == 0.0) return 0.0;
        return HT_eta24 / MET;
    }, {"HT_eta24","MET"});

    CutDef cut2;
    cut2.name = "HT_eta24_over_MET_gt_1p5";
    cut2.columns = {"HT_eta24_over_MET"};
    cut2.expression = "HT_eta24_over_MET > 1.5";
    cuts[cut2.name] = cut2;

    // -----------------------------------------------------------------
    // Example 3: Combined lepton-jet cut (pT + DeltaR)
    // Compute DeltaR using TLorentzVector::DeltaR, return double
    // Also define leading pT columns to avoid unsafe indexing
    // -----------------------------------------------------------------
    
    // Leading lepton pT
    node = node.Define("PT_lep0", [](const std::vector<double> &pt){
        return pt.empty() ? 0.0 : pt[0];
    }, {"PT_lep"});
    
    // Leading jet pT
    node = node.Define("PT_jet0", [](const std::vector<double> &pt){
        return pt.empty() ? 0.0 : pt[0];
    }, {"PT_jet"});
    */
    
    /*
    // DeltaR between leading lepton and leading jet
    node = node.Define("DeltaR_lep0_jet0", [](const std::vector<double> &pt_lep,
                                             const std::vector<double> &eta_lep,
                                             const std::vector<double> &phi_lep,
                                             const std::vector<double> &m_lep,
                                             const std::vector<double> &pt_jet,
                                             const std::vector<double> &eta_jet,
                                             const std::vector<double> &phi_jet,
                                             const std::vector<double> &m_jet) {
        if(pt_lep.empty() || eta_lep.empty() || phi_lep.empty() || m_lep.empty() ||
           pt_jet.empty() || eta_jet.empty() || phi_jet.empty() || m_jet.empty()) {
            return 999.0; // safe fallback
        }
    
        TLorentzVector lep, jet;
        lep.SetPtEtaPhiM(pt_lep[0], eta_lep[0], phi_lep[0], m_lep[0]);
        jet.SetPtEtaPhiM(pt_jet[0], eta_jet[0], phi_jet[0], m_jet[0]);
    
        return lep.DeltaR(jet);
    }, {"PT_lep","Eta_lep","Phi_lep","M_lep",
        "PT_jet","Eta_jet","Phi_jet","M_jet"});
    
    // Define the user cut using only doubles, safe for validation
    CutDef cut3;
    cut3.name = "lep0_pt25_jet0_pt30_dR0p4";
    cut3.columns = {"PT_lep0","PT_jet0","DeltaR_lep0_jet0"};
    cut3.expression = "(PT_lep0 > 25) && (PT_jet0 > 30) && (DeltaR_lep0_jet0 > 0.4)";
    cuts[cut3.name] = cut3;
    */

    // -----------------------------------------------------------------------------
    // Store 4-vectors as ROOT::Math::PtEtaPhiMVector (replaces TLV for RDataFrame)
    // How to create 4-vector columns (less recommended for simple cuts)
    // -----------------------------------------------------------------------------
    /*
    node = node
        .Define("p4_jet0_vect", [](const std::vector<double> &pt,
                                   const std::vector<double> &eta,
                                   const std::vector<double> &phi,
                                   const std::vector<double> &mass) {
            ROOT::Math::PtEtaPhiMVector v;
            if (!pt.empty() && pt.size() == eta.size() && eta.size() == phi.size() && phi.size() == mass.size()) {
                v.SetPtEtaPhiM(pt[0], eta[0], phi[0], mass[0]);
            }
            return v;
        }, {"PT_jet","Eta_jet","Phi_jet","M_jet"})
        .Define("p4_jet1_vect", [](const std::vector<double> &pt,
                                   const std::vector<double> &eta,
                                   const std::vector<double> &phi,
                                   const std::vector<double> &mass) {
            ROOT::Math::PtEtaPhiMVector v;
            if (pt.size() > 1 && pt.size() == eta.size() && eta.size() == phi.size() && phi.size() == mass.size()) {
                v.SetPtEtaPhiM(pt[1], eta[1], phi[1], mass[1]);
            }
            return v;
        }, {"PT_jet","Eta_jet","Phi_jet","M_jet"});

    node = node.Define("M_jj_vect", [](const ROOT::Math::PtEtaPhiMVector &j0,
                                       const ROOT::Math::PtEtaPhiMVector &j1) {
        return (j0 + j1).M();
    }, {"p4_jet0_vect", "p4_jet1_vect"});
    // Used *_vect in the names to avoid conflicts with the scalar-versions above.
    */

    // Validate the cuts that the user wrote
    ValidCuts = ValidateCuts(node, cuts);
    for (const auto &kv : cuts) {
        if (!ValidCuts.count(kv.first)) {
            std::cerr << "[BuildFitInput loadUserCuts WARN] User cut \"" << kv.first
                      << "\" failed validation and will be ignored.\n";
        }
    }
    return node;
}
