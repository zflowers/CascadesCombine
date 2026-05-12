#include <TFile.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <set>
#include <map>
#include <tuple>
#include <functional>
#include <filesystem>
#include <TGraphAsymmErrors.h>
#include <TMultiGraph.h>
#include <TAxis.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TLatex.h>
#include <TColor.h>
#include <TStyle.h>
#include <TLine.h>
#include <TEfficiency.h>
#include <TMinuit.h>
#include <TKey.h>
#include <Math/ProbFunc.h>
#include "nlohmann/json.hpp"
#include "Trigger_Fitter_Helpers.h"

// -----------------------------------------------------------------------
// Fit one efficiency graph, write fit params + seed band params to JSON
// -----------------------------------------------------------------------
void Fit_And_Save(const string& fname,
                  const string& canvasName,
                  const string& jsonKey,
                  int color,
                  const vector<int>& fit_colors,
                  const string& outdir)
{
    TLegend* dummy_leg = new TLegend(0,0,1,1);
    TGraphAsymmErrors* gr = Get_Graph_From_Canvas(fname, canvasName, color, dummy_leg, "");
    delete dummy_leg;
    if (!gr) return;

    double x_min = gr->GetXaxis()->GetXmin();
    double x_max = gr->GetXaxis()->GetXmax();

    for (int j = 0; j < gr->GetN(); j++) {
        gr->SetPointEXlow(j,  0.);
        gr->SetPointEXhigh(j, 0.);
    }

    // Guard: ensure the graph has enough points to fit
    if (!gr || gr->GetN() < 3) {
        cerr << "[Trigger_SFs]  Skip " << jsonKey << ": graph has "
             << (gr ? gr->GetN() : 0) << " point(s) -> too few to fit.\n";
        delete gr;
        return;
    }

    vector<TF1*> funcs;

    TF1* f_gauss = new TF1(("fGauss_"+jsonKey).c_str(), Gaussian_CDF_Func, x_min, x_max, 3);
    f_gauss->SetParameter(0, 0.99);  f_gauss->SetParName(0,"Norm");
    f_gauss->SetParameter(1, 200.);  f_gauss->SetParName(1,"Mean");
    f_gauss->SetParameter(2, 40.);   f_gauss->SetParName(2,"Sigma");
    funcs.push_back(f_gauss);

    TF1* f_dgauss = new TF1(("fDGauss_"+jsonKey).c_str(), Double_Gaussian_CDF_Func_Add, x_min, x_max, 5);
    f_dgauss->SetParameter(0, 0.99);  f_dgauss->SetParName(0,"Norm");
    f_dgauss->SetParameter(1, 200.);  f_dgauss->SetParName(1,"Mean");
    f_dgauss->SetParameter(2, 40.);   f_dgauss->SetParName(2,"Sigma");
    f_dgauss->SetParameter(3, 40.);   f_dgauss->SetParName(3,"Scale");
    f_dgauss->SetParameter(4, 0.5);   f_dgauss->SetParName(4,"Weight");
    funcs.push_back(f_dgauss);

    Get_Fit(gr, funcs, fit_colors, outdir+"output_Fits.root", jsonKey, outdir);

    // Pick best fit by chi2/NDF, write chosen params to JSON under the
    // standard "fit" key that Make_SF_Plot will read back
    TF1* best = f_gauss;
    double chi2_g  = (f_gauss->GetNDF()  > 0) ? f_gauss->GetChisquare()  / f_gauss->GetNDF()  : 1e9;
    double chi2_dg = (f_dgauss->GetNDF() > 0) ? f_dgauss->GetChisquare() / f_dgauss->GetNDF() : 1e9;
    if (chi2_dg < chi2_g) best = f_dgauss;

    cout << "  -> Best fit for " << jsonKey << ": " << best->GetName()
         << "  chi2/NDF=" << Round(min(chi2_g,chi2_dg)) << endl;

    Write_Fit_Params_JSON(jsonKey, outdir+"Fit_Parameters.json",
                          best->GetParameter(0),
                          best->GetParameter(1),
                          best->GetParameter(2),
                          (best->GetNpar() > 3) ? best->GetParameter(3) : 0.,
                          (best->GetNpar() > 4) ? best->GetParameter(4) : 0.);

    // Seed band params with conservative defaults only if not already set
    // this preserves any values the user has dialled in by hand
    string json_path = outdir + "Fit_Parameters.json";
    json j = Load_JSON(json_path);
    if ((!j.contains(jsonKey) || !j[jsonKey].contains("bands")) && jsonKey.find("Data")==std::string::npos) {
        cout << "  -> Seeding default band params for " << jsonKey
             << " (edit JSON to tune)" << endl;
        auto bp = Auto_Scale_Bands(gr, best, x_min, x_max);
        Write_Band_Params_JSON(jsonKey, json_path,
                       bp.a1, bp.a2, bp.b1, bp.b2, bp.c1, bp.c2);
    } else {
        cout << "  -> Band params already set for " << jsonKey
             << ", leaving unchanged." << endl;
    }

    delete gr;
}

// -----------------------------------------------------------------------
// Build a TF1 from JSON-stored fit parameters
// -----------------------------------------------------------------------
TF1* Make_TF1_From_JSON(const string& tfname, const string& jsonKey,
                         const string& json_path,
                         double x_min, double x_max)
{
    double norm=1., mean=200., sigma=40., scale=0., weight=0.;
    Read_Fit_Params_JSON(jsonKey, json_path, norm, mean, sigma, scale, weight);

    TF1* fn = nullptr;
    if (scale == 0. && weight == 0.) {
        fn = new TF1(tfname.c_str(),
                     "[0]*ROOT::Math::normal_cdf(x,[2],[1])", x_min, x_max);
        fn->SetParameters(norm, mean, sigma);
    } else {
        fn = new TF1(tfname.c_str(),
                     "[0]*((TMath::Cos([4])*TMath::Cos([4]))*ROOT::Math::normal_cdf(x,[2],[1])"
                     "+(TMath::Sin([4])*TMath::Sin([4]))*ROOT::Math::normal_cdf(x,[2]+[3],[1]))",
                     x_min, x_max);
        fn->SetParameters(norm, mean, sigma, scale, weight);
    }
    return fn;
}

// -----------------------------------------------------------------------
// Plot Data vs MC efficiency + Data/MC ratio for one config
// -----------------------------------------------------------------------
void Make_SF_Plot(const string& fname, const TriggerConfig& cfg,
                  const vector<int>& colors, const string& outdir)
{
    string json_path   = outdir + "Fit_Parameters.json";
    string bkg_canvas  = Make_Canvas_Name(cfg.bin, cfg.bkg_sample);
    string data_canvas = Make_Canvas_Name(cfg.bin, cfg.data_sample);
    string json_bkg    = JSON_Key(cfg.bin, cfg.bkg_sample);
    string json_data   = JSON_Key(cfg.bin, cfg.data_sample);
    string plot_name   = "SF_" + cfg.bin + "_" + cfg.data_sample;

    TLegend* leg = new TLegend(0.55, 0.05, 0.89, 0.3);
    leg->SetTextFont(132); leg->SetTextSize(0.045);
    if (invert_colors) {
        leg->SetTextColor(kWhite); leg->SetFillColor(kBlack);
        leg->SetLineColor(kBlack); leg->SetShadowColor(kBlack);
    }

    TGraphAsymmErrors* gr_bkg  = Get_Graph_From_Canvas(fname, bkg_canvas,  colors[0], leg, cfg.bkg_sample);
    TGraphAsymmErrors* gr_data = Get_Graph_From_Canvas(fname, data_canvas, colors[1], leg, cfg.data_sample);

    if (!gr_bkg || !gr_data) {
        cout << "Failed to get graphs for " << cfg.bin
             << " " << cfg.data_sample << endl;
        delete leg; return;
    }

    double x_min = gr_bkg->GetXaxis()->GetXmin();
    double x_max = gr_bkg->GetXaxis()->GetXmax();

    TF1* Bkg_Nominal  = Make_TF1_From_JSON("Bkg_Nominal",  json_bkg,  json_path, x_min, x_max);
    TF1* Data_Nominal = Make_TF1_From_JSON("Data_Nominal", json_data, json_path, x_min, x_max);
    Bkg_Nominal->SetLineColor(kGreen);
    Data_Nominal->SetLineColor(kAzure+10);

    // Read band params -> these are what the user tunes by hand in the JSON
    double a1, a2, b1, b2, c1, c2;
    if (!Read_Band_Params_JSON(json_bkg, json_path, a1, a2, b1, b2, c1, c2)) {
        cout << "WARNING: using hardcoded band defaults for " << json_bkg << endl;
        a1=.4e-5; a2=-.6e-5; b1=220.; b2=220.; c1=1.01; c2=0.99;
    }

    // Print what's being used so it's easy to see in the log while tuning
    cout << "\nBand params for " << json_bkg << ":" << endl;
    cout << "  a1=" << a1 << "  a2=" << a2 << endl;
    cout << "  b1=" << b1 << "  b2=" << b2 << endl;
    cout << "  c1=" << c1 << "  c2=" << c2 << endl;

    if (invert_colors) {
        gStyle->SetFrameFillColor(kBlack);
        gStyle->SetFrameLineColor(kWhite);
    }

    TCanvas* can = new TCanvas(plot_name.c_str(), "", 864, 468);
    can->SetGridx(); can->SetGridy(); can->Draw(); can->cd();
    if (invert_colors) can->SetFillColor(kBlack);

    // ---- Lower ratio pad ----
    TPad* pad_res = new TPad("pad_res","pad_res",0,0.03,1,0.349);
    pad_res->SetGridx(); pad_res->SetGridy();
    pad_res->SetTopMargin(0.1); pad_res->SetBottomMargin(0.2);
    pad_res->Draw(); pad_res->cd(); pad_res->Update(); can->Update();
    if (invert_colors) pad_res->SetFillColor(kBlack);

    TGraphAsymmErrors* res_ratio = TGAE_Ratio(gr_bkg, gr_data);
    if (!res_ratio) { delete can; delete leg; return; }
    res_ratio->SetMarkerColor(colors[1]);
    res_ratio->SetLineColor(colors[1]);
    res_ratio->SetMarkerStyle(20);
    res_ratio->SetMarkerSize(1.1);
    res_ratio->SetName("gr_res_ratio");

    TGraphErrors* gr_bands_ratio = Get_Bands_Ratio(x_min, x_max, res_ratio,
                                                    Bkg_Nominal, Data_Nominal,
                                                    a1, a2, b1, b2, c1, c2);
    gr_bands_ratio->SetFillColor(kCyan+2);
    gr_bands_ratio->SetFillStyle(3003);
    gr_bands_ratio->SetMarkerSize(0);

    TGraph* Fit_Ratio = Get_Fit_Ratio(x_min, x_max, Bkg_Nominal, Data_Nominal);
    Fit_Ratio->SetLineColor(kAzure+10);

    TMultiGraph* mg_res = new TMultiGraph();
    mg_res->Add(res_ratio);
    mg_res->Add(Fit_Ratio);
    mg_res->Draw("AP");
    Format_Graph_res(mg_res);
    mg_res->GetXaxis()->SetLimits(x_min, x_max);
    mg_res->GetXaxis()->SetRangeUser(150., 500.);
    mg_res->GetYaxis()->SetRangeUser(0.5, 1.15);
    mg_res->GetYaxis()->SetTitle("Data/MC Bkg");
    mg_res->GetXaxis()->SetTitle("MET [GeV]");
    pad_res->Modified(); pad_res->Update();
    gr_bands_ratio->Draw("30");
    Fit_Ratio->Draw("C");
    res_ratio->Draw("P");
    pad_res->Modified(); pad_res->Update();
    can->Modified(); can->Update();

    // ---- Upper efficiency pad ----
    can->cd();
    TPad* pad_gr = new TPad("pad_gr","pad_gr",0,0.35,1.,1.);
    pad_gr->SetGridx(); pad_gr->SetGridy();
    pad_gr->SetBottomMargin(0.01);
    pad_gr->Draw(); pad_gr->cd(); can->Update();
    if (invert_colors) pad_gr->SetFillColor(kBlack);

    TGraphErrors* gr_bands = Get_Bands(x_min, x_max, Data_Nominal,
                                        gr_bands_ratio->GetN(),
                                        a1, a2, b1, b2, c1, c2);
    gr_bands->SetFillColor(kCyan+2);
    gr_bands->SetFillStyle(3003);
    gr_bands->SetMarkerSize(0);

    gr_bands->Draw("A3");
    Data_Nominal->Draw("SAME");
    Bkg_Nominal->Draw("SAME");
    Format_Graph(gr_bands);
    gr_bands->SetTitle("");
    gr_bands->GetXaxis()->SetLimits(x_min, x_max);
    gr_bands->GetXaxis()->SetRangeUser(150., 500.);
    gr_bands->GetYaxis()->SetRangeUser(0.15, 1.05);
    gr_bands->GetYaxis()->SetTitle("Efficiency");
    gr_bkg->Draw("P");
    gr_data->Draw("P");
    pad_gr->Modified(); pad_gr->Update();
    can->Modified(); can->Update();

    leg->AddEntry(gr_bands,    "Systematic Uncertainty", "F");
    leg->AddEntry(Data_Nominal, (cfg.data_sample+" Fit").c_str(), "L");
    leg->AddEntry(Bkg_Nominal,  (cfg.bkg_sample+" Fit").c_str(),  "L");
    leg->Draw("SAME");

    TLatex l;
    if (invert_colors) l.SetTextColor(kWhite);
    l.SetTextFont(42); l.SetNDC(); l.SetTextSize(0.06);
    l.DrawLatex(0.65, 0.93, (cfg.bin + " " + cfg.year).c_str());
    l.DrawLatex(0.10, 0.93, "#bf{#it{CMS}} Preliminary");
    pad_gr->Update(); can->Update(); can->cd();

    can->SaveAs((outdir+"SF_Plot_"+cfg.bin+"_"+cfg.data_sample+".pdf").c_str());
    TFile* fout = TFile::Open((outdir+"output_Scale.root").c_str(),"UPDATE");
    can->Write();
    fout->Close();
    delete fout;
    delete leg; delete can;
}

// -----------------------------------------------------------------------
// Colors for syst labels -> consistent across all plots
// -----------------------------------------------------------------------
int Syst_Color(const string& systLabel)
{
    if (systLabel == "Gold")   return kOrange+1;
    if (systLabel == "Silver") return kGray+2;
    if (systLabel == "Bronze") return kRed+1;
    // Fallback for unknown labels
    static map<string,int> extra;
    static int idx = 0;
    static const vector<int> pool = {kViolet+2, kGreen+3, kCyan+2, kMagenta+2};
    if (!extra.count(systLabel)) extra[systLabel] = pool[(idx++) % pool.size()];
    return extra[systLabel];
}

// -----------------------------------------------------------------------
// Plot nominal + all syst overlays for one (bin, sample) pair.
// Top pad:    nominal efficiency + each syst efficiency
// Bottom pad: syst/nominal ratio + systematic band from JSON
// One plot for bkg, one for data, called separately.
// -----------------------------------------------------------------------
void Make_Syst_Plot(const string& fname,
                    const string& bin,
                    const string& nominalSample,   // e.g. "bkg_2018" or "Data_Muon_2018"
                    const vector<string>& systLabels, // e.g. {"Gold","Silver","Bronze"}
                    const string& year,
                    const string& outdir)
{
    string json_path  = outdir + "Fit_Parameters.json";
    string json_nom   = JSON_Key(bin, nominalSample);
    string plot_name  = "SystPlot_" + bin + "_" + nominalSample;

    // ---- Fetch nominal graph ----
    string nom_canvas = Make_Canvas_Name(bin, nominalSample);
    TLegend* leg = new TLegend(0.62, 0.05, 0.88, 0.08 + 0.06*(1+systLabels.size()));
    leg->SetTextFont(132); leg->SetTextSize(0.042);
    leg->SetBorderSize(0);
    if (invert_colors) {
        leg->SetTextColor(kWhite); leg->SetFillColor(kBlack);
        leg->SetLineColor(kBlack); leg->SetShadowColor(kBlack);
    }

    TGraphAsymmErrors* gr_nom = Get_Graph_From_Canvas(
        fname, nom_canvas, kBlack, leg, ("Nominal: "+nominalSample).c_str());
    if (!gr_nom) {
        cout << "Make_Syst_Plot: missing nominal for " << nom_canvas << endl;
        delete leg; return;
    }
    gr_nom->SetMarkerStyle(20);
    gr_nom->SetMarkerSize(1.1);

    double x_min = 150.; //gr_nom->GetXaxis()->GetXmin();
    double x_max = 500.; //gr_nom->GetXaxis()->GetXmax();

    // Nominal fit curve from JSON
    TF1* Nom_Fit = Make_TF1_From_JSON("Nom_Fit", json_nom, json_path, x_min, x_max);
    Nom_Fit->SetLineColor(kBlack);
    Nom_Fit->SetLineStyle(2);

    // Band params from JSON (keyed to the nominal sample)
    double a1, a2, b1, b2, c1, c2;
    if (!Read_Band_Params_JSON(json_nom, json_path, a1, a2, b1, b2, c1, c2)) {
        a1=.4e-5; a2=-.6e-5; b1=220.; b2=220.; c1=1.01; c2=0.99;
    }

    // ---- Fetch syst graphs ----
    vector<TGraphAsymmErrors*> gr_systs;
    vector<string>             syst_labels_found;

    for (const auto& lbl : systLabels) {
        string sc = Make_Syst_Canvas_Name(bin, lbl, nominalSample);
        int col   = Syst_Color(lbl);
        TGraphAsymmErrors* gs = Get_Graph_From_Canvas(fname, sc, col, leg, lbl.c_str());
        if (!gs) {
            cout << "Make_Syst_Plot: missing syst canvas " << sc << " -- skipping" << endl;
            continue;
        }
        gs->SetMarkerStyle(24); // open circle to distinguish from nominal
        gs->SetMarkerSize(0.9);
        gr_systs.push_back(gs);
        syst_labels_found.push_back(lbl);
    }

    if (gr_systs.empty()) {
        cout << "Make_Syst_Plot: no syst graphs found for " << nominalSample << endl;
        delete leg; delete gr_nom; return;
    }

    // ---- Build ratio graphs: syst / nominal ----
    // TGAE_Ratio(bkg, data) computes data/bkg, so pass nominal as "bkg"
    vector<TGraphAsymmErrors*> gr_ratios;
    for (auto* gs : gr_systs) {
        TGraphAsymmErrors* r = TGAE_Ratio(gr_nom, gs);
        if (!r) {
            cout << "Make_Syst_Plot: TGAE_Ratio failed (point count mismatch?)" << endl;
            gr_ratios.push_back(nullptr);
        } else {
            gr_ratios.push_back(r);
        }
    }

    // ---- Systematic band in ratio same params as SF plot ----
    // The band here represents the same uncertainty envelope but
    // centred on 1 (i.e. relative to nominal), so we use a flat
    // TF1=1 for both "Bkg" and "Data" in Get_Bands_Ratio
    TF1* unity = new TF1("unity","1.0", x_min, x_max);
    TGraphErrors* gr_band_ratio = Get_Bands_Ratio(
        x_min, x_max, gr_nom, unity, unity, a1, a2, b1, b2, c1, c2);
    gr_band_ratio->SetFillColor(kCyan+2);
    gr_band_ratio->SetFillStyle(3003);
    gr_band_ratio->SetMarkerSize(0);

    // Upper efficiency band around nominal fit
    TGraphErrors* gr_band_eff = Get_Bands(
        x_min, x_max, Nom_Fit, 1000, a1, a2, b1, b2, c1, c2);
    gr_band_eff->SetFillColor(kCyan+2);
    gr_band_eff->SetFillStyle(3003);
    gr_band_eff->SetMarkerSize(0);
    leg->AddEntry(gr_band_eff, "Syst. band", "F");

    // ---- Canvas layout ----
    if (invert_colors) {
        gStyle->SetFrameFillColor(kBlack);
        gStyle->SetFrameLineColor(kWhite);
    }
    TCanvas* can = new TCanvas(plot_name.c_str(), "", 864, 468);
    can->SetGridx(); can->SetGridy(); can->Draw(); can->cd();
    if (invert_colors) can->SetFillColor(kBlack);

    // ---- Lower ratio pad ----
    TPad* pad_res = new TPad("pad_res","pad_res",0,0.03,1,0.349);
    pad_res->SetGridx(); pad_res->SetGridy();
    pad_res->SetTopMargin(0.1); pad_res->SetBottomMargin(0.2);
    pad_res->Draw(); pad_res->cd(); pad_res->Update(); can->Update();
    if (invert_colors) pad_res->SetFillColor(kBlack);

    TMultiGraph* mg_res = new TMultiGraph();
    for (int i = 0; i < int(gr_ratios.size()); i++) {
        if (!gr_ratios[i]) continue;
        gr_ratios[i]->SetMarkerColor(Syst_Color(syst_labels_found[i]));
        gr_ratios[i]->SetLineColor(Syst_Color(syst_labels_found[i]));
        gr_ratios[i]->SetMarkerStyle(24);
        gr_ratios[i]->SetMarkerSize(0.9);
        mg_res->Add(gr_ratios[i]);
    }
    mg_res->Draw("AP");
    Format_Graph_res(mg_res);
    mg_res->GetXaxis()->SetLimits(x_min, x_max);
    mg_res->GetYaxis()->SetRangeUser(0.85, 1.15);
    mg_res->GetYaxis()->SetTitle("Syst / Nominal");
    pad_res->Modified(); pad_res->Update();

    // Draw band first so points sit on top
    gr_band_ratio->Draw("30");

    // Reference line at 1
    TLine* line = new TLine(x_min, 1., x_max, 1.);
    line->SetLineColor(kBlack); line->SetLineStyle(2); line->SetLineWidth(1);
    line->Draw("SAME");

    for (auto* r : gr_ratios)
        if (r) r->Draw("P");
    pad_res->Modified(); pad_res->Update();
    can->Modified(); can->Update();

    // ---- Upper efficiency pad ----
    can->cd();
    TPad* pad_gr = new TPad("pad_gr","pad_gr",0,0.35,1.,1.);
    pad_gr->SetGridx(); pad_gr->SetGridy();
    pad_gr->SetBottomMargin(0.01);
    pad_gr->Draw(); pad_gr->cd(); can->Update();
    if (invert_colors) pad_gr->SetFillColor(kBlack);

    // Draw band, then fits, then points on top
    gr_band_eff->Draw("A3");
    Format_Graph(gr_band_eff);
    gr_band_eff->SetTitle("");
    gr_band_eff->GetXaxis()->SetLimits(x_min, x_max);
    gr_band_eff->GetXaxis()->SetRangeUser(150., 500.);
    gr_band_eff->GetYaxis()->SetRangeUser(0.0, 1.05);
    gr_band_eff->GetYaxis()->SetTitle("Efficiency");

    Nom_Fit->Draw("SAME");
    gr_nom->Draw("P");
    for (auto* gs : gr_systs) gs->Draw("P");

    pad_gr->Modified(); pad_gr->Update();
    can->Modified(); can->Update();

    leg->Draw("SAME");

    TLatex l;
    if (invert_colors) l.SetTextColor(kWhite);
    l.SetTextFont(42); l.SetNDC(); l.SetTextSize(0.06);
    bool is_data = (nominalSample.find("Data") != string::npos);
    string typeLabel = is_data ? "Data" : "MC Bkg";
    l.DrawLatex(0.55, 0.93, (bin + " " + year + " " + typeLabel).c_str());
    l.DrawLatex(0.10, 0.93, "#bf{#it{CMS}} Preliminary");

    pad_gr->Update(); can->Update(); can->cd();

    can->SaveAs((outdir+"SystPlot_"+bin+"_"+nominalSample+".pdf").c_str());
    TFile* fout = TFile::Open((outdir+"output_Scale.root").c_str(),"UPDATE");
    can->Write();
    fout->Close();
    delete fout;
    delete leg; delete can; delete unity;
}

// -----------------------------------------------------------------------
// Overlay SF ratio curves for all electron bins in one year
// -----------------------------------------------------------------------
void Make_Comparison_Plot(const vector<TriggerConfig>& configs_for_year,
                           const string& year,
                           const string& outdir)
{
    string json_path = outdir + "Fit_Parameters.json";

    // Pick a range consistent with the SF plots
    double x_min = 150.;
    double x_max = 500.;

    // Colors per electron bin
    auto bin_color = [](const string& bin) -> int {
        if (bin == "Electron0") return kAzure-2;
        if (bin == "Electron1") return kGreen+2;
        if (bin == "Electron2") return kOrange+1;
        if (bin == "Muon0")     return kViolet+2;
        if (bin == "Muon1")     return kRed+1;
        if (bin == "Muon2")     return kCyan+2;
        static map<string,int> extra;
        static int idx = 0;
        static const vector<int> pool = {kViolet+2, kRed+1, kCyan+2};
        if (!extra.count(bin)) extra[bin] = pool[(idx++) % pool.size()];
        return extra[bin];
    };

    string plot_name = "SFComparison_" + year;

    TLegend* leg = new TLegend(0.65, 0.17, 0.88, 0.17 + 0.07*configs_for_year.size());
    leg->SetTextFont(132);
    leg->SetTextSize(0.05);
    leg->SetBorderSize(0);
    if (invert_colors) {
        leg->SetTextColor(kWhite); leg->SetFillColor(kBlack);
        leg->SetLineColor(kBlack); leg->SetShadowColor(kBlack);
    }

    if (invert_colors) {
        gStyle->SetFrameFillColor(kBlack);
        gStyle->SetFrameLineColor(kWhite);
    }

    TCanvas* can = new TCanvas(plot_name.c_str(), "", 864, 468);
    can->SetGridx(); can->SetGridy(); can->Draw(); can->cd();
    can->SetBottomMargin(0.14);
    if (invert_colors) can->SetFillColor(kBlack);

    TMultiGraph* mg = new TMultiGraph();

    // Reference line at 1
    TLine* line = new TLine(x_min, 1., x_max, 1.);
    line->SetLineColor(kBlack); line->SetLineStyle(2); line->SetLineWidth(1);

    vector<TGraph*> ratio_curves;
    for (const auto& cfg : configs_for_year) {
        int col = bin_color(cfg.bin);

        string json_bkg  = JSON_Key(cfg.bin, cfg.bkg_sample);
        string json_data = JSON_Key(cfg.bin, cfg.data_sample);

        // Check both keys exist before trying to plot
        double norm, mean, sigma, scale, weight;
        if (!Read_Fit_Params_JSON(json_bkg,  json_path, norm, mean, sigma, scale, weight) ||
            !Read_Fit_Params_JSON(json_data, json_path, norm, mean, sigma, scale, weight)) {
            cout << "Make_Comparison_Plot: missing JSON params for "
                 << cfg.bin << " " << year << " -- skipping." << endl;
            continue;
        }

        TF1* Bkg_Fit  = Make_TF1_From_JSON("Bkg_"  + cfg.bin, json_bkg,
                                             json_path, x_min, x_max);
        TF1* Data_Fit = Make_TF1_From_JSON("Data_" + cfg.bin, json_data,
                                             json_path, x_min, x_max);

        TGraph* ratio = Get_Fit_Ratio(x_min, x_max, Bkg_Fit, Data_Fit);
        ratio->SetLineColor(col);
        ratio->SetLineWidth(2);
        ratio->SetName(("ratio_"+cfg.bin).c_str());
        ratio_curves.push_back(ratio);

        mg->Add(ratio, "L");
        leg->AddEntry(ratio, cfg.bin.c_str(), "L");

        delete Bkg_Fit;
        delete Data_Fit;
    }

    if (ratio_curves.empty()) {
        cout << "Make_Comparison_Plot: no curves for year " << year << endl;
        delete can; delete leg; delete mg; delete line;
        return;
    }

    mg->Draw("A");
    mg->GetXaxis()->SetLimits(x_min, x_max);
    mg->GetXaxis()->SetRangeUser(x_min, x_max);
    mg->GetYaxis()->SetRangeUser(0.6, 1.05);
    mg->GetXaxis()->SetTitle("MET [GeV]");
    mg->GetYaxis()->SetTitle("Data/MC SF");
    mg->SetTitle("");
    Format_Graph(mg);
    // Re-enable x-axis labels since this is a standalone plot (no ratio pad below)
    mg->GetXaxis()->SetLabelSize(0.05);
    mg->GetXaxis()->SetLabelOffset(0.015);
    mg->GetXaxis()->SetTitleOffset(1.02);
    mg->GetYaxis()->SetTitleOffset(0.75);

    line->Draw("SAME");
    leg->Draw("SAME");

    TLatex l;
    if (invert_colors) l.SetTextColor(kWhite);
    l.SetTextFont(42); l.SetNDC(); l.SetTextSize(0.05);
    l.DrawLatex(0.65, 0.93, year.c_str());
    l.DrawLatex(0.10, 0.93, "#bf{#it{CMS}} Preliminary");

    can->Modified(); can->Update();

    can->SaveAs((outdir + "SFComparison_" + year + ".pdf").c_str());
    TFile* fout = TFile::Open((outdir + "output_Scale.root").c_str(), "UPDATE");
    can->Write();
    fout->Close();
    delete fout;
    delete leg; delete can; delete mg; delete line;
}


// -----------------------------------------------------------------------
// Top-level
// -----------------------------------------------------------------------
void Run_Fits(const string& fname, const string& outdir)
{
    vector<int> colors     = {kGreen+2, kAzure-2};
    vector<int> fit_colors = {kRed, kBlue};

    vector<TriggerConfig> configs = Discover_Configs(fname);
    if (configs.empty()) { cout << "No configs found." << endl; return; }

    // Only wipe the JSON on a fresh fit run so band edits survive
    // a --plot-only rerun but a full --fit wipes stale params.
    string json_path = outdir + "Fit_Parameters.json";
    if (fileExists(json_path)) {
        cout << "Removing existing " << json_path << " for fresh fit run." << endl;
        remove(json_path.c_str());
    }

    for (auto& cfg : configs) {
        cout << "\n=== Fitting: " << cfg.bin << " " << cfg.year << " ===" << endl;
        Fit_And_Save(fname, Make_Canvas_Name(cfg.bin, cfg.bkg_sample),
                     JSON_Key(cfg.bin, cfg.bkg_sample),
                     kGreen+2, fit_colors, outdir);
        Fit_And_Save(fname, Make_Canvas_Name(cfg.bin, cfg.data_sample),
                     JSON_Key(cfg.bin, cfg.data_sample),
                     kAzure-2, fit_colors, outdir);
    }

    cout << "\nFit params and seed band params written to: " << json_path << endl;
    cout << "Edit the \"bands\" blocks in that file to tune systematics," << endl;
    cout << "then rerun with --plot to regenerate SF plots." << endl;
}

void Run_Plots(const string& fname, const string& outdir)
{
    vector<int> colors = {kGreen+2, kAzure-2};

    string json_path = outdir + "Fit_Parameters.json";
    if (!fileExists(json_path)) {
        cout << "ERROR: " << json_path << " not found. Run with --fit first." << endl;
        return;
    }

    vector<TriggerConfig> configs = Discover_Configs(fname);
    if (configs.empty()) { cout << "No configs found." << endl; return; }

    // Existing per-config SF plots
    for (auto& cfg : configs)
        Make_SF_Plot(fname, cfg, colors, outdir);

    // Group by year and make one comparison plot per year
    map<string, vector<TriggerConfig>> by_year;
    for (auto& cfg : configs)
        by_year[cfg.year].push_back(cfg);

    for (auto& [yr, cfgs] : by_year) {
        cout << "\n=== Comparison plot: " << yr << " ===" << endl;
        Make_Comparison_Plot(cfgs, yr, outdir);
    }
}

void Run_Syst_Plots(const string& fname, const string& outdir)
{
    string json_path = outdir + "Fit_Parameters.json";
    if (!fileExists(json_path)) {
        cout << "ERROR: " << json_path << " not found. Run with --fit first." << endl;
        return;
    }

    vector<TriggerConfig> configs = Discover_Configs(fname);
    if (configs.empty()) { cout << "No configs found." << endl; return; }

    for (auto& cfg : configs) {
        vector<string> syst_labels;
        for (auto& [lbl, _] : cfg.systs) syst_labels.push_back(lbl);
        if (syst_labels.empty()) {
            cout << "No syst labels for " << cfg.bin << " " << cfg.year << endl;
            continue;
        }

        // One plot per sample type (bkg and data separately)
        cout << "\n=== Syst plot: " << cfg.bin
             << " " << cfg.year << " bkg ===" << endl;
        Make_Syst_Plot(fname, cfg.bin, cfg.bkg_sample,
                       syst_labels, cfg.year, outdir);

        //cout << "\n=== Syst plot: " << cfg.bin
        //     << " " << cfg.year << " data ===" << endl;
        //Make_Syst_Plot(fname, cfg.bin, cfg.data_sample,
        //               syst_labels, cfg.year, outdir);
    }
}

int main(int argc, char* argv[])
{
    string fname  = "";
    bool do_fit   = false;
    bool do_plot  = false;
    bool do_syst  = false;

    for (int i = 1; i < argc; i++) {
        string arg(argv[i]);
        if (arg.rfind("-f",0)==0) { fname = string(argv[i+1]); i++; }
        if (arg == "--fit")  do_fit  = true;
        if (arg == "--plot") do_plot = true;
        if (arg == "--syst") do_syst = true;
    }

    if (fname.empty()) {
        cout << "Usage: ./Trigger_SFs.x -f <input.root> [--fit] [--plot] [--syst]" << endl;
        cout << "  --fit   run fits, seed JSON with band params" << endl;
        cout << "  --plot  make Data/MC SF plots using JSON" << endl;
        cout << "  --syst  make nominal+syst overlay plots" << endl;
        return 1;
    }

    if (!do_fit && !do_plot && !do_syst) { do_fit = true; do_plot = true; do_syst = true; }

    string outdir = Derive_Output_Dir(fname);
    if (do_fit)  { std::cout << "Running Fit" << std::endl; Run_Fits(fname, outdir); }
    if (do_plot) { std::cout << "Running Plot" << std::endl; Run_Plots(fname, outdir); }
    if (do_syst) { std::cout << "Running Syst" << std::endl; Run_Syst_Plots(fname, outdir); }

    return 0;
}
