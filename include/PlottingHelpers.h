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
#include <TGraphAsymmErrors.h>

#include "SampleTool.h"

using namespace std;

// global out root file pointer
TFile* outFile = nullptr;
int lumi = 1;
string outputDir = "plots/";
map<string,string>          m_Title;
map<string,int>             m_Color;

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
  
  m_Title["QCD"] = "QCD multijets";
  m_Color["QCD"] = 7023;
  //m_Color["QCD"] = 8007;

  m_Title["ZInv"] = "Z Inv";
  m_Color["ZInv"] = 7022;
  //m_Color["ZInv"] = 8005;
  
  m_Title["DY"] = "DY";
  m_Color["DY"] = 7021;
  //m_Color["DY"] = 8006;
  
  m_Title["Cascades_180"] = "Cascades 180";
  m_Color["Cascades_180"] = 7040; // 7072 might look better...?
  m_Title["Cascades_220"] = "Cascades 220";
  m_Color["Cascades_220"] = 7071;
  m_Title["Cascades_220_SMS"] = "Cascades 220 SMS";
  m_Color["Cascades_220_SMS"] = 7071;
  //m_Color["Cascades_220"] = 8007;
  m_Title["Cascades_260"] = "Cascades 260";
  m_Color["Cascades_260"] = 7041;
  m_Title["Cascades_260_SMS"] = "Cascades 260 SMS";
  m_Color["Cascades_260_SMS"] = 7041;
  //m_Color["Cascades_260"] = 8008;
  m_Title["Cascades_270"] = "Cascades 270";
  m_Color["Cascades_270"] = 7061;
  m_Title["Cascades_270_SMS"] = "Cascades 270 SMS";
  m_Color["Cascades_270_SMS"] = 7061;
  //m_Color["Cascades_270"] = 8009;

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

struct HistId { string bin; string proc; string var; };

HistId ParseHistName(const string &name) {
    string s = name;
    size_t sem = s.find(';');
    if (sem != string::npos) s = s.substr(0, sem);
    if (s.rfind("can_", 0) == 0) s = s.substr(4);
    if (s.rfind("c_", 0) == 0) s = s.substr(2);
    HistId out{"", "", ""};
    size_t first = s.find("__");
    if (first == string::npos) { out.var = s; return out; }
    size_t second = s.find("__", first + 2);
    if (second == string::npos) { out.bin = s.substr(0, first); out.var = s.substr(first + 2); return out; }
    out.bin = s.substr(0, first);
    out.proc = s.substr(first + 2, second - (first + 2));
    out.var = s.substr(second + 2);
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
static void SortBackgroundsByYield(
    std::vector<TH1*>& bkgHists,
    std::vector<std::string>& bkgProcs)
{
    if(bkgHists.empty() || bkgHists.size() != bkgProcs.size()) return;

    // Create vector of pairs {integral, index}
    std::vector<std::pair<double,int>> yields_idx;
    for(size_t i=0;i<bkgHists.size();++i)
        yields_idx.push_back({bkgHists[i]->Integral(), (int)i});

    // Sort descending
    std::sort(yields_idx.rbegin(), yields_idx.rend());

    // Reorder histograms and process names
    std::vector<TH1*> sortedHists;
    std::vector<std::string> sortedProcs;
    for(auto &p : yields_idx){
        sortedHists.push_back(bkgHists[p.second]);
        sortedProcs.push_back(bkgProcs[p.second]);
    }

    bkgHists.swap(sortedHists);
    bkgProcs.swap(sortedProcs);
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

