// Helpers for Plotting Tools
#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <cstdio>
#include <set>
#include <iomanip>
#include <sstream>

#include <TFile.h>
#include <TH1.h>
#include <TH2.h>
#include <TKey.h>
#include <TCanvas.h>
#include <TSystem.h>
#include <TString.h>
#include <TLatex.h>
#include <TLegend.h>
#include <TCollection.h>
#include <TPad.h>
#include <TStyle.h>
#include <TEfficiency.h>
#include <TGraph.h>
#include <TMultiGraph.h>
#include <TGraphAsymmErrors.h>

#include "SampleTool.h"

using namespace std;

// globals

TFile* outFile = nullptr;
int lumi = 1;
string outputDir = "plots/";
map<string,string> m_Title;
map<string,int>    m_Color;

double hlo = 0.09;
double hhi = 0.22;
double hbo = 0.15;
double hto = 0.07;

// ----------------------
// Helpers
// ----------------------
void copyConfigsToOutput(const std::string &outputDir,
                         const std::string &histCfg,
                         const std::string &processCfg,
                         const std::string &binsCfg) 
{
    if (gSystem->AccessPathName(outputDir.c_str())) {
        gSystem->mkdir(outputDir.c_str(), true); 
    }
    if (!histCfg.empty()) gSystem->CopyFile(histCfg.c_str(), (outputDir + "/" + histCfg).c_str(), true);
    if (!processCfg.empty()) gSystem->CopyFile(processCfg.c_str(), (outputDir + "/" + processCfg).c_str(), true);
    if (!binsCfg.empty()) gSystem->CopyFile(binsCfg.c_str(), (outputDir + "/" + binsCfg).c_str(), true);
}

void SetMinimumBinContent(TH1* h, double minVal) {
    if (!h) return;
    int nb = h->GetNbinsX();
    for (int i = 1; i <= nb; ++i) {
        double c = h->GetBinContent(i);
        if (c < minVal) h->SetBinContent(i, minVal);
    }
}

void GetMinMaxIntegral(const vector<TH1*>& vect, double &hmin, double &hmax) {
    hmin = 1e99; hmax = 0.;
    for (auto h : vect) {
        if (!h) continue;
        int nb = h->GetNbinsX();
        double localmin = 1e99;
        for (int i = 1; i <= nb; ++i) {
            double c = h->GetBinContent(i);
            if (c > 0. && c < localmin) localmin = c;
            if (c > hmax) hmax = c;
        }
        if (localmin < hmin) hmin = localmin;
    }
    if (hmin > 1e98) hmin = 0.;
}


const TColor rf_blue0(7000,0.749,0.78,0.933);
const TColor rf_blue1(7001,0.424,0.467,0.651);
const TColor rf_blue2(7002,0.255,0.302,0.522);
const TColor rf_blue3(7003,0.114,0.165,0.396);
const TColor rf_blue4(7004,0.024,0.063,0.251);
const TColor rf_green0(7010,0.737,0.949,0.784);
const TColor rf_green1(7011,0.435,0.722,0.498);
const TColor rf_green2(7012,0.239,0.576,0.314);
const TColor rf_green3(7013,0.082,0.439,0.161);
const TColor rf_green4(7014,0,0.275,0.063);
const TColor rf_red0(7020,1,0.796,0.776);
const TColor rf_red1(7021,0.957,0.612,0.576);
const TColor rf_red2(7022,0.765,0.361,0.318);
const TColor rf_red3(7023,0.58,0.157,0.11);
const TColor rf_red4(7024,0.365,0.035,0);
const TColor rf_yellow0(7030,1,0.933,0.776);
const TColor rf_yellow1(7031,0.957,0.843,0.576);
const TColor rf_yellow2(7032,0.765,0.631,0.318);
const TColor rf_yellow3(7033,0.58,0.443,0.11);
const TColor rf_yellow4(7034,0.365,0.259,0);
const TColor rf_purple0(7040,0.937,0.729,0.898);
const TColor rf_purple1(7041,0.753,0.478,0.702);
const TColor rf_purple2(7042,0.6,0.286,0.541);
const TColor rf_purple3(7043,0.42,0.075,0.353);
const TColor rf_purple4(7044,0.196,0,0.161);
const TColor rf_cyan0(7050,0.714,0.898,0.918);
const TColor rf_cyan1(7051,0.424,0.639,0.659);
const TColor rf_cyan2(7052,0.247,0.49,0.51);
const TColor rf_cyan3(7053,0.067,0.329,0.357);
const TColor rf_cyan4(7054,0,0.153,0.169);
const TColor rf_orange0(7060,1,0.882,0.776);
const TColor rf_orange1(7061,1,0.808,0.639);
const TColor rf_orange2(7062,0.839,0.608,0.4);
const TColor rf_orange3(7063,0.584,0.329,0.106);
const TColor rf_orange4(7064,0.275,0.129,0);
const TColor rf_lime0(7070,0.941,0.992,0.769);
const TColor rf_lime1(7071,0.882,0.961,0.612);
const TColor rf_lime2(7072,0.706,0.8,0.38);
const TColor rf_lime3(7073,0.455,0.557,0.098);
const TColor rf_lime4(7074,0.204,0.263,0);

void loadFormatMaps(){

  m_Title["ttbar"] = "t #bar{t} + X";
  m_Color["ttbar"] = 7011;
  //m_Color["ttbar"] = 8003;

  m_Title["ST"] = "single top";
  m_Color["ST"] = 7010;
  //m_Color["ST"] = 8004;

  m_Title["DB"] = "dibosons";
  m_Color["DB"] = 7051;
  //m_Color["DB"] = 8002;

  m_Title["TB"] = "tribosons";
  m_Color["TB"] = 7050;
  //m_Color["TB"] = 8006;
  
  m_Title["DBTB"] = "di & tri-bosons";
  m_Color["DBTB"] = 7050;
  //m_Color["DBTB"] = 8006;

  m_Title["ZDY"] = "Z / #gamma* + jets";
  m_Color["ZDY"] = 7000;
  //m_Color["ZDY"] = 8005;

  m_Title["Wjets"] = "W + jets";
  m_Color["Wjets"] = 7001;
  //m_Color["Wjets"] = 8001; 
  
  m_Title["Gjets"] = "#gamma + jets";
  m_Color["Gjets"] = 7051;
  //m_Color["Gjets"] = 8002; 

  m_Title["QCD"] = "QCD multijets";
  m_Color["QCD"] = 7023;
  //m_Color["QCD"] = 8007;

  m_Title["ZInv"] = "Z Inv";
  m_Color["ZInv"] = 7022;
  //m_Color["ZInv"] = 8005;
  
  m_Title["DY"] = "DY";
  m_Color["DY"] = 7021;
  //m_Color["DY"] = 8006;

  m_Title["Cascades_300_300_289_260_240_220_220_209_200_190_180"] = "Cascades 180";
  m_Color["Cascades_300_300_289_260_240_220_220_209_200_190_180"] = 7040; // 7072 might look better...?
  m_Title["Cascades_209_220_209_200_190_180"] = "Cascades 180";
  m_Color["Cascades_209_220_209_200_190_180"] = 7040; // 7072 might look better...?

  m_Title["Cascades_300_300_289_260_240_220"] = "Cascades 220";
  m_Color["Cascades_300_300_289_260_240_220"] = 7071;
  m_Title["Cascades_300_300_289_260_240_220"] = "Cascades 220";
  m_Color["Cascades_300_300_289_260_240_220"] = 7071;
  m_Title["Cascades_300_300_289_260_240_220_SMS"] = "Cascades 220 SMS";
  m_Color["Cascades_300_300_289_260_240_220_SMS"] = 7071;
  //m_Color["Cascades_300_300_289_260_240_220"] = 8007;

  m_Title["Cascades_300_300_289_280_270_260"] = "Cascades 260";
  m_Color["Cascades_300_300_289_280_270_260"] = 7041;
  m_Title["Cascades_289_300_289_280_270_260"] = "Cascades 260";
  m_Color["Cascades_289_300_289_280_270_260"] = 7041;
  m_Title["Cascades_300_300_289_280_270_260_SMS"] = "Cascades 260 SMS";
  m_Color["Cascades_300_300_289_280_270_260_SMS"] = 7041;
  //m_Color["Cascades_300_300_289_280_270_260"] = 8008;

  m_Title["Cascades_300_300_289_280_275_270"] = "Cascades 270";
  m_Color["Cascades_300_300_289_280_275_270"] = 7061;
  m_Title["Cascades_300_300_289_280_275_270"] = "Cascades 270";
  m_Color["Cascades_300_300_289_280_275_270"] = 7061;
  m_Title["Cascades_300_300_289_280_275_270_SMS"] = "Cascades 270 SMS";
  m_Color["Cascades_300_300_289_280_275_270_SMS"] = 7061;
  //m_Color["Cascades_300_300_289_280_275_270"] = 8009;

  m_Title["SMS_TChiWZ_Sandwich_SMS_300_290"] = "#tilde{#chi}^{0}_{2} #tilde{#chi}^{#pm}_{1} 300, 295, 290";
  m_Color["SMS_TChiWZ_Sandwich_SMS_300_290"] = 7043;
  m_Title["SMS_TChiWZ_SMS_300_290"] = "#tilde{#chi}^{0}_{2} #tilde{#chi}^{#pm}_{1} 300, 300, 290";
  m_Color["SMS_TChiWZ_SMS_300_290"] = 7072;

  m_Title["T1bbbb_1500_SMS"] = "T1bbbb 1500";
  m_Color["T1bbbb_1500_SMS"] = 7071;
  m_Title["T1bbbb_1752_SMS"] = "T1bbbb 1752";
  m_Color["T1bbbb_1752_SMS"] = 7041;
  m_Title["T1bbbb_1900_SMS"] = "T1bbbb 1900";
  m_Color["T1bbbb_1900_SMS"] = 7061;

  m_Title["HF_Fakes"] = "HF leptons";
  m_Color["HF_Fakes"] = 7022;
  //m_Color["HF_Fakes"] = 8008;

  m_Title["LF_Fakes"] = "LF/fake leptons";
  m_Color["LF_Fakes"] = 7021;
  //m_Color["LF_Fakes"] = 8009;
  
  m_Title["Fakes"] = "fake leptons";
  m_Color["Fakes"] = 7021;
  //m_Color["Fakes"] = 8010;

  m_Title["HF"] = "heavy flavor";
  m_Color["HF"] = 7022;

  m_Title["LF"] = "light flavor";
  m_Color["LF"] = 7021;

  m_Title["ttbar_Fakes"] = "t #bar{t} fakes";
  m_Color["ttbar_Fakes"] = 7020;

  m_Title["Wjets_Fakes"] = "W+jets fakes";
  m_Color["Wjets_Fakes"] = 7023;

  m_Title["ST_Fakes"] = "single top fakes";
  m_Color["ST_Fakes"] = 7024;

  m_Title["DB_Fakes"] = "di-boson fakes";
  m_Color["DB_Fakes"] = 7012;

  m_Title["TB_Fakes"] = "tri-boson fakes";
  m_Color["TB_Fakes"] = 7013;

  m_Title["ZDY_Fakes"] = "Z / #gamma* + jets fakes";
  m_Color["ZDY_Fakes"] = 7014;

  m_Title["ttbar_all"] = "t #bar{t} + jets";
  m_Color["ttbar_all"] = 7011;

  m_Title["ST_all"] = "single top";
  m_Color["ST_all"] = 7010;

  m_Title["DB_all"] = "di-bosons";
  m_Color["DB_all"] = 7051;

  m_Title["TB_all"] = "tri-bosons";
  m_Color["TB_all"] = 7050;

  m_Title["ZDY_all"] = "Z / #gamma* + jets";
  m_Color["ZDY_all"] = 7000;

  m_Title["Wjets_all"] = "W + jets";
  m_Color["Wjets_all"] = 7001;

  m_Title["Total"] = "total background";
  m_Color["Total"] = 7000;

}

struct HistId {
    string bin; string proc; string var; 
    bool operator<(const HistId& other) const {
        return std::tie(bin, proc, var) < std::tie(other.bin, other.proc, other.var);
    }
};

HistId ParseHistName(const std::string &name) {
    std::string s = name;
    // strip ";..." suffix
    size_t sem = s.find(';');
    if (sem != std::string::npos)
        s = s.substr(0, sem);
    // strip leading "can_" or "c_"
    if (s.rfind("can_", 0) == 0) s = s.substr(4);
    else if (s.rfind("c_", 0) == 0) s = s.substr(2);
    HistId out{"", "", ""};
    // split by "__"
    size_t first = s.find("__");
    if (first == std::string::npos) {
        out.var = s;
        return out;
    }
    size_t second = s.find("__", first + 2);
    if (second == std::string::npos) {
        // only one "__" -> treat as bin + var
        out.bin = s.substr(0, first);
        out.var = s.substr(first + 2);
        return out;
    }
    out.bin  = s.substr(0, first);
    out.proc = s.substr(first + 2, second - (first + 2));
    out.var  = s.substr(second + 2); 
    return out;
}

// Return just the bin name from the hist title
std::string ExtractBinName(const std::string &histName) {
    HistId id = ParseHistName(histName);
    return id.bin;
}

// Return just the process name from the hist title
std::string ExtractProcName(const std::string &histName) {
    HistId id = ParseHistName(histName);
    return id.proc;
}

// Return just the variable name from the hist title
std::string ExtractVarName(const std::string &histName) {
    HistId id = ParseHistName(histName);
    return id.var;
}

// Return true if the histogram belongs to a signal sample
bool IsSignalHist(const std::string &histName, const SampleTool &tool) {
    HistId id = ParseHistName(histName);
    for (const auto &sigKey : tool.SignalKeys) {
        if (id.proc.find(sigKey) != std::string::npos) {
            return true;
        }
    }
    return false;
}

// Return true if the histogram belongs to a background sample
bool IsBkgHist(const std::string &histName, const SampleTool &tool) {
    HistId id = ParseHistName(histName);
    return tool.BkgDict.count(id.proc) > 0;
}

template<typename T>
T* GetHistClone(TFile *f, const string &name) {
    T* h = dynamic_cast<T*>(f->Get(name.c_str()));
    if (!h) return nullptr;
    T* clone = dynamic_cast<T*>(h->Clone());
    clone->SetDirectory(nullptr);
    return clone;
}

// --------------------------------------------------
// Sort background histograms and process names by total yield (descending)
// --------------------------------------------------
static void SortByYield(
    std::vector<TH1*>& Hists,
    std::vector<std::string>& Procs)
{
    if(Hists.empty() || Hists.size() != Procs.size()) return;

    // Create vector of pairs {integral, index}
    std::vector<std::pair<double,int>> yields_idx;
    for(size_t i=0;i<Hists.size();++i)
        yields_idx.push_back({Hists[i]->Integral(), (int)i});

    // Sort descending
    std::sort(yields_idx.rbegin(), yields_idx.rend());

    // Reorder histograms and process names
    std::vector<TH1*> sortedHists;
    std::vector<std::string> sortedProcs;
    for(auto &p : yields_idx){
        sortedHists.push_back(Hists[p.second]);
        sortedProcs.push_back(Procs[p.second]);
    }

    Hists.swap(sortedHists);
    Procs.swap(sortedProcs);
}

// --------------------------------------------------
// Sort cutflow histograms and process names by last-bin yield (descending)
// --------------------------------------------------
static void SortCutFlowsByLastBin(
    std::vector<TH1*>& cutflowHists,
    std::vector<std::string>& cutflowProcs)
{
    if(cutflowHists.empty() || cutflowHists.size() != cutflowProcs.size()) return;

    // Create vector of pairs {last-bin content, index}
    std::vector<std::pair<double,int>> yields_idx;
    for(size_t i=0;i<cutflowHists.size();++i){
        int lastBin = cutflowHists[i]->GetNbinsX();
        yields_idx.push_back({cutflowHists[i]->GetBinContent(lastBin), (int)i});
    }

    // Sort descending
    std::sort(yields_idx.rbegin(), yields_idx.rend());

    // Reorder histograms and process names
    std::vector<TH1*> sortedHists;
    std::vector<std::string> sortedProcs;
    for(auto &p : yields_idx){
        sortedHists.push_back(cutflowHists[p.second]);
        sortedProcs.push_back(cutflowProcs[p.second]);
    }

    cutflowHists.swap(sortedHists);
    cutflowProcs.swap(sortedProcs);
}

// ------------------ TH1 ------------------
void DrawLog(TH1* h, const char* opt = "",
             double fallbackMin = 1e-3, double rangeFactor = 1.1) {
    if (!h) return;
    double max = h->GetMaximum();
    if (max <= 0) {
        // std::cerr << "TH1: " << h->GetName() << " has no positive entries\n";
        h->Draw(opt);
        return;
    }
    if (h->GetMinimum() <= 0) h->SetMinimum(fallbackMin);
    h->Draw(opt);
    gPad->SetLogy(1);
    h->SetMaximum(max * rangeFactor);
    gPad->Update();
}

// ------------------ TH2 ------------------
void DrawLog(TH2* h, const char* opt = "",
             double fallbackMin = 1e-3, double rangeFactor = 1.1) {
    if (!h) return;
    double max = h->GetMaximum();
    if (max <= 0) {
        // std::cerr << "TH2: " << h->GetName() << " has no positive entries\n";
        h->Draw(opt);
        return;
    }
    if (h->GetMinimum() <= 0) h->SetMinimum(fallbackMin);
    h->Draw(opt);
    gPad->SetLogz(1);
    h->SetMaximum(max * rangeFactor);
    gPad->Update();
}

// ------------------ TEfficiency ------------------
void DrawLog(TEfficiency* e, const char* opt = "",
             double fallbackMin = 1e-3, double rangeFactor = 1.1) {
    if (!e) return;
    double ymin = 1e30, ymax = -1e30;
    for (int i = 1; i <= e->GetTotalHistogram()->GetNbinsX(); ++i) {
        double val = e->GetEfficiency(i);
        if (val > 0) {
            ymin = std::min(ymin, val);
            ymax = std::max(ymax, val);
        }
    }
    if (ymax <= 0 || ymin <= 0) {
        // std::cerr << "TEfficiency: " << e->GetName() << " has no positive values\n";
        e->Draw(opt);
        return;
    }
    e->Draw(opt);
    gPad->SetLogy(1);
    gPad->Update();
    e->GetPaintedGraph()->GetYaxis()->SetRangeUser(
        std::max(ymin / rangeFactor, fallbackMin), ymax * rangeFactor);
}

// ------------------ TGraph / TGraphAsymmErrors ------------------
void DrawLog(TGraph* g, const char* opt = "AP",
             double fallbackMin = 1e-3, double rangeFactor = 1.1) {
    if (!g) return;
    double ymin = 1e30, ymax = -1e30;
    for (int i = 0; i < g->GetN(); ++i) {
        double x, y;
        g->GetPoint(i, x, y);
        if (y > 0) {
            ymin = std::min(ymin, y);
            ymax = std::max(ymax, y);
        }
    }
    if (ymax <= 0 || ymin <= 0 || ymin > ymax) {
        // std::cerr << "TGraph: " << g->GetName() << " has no positive points\n";
        g->Draw(opt);
        return;
    }
    g->Draw(opt);
    gPad->SetLogy(1);
    gPad->Update();
    g->GetYaxis()->SetRangeUser(
        std::max(ymin / rangeFactor, fallbackMin), ymax * rangeFactor);
}

// ------------------ Smart Dispatcher ------------------
template <typename T>
void DrawLogSmart(T* obj, const char* opt = "",
                  double fallbackMin = 1e-3, double rangeFactor = 1.1) {
    DrawLog(obj, opt, fallbackMin, rangeFactor);
}
