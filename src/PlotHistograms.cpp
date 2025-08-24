// PlotHistograms.cpp
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

#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <cstdio>
#include <set>

#include "SampleTool.h"

using namespace std;

// global out root file pointer
TFile* outFile = nullptr;
int Lumi = 1;
string outputDir = "plots/";

// ----------------------
// Helpers
// ----------------------
void copyConfigsToOutput(const std::string &outputDir,
                         const std::string &histCfg,
                         const std::string &datasetCfg,
                         const std::string &binsCfg) 
{
    if (gSystem->AccessPathName(outputDir.c_str())) {
        gSystem->mkdir(outputDir.c_str(), true); 
    }
    if (!histCfg.empty()) gSystem->CopyFile(histCfg.c_str(), (outputDir + "/" + histCfg).c_str(), true);
    if (!datasetCfg.empty()) gSystem->CopyFile(datasetCfg.c_str(), (outputDir + "/" + datasetCfg).c_str(), true);
    if (!binsCfg.empty()) gSystem->CopyFile(binsCfg.c_str(), (outputDir + "/" + binsCfg).c_str(), true);
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

int getColorForIndex(int i) {
    static int palette[] = {kAzure-3, kGreen+2, kOrange-3, kViolet-4, kCyan+2, kMagenta-9, kYellow+2, kBlue+1};
    int n = sizeof(palette)/sizeof(int);
    return palette[i % n];
}

string ExtractProcName(const string &histName) {
    size_t first = histName.find("__");
    if (first == string::npos) return "";
    size_t second = histName.find("__", first+2);
    if (second == string::npos) return "";
    return histName.substr(first+2, second - (first+2));
}

bool IsSignalHist(const string &histName, const SampleTool &tool) {
    string proc = ExtractProcName(histName);
    for (const auto &sigKey : tool.SignalKeys) if (proc.find(sigKey) != string::npos) return true;
    return false;
}

bool IsBkgHist(const string &histName, const SampleTool &tool) {
    string proc = ExtractProcName(histName);
    return tool.BkgDict.count(proc) > 0;
}

template<typename T>
T* GetHistClone(TFile *f, const string &name) {
    T* h = dynamic_cast<T*>(f->Get(name.c_str()));
    if (!h) return nullptr;
    T* clone = dynamic_cast<T*>(h->Clone());
    clone->SetDirectory(nullptr);
    return clone;
}

// ----------------------
// Plot 1D
// ----------------------
void Plot_Hist1D(TH1* h) {
    if (!h) return;
    string title = h->GetName();
    TCanvas* can = new TCanvas(("can_"+title).c_str(), ("can_"+title).c_str(), 700, 600);
    can->SetLeftMargin(0.15); can->SetRightMargin(0.18); can->SetBottomMargin(0.15);
    can->SetGridx(); can->SetGridy();
    h->Draw("HIST");
    h->GetXaxis()->CenterTitle();
    h->GetYaxis()->CenterTitle();
    h->GetYaxis()->SetTitle(("N_{events} / "+std::to_string(int(Lumi))+" fb^{-1}").c_str());
    h->GetYaxis()->SetRangeUser(0.0, 1.1*h->GetMaximum());
    TLatex l; l.SetTextFont(42); l.SetNDC();
    l.SetTextSize(0.035); l.DrawLatex(0.57,0.943,"sampleName");
    l.SetTextSize(0.04); l.DrawLatex(0.01,0.943,"#bf{CMS} Simulation Preliminary");
    l.SetTextSize(0.045); l.DrawLatex(0.7,0.04,"binName");
    TString pdfName = Form("%spdfs/%s.pdf", outputDir.c_str(), title.c_str());
    can->SaveAs(pdfName);
    if (outFile) { outFile->cd(); can->Write(0, TObject::kWriteDelete); }
    delete can;
}

// ----------------------
// Plot 2D
// ----------------------
void Plot_Hist2D(TH2* h) {
    if (!h) return;
    string title = h->GetName();
    TCanvas* can = new TCanvas(("can_"+title).c_str(), ("can_"+title).c_str(), 700, 600);
    can->SetLeftMargin(0.15); can->SetRightMargin(0.18); can->SetBottomMargin(0.15);
    can->SetGridx(); can->SetGridy(); can->SetLogz();
    h->Draw("COLZ");
    h->GetZaxis()->SetTitle(("N_{events} / "+std::to_string(int(Lumi))+" fb^{-1}").c_str());
    TLatex l; l.SetTextFont(42); l.SetNDC();
    l.SetTextSize(0.035); l.DrawLatex(0.65,0.943,"SampleName");
    l.SetTextSize(0.04); l.DrawLatex(0.01,0.943,"#bf{CMS} Simulation Preliminary");
    l.SetTextSize(0.045); l.DrawLatex(0.7,0.04,"binName");
    TString pdfName = Form("%spdfs/%s.pdf", outputDir.c_str(), title.c_str());
    can->SaveAs(pdfName);
    if (outFile) { outFile->cd(); can->Write(0, TObject::kWriteDelete); }
    delete can;
}

// ----------------------
// Plot Eff
// ----------------------
void Plot_Eff(TEfficiency* e){
  string title = e->GetName();
  TCanvas* can = (TCanvas*) new TCanvas(("can_eff_"+title).c_str(),("can_"+title).c_str(),700.,600);
  can->SetLeftMargin(0.15); can->SetRightMargin(0.18); can->SetBottomMargin(0.15);
  can->SetGridx(); can->SetGridy();
  can->Draw();
  can->cd();
  e->Draw("AP");
  gPad->Update();
  e->GetPaintedGraph()->GetXaxis()->CenterTitle();
  e->GetPaintedGraph()->GetXaxis()->SetTitleFont(42);
  e->GetPaintedGraph()->GetXaxis()->SetTitleSize(0.06);
  e->GetPaintedGraph()->GetXaxis()->SetTitleOffset(1.06);
  e->GetPaintedGraph()->GetXaxis()->SetLabelFont(42);
  e->GetPaintedGraph()->GetXaxis()->SetLabelSize(0.05);
  double xmin = e->GetTotalHistogram()->GetXaxis()->GetXmin();
  double xmax = e->GetTotalHistogram()->GetXaxis()->GetXmax();
  if(xmin < 0) xmin = xmin*1.1;
  else xmin = xmin*0.9;
  if(xmax > 0) xmax = xmax*1.1;
  else xmax = xmax*0.9;
  e->GetPaintedGraph()->GetXaxis()->SetRangeUser(xmin,xmax);
  //e->GetPaintedGraph()->GetXaxis()->SetTitle(g_Xname.c_str());
  e->GetPaintedGraph()->GetYaxis()->CenterTitle();
  e->GetPaintedGraph()->GetYaxis()->SetTitleFont(42);
  e->GetPaintedGraph()->GetYaxis()->SetTitleSize(0.06);
  e->GetPaintedGraph()->GetYaxis()->SetTitleOffset(1.12);
  e->GetPaintedGraph()->GetYaxis()->SetLabelFont(42);
  e->GetPaintedGraph()->GetYaxis()->SetLabelSize(0.05);
  e->GetPaintedGraph()->GetYaxis()->SetRangeUser(0.,1.05);
  //e->GetPaintedGraph()->GetYaxis()->SetRangeUser(0.9*h->GetMinimum(0.0),1.1*h->GetMaximum());

  TLatex l; l.SetTextFont(42); l.SetNDC();
  l.SetTextSize(0.035); l.DrawLatex(0.65,0.943,"SampleName");
  l.SetTextSize(0.04); l.DrawLatex(0.01,0.943,"#bf{CMS} Simulation Preliminary");
  l.SetTextSize(0.045); l.DrawLatex(0.7,0.04,"binName");
  TString pdfName = Form("%spdfs/%s.pdf", outputDir.c_str(), title.c_str());
  can->SaveAs(pdfName);
  if (outFile) { outFile->cd(); can->Write(0, TObject::kWriteDelete); }
  delete can;
}

// ----------------------
// Plot stack
// ----------------------
void Plot_Stack(const string& hname,
                const vector<TH1*>& bkgHists,
                const vector<TH1*>& sigHists,
                TH1* dataHist = nullptr,
                double signal_boost = 1.0)
{
    if (bkgHists.empty() && sigHists.empty() && !dataHist) return;
    vector<TH1*> allHists = bkgHists; allHists.insert(allHists.end(), sigHists.begin(), sigHists.end());
    if (dataHist) allHists.push_back(dataHist);
    double hmin, hmax; GetMinMaxIntegral(allHists, hmin, hmax);
    if (hmin <= 0.) hmin = 1.e-1;

    TH1D* h_BKG = nullptr;
    for (auto* h : bkgHists) { if (!h) continue; SetMinimumBinContent(h, 1.e-6); 
        if (!h_BKG) h_BKG = (TH1D*) h->Clone("TOT_BKG"); else h_BKG->Add(h); }

    TH1D* h_DATA = nullptr;
    if (dataHist) h_DATA = (TH1D*) dataHist->Clone("TOT_DATA");

    string canvas_name = "can_stack_" + hname;
    TCanvas* can = new TCanvas(canvas_name.c_str(), canvas_name.c_str(), 1200, 700);
    can->SetGridx(); can->SetGridy(); can->SetLogy();
    TH1* axisHist = !allHists.empty() ? allHists.front() : nullptr;
    if (!axisHist) return;
    axisHist->Draw("HIST");
    axisHist->GetYaxis()->SetRangeUser(max(0.9*hmin, 1e-6), 1.1*hmax);

    for (size_t i = 0; i < bkgHists.size(); ++i) { TH1* h = bkgHists[i]; if (!h || h->GetEntries()==0) continue;
        h->SetLineColor(kBlack); h->SetLineWidth(1); h->SetFillColor(getColorForIndex((int)i)); h->SetFillStyle(1001);
        h->Draw("SAME HIST"); }

    if (h_BKG) { h_BKG->SetLineWidth(3); h_BKG->SetLineColor(kRed); h_BKG->Draw("SAME HIST"); }

    for (size_t i = 0; i < sigHists.size(); ++i) { TH1* h = sigHists[i]; if (!h || h->GetEntries()==0) continue;
        h->SetLineWidth(3); h->SetLineStyle(7); h->SetLineColor(getColorForIndex((int)i + 10));
        h->Scale(signal_boost); h->Draw("SAME HIST"); }

    if (h_DATA) { h_DATA->SetMarkerStyle(20); h_DATA->SetMarkerSize(0.8); h_DATA->SetLineColor(kBlack); h_DATA->Draw("SAME E"); }

    TLegend* leg = new TLegend(0.7,0.7,0.9,0.9);
    if (h_BKG) leg->AddEntry(h_BKG,"SM total","F");
    for (size_t i=0;i<bkgHists.size();++i) if(bkgHists[i]) leg->AddEntry(bkgHists[i],Form("Bkg %zu",i),"F");
    for (size_t i=0;i<sigHists.size();++i) if(sigHists[i]) leg->AddEntry(sigHists[i],Form("Sig %zu",i),"L");
    if(h_DATA) leg->AddEntry(h_DATA,"Data","P");
    leg->Draw();

    if(outFile){ outFile->cd(); can->Write(0, TObject::kWriteDelete); }
    TString stackPdf = Form("%spdfs/%s.pdf", outputDir.c_str(), hname.c_str()); can->SaveAs(stackPdf);

    delete can; if(h_BKG) delete h_BKG; if(h_DATA) delete h_DATA;
}

// ----------------------
// Main
// ----------------------
int main(int argc, char* argv[]) {
    string inputFile, histCfg, datasetCfg, binsCfg;
    for(int i=1;i<argc;++i){
        string arg=argv[i];
        if(arg=="-i"||arg=="--input"){ if(i+1<argc) inputFile=argv[++i]; else{ cerr<<"[ERROR] Missing "<<arg<<endl; return 1;} }
        else if(arg=="-h"||arg=="--hist"){ if(i+1<argc) histCfg=argv[++i]; else{ cerr<<"[ERROR] Missing "<<arg<<endl; return 1;} }
        else if(arg=="-d"||arg=="--dataset"){ if(i+1<argc) datasetCfg=argv[++i]; else{ cerr<<"[ERROR] Missing "<<arg<<endl; return 1;} }
        else if(arg=="-b"||arg=="--bins"){ if(i+1<argc) binsCfg=argv[++i]; else{ cerr<<"[ERROR] Missing "<<arg<<endl; return 1;} }
        else if(arg=="--help"){ cout<<"Usage: "<<argv[0]<<" [options]\n -i <file.root>\n -h <hist.yaml>\n -d <dataset.yaml>\n -b <bins.yaml>\n"; return 0; }
        else{ cerr<<"[ERROR] Unknown arg "<<arg<<endl; return 1;}
    }
    if(inputFile.empty()){ cerr<<"[ERROR] No input ROOT file provided.\n"; return 1; }

    TString inputFileName=inputFile;
    TFile* inFile=TFile::Open(inputFileName,"READ");
    if(!inFile||inFile->IsZombie()){ cerr<<"Error: cannot open input "<<inputFileName<<endl; return 1; }

    SampleTool tool; tool.LoadAllFromMaster();

    map<string,map<string,TH1*>> groups;
    vector<TH1*> all_clones;
    set<string> uniqueBinNames;

    TIter next(inFile->GetListOfKeys());
    TKey* key;
    while((key=(TKey*)next())){
        TObject* obj=key->ReadObj(); if(!obj) continue;
        if(std::string(obj->GetName()).find("CutFlow") != std::string::npos) continue; // skip doing stuff with CutFlow for now while it's new
        if(obj->InheritsFrom(TH1::Class())){
            TH1* hIn=(TH1*)obj;
            HistId id=ParseHistName(hIn->GetName());
            string groupKey=!id.bin.empty()&&!id.var.empty()?id.bin+"__"+id.var:id.var;
            if(!id.bin.empty()) uniqueBinNames.insert(id.bin);
            TH1* clone=(TH1*)hIn->Clone(); if(!clone) continue; clone->SetDirectory(0); all_clones.push_back(clone);
            string procKey=id.proc.empty()?string(clone->GetName()):id.proc;
            groups[groupKey][procKey]=clone;
        }
    }

    for(const auto& bin:uniqueBinNames){ if(outputDir.back()!='/') outputDir+="__"; outputDir+=bin; }
    if(outputDir.back()=='_')
      outputDir.erase(outputDir.length()-2); 
    outputDir+="/";
    gSystem->mkdir(outputDir.c_str(), kTRUE); gSystem->mkdir((outputDir+"pdfs").c_str(), kTRUE);
    copyConfigsToOutput(outputDir,histCfg,datasetCfg,binsCfg);

    TString baseName=gSystem->BaseName(inputFileName); baseName.ReplaceAll(".root","");
    TString outRootName=Form("%soutput_%s.root",outputDir.c_str(),baseName.Data());
    outFile=new TFile(outRootName,"RECREATE");

    for(auto &gpair:groups){
        string groupKey=gpair.first;
        auto &procmap=gpair.second; if(procmap.empty()) continue;

        vector<string> ordered_procs; vector<TH1*> ordered_hists;
        for(const auto &entry:tool.BkgDict){ auto it=procmap.find(entry.first); if(it!=procmap.end()){ ordered_procs.push_back(it->first); ordered_hists.push_back(it->second); } }
        for(const auto &skey:tool.SignalKeys){ auto it=procmap.find(skey); if(it!=procmap.end()){ ordered_procs.push_back(it->first); ordered_hists.push_back(it->second); } }
        auto itdata=procmap.find("data"); if(itdata==procmap.end()) itdata=procmap.find("Data"); if(itdata!=procmap.end()){ ordered_procs.push_back(itdata->first); ordered_hists.push_back(itdata->second); }
        for(auto &pp:procmap){ if(find(ordered_procs.begin(),ordered_procs.end(),pp.first)==ordered_procs.end()){ ordered_procs.push_back(pp.first); ordered_hists.push_back(pp.second); } }

        // --- individual plots ---
        for(size_t i=0;i<ordered_hists.size();++i){ TH1* h=ordered_hists[i]; if(!h) continue; if(h->InheritsFrom(TH2::Class())) Plot_Hist2D(dynamic_cast<TH2*>(h)); else Plot_Hist1D(h); }

        // --- stacks ---
        vector<TH1*> bkgHists,sigHists; TH1* dataHist=nullptr;
        for(size_t i=0;i<ordered_hists.size();++i){
            TH1* h=ordered_hists[i]; const string& proc=ordered_procs[i]; if(!h||h->InheritsFrom(TH2::Class())) continue;
            if(tool.BkgDict.count(proc)) bkgHists.push_back(h);
            else if(find(tool.SignalKeys.begin(),tool.SignalKeys.end(),proc)!=tool.SignalKeys.end()) sigHists.push_back(h);
            else if(proc=="data"||proc=="Data") dataHist=h;
        }
        if(!bkgHists.empty()||!sigHists.empty()||dataHist) Plot_Stack(groupKey,bkgHists,sigHists,dataHist,1.0);

        // --- TEfficiency ---
        map<string, TH1*> numHists, denHists;
        for(const auto &pp:procmap){
            if(pp.first.find("num__")!=string::npos) numHists[pp.first]=pp.second;
            else if(pp.first.find("den__")!=string::npos) denHists[pp.first]=pp.second;
        }
        for(const auto &nume: numHists){
            string procName=nume.first; procName.replace(0,5,""); 
            string denKey="den__"+procName;
            if(denHists.count(denKey)){
                TEfficiency* eff=new TEfficiency(*nume.second,*denHists[denKey]);
                Plot_Eff(eff);
            }
        }
    }

    outFile->Close();
    inFile->Close();
    for(TH1* p:all_clones) delete p;
    all_clones.clear(); groups.clear();

    cout<<"All histograms and efficiencies plotted and saved to "<<outRootName.Data()<<" and "<<outputDir<<"pdfs/"<<endl;
    return 0;
}

