#include "BuildFitInput.h"
#include <iostream>
#include <TLorentzVector.h>

// -------------------------------------
// User-defined cuts loader (with examples)
// -------------------------------------
ROOT::RDF::RNode BuildFitInput::loadCutsUser(ROOT::RDF::RNode &node, std::map<std::string, CutDef>& ValidCuts){
    std::map<std::string, CutDef> cuts;

    CutDef cut_leadSjetPt45;
    cut_leadSjetPt45.name = "leadSjet_pt_lt45";
    cut_leadSjetPt45.columns = {"PT_jet", "index_jet_S", "Njet_S"};
    cut_leadSjetPt45.expression =
        "(Njet_S == 0) || (Njet_S > 0 && SafeIndex(PT_jet, SafeIndex(index_jet_S, 0, -1), -1.0) < 45)";
    cuts[cut_leadSjetPt45.name] = cut_leadSjetPt45;

    //node = node
    //    .Define("My_p4_lep0_a", [](const std::vector<double> &pt,
    //                          const std::vector<double> &eta,
    //                          const std::vector<double> &phi,
    //                          const std::vector<double> &mass,
    //                          const std::vector<int> &index_lep_a_LEP,
    //                          const int &Nlep) {
    //            // Construct TLV for lepton 0 on A side if present; otherwise returns a zero TLV.
    //            TLorentzVector v;
    //            if (index_lep_a_LEP.size() >= 1 && Nlep > 2) {
    //                // safe access to [index_lep_a_LEP[0]]
    //                v.SetPtEtaPhiM(pt[index_lep_a_LEP[0]], eta[index_lep_a_LEP[0]], phi[index_lep_a_LEP[0]], mass[index_lep_a_LEP[0]]);
    //            }
    //            else if (Nlep == 2) {
    //                // safe access to [0]
    //                v.SetPtEtaPhiM(pt[0], eta[0], phi[0], mass[0]);
    //            }
    //            return v;
    //        }, {"PT_lep","Eta_lep","Phi_lep","M_lep","index_lep_a_LEP","Nlep"})
    //    .Define("My_p4_lep1_a", [](const std::vector<double> &pt,
    //                          const std::vector<double> &eta,
    //                          const std::vector<double> &phi,
    //                          const std::vector<double> &mass,
    //                          const std::vector<int> &index_lep_a_LEP,
    //                          const int &Nlep) {
    //            // Construct TLV for lepton 1 on A side if present; otherwise returns a zero TLV.
    //            TLorentzVector v;
    //            if (index_lep_a_LEP.size() >= 2 && Nlep > 2) {
    //                // safe access to [index_lep_a_LEP[1]]
    //                v.SetPtEtaPhiM(pt[index_lep_a_LEP[1]], eta[index_lep_a_LEP[1]], phi[index_lep_a_LEP[1]], mass[index_lep_a_LEP[1]]);
    //            }
    //            else if (Nlep == 2) {
    //                // safe access to [1]
    //                v.SetPtEtaPhiM(pt[1], eta[1], phi[1], mass[1]);
    //            }
    //            return v;
    //        }, {"PT_lep","Eta_lep","Phi_lep","M_lep","index_lep_a_LEP","Nlep"})
    //    .Define("Q_lep0_a", [](const std::vector<int> &charge,
    //                           const std::vector<int> &index_lep_a_LEP,
    //                           const int &Nlep){
    //            if(Nlep > 2)       return index_lep_a_LEP.size() >= 1 ? charge[index_lep_a_LEP[0]] : 0;
    //            else if(Nlep == 2) return charge[0];
    //            else return 0;
    //        }, {"Charge_lep","index_lep_a_LEP","Nlep"})
    //    .Define("Q_lep1_a", [](const std::vector<int> &charge,
    //                           const std::vector<int> &index_lep_a_LEP,
    //                           const int &Nlep){
    //            if(Nlep > 2)       return index_lep_a_LEP.size() >= 2 ? charge[index_lep_a_LEP[1]] : 0;
    //            else if(Nlep == 2) return charge[1];
    //            else return 0;
    //        }, {"Charge_lep","index_lep_a_LEP","Nlep"})
    //    .Define("TLV_Cand", [](const TLorentzVector &l0,
    //                           const TLorentzVector &l1
    //                          ) {
    //            return l0 + l1;
    //        }, {"My_p4_lep0_a","My_p4_lep1_a"
    //           })
    //    .Define("CandBetaZLab", [](const TLorentzVector &Cand){
    //            return (Cand.E() != 0.0) ? Cand.Pz() / Cand.E() : 0.0;
    //        }, {"TLV_Cand"})
    //    .Define("CandCosDecayAngleLab",
    //        [](const TLorentzVector &cand,
    //           const TLorentzVector &l0,
    //           const TLorentzVector &l1,
    //           int q0, int q1) -> double
    //        {
    //            // choose the positively charged child TLV (prefer l0 if both >0, fallback to l1)
    //            TLorentzVector child = TLorentzVector{};
    //            if (q0 > 0) child = l0;
    //            else if (q1 > 0) child = l1;
    //            else return -2.; // no positively charged child -> single lep
    //    
    //            // safety: candidate must have nonzero energy (otherwise boost undefined)
    //            if (cand.E() == 0.0) return -2.;
    //    
    //            TVector3 boost = cand.BoostVector();
    //    
    //            // boost the child into the candidate rest frame
    //            child.Boost(-boost);
    //    
    //            TVector3 childVec = child.Vect();
    //            if (childVec.Mag() == 0.0 || boost.Mag() == 0.0) return -2.;
    //    
    //            return fabs(childVec.Unit().Dot(boost.Unit()));
    //        },
    //        {"TLV_Cand", "My_p4_lep0_a", "My_p4_lep1_a", "Q_lep0_a", "Q_lep1_a"})
    //;
    //CutDef cutBetaZLab0p9;
    //cutBetaZLab0p9.name = "BetaZLab0p9";
    //cutBetaZLab0p9.columns = {"CandBetaZLab"};
    //cutBetaZLab0p9.expression = "CandBetaZLab < 0.9";
    //cuts[cutBetaZLab0p9.name] = cutBetaZLab0p9;
    //CutDef cutBetaZLab0p95;
    //cutBetaZLab0p95.name = "BetaZLab0p95";
    //cutBetaZLab0p95.columns = {"CandBetaZLab"};
    //cutBetaZLab0p95.expression = "CandBetaZLab < 0.95";
    //cuts[cutBetaZLab0p95.name] = cutBetaZLab0p95;
    //CutDef cutCosDecayAngleLab0p9;
    //cutCosDecayAngleLab0p9.name = "CosDecayAngleLab0p9";
    //cutCosDecayAngleLab0p9.columns = {"CandCosDecayAngleLab"};
    //cutCosDecayAngleLab0p9.expression = "CandCosDecayAngleLab < 0.9";
    //cuts[cutCosDecayAngleLab0p9.name] = cutCosDecayAngleLab0p9;
    //CutDef cutCosDecayAngleLab0p95;
    //cutCosDecayAngleLab0p95.name = "CosDecayAngleLab0p95";
    //cutCosDecayAngleLab0p95.columns = {"CandCosDecayAngleLab"};
    //cutCosDecayAngleLab0p95.expression = "CandCosDecayAngleLab < 0.95";
    //cuts[cutCosDecayAngleLab0p95.name] = cutCosDecayAngleLab0p95;

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
