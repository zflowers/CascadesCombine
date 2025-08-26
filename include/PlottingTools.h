#include "PlottingHelpers.h"

// ----------------------
// Plot 1D
// ----------------------
void Plot_Hist1D(TH1* h) {
    if (!h) return;
    string title = h->GetName();
    TCanvas* can = new TCanvas(("can_"+title).c_str(), ("can_"+title).c_str(), 700, 600);
    can->SetLeftMargin(0.15); can->SetRightMargin(0.18); can->SetBottomMargin(0.15);
    can->SetGridx(); can->SetGridy();
    DrawLogSmart(h, "HIST");
    h->GetXaxis()->CenterTitle();
    h->GetYaxis()->CenterTitle();
    h->GetYaxis()->SetTitle(("N_{events} / "+std::to_string(int(lumi))+" fb^{-1}").c_str());
    h->GetYaxis()->SetRangeUser(h->GetMinimum()*0.9, 1.1*h->GetMaximum());
    TLatex l; l.SetTextFont(42); l.SetNDC();
    l.SetTextSize(0.035); l.DrawLatex(0.57,0.943,m_Title[ExtractProcName(title)].c_str());
    l.SetTextSize(0.04); l.DrawLatex(0.01,0.943,"#bf{CMS} Simulation Preliminary");
    l.SetTextSize(0.045); l.DrawLatex(0.7,0.04,ExtractBinName(title).c_str());
    TString pdfName = Form("%spdfs/%s/%s.pdf", outputDir.c_str(), ExtractBinName(title).c_str(), title.c_str());
    gErrorIgnoreLevel = 1001;
    can->SaveAs(pdfName);
    gErrorIgnoreLevel = 0;
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
    can->SetGridx(); can->SetGridy();
    DrawLogSmart(h, "COLZ");
    h->GetXaxis()->CenterTitle(); h->GetYaxis()->CenterTitle();
    h->GetZaxis()->SetTitle(("N_{events} / "+std::to_string(int(lumi))+" fb^{-1}").c_str());
    TLatex l; l.SetTextFont(42); l.SetNDC();
    l.SetTextSize(0.035); l.DrawLatex(0.65,0.943,m_Title[ExtractProcName(title)].c_str());
    l.SetTextSize(0.04); l.DrawLatex(0.01,0.943,"#bf{CMS} Simulation Preliminary");
    l.SetTextSize(0.045); l.DrawLatex(0.7,0.04,ExtractBinName(title).c_str());
    TString pdfName = Form("%spdfs/%s/%s.pdf", outputDir.c_str(), ExtractBinName(title).c_str(), title.c_str());
    gErrorIgnoreLevel = 1001;
    can->SaveAs(pdfName);
    gErrorIgnoreLevel = 0;
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
    e->SetStatisticOption(TEfficiency::kFNormal); // assume effs were made from weighted hists
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

    TLatex l; l.SetTextFont(42); l.SetNDC();
    l.SetTextSize(0.035); l.DrawLatex(0.65,0.943,m_Title[ExtractProcName(title)].c_str());
    l.SetTextSize(0.04); l.DrawLatex(0.01,0.943,"#bf{CMS} Simulation Preliminary");
    l.SetTextSize(0.045); l.DrawLatex(0.7,0.04,ExtractBinName(title).c_str());
    TString pdfName = Form("%spdfs/%s/%s.pdf", outputDir.c_str(), ExtractBinName(title).c_str(), title.c_str());
    gErrorIgnoreLevel = 1001;
    can->SaveAs(pdfName);
    gErrorIgnoreLevel = 0;
    if (outFile) { outFile->cd(); can->Write(0, TObject::kWriteDelete); }
    delete can;
}

// ----------------------
// Plot stack
// ----------------------
void Plot_Stack(const string& hname,
                vector<TH1*>& bkgHists,
                vector<TH1*>& sigHists,
                TH1* dataHist = nullptr,
                double signal_boost = 1.0)
{
    if (bkgHists.empty() && (sigHists.empty() || !dataHist)) return;
    vector<TH1*> allHists = bkgHists; allHists.insert(allHists.end(), sigHists.begin(), sigHists.end());
    if (dataHist) allHists.push_back(dataHist);
    double hmin, hmax; GetMinMaxIntegral(allHists, hmin, hmax);
    if (hmin <= 0.) hmin = 1.e-1;

    int stack_index = 0;
    TH1D* h_BKG = nullptr;
    for (auto* h : bkgHists) {
        if (!h) continue;
        SetMinimumBinContent(h, 1.e-6); 
        if (!h_BKG) { 
            h_BKG = (TH1D*) h->Clone("TOT_BKG"); 
        } else {
            for(int k = 0; k < stack_index; k++){
              bkgHists[k]->Add(h);
            }
            h_BKG->Add(h);
        }
        stack_index++;
    }
    TH1D* h_DATA = nullptr;
    if (dataHist) h_DATA = (TH1D*) dataHist->Clone("TOT_DATA");

    string canvas_name = "can_stack_" + hname;
    TCanvas* can = new TCanvas(canvas_name.c_str(), canvas_name.c_str(), 1200, 700);
    can->SetLeftMargin(hlo);
    can->SetRightMargin(hhi);
    can->SetBottomMargin(hbo);
    can->SetTopMargin(hto);
    can->SetGridx(); can->SetGridy();
    TH1* axisHist = !allHists.empty() ? allHists.front() : nullptr;
    if (!axisHist) return;
    DrawLogSmart(axisHist, "HIST");
    axisHist->GetYaxis()->SetRangeUser(max(0.9*hmin, 1e-6), 1.1*hmax);

    for (size_t i = 0; i < bkgHists.size(); ++i) { TH1* h = bkgHists[i]; if (!h || h->GetEntries()==0) continue;
        h->SetLineColor(kBlack); h->SetLineWidth(1); h->SetFillColor(m_Color[ExtractProcName(bkgHists[i]->GetName())]); h->SetFillStyle(1001);
        DrawLogSmart(h, "SAME HIST"); }

    if (h_BKG) { h_BKG->SetLineWidth(3); h_BKG->SetLineColor(kRed); DrawLogSmart(h_BKG, "SAME HIST"); }

    for (size_t i = 0; i < sigHists.size(); ++i) { TH1* h = sigHists[i]; if (!h || h->GetEntries()==0) continue;
        SetMinimumBinContent(h, 1.e-6);
        h->SetLineWidth(3); h->SetLineStyle(7); h->SetLineColor(m_Color[ExtractProcName(sigHists[i]->GetName())]);
        h->Scale(signal_boost); DrawLogSmart(h, "SAME HIST"); }

    if (h_DATA) { h_DATA->SetMarkerStyle(20); h_DATA->SetMarkerSize(0.8); h_DATA->SetLineColor(kBlack); DrawLogSmart(h_DATA, "SAME E"); }

    TLegend* leg = new TLegend(1.-hhi+0.01, 1.- (bkgHists.size()+sigHists.size()+2)*(1.-0.49)/9., 0.98, 1.-hto-0.005);
    leg->SetTextFont(132);
    leg->SetTextSize(0.042);
    leg->SetFillColor(kWhite);
    leg->SetLineColor(kWhite);
    leg->SetShadowColor(kWhite);
    if (h_BKG) leg->AddEntry(h_BKG,"SM total","F");
    for (size_t i=0;i<bkgHists.size();++i) if(bkgHists[i]) leg->AddEntry(bkgHists[i],m_Title[ExtractProcName(bkgHists[i]->GetName())].c_str(),"F");
    for (size_t i=0;i<sigHists.size();++i) if(sigHists[i]) {
        std::string tmp_label = m_Title[ExtractProcName(sigHists[i]->GetName())];
        if (signal_boost != 1.0) {
            std::ostringstream boost_str;
            boost_str << std::setprecision(3) << std::defaultfloat << signal_boost;
            tmp_label += " * " + boost_str.str();
        }
        leg->AddEntry(sigHists[i], tmp_label.c_str(), "L");
    }
    if(h_DATA) leg->AddEntry(h_DATA,"Data","P");
    leg->Draw();

    if(outFile){ outFile->cd(); can->Write(0, TObject::kWriteDelete); }
    TString stackPdf = Form("%spdfs/%s/%s.pdf", outputDir.c_str(), ExtractBinName(bkgHists[0]->GetName()).c_str(), canvas_name.c_str());
    gErrorIgnoreLevel = 1001; can->SaveAs(stackPdf); gErrorIgnoreLevel = 0;

    delete can; if(h_BKG) delete h_BKG; if(h_DATA) delete h_DATA;
}

// ----------------------
// Plot CutFlow
// ----------------------
void Plot_CutFlow(const std::string &hname,
                       const std::vector<TH1*> &bkgHists,
                       const std::vector<TH1*> &sigHists,
                       TH1* dataHist,
                       double signal_boost)
{
    if (bkgHists.empty() && (sigHists.empty() || !dataHist)) return;

    // Collect all hists for range finding
    vector<TH1*> allHists = bkgHists;
    allHists.insert(allHists.end(), sigHists.begin(), sigHists.end());
    if (dataHist) allHists.push_back(dataHist);

    double hmin, hmax;
    GetMinMaxIntegral(allHists, hmin, hmax);
    if (hmin <= 0.) hmin = 1.e-4;

    // Total background
    TH1D* h_BKG = nullptr;
    for (auto* h : bkgHists) {
        if (!h) continue;
        SetMinimumBinContent(h, 1.e-6);
        if (!h_BKG) h_BKG = (TH1D*) h->Clone("TOT_BKG");
        else h_BKG->Add(h);
    }

    // Clone data
    TH1D* h_DATA = nullptr;
    if (dataHist) h_DATA = (TH1D*) dataHist->Clone("TOT_DATA");

    // Canvas
    string canvas_name = "can_cutflow_" + hname;
    TCanvas* can = new TCanvas(canvas_name.c_str(), canvas_name.c_str(), 1200, 700);
    can->SetLeftMargin(hlo);
    can->SetRightMargin(hhi);
    can->SetBottomMargin(hbo);
    can->SetTopMargin(hto);
    can->SetGridx(); can->SetGridy();
    can->SetLogy();

    // Axis from first available hist
    TH1* axisHist = !allHists.empty() ? allHists.front() : nullptr;
    if (!axisHist) return;
    axisHist->Draw("");
    axisHist->GetYaxis()->SetRangeUser(max(0.9*hmin, 1e-6), 1.1*hmax);
    axisHist->GetXaxis()->CenterTitle();
    axisHist->GetXaxis()->SetTitleFont(42);
    axisHist->GetXaxis()->SetTitleSize(0.05);
    axisHist->GetXaxis()->SetTitleOffset(1.0);
    axisHist->GetXaxis()->SetLabelFont(42);
    axisHist->GetXaxis()->SetLabelSize(0.04);
    axisHist->GetXaxis()->SetTickSize(0.);
    axisHist->GetYaxis()->CenterTitle();
    axisHist->GetYaxis()->SetTitleFont(42);
    axisHist->GetYaxis()->SetTitleSize(0.04);
    axisHist->GetYaxis()->SetTitleOffset(0.9);
    axisHist->GetYaxis()->SetLabelFont(42);
    axisHist->GetYaxis()->SetLabelSize(0.035);

    // Draw bkg
    for (size_t i = 0; i < bkgHists.size(); ++i) {
        TH1* h = bkgHists[i]; if (!h || h->GetEntries()==0) continue;
        h->SetLineColor(kBlack);
        h->SetLineWidth(1);
        h->SetLineColor(m_Color[ExtractProcName(bkgHists[i]->GetName())]);
        h->SetMarkerColor(m_Color[ExtractProcName(bkgHists[i]->GetName())]);
        h->SetFillStyle(1001);
        h->Draw("SAME");
    }

    if (h_BKG) {
        h_BKG->SetLineWidth(3);
        h_BKG->SetLineColor(kRed);
        h_BKG->SetMarkerColor(kRed);
        h_BKG->Draw("SAME");
    }

    // Draw signals
    for (size_t i = 0; i < sigHists.size(); ++i) {
        TH1* h = sigHists[i]; if (!h || h->GetEntries()==0) continue;
        h->Scale(signal_boost);
        h->SetLineWidth(1);
        h->SetLineStyle(7);
        h->SetLineColor(m_Color[ExtractProcName(sigHists[i]->GetName())]);
        h->SetMarkerColor(m_Color[ExtractProcName(sigHists[i]->GetName())]);
        h->Draw("SAME");
    }

    // Data
    if (h_DATA) {
        h_DATA->SetMarkerStyle(20);
        h_DATA->SetMarkerSize(0.8);
        h_DATA->SetLineColor(kBlack);
        h_DATA->Draw("SAME E");
    }

    // Add Legend
    TLegend* leg = new TLegend(1.-hhi+0.01, 1.- (bkgHists.size()+sigHists.size()+2)*(1.-0.49)/9., 0.98, 1.-hto-0.005);
    leg->SetTextFont(132);
    leg->SetTextSize(0.042);
    leg->SetFillColor(kWhite);
    leg->SetLineColor(kWhite);
    leg->SetShadowColor(kWhite);
    if (h_BKG) leg->AddEntry(h_BKG,"SM total","L");
    for (size_t i=0;i<bkgHists.size();++i) if(bkgHists[i]) leg->AddEntry(bkgHists[i],m_Title[ExtractProcName(bkgHists[i]->GetName())].c_str(),"L");
    for (size_t i=0;i<sigHists.size();++i) if(sigHists[i]) {
        std::string tmp_label = m_Title[ExtractProcName(sigHists[i]->GetName())];
        if (signal_boost != 1.0) {
            std::ostringstream boost_str;
            boost_str << std::setprecision(3) << std::defaultfloat << signal_boost;
            tmp_label += " * " + boost_str.str();
        }
        leg->AddEntry(sigHists[i], tmp_label.c_str(), "L");
    }
    if (h_DATA) leg->AddEntry(h_DATA,"Data","P");
    leg->Draw();

    TLatex l;
    l.SetNDC();
    l.SetTextSize(0.04);
    l.SetTextFont(42);
    l.DrawLatex(0.1,0.943,"#bf{#it{CMS}} Internal 13 TeV Simulation");
    l.SetTextSize(0.045); l.DrawLatex(0.7,0.04,ExtractBinName(string(axisHist->GetName())).c_str());

    // Save
    if(outFile){ outFile->cd(); can->Write(0, TObject::kWriteDelete); }
    TString pdfOut = Form("%spdfs/%s/%s.pdf", outputDir.c_str(), ExtractBinName(string(axisHist->GetName())).c_str(), hname.c_str());
    gErrorIgnoreLevel = 1001;
    can->SaveAs(pdfOut);
    gErrorIgnoreLevel = 0;

    delete can;
    if(h_BKG) delete h_BKG;
    if(h_DATA) delete h_DATA;
}
