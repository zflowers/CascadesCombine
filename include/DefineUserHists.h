#include "HistTools.h"
#include "TLorentzVector.h"

// Match existing HistDef type
static std::vector<HistDef> loadHistogramsUser(ROOT::RDF::RNode &node) {
    std::vector<HistDef> hdefs;
    // ---------------------------------------------------------------------
    // Step 1: Build TLorentzVectors for the leading leptons from vector branches
    // ---------------------------------------------------------------------
    // We assume the tree stores lepton kinematics as vectors:
    //   vector<double>  *PT_lep, *Eta_lep, *Phi_lep, *M_lep
    //
    // For safety we check vector sizes before accessing indices.
    // My_p4_lep0 and My_p4_lep1 are TLorentzVector objects for the leading and
    // sub-leading leptons respectively (by ordering already present in the trees).
    node = node
        .Define("My_p4_lep0", [](const std::vector<double> &pt,
                              const std::vector<double> &eta,
                              const std::vector<double> &phi,
                              const std::vector<double> &mass) {
                // Construct TLV for lepton 0 if present; otherwise returns a zero TLV.
                TLorentzVector v;
                if (!pt.empty() && pt.size() == eta.size() && eta.size() == phi.size() && phi.size() == mass.size()) {
                    // safe access to [0]
                    v.SetPtEtaPhiM(pt[0], eta[0], phi[0], mass[0]);
                }
                return v;
            }, {"PT_lep","Eta_lep","Phi_lep","M_lep"})
        .Define("My_p4_lep1", [](const std::vector<double> &pt,
                              const std::vector<double> &eta,
                              const std::vector<double> &phi,
                              const std::vector<double> &mass) {
                // Construct TLV for lepton 1 if present; otherwise returns a zero TLV.
                TLorentzVector v;
                if (pt.size() > 1 && pt.size() == eta.size() && eta.size() == phi.size() && phi.size() == mass.size()) {
                    // safe access to [1]
                    v.SetPtEtaPhiM(pt[1], eta[1], phi[1], mass[1]);
                }
                return v;
            }, {"PT_lep","Eta_lep","Phi_lep","M_lep"});

    // ---------------------------------------------------------------------
    // Step 2: Calculate invariant mass of the leading two leptons (M_ll)
    // ---------------------------------------------------------------------
    // This defines a column "M_ll" computed from the two TLVs. If either TLV
    // is zero (insufficient leptons) the TLV addition yields a zero mass (0).
    node = node.Define("M_ll", [](const TLorentzVector &l0, const TLorentzVector &l1){
                return (l0 + l1).M();
            }, {"My_p4_lep0","My_p4_lep1"});

    // ---------------------------------------------------------------------
    // Step 3: Extract charges and flavors for the first two leptons (convenience)
    // ---------------------------------------------------------------------
    // These are convenience columns for users who want to filter on charge/flavor.
    // They are defensive: if the vector doesn't have enough entries we return 0.
    node = node
        .Define("Q_lep0", [](const std::vector<int> &charge){
                return charge.empty() ? 0 : charge[0];
            }, {"Charge_lep"})
        .Define("Q_lep1", [](const std::vector<int> &charge){
                return (charge.size() > 1) ? charge[1] : 0;
            }, {"Charge_lep"})
        // Flavor encoding function: 1 = electron, 2 = muon, 0 = other/unknown.
        // We keep flavors simple and explicit so it's straightforward to filter on.
        .Define("F_lep0", [](const std::vector<int> &pdgid){
                if (pdgid.empty()) return 0;
                int id = std::abs(pdgid[0]);
                return (id == 11 ? 1 : (id == 13 ? 2 : 0));
            }, {"PDGID_lep"})
        .Define("F_lep1", [](const std::vector<int> &pdgid){
                if (pdgid.size() < 2) return 0;
                int id = std::abs(pdgid[1]);
                return (id == 11 ? 1 : (id == 13 ? 2 : 0));
            }, {"PDGID_lep"});

    // ---------------------------------------------------------------------
    // Step 4: Define OSSF boolean robustly (explicitly check vector sizes)
    // ---------------------------------------------------------------------
    // IMPORTANT: instead of relying on the convenience Q/F columns above,
    // we compute OSSF_pair directly from the vectors to be *explicitly*
    // robust for events with <2 leptons.
    //
    // OSSF_pair = true when:
    //   - there are at least two leptons (vectors have size >= 2)
    //   - the first two leptons are opposite sign (q0 * q1 < 0)
    //   - the first two leptons have the same flavor (both electrons or both muons)
    node = node.Define("OSSF_pair",
            [](const std::vector<int> &charge, const std::vector<int> &pdgid) -> bool {
                // must have at least two leptons
                if (charge.size() < 2 || pdgid.size() < 2) return false;

                int q0 = charge[0], q1 = charge[1];
                int id0 = std::abs(pdgid[0]), id1 = std::abs(pdgid[1]);

                // require electron-electron or muon-muon
                bool sameFlavor = (id0 == 11 && id1 == 11) || (id0 == 13 && id1 == 13);
                bool oppositeSign = (q0 * q1 < 0);

                return (sameFlavor && oppositeSign);
            }, {"Charge_lep","PDGID_lep"});

    // ---------------------------------------------------------------------
    // Step 5: Define HT_eta24 / MET ratio safely
    // ---------------------------------------------------------------------
    // Add a derived column "HTeta24_over_MET". Protect against MET==0.
    node = node.Define("HTeta24_over_MET", [](double HT_eta24, double MET) {
                if (MET == 0.0) return 0.0; // avoid divide-by-zero; choose safe default
                return HT_eta24 / MET;
            }, {"HT_eta24","MET"});

    // ---------------------------------------------------------------------
    // Step 6: Build HistDefs to return (do NOT fill/write here)
    // ---------------------------------------------------------------------
    // 1) 1D histogram for M_ll but restricted to OSSF pairs. We express that
    //    restriction by adding "OSSF_pair" to the histogram-specific cuts;
    //    the main histogram loop will apply this cut as node.Filter(BFI->ExpandMacros(...)).
    HistDef h1;
    h1.name = "M_ll_lead2_OSSF";
    h1.type = "1D";
    h1.expr = "M_ll";          // histogram the M_ll column (computed above)
    h1.nbins = 50;
    h1.xmin = 0;
    h1.xmax = 200;
    h1.x_title = "M_{ll} for OSSF pair of lead leps";
    // The main loop will interpret these strings as filters, so use the column name:
    // it will call node = node.Filter(BFI->ExpandMacros("OSSF_pair"));
    h1.cuts = {"OSSF_pair"};
    h1.lepCuts = {};
    h1.predefCuts = {};
    h1.userCuts = {};
    hdefs.push_back(h1);

    // 2) 2D histogram: M_ll (x) vs HTeta24_over_MET (y)
    //    This uses the derived ratio defined above; main loop will validate axes.
    HistDef h2;
    h2.name = "M_ll_lead2_vs_HTeta24overMET";
    h2.type = "2D";
    h2.expr = "M_ll";               // X-axis: invariant mass of leading two leptons
    h2.yexpr = "HTeta24_over_MET";   // Y-axis: derived HT/MET ratio
    h2.nbins = 50;
    h2.xmin = 0;
    h2.xmax = 500;
    h2.nybins = 50;
    h2.ymin = 0;
    h2.ymax = 5;
    h2.x_title = "M_{ll} for OSSF pair of lead leps";
    h2.y_title = "HT/MET";
    h2.cuts = {};       // no extra event-level cuts here (hist loop may also apply global cuts)
    h2.lepCuts = {};
    h2.predefCuts = {};
    h2.userCuts = {};
    hdefs.push_back(h2);

    // Return the user-provided histogram definitions. The main loop will:
    //   - apply h.cuts / h.lepCuts / h.predefCuts (via BFI->ExpandMacros) / h.userCuts
    //   - perform derived-variable validation for 2D axes
    //   - call Histo1D/Histo2D to fill and write histograms
    return hdefs;
}
