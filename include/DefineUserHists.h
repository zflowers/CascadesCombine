#include "HistTools.h"
#include "TLorentzVector.h"

static ROOT::RDF::RNode loadHistogramsUserCols(ROOT::RDF::RNode &node){

    //if (node.HasColumn("minM_ll_all") == false) {
    //    node = node.Define("minM_ll_all",
    //        [](const std::vector<double> &pt,
    //           const std::vector<double> &eta,
    //           const std::vector<double> &phi,
    //           const std::vector<double> &mass) -> double
    //        {
    //            // Safety: require consistent vector sizes and at least 2 leptons
    //            if (pt.size() < 2 || eta.size() != pt.size() || phi.size() != pt.size() || mass.size() != pt.size())
    //                return -1.0;
    //    
    //            double minM = std::numeric_limits<double>::infinity();
    //            for (size_t i = 0; i < pt.size(); ++i) {
    //                TLorentzVector v1; v1.SetPtEtaPhiM(pt[i], eta[i], phi[i], mass[i]);
    //                for (size_t j = i + 1; j < pt.size(); ++j) {
    //                    TLorentzVector v2; v2.SetPtEtaPhiM(pt[j], eta[j], phi[j], mass[j]);
    //                    double m = (v1 + v2).M();
    //                    if (m < minM) minM = m;
    //                }
    //            }
    //            return (std::isfinite(minM) ? minM : -1.0);
    //        }, {"PT_lep","Eta_lep","Phi_lep","M_lep"});
    //}
    //
    //// Convenience boolean: true if a valid min mass exists
    //if (node.HasColumn("HasMinPair_all") == false) 
    //    node = node.Define("HasMinPair_all", [](double minM){ return minM >= 0.0; }, {"minM_ll_all"});
    //
    //if (!node.HasColumn("minDR_ll_all") || !node.HasColumn("DR_at_minM_ll_all")) {
    //    node = node.Define(
    //        "minDR_and_DRatMinM_ll_all",
    //        [](const std::vector<double> &pt,
    //           const std::vector<double> &eta,
    //           const std::vector<double> &phi,
    //           const std::vector<double> &mass)
    //        {
    //            // return pair: (minDR_all, DR_at_minM)
    //            double minDR  = std::numeric_limits<double>::infinity();
    //            double minM   = std::numeric_limits<double>::infinity();
    //            double DRatMinM = -1.0;
    //
    //            if (pt.size() < 2 ||
    //                eta.size() != pt.size() ||
    //                phi.size() != pt.size() ||
    //                mass.size() != pt.size())
    //                return std::make_pair(-1.0, -1.0);
    //
    //            for (size_t i = 0; i < pt.size(); ++i) {
    //                TLorentzVector v1;
    //                v1.SetPtEtaPhiM(pt[i], eta[i], phi[i], mass[i]);
    //
    //                for (size_t j = i + 1; j < pt.size(); ++j) {
    //                    TLorentzVector v2;
    //                    v2.SetPtEtaPhiM(pt[j], eta[j], phi[j], mass[j]);
    //
    //                    const double dr = v1.DeltaR(v2);
    //                    const double m  = (v1 + v2).M();
    //
    //                    // global min DR
    //                    if (dr < minDR)
    //                        minDR = dr;
    //
    //                    // track DR for the min-mass pair
    //                    if (m < minM) {
    //                        minM = m;
    //                        DRatMinM = dr;
    //                    }
    //                }
    //            }
    //
    //            if (!std::isfinite(minDR))   minDR   = -1.0;
    //            if (!std::isfinite(minM))    DRatMinM = -1.0;
    //
    //            return std::make_pair(minDR, DRatMinM);
    //        },
    //        {"PT_lep","Eta_lep","Phi_lep","M_lep"}
    //    )
    //    .Define("minDR_ll_all",
    //        [](const std::pair<double,double> &p){ return p.first; },
    //        {"minDR_and_DRatMinM_ll_all"})
    //    .Define("DR_at_minM_ll_all",
    //        [](const std::pair<double,double> &p){ return p.second; },
    //        {"minDR_and_DRatMinM_ll_all"});
    //}
    //
    //// convenience validity flags
    //if (!node.HasColumn("HasMinDR_all"))
    //    node = node.Define("HasMinDR_all",
    //        [](double dr){ return dr >= 0.0; },
    //        {"minDR_ll_all"});
    //
    //if (!node.HasColumn("HasDRatMinM_all"))
    //    node = node.Define("HasDRatMinM_all",
    //        [](double dr){ return dr >= 0.0; },
    //        {"DR_at_minM_ll_all"});

    //
    //// --- min M_ll but only over OSSF pairs ---------------------------------
    //if (node.HasColumn("minM_ll_OSSF") == false) {
    //    node = node.Define("minM_ll_OSSF",
    //        [](const std::vector<double> &pt,
    //           const std::vector<double> &eta,
    //           const std::vector<double> &phi,
    //           const std::vector<double> &mass,
    //           const std::vector<int> &pdgid,
    //           const std::vector<int> &charge) -> double
    //        {
    //            // Safety checks
    //            if (pt.size() < 2 || eta.size() != pt.size() || phi.size() != pt.size() || mass.size() != pt.size())
    //                return -1.0;
    //            if (pdgid.size() != pt.size() || charge.size() != pt.size())
    //                return -1.0;
    //    
    //            double minM = std::numeric_limits<double>::infinity();
    //            for (size_t i = 0; i < pt.size(); ++i) {
    //                for (size_t j = i + 1; j < pt.size(); ++j) {
    //                    // same flavor (abs PDG equal 11 or 13) and opposite sign
    //                    int id0 = std::abs(pdgid[i]), id1 = std::abs(pdgid[j]);
    //                    if (!((id0 == 11 && id1 == 11) || (id0 == 13 && id1 == 13))) continue;
    //                    if (charge[i] * charge[j] >= 0) continue;
    //    
    //                    TLorentzVector v1; v1.SetPtEtaPhiM(pt[i], eta[i], phi[i], mass[i]);
    //                    TLorentzVector v2; v2.SetPtEtaPhiM(pt[j], eta[j], phi[j], mass[j]);
    //                    double m = (v1 + v2).M();
    //                    if (m < minM) minM = m;
    //                }
    //            }
    //            return (std::isfinite(minM) ? minM : -1.0);
    //        }, {"PT_lep","Eta_lep","Phi_lep","M_lep","PDGID_lep","Charge_lep"});
    //}
    //
    //if (node.HasColumn("HasMinPair_OSSF") == false) 
    //    node = node.Define("HasMinPair_OSSF", [](double minM){ return minM >= 0.0; }, {"minM_ll_OSSF"});

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
    //    .Define("F_lep0_a", [](const std::vector<int> &pdgid,
    //                           const std::vector<int> &index_lep_a_LEP,
    //                           const int &Nlep){
    //            if (index_lep_a_LEP.size() < 1 && Nlep > 2) return 0;
    //            int id = 0;
    //            if (Nlep > 2) id = std::abs(pdgid[index_lep_a_LEP[0]]);
    //            else if (Nlep == 2) id = std::abs(pdgid[0]);
    //            return (id == 11 ? 1 : (id == 13 ? 2 : 0));
    //        }, {"PDGID_lep","index_lep_a_LEP","Nlep"})
    //    .Define("F_lep1_a", [](const std::vector<int> &pdgid,
    //                           const std::vector<int> &index_lep_a_LEP,
    //                           const int &Nlep){
    //            if (index_lep_a_LEP.size() < 2 && Nlep > 2) return 0;
    //            int id = 0;
    //            if (Nlep > 2) id = std::abs(pdgid[index_lep_a_LEP[1]]);
    //            else if (Nlep == 2) id = std::abs(pdgid[1]);
    //            return (id == 11 ? 1 : (id == 13 ? 2 : 0));
    //        }, {"PDGID_lep","index_lep_a_LEP","Nlep"})
    //    .Define("TLV_Cand", [](const TLorentzVector &l0,
    //                           const TLorentzVector &l1
    //                          //, int q0, int q1, int f0, int f1
    //                          ) {
    //            // Only build TLV if OSSF
    //            //if (f0 != 0 && f0 == f1 && q0 * q1 == -1) {
    //            //    return l0 + l1;
    //            //}
    //            //return TLorentzVector{}; // return zero TLV otherwise
    //            return l0 + l1;
    //        }, {"My_p4_lep0_a","My_p4_lep1_a"
    //           //,"Q_lep0_a","Q_lep1_a","F_lep0_a","F_lep1_a"
    //           })
    //    .Define("HasCand", [](const TLorentzVector &cand){
    //            return cand.E() > 0.0;  // nonzero candidate
    //        }, {"TLV_Cand"})
    //    .Define("CandBetaZLab", [](const TLorentzVector &Cand){
    //            return (Cand.E() != 0.0) ? Cand.Pz() / Cand.E() : 0.0;
    //        }, {"TLV_Cand"})
    //    .Define("CandRapidityLab", [](const TLorentzVector &Cand){
    //            return Cand.Rapidity();
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
    // Examples below \/
    //// ---------------------------------------------------------------------
    //// Step 1: Build TLorentzVectors for the leading leptons from vector branches
    //// ---------------------------------------------------------------------
    //// We assume the tree stores lepton kinematics as vectors:
    ////   vector<double>  *PT_lep, *Eta_lep, *Phi_lep, *M_lep
    ////
    //// For safety, we check vector sizes before accessing indices.
    //// My_p4_lep0 and My_p4_lep1 are TLorentzVector objects for the leading and
    //// sub-leading leptons respectively (by ordering already present in the trees).
    //node = node
    //    .Define("My_p4_lep0", [](const std::vector<double> &pt,
    //                          const std::vector<double> &eta,
    //                          const std::vector<double> &phi,
    //                          const std::vector<double> &mass) {
    //            // Construct TLV for lepton 0 if present; otherwise returns a zero TLV.
    //            TLorentzVector v;
    //            if (!pt.empty() && pt.size() == eta.size() && eta.size() == phi.size() && phi.size() == mass.size()) {
    //                // safe access to [0]
    //                v.SetPtEtaPhiM(pt[0], eta[0], phi[0], mass[0]);
    //            }
    //            return v;
    //        }, {"PT_lep","Eta_lep","Phi_lep","M_lep"})
    //    .Define("My_p4_lep1", [](const std::vector<double> &pt,
    //                          const std::vector<double> &eta,
    //                          const std::vector<double> &phi,
    //                          const std::vector<double> &mass) {
    //            // Construct TLV for lepton 1 if present; otherwise returns a zero TLV.
    //            TLorentzVector v;
    //            if (pt.size() > 1 && pt.size() == eta.size() && eta.size() == phi.size() && phi.size() == mass.size()) {
    //                // safe access to [1]
    //                v.SetPtEtaPhiM(pt[1], eta[1], phi[1], mass[1]);
    //            }
    //            return v;
    //        }, {"PT_lep","Eta_lep","Phi_lep","M_lep"});

    //// ---------------------------------------------------------------------
    //// Step 2: Calculate invariant mass of the leading two leptons (M_ll)
    //// ---------------------------------------------------------------------
    //// This defines a column "M_ll" computed from the two TLVs. If either TLV
    //// is zero (insufficient leptons) the TLV addition yields a zero mass (0).
    //node = node.Define("M_ll", [](const TLorentzVector &l0, const TLorentzVector &l1){
    //            return (l0 + l1).M();
    //        }, {"My_p4_lep0","My_p4_lep1"});

    //// ---------------------------------------------------------------------
    //// Step 3: Extract charges and flavors for the first two leptons (convenience)
    //// ---------------------------------------------------------------------
    //// These are convenience columns for users who want to filter on charge/flavor.
    //// They are defensive: if the vector doesn't have enough entries we return 0.
    //node = node
    //    .Define("Q_lep0", [](const std::vector<int> &charge){
    //            return charge.empty() ? 0 : charge[0];
    //        }, {"Charge_lep"})
    //    .Define("Q_lep1", [](const std::vector<int> &charge){
    //            return (charge.size() > 1) ? charge[1] : 0;
    //        }, {"Charge_lep"})
    //    // Flavor encoding function: 1 = electron, 2 = muon, 0 = other/unknown.
    //    // We keep flavors simple and explicit so it's straightforward to filter on.
    //    .Define("F_lep0", [](const std::vector<int> &pdgid){
    //            if (pdgid.empty()) return 0;
    //            int id = std::abs(pdgid[0]);
    //            return (id == 11 ? 1 : (id == 13 ? 2 : 0));
    //        }, {"PDGID_lep"})
    //    .Define("F_lep1", [](const std::vector<int> &pdgid){
    //            if (pdgid.size() < 2) return 0;
    //            int id = std::abs(pdgid[1]);
    //            return (id == 11 ? 1 : (id == 13 ? 2 : 0));
    //        }, {"PDGID_lep"});

    //// ---------------------------------------------------------------------
    //// Step 4: Define OSSF boolean robustly (explicitly check vector sizes)
    //// ---------------------------------------------------------------------
    //// IMPORTANT: instead of relying on the convenience Q/F columns above,
    //// we compute OSSF_pair directly from the vectors to be *explicitly*
    //// robust for events with <2 leptons.
    ////
    //// OSSF_pair = true when:
    ////   - there are at least two leptons (vectors have size >= 2)
    ////   - the first two leptons are opposite sign (q0 * q1 < 0)
    ////   - the first two leptons have the same flavor (both electrons or both muons)
    //node = node.Define("OSSF_pair",
    //        [](const std::vector<int> &charge, const std::vector<int> &pdgid) -> bool {
    //            // must have at least two leptons
    //            if (charge.size() < 2 || pdgid.size() < 2) return false;

    //            int q0 = charge[0], q1 = charge[1];
    //            int id0 = std::abs(pdgid[0]), id1 = std::abs(pdgid[1]);

    //            // require electron-electron or muon-muon
    //            bool sameFlavor = (id0 == 11 && id1 == 11) || (id0 == 13 && id1 == 13);
    //            bool oppositeSign = (q0 * q1 < 0);

    //            return (sameFlavor && oppositeSign);
    //        }, {"Charge_lep","PDGID_lep"});

    //// ---------------------------------------------------------------------
    //// Step 5: Define HT_eta24 / MET ratio safely
    //// ---------------------------------------------------------------------
    //// Add a derived column "HTeta24_over_MET". Protect against MET==0.
    //node = node.Define("HTeta24_over_MET", [](double HT_eta24, double MET) {
    //            if (MET == 0.0) return 0.0; // avoid divide-by-zero; choose safe default
    //            return HT_eta24 / MET;
    //        }, {"HT_eta24","MET"});
    return node;
}

static std::vector<HistDef> loadHistogramsUser() {
    std::vector<HistDef> hdefs;

    // 1D histogram for min M_ll (all pairs)
    HistDef h_min_all;
    h_min_all.name    = "minM_ll_all";
    h_min_all.type    = "1D";
    h_min_all.expr    = "minM_ll_all";
    h_min_all.nbins   = 50;
    h_min_all.xmin    = 0;
    h_min_all.xmax    = 5;
    h_min_all.x_title = "min M_{ll} (GeV)";
    h_min_all.cuts    = {"HasMinPair_all"}; // require a valid pair
    h_min_all.lepCuts = {};
    h_min_all.predefCuts = {};
    h_min_all.userCuts = {};
    //hdefs.push_back(h_min_all);
    
    // 1D histogram for min M_ll (OSSF pairs only)
    HistDef h_min_ossf;
    h_min_ossf.name    = "minM_ll_OSSF";
    h_min_ossf.type    = "1D";
    h_min_ossf.expr    = "minM_ll_OSSF";
    h_min_ossf.nbins   = 60;
    h_min_ossf.xmin    = 0;
    h_min_ossf.xmax    = 200;
    h_min_ossf.x_title = "min M_{ll} (OSSF pairs) [GeV]";
    h_min_ossf.cuts    = {"HasMinPair_OSSF"};
    h_min_ossf.lepCuts = {};
    h_min_ossf.predefCuts = {};
    h_min_ossf.userCuts = {};
    //hdefs.push_back(h_min_ossf);

    HistDef h_CandCosDecayAngleLab_CandBetaZLab;
    h_CandCosDecayAngleLab_CandBetaZLab.name = "CandCosDecayAngleLab_vs_CandBetaZLab";
    h_CandCosDecayAngleLab_CandBetaZLab.type = "2D";
    h_CandCosDecayAngleLab_CandBetaZLab.expr = "CandCosDecayAngleLab";
    h_CandCosDecayAngleLab_CandBetaZLab.yexpr = "CandBetaZLab";
    h_CandCosDecayAngleLab_CandBetaZLab.nbins = 64;
    h_CandCosDecayAngleLab_CandBetaZLab.xmin = 0;
    h_CandCosDecayAngleLab_CandBetaZLab.xmax = 1;
    h_CandCosDecayAngleLab_CandBetaZLab.x_title = "cos#theta Z^{*}";
    h_CandCosDecayAngleLab_CandBetaZLab.nybins = 64;
    h_CandCosDecayAngleLab_CandBetaZLab.ymin = 0;
    h_CandCosDecayAngleLab_CandBetaZLab.ymax = 1;
    h_CandCosDecayAngleLab_CandBetaZLab.y_title = "#beta Z^{*}";
    h_CandCosDecayAngleLab_CandBetaZLab.cuts = {"HasCand"};
    h_CandCosDecayAngleLab_CandBetaZLab.lepCuts = {};
    h_CandCosDecayAngleLab_CandBetaZLab.predefCuts = {};
    h_CandCosDecayAngleLab_CandBetaZLab.userCuts = {};
    //hdefs.push_back(h_CandCosDecayAngleLab_CandBetaZLab);

    HistDef h_CandCosDecayAngleLab_CandRapidityLab;
    h_CandCosDecayAngleLab_CandRapidityLab.name = "CandCosDecayAngleLab_vs_CandRapidityLab";
    h_CandCosDecayAngleLab_CandRapidityLab.type = "2D";
    h_CandCosDecayAngleLab_CandRapidityLab.expr = "CandCosDecayAngleLab";
    h_CandCosDecayAngleLab_CandRapidityLab.yexpr = "CandRapidityLab";
    h_CandCosDecayAngleLab_CandRapidityLab.nbins = 64;
    h_CandCosDecayAngleLab_CandRapidityLab.xmin = 0;
    h_CandCosDecayAngleLab_CandRapidityLab.xmax = 1;
    h_CandCosDecayAngleLab_CandRapidityLab.x_title = "cos#theta Z^{*}";
    h_CandCosDecayAngleLab_CandRapidityLab.nybins = 64;
    h_CandCosDecayAngleLab_CandRapidityLab.ymin = -2.5;
    h_CandCosDecayAngleLab_CandRapidityLab.ymax = 2.5;
    h_CandCosDecayAngleLab_CandRapidityLab.y_title = "Rapidity Z^{*}";
    h_CandCosDecayAngleLab_CandRapidityLab.cuts = {"HasCand"};
    h_CandCosDecayAngleLab_CandRapidityLab.lepCuts = {};
    h_CandCosDecayAngleLab_CandRapidityLab.predefCuts = {};
    h_CandCosDecayAngleLab_CandRapidityLab.userCuts = {};
    //hdefs.push_back(h_CandCosDecayAngleLab_CandRapidityLab);

    // ---------------------------------------------------------------------
    // Step 6: Build HistDefs to return (do NOT fill/write here)
    // ---------------------------------------------------------------------
    // 1) 1D histogram for M_ll but restricted to OSSF pairs. We express that
    //    restriction by adding "OSSF_pair" to the histogram-specific cuts;
    //    the main histogram loop will apply the cut to the node.
    HistDef h1;
    h1.name = "M_ll_lead2_OSSF";
    h1.type = "1D";
    h1.expr = "M_ll";          // histogram the M_ll column (computed above)
    h1.nbins = 50;
    h1.xmin = 0;
    h1.xmax = 200;
    h1.x_title = "M_{ll} for OSSF pair of lead leps";
    h1.cuts = {"OSSF_pair"};
    h1.lepCuts = {};
    h1.predefCuts = {};
    h1.userCuts = {};
    // hdefs.push_back(h1); // uncomment to load hist

    // 2) 2D histogram: M_ll (x) vs HTeta24_over_MET (y)
    //    This uses the derived ratio defined above
    HistDef h2;
    h2.name = "M_ll_lead2_vs_HTeta24overMET";
    h2.type = "2D";
    h2.expr = "M_ll";               // X-axis: invariant mass of leading two leptons
    h2.yexpr = "HTeta24_over_MET";   // Y-axis: derived HT/MET ratio
    h2.nbins = 50;
    h2.xmin = 0;
    h2.xmax = 100;
    h2.nybins = 50;
    h2.ymin = 0;
    h2.ymax = 3;
    h2.x_title = "M_{ll} for OSSF pair of lead leps";
    h2.y_title = "HT/MET";
    h2.cuts = {};       // no extra event-level cuts here (hist loop may also apply global cuts)
    h2.lepCuts = {};
    h2.predefCuts = {};
    h2.userCuts = {};
    // hdefs.push_back(h2); // uncomment to load hist

    HistDef h_minM_vs_minDR;
    h_minM_vs_minDR.name    = "minM_ll_vs_minDR_ll";
    h_minM_vs_minDR.type    = "2D";
    h_minM_vs_minDR.expr    = "minDR_ll_all";
    h_minM_vs_minDR.yexpr   = "minM_ll_all";
    h_minM_vs_minDR.nbins   = 50;
    h_minM_vs_minDR.xmin    = 0.0;
    h_minM_vs_minDR.xmax    = 2.0;
    h_minM_vs_minDR.nybins  = 50;
    h_minM_vs_minDR.ymin    = 0.0;
    h_minM_vs_minDR.ymax    = 6.0;
    h_minM_vs_minDR.x_title = "min #DeltaR(ll)";
    h_minM_vs_minDR.y_title = "min M_{ll} [GeV]";
    h_minM_vs_minDR.cuts    = {"HasMinPair_all", "HasMinDR_all"};
    h_minM_vs_minDR.lepCuts = {};
    h_minM_vs_minDR.predefCuts = {};
    h_minM_vs_minDR.userCuts = {};
    //hdefs.push_back(h_minM_vs_minDR);

    HistDef h_minM_vs_DRatMinM;
    h_minM_vs_DRatMinM.name    = "minM_ll_vs_DR_at_minM";
    h_minM_vs_DRatMinM.type    = "2D";
    h_minM_vs_DRatMinM.expr    = "DR_at_minM_ll_all";
    h_minM_vs_DRatMinM.yexpr   = "minM_ll_all";
    h_minM_vs_DRatMinM.nbins   = 50;
    h_minM_vs_DRatMinM.xmin    = 0.0;
    h_minM_vs_DRatMinM.xmax    = 2.0;
    h_minM_vs_DRatMinM.nybins  = 50;
    h_minM_vs_DRatMinM.ymin    = 0.0;
    h_minM_vs_DRatMinM.ymax    = 6.0;
    h_minM_vs_DRatMinM.x_title = "#DeltaR(ll) of min-M_{ll} pair";
    h_minM_vs_DRatMinM.y_title = "min M_{ll} [GeV]";
    h_minM_vs_DRatMinM.cuts    = {"HasMinPair_all", "HasDRatMinM_all"};
    h_minM_vs_DRatMinM.lepCuts = {};
    h_minM_vs_DRatMinM.predefCuts = {};
    h_minM_vs_DRatMinM.userCuts = {};
    //hdefs.push_back(h_minM_vs_DRatMinM);

    return hdefs;
}
