#include <TFile.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <set>
#include <map>
#include <tuple>
#include <TTree.h>
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
#include <sys/stat.h>
#include <filesystem>
#include "nlohmann/json.hpp"
#include "Trigger_Fitter_Helpers.h"

using namespace std;
using json = nlohmann::json;

// -----------------------------------------------------------------------
// JSON parameter I/O
// -----------------------------------------------------------------------

const string JSON_NAME = "Fit_Parameters.json";

json Load_JSON(const string& path)
{
    json j;
    if (!fileExists(path)) return j;
    ifstream f(path);
    f >> j;
    return j;
}

void Save_JSON(const json& j, const string& path)
{
    ofstream f(path);
    f << j.dump(4) << endl;
}

void Write_Fit_Params_JSON(const string& key, const string& json_path,
                            double norm, double mean, double sigma,
                            double scale, double weight)
{
    json j = Load_JSON(json_path);
    j[key]["fit"]["norm"]   = norm;
    j[key]["fit"]["mean"]   = mean;
    j[key]["fit"]["sigma"]  = sigma;
    j[key]["fit"]["scale"]  = scale;
    j[key]["fit"]["weight"] = weight;
    Save_JSON(j, json_path);
}

void Write_Band_Params_JSON(const string& key, const string& json_path,
                             double a1, double a2,
                             double b1, double b2,
                             double c1, double c2)
{
    json j = Load_JSON(json_path);
    j[key]["bands"]["a1"] = a1;
    j[key]["bands"]["a2"] = a2;
    j[key]["bands"]["b1"] = b1;
    j[key]["bands"]["b2"] = b2;
    j[key]["bands"]["c1"] = c1;
    j[key]["bands"]["c2"] = c2;
    Save_JSON(j, json_path);
}

bool Read_Fit_Params_JSON(const string& key, const string& json_path,
                           double& norm, double& mean, double& sigma,
                           double& scale, double& weight)
{
    json j = Load_JSON(json_path);
    if (!j.contains(key) || !j[key].contains("fit")) {
        cout << "WARNING: No fit params in JSON for key: " << key << endl;
        return false;
    }
    auto& f = j[key]["fit"];
    norm   = f.value("norm",   1.0);
    mean   = f.value("mean",   200.0);
    sigma  = f.value("sigma",  40.0);
    scale  = f.value("scale",  0.0);
    weight = f.value("weight", 0.0);
    return true;
}

bool Read_Band_Params_JSON(const string& key, const string& json_path,
                            double& a1, double& a2,
                            double& b1, double& b2,
                            double& c1, double& c2)
{
    json j = Load_JSON(json_path);
    if (!j.contains(key) || !j[key].contains("bands")) {
        cout << "WARNING: No band params in JSON for key: " << key << endl;
        return false;
    }
    auto& b = j[key]["bands"];
    a1 = b.value("a1",  .4e-5);
    a2 = b.value("a2", -.6e-5);
    b1 = b.value("b1", 220.0);
    b2 = b.value("b2", 220.0);
    c1 = b.value("c1", 1.004);
    c2 = b.value("c2", 0.996);
    return true;
}

// -----------------------------------------------------------------------
// Discover configs from canvas names in the root file.
//
// Canvas naming convention:
//   can_eff_MET_trigger_eff_{electronBin}__TriggerBin_{electronBin}__{sample}
//
// We scan all TCanvas keys whose name contains "can_eff_MET_trigger_eff",
// then parse out the electronBin and sample. From the sample we extract
// the year (last 4 chars) and whether it's data or bkg, and what lepton
// type (Muon/Electron).
//
// Returns a list of unique (electronBin, bkg_sample, data_sample) triples,
// one per (electronBin, year) combination found, where:
//   - Electron0 -> paired with Data_Muon_{year}   (>=2 leptons, 0 electrons)
//   - Electron1 -> paired with Data_Electron_{year}
//   - Electron2 -> paired with Data_Electron_{year}
// -----------------------------------------------------------------------

struct TriggerConfig {
    string electronBin;  // e.g. "Electron0"
    string bkg_sample;   // e.g. "bkg_2018"
    string data_sample;  // e.g. "Data_Muon_2018"
    string year;         // e.g. "2018"
};

string JSON_Key(const string& electronBin, const string& sample)
{
    return electronBin + "__" + sample;
}

// Parse "Electron0" -> 0, "Electron1" -> 1, etc.
int Parse_Electron_N(const string& electronBin)
{
    string s = electronBin.substr(string("Electron").size());
    return stoi(s);
}

vector<TriggerConfig> Discover_Configs(const string& fname)
{
    TFile* f = TFile::Open(fname.c_str(), "READ");
    if (!f || f->IsZombie()) {
        cout << "Cannot open " << fname << " for config discovery." << endl;
        return {};
    }

    // Collect all canvas names matching our prefix
    const string PREFIX = "can_eff_MET_trigger_eff";

    // Use sets to track what (electronBin, year) combinations exist
    // and which samples are present for each.
    // map: (electronBin, year) -> set of samples found
    map<pair<string,string>, set<string>> found;

    TIter next(f->GetListOfKeys());
    TKey* key = nullptr;
    while ((key = (TKey*)next())) {
        string cname = key->GetName();
        if (cname.find(PREFIX) == string::npos) continue;
        if (string(key->GetClassName()) != "TCanvas") continue;

        // Format: can_eff_MET_trigger_eff_{eBin}__TriggerBin_{eBin}__{sample}
        // Strip the prefix + "_"
        string rest = cname.substr(PREFIX.length() + 1); // e.g. "Electron0__TriggerBin_Electron0__bkg_2018"

        // electronBin is everything before the first "__"
        size_t sep1 = rest.find("__");
        if (sep1 == string::npos) continue;
        string electronBin = rest.substr(0, sep1); // "Electron0"

        // sample is everything after "__TriggerBin_{electronBin}__"
        string triggerTag = "TriggerBin_" + electronBin + "__";
        size_t sep2 = rest.find(triggerTag);
        if (sep2 == string::npos) continue;
        string sample = rest.substr(sep2 + triggerTag.length()); // e.g. "bkg_2018" or "Data_Muon_2018"

        // Extract year: last 4 characters of sample
        if (sample.length() < 4) continue;
        string year = sample.substr(sample.length() - 4);
        if (year.find_first_not_of("0123456789") != string::npos) continue;

        found[{electronBin, year}].insert(sample);
    }
    f->Close(); delete f;

    // Build configs: for each (electronBin, year), pair bkg with the
    // appropriate data sample based on electron count.
    vector<TriggerConfig> configs;
    for (auto& [key_pair, samples] : found) {
        const string& electronBin = key_pair.first;
        const string& year        = key_pair.second;

        int nElectrons = Parse_Electron_N(electronBin);

        // Find bkg sample
        string bkg_sample = "";
        for (auto& s : samples)
            if (s.find("bkg") != string::npos) { bkg_sample = s; break; }

        // Find data sample based on lepton content:
        // Electron0: >=2 leptons, 0 electrons -> Muon data
        // Electron1+: at least 1 electron -> Electron data
        string data_sample = "";
        string data_type = (nElectrons == 0) ? "Muon" : "Electron";
        for (auto& s : samples)
            if (s.find("Data_"+data_type) != string::npos) { data_sample = s; break; }

        if (bkg_sample.empty() || data_sample.empty()) {
            cout << "WARNING: Incomplete sample set for " << electronBin
                 << " year " << year
                 << " (bkg='" << bkg_sample
                 << "' data='" << data_sample << "') -- skipping." << endl;
            continue;
        }

        configs.push_back({electronBin, bkg_sample, data_sample, year});
        cout << "Discovered config: " << electronBin
             << "  bkg=" << bkg_sample
             << "  data=" << data_sample
             << "  year=" << year << endl;
    }

    // Sort for deterministic ordering
    sort(configs.begin(), configs.end(), [](const TriggerConfig& a, const TriggerConfig& b){
        if (a.year != b.year) return a.year < b.year;
        return a.electronBin < b.electronBin;
    });

    return configs;
}

TGraphAsymmErrors* Get_Graph_From_Canvas(const string& fname,
                                          const string& canvasName,
                                          int color,
                                          TLegend*& leg,
                                          const string& legLabel)
{
    TFile* f = TFile::Open(fname.c_str(), "READ");
    if (!f || f->IsZombie()) {
        cout << "Cannot open " << fname << endl;
        return nullptr;
    }

    TCanvas* src = nullptr;
    f->GetObject(canvasName.c_str(), src);
    if (!src) {
        cout << "Cannot find canvas: " << canvasName << endl;
        f->Close(); delete f;
        return nullptr;
    }

    // Find the TEfficiency in the canvas primitives
    TEfficiency* eff = nullptr;
    TIter next(src->GetListOfPrimitives());
    TObject* obj = nullptr;
    while ((obj = next())) {
        if (obj->InheritsFrom(TEfficiency::Class())) {
            eff = (TEfficiency*)obj;
            break;
        }
    }

    if (!eff) {
        cout << "No TEfficiency found in canvas: " << canvasName << endl;
        f->Close(); delete f;
        return nullptr;
    }

    // Need to draw into a temporary canvas to populate the painted graph
    TCanvas* tmp = new TCanvas("tmp_paint","tmp_paint",10,10);
    tmp->cd();
    eff->Draw("AP");
    tmp->Update();

    TGraphAsymmErrors* painted = eff->GetPaintedGraph();
    if (!painted) {
        cout << "GetPaintedGraph() returned null for: " << canvasName << endl;
        delete tmp;
        f->Close(); delete f;
        return nullptr;
    }

    // Clone before closing the file and deleting the temp canvas
    TGraphAsymmErrors* gr = (TGraphAsymmErrors*)painted->Clone();
    delete tmp;
    f->Close(); delete f;

    gr->SetMarkerStyle(20);
    gr->SetMarkerColor(color);
    gr->SetLineColor(color);
    leg->AddEntry(gr, legLabel.c_str(), "PL");
    return gr;
}

// -----------------------------------------------------------------------
// Build the canvas key name from parts
// -----------------------------------------------------------------------
string Make_Canvas_Name(const string& electronBin, const string& sample)
{
    return "can_eff_MET_trigger_eff_" + electronBin
           + "__TriggerBin_" + electronBin
           + "__" + sample;
}

// -----------------------------------------------------------------------
// Fit one efficiency graph, write params to JSON
// -----------------------------------------------------------------------
void Fit_And_Save(const string& fname,
                  const string& canvasName,
                  const string& jsonKey,
                  int color,
                  const vector<int>& fit_colors,
                  const string& outdir
                 )
{
    TLegend* dummy_leg = new TLegend(0,0,1,1);
    TGraphAsymmErrors* gr = Get_Graph_From_Canvas(fname, canvasName, color, dummy_leg, "");
    delete dummy_leg;
    if (!gr) return;

    double x_min = gr->GetXaxis()->GetXmin();
    double x_max = gr->GetXaxis()->GetXmax();

    // Zero out X errors for fitting
    for (int j = 0; j < gr->GetN(); j++) {
        gr->SetPointEXlow(j, 0.);
        gr->SetPointEXhigh(j, 0.);
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

    // We intercept after the fact to pull params and write to JSON.
    Get_Fit(gr, funcs, fit_colors, outdir+"output_Fits.root", jsonKey);

    // After fitting, read back best-fit parameters from whichever function
    // converged better (lower chi2/NDF), then persist to JSON.
    TF1* best = f_gauss;
    double chi2_gauss  = (f_gauss->GetNDF()  > 0) ? f_gauss->GetChisquare()  / f_gauss->GetNDF()  : 1e9;
    double chi2_dgauss = (f_dgauss->GetNDF() > 0) ? f_dgauss->GetChisquare() / f_dgauss->GetNDF() : 1e9;
    if (chi2_dgauss < chi2_gauss) best = f_dgauss;

    double norm   = best->GetParameter(0);
    double mean   = best->GetParameter(1);
    double sigma  = best->GetParameter(2);
    double scale  = (best->GetNpar() > 3) ? best->GetParameter(3) : 0.;
    double weight = (best->GetNpar() > 4) ? best->GetParameter(4) : 0.;

    Write_Fit_Params_JSON(jsonKey, outdir+JSON_NAME, norm, mean, sigma, scale, weight);

    delete gr;
}

// -----------------------------------------------------------------------
// Build a TF1 from JSON-stored fit parameters
// -----------------------------------------------------------------------
TF1* Make_TF1_From_JSON(const string& tfname, const string& json_path, const string& jsonKey,
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
void Make_SF_Plot(const string& fname, const TriggerConfig& cfg, const vector<int>& colors, const string& outdir)
{
    string json_path = outdir + JSON_NAME;
    string bkg_canvas  = Make_Canvas_Name(cfg.electronBin, cfg.bkg_sample);
    string data_canvas = Make_Canvas_Name(cfg.electronBin, cfg.data_sample);
    string json_bkg    = JSON_Key(cfg.electronBin, cfg.bkg_sample);
    string json_data   = JSON_Key(cfg.electronBin, cfg.data_sample);
    string plot_name   = "SF_" + cfg.electronBin + "_" + cfg.data_sample;

    TLegend* leg = new TLegend(0.65, 0.05, 0.85, 0.3);
    leg->SetTextFont(132);
    leg->SetTextSize(0.045);
    if (invert_colors) {
        leg->SetTextColor(kWhite);
        leg->SetFillColor(kBlack);
        leg->SetLineColor(kBlack);
        leg->SetShadowColor(kBlack);
    }

    TGraphAsymmErrors* gr_bkg  = Get_Graph_From_Canvas(fname, bkg_canvas,  colors[0], leg, cfg.bkg_sample);
    TGraphAsymmErrors* gr_data = Get_Graph_From_Canvas(fname, data_canvas, colors[1], leg, cfg.data_sample);

    if (!gr_bkg || !gr_data) {
        cout << "Failed to get graphs for " << cfg.electronBin
             << " " << cfg.data_sample << endl;
        delete leg;
        return;
    }

    double x_min = gr_bkg->GetXaxis()->GetXmin();
    double x_max = gr_bkg->GetXaxis()->GetXmax();

    TF1* Bkg_Nominal  = Make_TF1_From_JSON("Bkg_Nominal",  json_path, json_bkg,  x_min, x_max);
    TF1* Data_Nominal = Make_TF1_From_JSON("Data_Nominal", json_path, json_data, x_min, x_max);
    Bkg_Nominal->SetLineColor(kGreen);
    Data_Nominal->SetLineColor(kAzure+10);

    // Band parameters from JSON (or defaults if not yet tuned)
    double a1=.4e-5, a2=-.6e-5, b1=220., b2=220., c1=1.004, c2=0.996;
    Read_Band_Params_JSON(json_data, json_path, a1, a2, b1, b2, c1, c2);

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
    mg_res->GetYaxis()->SetRangeUser(0.75, 1.25);
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
    l.DrawLatex(0.65, 0.93, (cfg.electronBin + " " + cfg.year).c_str());
    l.DrawLatex(0.10, 0.93, "#bf{#it{CMS}} Preliminary");
    pad_gr->Update(); can->Update(); can->cd();

    can->SaveAs((outdir+"SF_Plot_"+cfg.electronBin+"_"+cfg.data_sample+".pdf").c_str());
    TFile* fout = TFile::Open((outdir+"output_Scale.root").c_str(),"UPDATE");
    can->Write();
    fout->Close();
    delete fout;
    delete leg; delete can;
}

// -----------------------------------------------------------------------
// Top-level
// -----------------------------------------------------------------------
void Run_All(const string& fname)
{
    string outdir = Derive_Output_Dir(fname);
    vector<int> colors     = {kGreen+2, kAzure-2, kYellow, kViolet+2, kAzure+7, kPink};
    vector<int> fit_colors = {kRed, kBlue};

    // Discover what's in the file rather than hard-coding
    vector<TriggerConfig> configs = Discover_Configs(fname);
    if (configs.empty()) {
        cout << "No valid configs found in " << fname << endl;
        return;
    }

    // Step 1: fit each efficiency curve and persist params to JSON
    string json_path = outdir + JSON_NAME;
    if (fileExists(json_path)) remove(json_path.c_str());

    for (auto& cfg : configs) {
        string bkg_canvas  = Make_Canvas_Name(cfg.electronBin, cfg.bkg_sample);
        string data_canvas = Make_Canvas_Name(cfg.electronBin, cfg.data_sample);
        string json_bkg    = JSON_Key(cfg.electronBin, cfg.bkg_sample);
        string json_data   = JSON_Key(cfg.electronBin, cfg.data_sample);

        cout << "\nFitting bkg:  " << json_bkg  << endl;
        Fit_And_Save(fname, bkg_canvas,  json_bkg,  colors[0], fit_colors, outdir);

        cout << "\nFitting data: " << json_data << endl;
        Fit_And_Save(fname, data_canvas, json_data, colors[1], fit_colors, outdir);
    }

    // Step 2: make SF plots using fitted parameters from JSON
    for (auto& cfg : configs)
        Make_SF_Plot(fname, cfg, colors, outdir);
}

int main(int argc, char* argv[])
{
    string fname = "";

    for (int i = 1; i < argc; i++) {
        string arg(argv[i]);
        if (arg.rfind("-f=",0)==0) fname = arg.substr(3);
    }

    if (fname.empty()) {
        cout << "Usage: ./Trigger_SFs.x -f=<input_root_file>" << endl;
        return 1;
    }

    Run_All(fname);
    return 0;
}
