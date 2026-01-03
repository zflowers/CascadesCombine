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
#include <limits>
#include <cmath>
#include <fstream>
#include <regex>
#include <fnmatch.h>
#include <unordered_set>

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
#include <TPaveText.h>
#include <TDirectory.h>
#include <TInterpreter.h>

#include "SampleTool.h"

using namespace std;

// globals

TFile* outFile = nullptr;
int lumi = 1;
string outputDir = "plots/";
map<string,string> m_Title;
map<string,int>    m_Color;
SampleTool tool;

double hlo = 0.09;
double hhi = 0.22;
double hbo = 0.15;
double hto = 0.07;

// ----------------------
// Helpers
// ----------------------

std::string SanitizeString(const std::string& name) {
    std::string out;
    for (char c : name) {
        if (std::isalnum(c) || c=='_' || c=='-') out += c;
        //else out += '_';
    }
    return out;
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

// helper: get last regular bin content (not overflow)
double LastBinContent(TH1* h){
    if(!h) return 0.0;
    int nb = h->GetNbinsX();
    return h->GetBinContent(nb);
}

// helper: get bin content at given 1-based bin index (safe)
double BinContentSafe(TH1* h, int bin){
    if(!h) return 0.0;
    int nb = h->GetNbinsX();
    if(bin < 1 || bin > nb) return 0.0;
    return h->GetBinContent(bin);
}

double CalculateZbi(double Nsig, double Nbkg, double deltaNbkg){
  if(deltaNbkg >= 1.) deltaNbkg /= 100; // assume user passed in percent rather than fraction
  double Nobs = Nsig+Nbkg;
  double tau = 1./Nbkg/(deltaNbkg*deltaNbkg);
  double aux = Nbkg*tau;
  double Pvalue = TMath::BetaIncomplete(1./(1.+tau),Nobs,aux+1.);
  double sigma = sqrt(2.)*TMath::ErfcInverse(Pvalue*2);

  return (isnan(sigma))?0:sigma;
}

void SortHistsByYield(std::vector<TH1*> &hists) {
    std::sort(hists.begin(), hists.end(),
              [](TH1* a, TH1* b) {
                  // Handle nulls (treat them as zero yield)
                  double ya = (a ? a->Integral() : 0.0);
                  double yb = (b ? b->Integral() : 0.0);
                  return ya > yb;   // descending
              });
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

std::vector<int> fallbackColors = {
    7000, 7010, 7020, 7030, 7040, 7050, 7060, 7070, // "0" group
    7001, 7011, 7021, 7031, 7041, 7051, 7061, 7071, // "1" group
    7002, 7012, 7022, 7032, 7042, 7052, 7062, 7072, // "2" group
    7003, 7013, 7023, 7033, 7043, 7053, 7063, 7073, // "3" group
    7004, 7014, 7024, 7034, 7044, 7054, 7064, 7074  // "4" group
};
static size_t fallbackIndex = 0;

std::string makeSMSChiTitle(const std::string& key) {
    int color = kBlack;
    auto it = m_Color.find(key);
    if (it == m_Color.end()) {
        // Fallback to rotating palette
        color = fallbackColors[fallbackIndex % fallbackColors.size()];
        fallbackIndex++;
        m_Color[it->first] = color;
    }
    // Determine model type
    std::string model;
    if (key.find("TChiWZ") != std::string::npos) model = "TChiWZ";
    else if (key.find("TChiHZ") != std::string::npos) model = "TChiHZ";
    else if (key.find("TChipmWW") != std::string::npos) model = "TChipmWW";
    else model = "Unknown";

    // Determine particle text for the model
    std::string particles;
    if (model == "TChiWZ" || model == "TChiHZ")
        particles = "#tilde{#chi}^{0}_{2} #tilde{#chi}^{#pm}_{1}";
    else if (model == "TChipmWW")
        particles = "#tilde{#chi}^{#pm}_{1} #tilde{#chi}^{#mp}_{1}";
    else
        particles = "#tilde{#chi} #tilde{#chi}"; // fallback

    // Extract mass numbers from key
    int m1 = 0, m3 = 0;
    size_t lastUnderscore = key.find_last_of('_');
    size_t secondLastUnderscore = key.find_last_of('_', lastUnderscore - 1);
    if (secondLastUnderscore == std::string::npos || lastUnderscore == std::string::npos)
        return "Invalid";

    try {
        m1 = std::stoi(key.substr(secondLastUnderscore + 1, lastUnderscore - secondLastUnderscore - 1));
        m3 = std::stoi(key.substr(lastUnderscore + 1));
    } catch (...) {
        return "Invalid";
    }

    // Handle Sandwich models
    bool isSandwich = key.find("Sandwich") != std::string::npos;
    int m2 = (isSandwich && model != "TChipmWW") ? (m1 + m3) / 2 : m1;

    // Compose final title
    std::ostringstream title;
    title << particles << " " << m1 << ", " << m2 << ", " << m3;
    return title.str();
}

void loadFormatMaps(){

  m_Title["data_obs"] = "Data";
  m_Color["data_obs"] = kBlack;

  m_Title["data"] = "Data";
  m_Color["data"] = kBlack;

  m_Title["ttbar"] = "t #bar{t} + X";
  m_Color["ttbar"] = 7011;
  //m_Color["ttbar"] = 8003;
  
  m_Title["top"] = "t + X";
  m_Color["top"] = 7011;
  //m_Color["top"] = 8003;

  m_Title["Vfakeleps"] = "fake enriched";
  m_Color["Vfakeleps"] = 7001;
  //m_Color["Vfakeleps"] = 8001;

  m_Title["boson"] = "boson";
  m_Color["boson"] = 7050;
  //m_Color["boson"] = 8006;

  m_Title["top_2018"] = "t + X";
  m_Color["top_2018"] = 7011;
  //m_Color["top_2018"] = 8003;

  m_Title["Vfakeleps_2018"] = "fake enriched";
  m_Color["Vfakeleps_2018"] = 7001;
  //m_Color["Vfakeleps_2018"] = 8001;

  m_Title["boson_2018"] = "boson";
  m_Color["boson_2018"] = 7050;
  //m_Color["boson_2018"] = 8006;

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

  m_Title["HF_FAKES"] = "HF leptons";
  m_Color["HF_FAKES"] = 7022;
  //m_Color["HF_FAKES"] = 8008;

  m_Title["LF_FAKES"] = "LF/fake leptons";
  m_Color["LF_FAKES"] = 7021;
  //m_Color["LF_FAKES"] = 8009;
  
  m_Title["FAKES"] = "fake leptons";
  m_Color["FAKES"] = 7021;
  //m_Color["FAKES"] = 8010;

  m_Title["HF"] = "heavy flavor";
  m_Color["HF"] = 7022;

  m_Title["LF"] = "light flavor";
  m_Color["LF"] = 7021;

  m_Title["ttbar_FAKES"] = "t #bar{t} fakes";
  m_Color["ttbar_FAKES"] = 7020;

  m_Title["top_FAKES"] = "t + X fakes";
  m_Color["top_FAKES"] = 7020;

  m_Title["V_FAKES"] = "Nonprompt";
  m_Color["V_FAKES"] = 7001;

  m_Title["boson_FAKES"] = "boson fakes";
  m_Color["boson_FAKES"] = 7012;

  m_Title["Wjets_FAKES"] = "W+jets fakes";
  m_Color["Wjets_FAKES"] = 7023;

  m_Title["ST_FAKES"] = "single top fakes";
  m_Color["ST_FAKES"] = 7024;

  m_Title["DB_FAKES"] = "di-boson fakes";
  m_Color["DB_FAKES"] = 7012;

  m_Title["TB_FAKES"] = "tri-boson fakes";
  m_Color["TB_FAKES"] = 7013;

  m_Title["ZDY_FAKES"] = "Z / #gamma* + jets fakes";
  m_Color["ZDY_FAKES"] = 7014;

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

  m_Title["Total"] = "Total Bkg";
  m_Color["Total"] = 7000;
  m_Title["Total Bkg"] = "Total Bkg";
  m_Color["Total Bkg"] = 7000;

  m_Title["Cascades_300_300_289_260_240_220_220_209_200_190_180"] = "Cascades 180";
  m_Color["Cascades_300_300_289_260_240_220_220_209_200_190_180"] = 7040; // 7072 might look better...?
  m_Title["Cascades_209_220_209_200_190_180"] = "Cascades 180";
  m_Color["Cascades_209_220_209_200_190_180"] = 7040; // 7072 might look better...?
  m_Title["Cascades_220_220_209_200_190_180"] = "Cascades 180";
  m_Color["Cascades_220_220_209_200_190_180"] = 7040; // 7072 might look better...?

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

  m_Title["SMS_Gluinos_SMS_1000_900"] = "T1qqqq 1000 900";
  m_Color["SMS_Gluinos_SMS_1000_900"] = 7071;
  m_Title["SMS_Gluinos_SMS_1100_1000"] = "T1qqqq 1100 1000";
  m_Color["SMS_Gluinos_SMS_1100_1000"] = 7041;
  m_Title["SMS_Gluinos_SMS_1200_1100"] = "T1qqqq 1200 1100";
  m_Color["SMS_Gluinos_SMS_1200_1100"] = 7061;
  m_Title["SMS_Gluinos_SMS_1200_1176"] = "T1qqqq 1200 1175";
  m_Color["SMS_Gluinos_SMS_1200_1176"] = 7071;
  m_Title["SMS_Gluinos_SMS_2000_1900"] = "T1qqqq 2000 1900";
  m_Color["SMS_Gluinos_SMS_2000_1900"] = 7041;
  m_Title["SMS_Gluinos_SMS_1500_1400"] = "T1qqqq 1500 1400";
  m_Color["SMS_Gluinos_SMS_1500_1400"] = 7041;

}

TH1D* TGraphToTH1(TGraphAsymmErrors* g, const std::string& name) {
    if (!g) return nullptr;
    int nbins = g->GetN();
    double xmin = g->GetX()[0] - g->GetEXlow()[0];
    double xmax = g->GetX()[nbins-1] + g->GetEXhigh()[nbins-1];

    TH1D* h = new TH1D(name.c_str(), name.c_str(), nbins, xmin, xmax);
    for (int i=0; i<nbins; ++i) {
        h->SetBinContent(i+1, g->GetY()[i]);
        h->SetBinError(i+1, 0.5*(g->GetEYlow()[i]+g->GetEYhigh()[i]));
    }
    return h;
}

struct MergedBinGroup {
    std::string group_name;
    std::vector<std::string> bin_names;
};

struct YamlBinPattern {
    std::string name;
    std::vector<std::string> include;
    std::vector<std::string> exclude;
};

struct YamlProcessPattern {
    std::string name;
    std::vector<std::string> include;
    std::vector<std::string> exclude;
};

struct YamlConfig {
    std::vector<YamlBinPattern> bins;
    std::vector<YamlProcessPattern> process_merges;
};

YamlConfig LoadYamlConfig(const std::string& yamlFile) {

    YamlConfig cfg;
    if (yamlFile.empty()) return cfg;

    YAML::Node root;
    try {
        root = YAML::LoadFile(yamlFile);
    }
    catch (const std::exception& e) {
        std::cerr << "[ERROR] Failed to parse YAML file: " << yamlFile << "\n"
                  << "Exception: " << e.what() << "\n";
        return cfg;
    }

    // -------------------------
    // Parse bins
    // -------------------------
    if (root["bins"]) {
        for (const auto& n : root["bins"]) {
            YamlBinPattern b;
            b.name = n["name"] ? n["name"].as<std::string>() : "";

            // Include: scalar or sequence
            if (n["include"]) {
                if (n["include"].IsSequence()) {
                    for (const auto& inc : n["include"])
                        b.include.push_back(inc.as<std::string>());
                }
                else if (n["include"].IsScalar()) {
                    b.include.push_back(n["include"].as<std::string>());
                }
            }
            else {
                b.include.push_back("*");
            }

            // Exclude: scalar or sequence
            if (n["exclude"]) {
                if (n["exclude"].IsSequence()) {
                    for (const auto& ex : n["exclude"])
                        b.exclude.push_back(ex.as<std::string>());
                }
                else if (n["exclude"].IsScalar()) {
                    b.exclude.push_back(n["exclude"].as<std::string>());
                }
            }

            cfg.bins.push_back(std::move(b));
        }
    }

    // -------------------------
    // Parse process merges
    // -------------------------
    if (root["processes"] && root["processes"]["merge"]) {
        for (const auto& n : root["processes"]["merge"]) {
            YamlProcessPattern p;
            p.name = n["name"].as<std::string>();

            if (n["include"]) {
                if (n["include"].IsSequence()) {
                    for (const auto& inc : n["include"])
                        p.include.push_back(inc.as<std::string>());
                }
                else if (n["include"].IsScalar()) {
                    p.include.push_back(n["include"].as<std::string>());
                }
            }

            if (n["exclude"]) {
                if (n["exclude"].IsSequence()) {
                    for (const auto& ex : n["exclude"])
                        p.exclude.push_back(ex.as<std::string>());
                }
                else if (n["exclude"].IsScalar()) {
                    p.exclude.push_back(n["exclude"].as<std::string>());
                }
            }

            cfg.process_merges.push_back(std::move(p));
        }
    }

    return cfg;
}

static std::string WildcardToRegex(const std::string& pat)
{
    std::string r;
    r.reserve(pat.size() * 2 + 4);
    for (char c : pat) {
        if (c == '*')      r += ".*";
        else if (c == '?') r += ".";
        else if (std::isalnum(static_cast<unsigned char>(c))) r.push_back(c);
        else {
            // escape any other character for regex safety
            r.push_back('\\');
            r.push_back(c);
        }
    }
    return r;
}


std::vector<MergedBinGroup>
BuildMergedBinGroupsFromYaml(const std::vector<std::string>& allBins,
                             const YamlConfig& cfg)
{
    std::vector<MergedBinGroup> out;
    for (const auto& b : cfg.bins) {
        MergedBinGroup g;
        g.group_name = b.name;
        for (const auto& bin : allBins) {
            bool included = false;
            for (const auto& inc : b.include) {
                std::regex incRegex(WildcardToRegex(inc));
                if (std::regex_match(bin, incRegex)) {
                    included = true;
                    break;
                }
            }
            if (!included) continue;
            bool excluded = false;
            for (const auto& ex : b.exclude) {
                std::regex exRegex(WildcardToRegex(ex));
                if (std::regex_search(bin, exRegex)) {
                    excluded = true;
                    break;
                }
            }
            if (!excluded)
                g.bin_names.push_back(bin);
        }
        if (!g.bin_names.empty())
            out.push_back(g);
    }
    return out;
}

struct CombinedBinHists {
    map<string, unique_ptr<TH1>> bkg;      // backgrounds
    map<string, unique_ptr<TH1>> signal;   // signals
    map<string, unique_ptr<TH1>> data;     // data
};

struct BinContent { double content; double error; };

template <typename T>
void ApplyMergingRules(
    std::map<std::string, std::vector<T>>& contents,
    const std::vector<YamlProcessPattern>& rules)
{
    if (contents.empty()) return;

    std::map<std::string, std::vector<T>> result;
    std::unordered_set<std::string> consumed;

    // Loop over YAML-defined merge rules
    for (const auto& rule : rules) {

        const std::string& target = rule.name;
        const auto& includes       = rule.include;
        const auto& excludes       = rule.exclude;

        std::vector<std::string> matches;

        // Find matching process keys
        for (const auto& kv : contents) {
            const std::string& key = kv.first;
            if (consumed.count(key)) continue;

            bool match = false;

            // include patterns
            for (const auto& inc : includes) {
                if (fnmatch(inc.c_str(), key.c_str(), 0) == 0) {
                    match = true;
                    break;
                }
            }
            if (!match) continue;

            // exclude patterns
            for (const auto& ex : excludes) {
                if (fnmatch(ex.c_str(), key.c_str(), 0) == 0) {
                    match = false;
                    break;
                }
            }

            if (match)
                matches.push_back(key);
        }

        if (matches.empty()) continue;

        // Consistency check
        size_t n = contents.at(matches.front()).size();
        for (const auto& m : matches) {
            if (contents.at(m).size() != n) {
                std::cerr << "[ApplyMergingRules] Inconsistent bin sizes for '"
                          << target << "'\n";
                matches.clear();
                break;
            }
        }
        if (matches.empty()) continue;

        // Build merged vector
        std::vector<T> mergedVals(n);
        for (size_t i = 0; i < n; ++i) {
            mergedVals[i].content = 0.0;
            mergedVals[i].error   = 0.0;
        }

        // Combine content + errors
        for (const auto& m : matches) {
            const auto& vec = contents.at(m);
            for (size_t i = 0; i < n; ++i) {
                mergedVals[i].content += vec[i].content;
                mergedVals[i].error    = std::hypot(mergedVals[i].error, vec[i].error);
            }
        }

        // Save and mark consumed
        result[target] = std::move(mergedVals);
        for (const auto& m : matches) consumed.insert(m);
    }

    // Copy unmerged keys
    for (const auto& kv : contents) {
        if (!consumed.count(kv.first))
            result[kv.first] = kv.second;
    }

    contents.swap(result);
}

CombinedBinHists LoadAndCombineBinHists(TDirectory* treeDir,
                                        const std::string& binTag,
                                        const std::vector<std::string>& binsToCombine,
                                        const std::vector<YamlProcessPattern>& processMerges
){
    CombinedBinHists out;

    if (!treeDir) {
        std::cerr << "[ERROR] Invalid TDirectory pointer passed to LoadAndCombineBinHists\n";
        return out;
    }

    auto MakeHistName = [](const std::string& bin, const std::string& proc) { return SanitizeString(bin)+"__"+proc+"__FD"; };

    // Map from process name to vector of bin contents/errors
    std::map<std::string, std::vector<BinContent>> bkgContents;
    std::map<std::string, std::vector<BinContent>> sigContents;
    std::map<std::string, std::vector<BinContent>> dataContents;
    std::vector<std::string> finalBinLabels;

    for (size_t binIndex = 0; binIndex < binsToCombine.size(); ++binIndex) {
        const auto& bin = binsToCombine[binIndex];
        TDirectory* binDir = dynamic_cast<TDirectory*>(treeDir->Get(bin.c_str()));
        if (!binDir) {
            std::cerr << "[warning] Bin directory '" << bin << "' not found, skipping.\n";
            continue;
        }
        finalBinLabels.push_back(bin);
    
        TIter next(binDir->GetListOfKeys());
        TKey* key;
        while ((key = (TKey*)next())) {
            TObject* obj = key->ReadObj();
            if (!obj) continue;
    
            std::string procName = obj->GetName();
    
            // Handle data_obs TGraphAsymmErrors
            if (obj->InheritsFrom(TGraphAsymmErrors::Class())) {
                auto g = dynamic_cast<TGraphAsymmErrors*>(obj);
                if (!g) continue;
    
                double y = g->GetY()[0];
                double e = 0.5*(g->GetEYlow()[0] + g->GetEYhigh()[0]);
    
                auto& vec = dataContents[procName];
                if (vec.size() < binIndex + 1) vec.resize(binIndex + 1, {0.0,0.0});
                vec[binIndex] = {y, e};
                continue;
            }
    
            // Only process TH1 objects
            if (!obj->InheritsFrom(TH1::Class())) continue;
            TH1* h = dynamic_cast<TH1*>(obj);
            if (!h) continue;
    
            // Skip totals/covariance
            if (procName.find("total") != std::string::npos) continue;
    
            bool isSignal = (procName.find("SMS") != std::string::npos || procName.find("Cascade") != std::string::npos);
    
            auto& targetContents = isSignal ? sigContents : bkgContents;
            double content = h->GetBinContent(1);
            double error = h->GetBinError(1);
    
            auto& vec = targetContents[procName];
            if (vec.size() < binIndex + 1) vec.resize(binIndex + 1, {0.0,0.0});
            vec[binIndex] = {content, error};
        }
        // for any procs that did not appear, pad with hist with 0 entries
        auto PadToBin = [](auto& contentsMap, size_t binIndex) {
            for (auto& kv : contentsMap) {
                auto& vec = kv.second;
                if (vec.size() < binIndex + 1)
                    vec.resize(binIndex + 1, {0.0, 0.0});
            }
        };
        PadToBin(bkgContents, binIndex);
        PadToBin(sigContents, binIndex);
        PadToBin(dataContents, binIndex);
    }

    ApplyMergingRules(bkgContents, processMerges);

    // Now build concatenated histograms with bin labels
    auto BuildConcatHist = [&](const std::string& name,
                                const std::vector<BinContent>& vals,
                                const std::vector<std::string>& labels) -> std::unique_ptr<TH1>
    {
        int nbins = vals.size();
        TH1D* h = new TH1D(name.c_str(), name.c_str(), nbins, 0.5, nbins + 0.5);
    
        for (int i = 0; i < nbins; ++i) {
            h->SetBinContent(i + 1, vals[i].content);
            h->SetBinError(i + 1, vals[i].error);
            if (i < (int)labels.size())
                h->GetXaxis()->SetBinLabel(i + 1, labels[i].c_str());
        }
    
        h->SetDirectory(nullptr);
        return std::unique_ptr<TH1>(h);
    };

    for (auto& [proc, vals] : bkgContents) out.bkg[proc] = BuildConcatHist(MakeHistName(binTag, proc), vals, finalBinLabels);
    for (auto& [proc, vals] : sigContents) out.signal[proc] = BuildConcatHist(MakeHistName(binTag, proc), vals, finalBinLabels);
    for (auto& [proc, vals] : dataContents) out.data[proc] = BuildConcatHist(MakeHistName(binTag, proc), vals, finalBinLabels);
    return out;
}

std::map<std::string, std::map<std::string, std::map<std::string, TH1*>>>
BuildMergedJsonCutflow(
    const std::map<std::string, std::map<std::string, TH1*>> &cutflowMap,
    const std::vector<MergedBinGroup> &groups,
    const YamlConfig &cfg
){
    // return: groupName -> ( binName -> ( procName -> TH1* ) )
    std::map<std::string, std::map<std::string, std::map<std::string, TH1*>>> out;

    // Helper: merge processes inside a single bin according to YAML rules
    auto MergeProcessesInBin = [&](const std::map<std::string, BinContent> &binContents)
        -> std::map<std::string, BinContent>
    {
        std::map<std::string, BinContent> result;
        std::unordered_set<std::string> consumed;

        for (const auto &rule : cfg.process_merges) {
            const std::string &target = rule.name;
            const auto &includes = rule.include;
            const auto &excludes = rule.exclude;

            std::vector<std::string> matches;
            for (const auto &kv : binContents) {
                const std::string &proc = kv.first;
                if (consumed.count(proc)) continue;

                bool ok = false;
                // include patterns
                for (const auto &inc : includes) {
                    if (fnmatch(inc.c_str(), proc.c_str(), 0) == 0) { ok = true; break; }
                }
                if (!ok) continue;

                // exclude patterns
                for (const auto &ex : excludes) {
                    if (fnmatch(ex.c_str(), proc.c_str(), 0) == 0) { ok = false; break; }
                }
                if (ok) matches.push_back(proc);
            }

            if (matches.empty()) continue;

            BinContent merged{0.0, 0.0};
            for (const auto &m : matches) {
                const auto &bc = binContents.at(m);
                merged.content += bc.content;
                merged.error = std::hypot(merged.error, bc.error);
            }
            result[target] = merged;
            for (const auto &m : matches) consumed.insert(m);
        }

        // copy any unmerged procs
        for (const auto &kv : binContents) {
            if (!consumed.count(kv.first)) result[kv.first] = kv.second;
        }

        return result;
    };

    // Loop over groups
    for (const auto &grp : groups) {
        const std::string groupName = grp.group_name;
        const auto &bins = grp.bin_names;

        // 1) Collect only existing bins (preserve order), warn about missing ones
        std::vector<std::string> existingBins;
        existingBins.reserve(bins.size());
        for (const auto &b : bins) {
            auto it = cutflowMap.find(b);
            if (it != cutflowMap.end()) existingBins.push_back(b);
            else {
                std::cerr << "[BuildMergedJsonCutflow] Warning: bin '" << b
                          << "' listed in group '" << groupName << "' not found in input; skipping\n";
            }
        }
        if (existingBins.empty()) {
            std::cerr << "[BuildMergedJsonCutflow] Warning: no bins found for group '" << groupName << "'; skipping\n";
            continue;
        }

        // 2) For each bin, compute merged per-bin yields (proc -> BinContent)
        //    and also collect the union of all merged proc names across the group.
        std::map<std::string, std::map<std::string, BinContent>> perBinMerged; // binName -> (proc -> BinContent)
        std::unordered_set<std::string> allMergedProcs;

        for (const auto &binName : existingBins) {
            std::map<std::string, BinContent> binContents;

            // read raw per-process values from cutflowMap[binName]
            const auto &procMap = cutflowMap.at(binName);
            for (const auto &kv : procMap) {
                const std::string proc = kv.first;
                TH1* h = kv.second;
                double c = 0.0, e = 0.0;
                if (h) {
                    // each input TH1 is 1-bin with yield in bin 1
                    c = h->GetBinContent(1);
                    e = h->GetBinError(1);
                }
                binContents[proc] = {c, e};
            }

            // Merge processes inside this bin only
            auto mergedBin = MergeProcessesInBin(binContents);

            // Save and record process names
            for (const auto &kv : mergedBin) {
                perBinMerged[binName][kv.first] = kv.second;
                allMergedProcs.insert(kv.first);
            }
        }

        // 3) Build groupCutflowMap: for each bin and for every proc in allMergedProcs,
        //    produce a single-bin TH1* (0 if proc missing in that bin).
        std::map<std::string, std::map<std::string, TH1*>> groupCutflowMap; // binName -> (proc -> TH1*)

        for (const auto &binName : existingBins) {
            for (const auto &proc : allMergedProcs) {
                double c = 0.0, e = 0.0;
                auto itb = perBinMerged.find(binName);
                if (itb != perBinMerged.end()) {
                    auto itp = itb->second.find(proc);
                    if (itp != itb->second.end()) {
                        c = itp->second.content;
                        e = itp->second.error;
                    }
                }
                std::string histName = Form("%s__%s__%s", groupName.c_str(), binName.c_str(), proc.c_str());
                TH1F* h = new TH1F(histName.c_str(), histName.c_str(), 1, 0, 1);
                h->SetBinContent(1, c);
                h->SetBinError(1, e);
                h->SetDirectory(nullptr);
                groupCutflowMap[binName][proc] = h;
            }
        }

        // 4) store into out
        out[groupName] = std::move(groupCutflowMap);
    } // end groups loop

    return out;
}

struct StackPlotInput {
    std::vector<TH1*> bkgHists;
    std::vector<TH1*> sigHists;
    TH1* dataHist = nullptr;
};

StackPlotInput ConvertToStackInput(const CombinedBinHists& mergedHists) {
    StackPlotInput out;

    // Background
    for (const auto& [name, hptr] : mergedHists.bkg)
        if (hptr) out.bkgHists.push_back(hptr.get());

    // Signal
    for (const auto& [name, hptr] : mergedHists.signal)
        if (hptr) out.sigHists.push_back(hptr.get());

    // Data
    if (!mergedHists.data.empty()) {
        auto it = mergedHists.data.begin();
        if (it->second) {
            out.dataHist = it->second.get();
        }
        out.sigHists.clear(); // remove signal if data exists to prevent accidental unblinding
    }
    return out;
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

bool HistsCompatible(TH1* num, const TH1* den, double tol=1e-1) {
    if (!num || !den) return false;
    int nbins = num->GetNbinsX();
    if (nbins != den->GetNbinsX()) return false;
    // Check bin edges match exactly
    for (int i = 1; i <= nbins; ++i) {
        if (num->GetBinLowEdge(i) != den->GetBinLowEdge(i)) return false;
        if (num->GetBinWidth(i) != den->GetBinWidth(i)) return false;
    }
    bool hasNonZeroBin = false;
    for (int i = 1; i <= nbins; ++i) {
        double n = num->GetBinContent(i);
        double d = den->GetBinContent(i);
        // Catch invalid numbers
        if (!std::isfinite(n) || !std::isfinite(d)) return false;
        // TEff requires numerator <= denominator
        // Allow small tolerance and clamp numerator
        if (n > d) {
            if ((n - d) > tol) {
                // Too large an overshoot -> incompatible
                std::cerr << "[FAIL] bin " << i << " num=" << n << " den=" << d << std::endl;
                return false;
            }
            // Otherwise, clamp numerator to denominator
            num->SetBinContent(i, d);
        }
        if (d > 0) hasNonZeroBin = true;
    }
    // If all denominator bins are zero, skip
    return hasNonZeroBin;
}

// --------------------------------------------------
// Sort background histograms and process names by total yield (descending)
// --------------------------------------------------
void SortByYield(
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
void SortCutFlowsByLastBin(
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
             double fallbackMin = 1e-1, double rangeFactor = 1.2) {
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
             double fallbackMin = 1e-1, double rangeFactor = 1.2) {
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
             double fallbackMin = 1e-1, double rangeFactor = 1.2) {
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
             double fallbackMin = 1e-1, double rangeFactor = 1.2) {
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
                  double fallbackMin = 1e-1, double rangeFactor = 1.2) {
    DrawLog(obj, opt, fallbackMin, rangeFactor);
}
