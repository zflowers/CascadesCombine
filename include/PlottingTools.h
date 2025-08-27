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
        h->SetLineColor(kBlack); h->SetLineWidth(1);
        h->SetMarkerColor(m_Color[ExtractProcName(bkgHists[i]->GetName())]);
        h->SetFillColor(m_Color[ExtractProcName(bkgHists[i]->GetName())]); h->SetFillStyle(1001);
        DrawLogSmart(h, "SAME HIST"); }

    if (h_BKG) { h_BKG->SetLineWidth(3); h_BKG->SetLineColor(kRed); DrawLogSmart(h_BKG, "SAME HIST"); }

    for (size_t i = 0; i < sigHists.size(); ++i) { TH1* h = sigHists[i]; if (!h || h->GetEntries()==0) continue;
        SetMinimumBinContent(h, 1.e-6);
        h->SetLineWidth(3); h->SetLineStyle(7);
        h->SetLineColor(m_Color[ExtractProcName(sigHists[i]->GetName())]);
        h->SetMarkerColor(m_Color[ExtractProcName(sigHists[i]->GetName())]);
        h->Scale(signal_boost); DrawLogSmart(h, "SAME HIST"); }

    if (h_DATA) { h_DATA->SetMarkerStyle(20); h_DATA->SetMarkerSize(0.8); h_DATA->SetLineColor(kBlack); DrawLogSmart(h_DATA, "SAME E"); }

    // Add Legend
    TLegend* leg = new TLegend(1.-hhi+0.01, 1.- (bkgHists.size()+sigHists.size()+2)*(1.-0.49)/9., 0.98, 1.-hto-0.005);
    leg->SetTextFont(132);
    leg->SetTextSize(0.039);
    leg->SetFillColor(kWhite);
    leg->SetLineColor(kWhite);
    leg->SetShadowColor(kWhite);
    if (h_BKG) leg->AddEntry(h_BKG,"SM total","L");
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
    if (h_DATA) leg->AddEntry(h_DATA,"Data","P");
    leg->Draw();

    TLatex l;
    l.SetNDC();
    l.SetTextSize(0.04);
    l.SetTextFont(42);
    l.DrawLatex(0.1,0.943,"#bf{#it{CMS}} Internal 13 TeV Simulation");
    l.DrawLatex(0.7,0.943,ExtractBinName(string(axisHist->GetName())).c_str());

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
    axisHist->GetYaxis()->SetRangeUser(max(0.8*hmin, 1e-6), 1.2*hmax);
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
        h->SetLineWidth(2);
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
        h->SetLineWidth(2);
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
    leg->SetTextSize(0.039);
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
    l.DrawLatex(0.09,0.943,"#bf{#it{CMS}} Internal 13 TeV Simulation");
    l.DrawLatex(0.69,0.943,ExtractBinName(string(axisHist->GetName())).c_str());

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

// ----------------------
// Plot TEff Multigraph (grouped by bin or process)
// groupType should be "Bin" or "Process"
// ----------------------
void Plot_Eff_Multi(const std::string& groupName,
                    const std::vector<TEfficiency*>& effs,
                    const std::string& groupType
                   )
{
    if(effs.empty()) return;

    // Canvas
    TCanvas* can = new TCanvas(("can_multi_"+groupName).c_str(), ("can_"+groupName).c_str(), 1200, 700);
    can->SetLeftMargin(hlo);
    can->SetRightMargin(hhi);
    can->SetBottomMargin(hbo);
    can->SetTopMargin(hto);
    can->SetGridx(); can->SetGridy();
    can->Draw();
    can->cd();

    double ymin = 1e6, ymax = -1e6;
    for(auto* e : effs){
        if(!e) continue;
    
        // check for effective points
        auto* htot = e->GetTotalHistogram();
        bool hasPoints = false;
        for(int i = 1; i <= htot->GetNbinsX(); ++i){
            if(htot->GetBinContent(i) > 0){ hasPoints = true; break; }
        }
        if(!hasPoints){
            // std::cerr << "[Warning] Skipping TEff with no valid points: " << e->GetName() << "\n";
            continue;
        }
    
        // create TGraph without painting
        TGraphAsymmErrors* gr = e->CreateGraph();
        if(!gr) continue;
    
        const int n = gr->GetN();
        for(int i = 0; i < n; ++i){
            double x, y;
            gr->GetPoint(i, x, y);
            double ylow = y - gr->GetErrorYlow(i);
            double yhigh = y + gr->GetErrorYhigh(i);
            ymin = std::min(ymin, ylow);
            ymax = std::max(ymax, yhigh);
        }
    }
    if(ymin < 0.) ymin = 0.;
    ymax *= 1.05;

    // Add Legend
    TLegend* leg = new TLegend(1.-hhi+0.01, 1.- (effs.size()+2)*(1.-0.49)/9., 0.98, 1.-hto-0.005);
    leg->SetTextFont(132);
    leg->SetTextSize(0.039);
    leg->SetFillColor(kWhite);
    leg->SetLineColor(kWhite);
    leg->SetShadowColor(kWhite);

    TMultiGraph* mg = new TMultiGraph();

    for(size_t i = 0; i < effs.size(); ++i){
        TEfficiency* e = effs[i];
        if(!e) continue;
    
        // Create TGraph safely
        TGraphAsymmErrors* gr = nullptr;
        try {
            gr = (TGraphAsymmErrors*)e->CreateGraph(); // safer than Draw() + GetPaintedGraph()
        } catch (...) {
            std::cerr << "[Warning] Failed to create graph for TEff: " << e->GetName() << "\n";
            continue;
        }
    
        if(!gr || gr->GetN() == 0){
            // std::cerr << "[Warning] Skipping TEff with no valid points: " << e->GetName() << "\n";
            continue;
        }
    
        // Parse name to get bin/proc
        HistId id = ParseHistName(e->GetName());
        std::string legendKey, legendTitle;
        int color = kBlack;
        static size_t fallbackIndex = 0;
        
        if (groupType == "Bin") {
            legendKey = id.proc.empty() ? "unknown_proc" : id.proc;
            legendTitle = m_Title.count(legendKey) ? m_Title[legendKey] : legendKey;
        } else {
            legendKey = id.bin.empty() ? "unknown_bin" : id.bin;
            legendTitle = m_Title.count(legendKey) ? m_Title[legendKey] : legendKey;
        }
        
        // Try to get color from map
        auto it = m_Color.find(legendKey);
        if (it != m_Color.end()) {
            color = it->second;
        } else {
            // Fallback to rotating palette
            color = fallbackColors[fallbackIndex % fallbackColors.size()];
            fallbackIndex++;
        }

        gr->SetMarkerStyle(20);
        gr->SetMarkerColor(color);
        gr->SetLineColor(color);
    
        mg->Add(gr);
        leg->AddEntry(gr, legendTitle.c_str(), "PL");
    }

    // Draw multi-graph and style axes
    mg->Draw("AP");
    mg->GetXaxis()->CenterTitle();
    mg->GetXaxis()->SetTitleFont(42);
    mg->GetXaxis()->SetTitleSize(0.06);
    mg->GetXaxis()->SetTitleOffset(1.06);
    mg->GetXaxis()->SetLabelFont(42);
    mg->GetXaxis()->SetLabelSize(0.05);
    mg->GetYaxis()->CenterTitle();
    mg->GetYaxis()->SetTitleFont(42);
    mg->GetYaxis()->SetTitleSize(0.06);
    mg->GetYaxis()->SetTitleOffset(1.12);
    mg->GetYaxis()->SetLabelFont(42);
    mg->GetYaxis()->SetLabelSize(0.05);
    mg->GetYaxis()->SetRangeUser(ymin, ymax);

    // Draw legend
    leg->Draw();

    // TLatex: CMS on top-left; group info on top-right
    TLatex l; l.SetNDC(); l.SetTextFont(42);
    l.SetTextSize(0.04);
    l.DrawLatex(0.09, 0.943, "#bf{CMS} Simulation Preliminary");

    // top-right: show what this group is (Bin or Process)
    std::string topRight;
    if(groupType == "Bin") topRight = groupName;
    else topRight = m_Title[groupName];
    l.SetTextSize(0.045);
    l.DrawLatex(0.69, 0.943, topRight.c_str());
    string varName = ParseHistName(effs[0]->GetName()).var;

    // Save
    TString dirName = Form("%spdfs/%s", outputDir.c_str(), groupName.c_str());
    gSystem->MakeDirectory(dirName);
    TString pdfName = Form("%s/%s_%s.pdf", dirName.Data(), groupName.c_str(), varName.c_str());
    gErrorIgnoreLevel = 1001; can->SaveAs(pdfName); gErrorIgnoreLevel = 0;
    if(outFile){ outFile->cd(); can->Write(0, TObject::kWriteDelete); }

    // Cleanup
    delete mg;
    delete leg;
    delete can;
}

void Plot_EventCount2D(TH2* h, const std::string &mode,
                       double zmin_override = std::numeric_limits<double>::quiet_NaN(),
                       double zmax_override = std::numeric_limits<double>::quiet_NaN()) {
    if(!h) return;

    // Locate Total Bkg row
    int totalRow = -1;
    for(int j=1; j<=h->GetNbinsY(); ++j){
      TString lab = h->GetYaxis()->GetBinLabel(j);
      if(lab.Contains("Total Bkg")) { totalRow = j; break; }
    }

    // Determine Z-axis range
    double use_zmin = std::numeric_limits<double>::quiet_NaN();
    double use_zmax = std::numeric_limits<double>::quiet_NaN();
    if(std::isfinite(zmin_override) && std::isfinite(zmax_override) && zmax_override > zmin_override){
      use_zmin = zmin_override;
      use_zmax = zmax_override;
    } else if(mode=="Zbi"){
      double zmin=1e300, zmax=-1e300;
      for(int iy=1; iy<=h->GetNbinsY(); ++iy){
        if(iy==totalRow) continue; // ignore Total Bkg
        for(int ix=1; ix<=h->GetNbinsX(); ++ix){
          double v = h->GetBinContent(ix, iy);
          if(!std::isfinite(v) || v<=0) continue;
          if(v<zmin) zmin=v;
          if(v>zmax) zmax=v;
        }
      }
      if(zmax>=zmin && zmin<1e299){
        use_zmin = std::max(0.0, 0.9*zmin);
        use_zmax = 1.1*zmax;
        if(use_zmax<=use_zmin) use_zmax = use_zmin + 1e-6;
      }
    }

    if(std::isfinite(use_zmin) && std::isfinite(use_zmax))
      h->GetZaxis()->SetRangeUser(use_zmin, use_zmax);

    // Canvas setup
    TCanvas* can = new TCanvas(("can_"+std::string(h->GetName())).c_str(), "", 1200, 700);
    can->SetLeftMargin(0.13);
    can->SetRightMargin(0.13);
    can->SetBottomMargin(0.06);
    can->SetTopMargin(0.06);
    can->SetGridx(); can->SetGridy();
    if(mode=="yield") can->SetLogz();

    // Draw histogram COLZ
    h->Draw("COLZ");
    gPad->Update();

    // Prepare TLatex for numbers
    TLatex tex;
    tex.SetTextFont(42);
    tex.SetTextAlign(22);
    float textSize = 0.045;
    if(h->GetNbinsX()>25) textSize*=0.6;
    if(h->GetNbinsY()>25) textSize*=0.8;
    tex.SetTextSize(textSize);

    // Draw numbers
    for(int iy=1; iy<=h->GetNbinsY(); ++iy){
      for(int ix=1; ix<=h->GetNbinsX(); ++ix){
        double val = h->GetBinContent(ix, iy);
        double xlow = h->GetXaxis()->GetBinLowEdge(ix);
        double xup  = h->GetXaxis()->GetBinUpEdge(ix);
        double ylow = h->GetYaxis()->GetBinLowEdge(iy);
        double yup  = h->GetYaxis()->GetBinUpEdge(iy);
        double xc = 0.5*(xlow+xup);
        double yc = 0.5*(ylow+yup);
        TString label = Form("%.3g", val);

        if(mode=="Zbi" && iy==totalRow){
          // Draw white box behind Total Bkg text
          double xpad = 0.003*(xup-xlow);
          double ypad = 0.006*(yup-ylow);
          TBox* box = new TBox(xlow+xpad, ylow+ypad, xup-xpad, yup-ypad);
          box->SetFillColor(kWhite);
          box->SetLineColor(kBlack);
          box->SetFillStyle(1001);
          box->Draw("F same");
          tex.SetTextColor(kBlack);
        } else {
          tex.SetTextColor(kRed);
        }

        tex.DrawLatex(xc, yc, label);
      }
    }

    gPad->Update(); // force redraw

    // Re-apply axis labels & formatting
    h->GetXaxis()->CenterTitle();
    h->GetXaxis()->SetTitleFont(42); h->GetXaxis()->SetTitleSize(0.06); h->GetXaxis()->SetTitleOffset(1.06);
    h->GetXaxis()->SetLabelFont(42); h->GetXaxis()->SetLabelSize(0.045);
    h->GetYaxis()->CenterTitle();
    h->GetYaxis()->SetTitleFont(42); h->GetYaxis()->SetTitleSize(0.06); h->GetYaxis()->SetTitleOffset(1.1);
    h->GetYaxis()->SetLabelFont(42); h->GetYaxis()->SetLabelSize(0.035);
    h->GetZaxis()->CenterTitle();
    h->GetZaxis()->SetTitleFont(42); h->GetZaxis()->SetTitleSize(0.03); h->GetZaxis()->SetTitleOffset(1.03);
    h->GetZaxis()->SetLabelFont(42); h->GetZaxis()->SetLabelSize(0.03);

    // Z-axis title
    if(mode == "yield")
      h->GetZaxis()->SetTitle(("N_{events} passing category scaled to "+std::to_string(lumi)+" fb^{-1}").c_str());
    else if(mode == "SoB")
      h->GetZaxis()->SetTitle(("#frac{N_{events}}{N_{TOT BKG}} for process in category scaled to "+std::to_string(lumi)+" fb^{-1}").c_str());
    else if(mode == "SoverSqrtB")
      h->GetZaxis()->SetTitle(("#frac{N_{events}}{#sqrt{N_{TOT BKG}}} for process in category scaled to "+std::to_string(lumi)+" fb^{-1}").c_str());
    else if(mode == "Zbi")
      h->GetZaxis()->SetTitle("Z_{bi} for signal in category");
    else
      h->GetZaxis()->SetTitle("Yield");

    TLatex l; l.SetNDC(); l.SetTextFont(42);
    l.SetTextSize(0.04);
    l.DrawLatex(0.13, 0.947, "#bf{CMS} Simulation Preliminary");

    // Save canvas
    TString pdfName = Form("%s/pdfs/CutFlow2D_%s.pdf", outputDir.c_str(), mode.c_str());
    gErrorIgnoreLevel = 1001; can->SaveAs(pdfName); gErrorIgnoreLevel = 0;

    delete can;
    delete h;
}

void MakeAndPlotCutflow2D(
    const std::map<std::string, std::map<std::string, TH1*>> &cutflowMap,
    const std::string &groupKey,
    const std::string &mode = "yield",
    double Zbi_unc = 0.0)
{
    if(cutflowMap.empty()) return;

    // --- 1) collect list of bins (X axis) ---
    std::vector<std::string> bins;
    bins.reserve(cutflowMap.size());
    for (const auto &bp : cutflowMap) bins.push_back(bp.first);

    int nx = (int)bins.size();

    // --- 2) collect set of all processes across bins (skip data) ---
    std::set<std::string> procSet;
    for (const auto &bp : cutflowMap) {
        for (const auto &pp : bp.second) {
            const std::string &pname = pp.first;
            if(pname == "data" || pname == "Data") continue;
            procSet.insert(pname);
        }
    }

    // --- 3) classify processes into bkg / sig (use tool) ---
    std::vector<std::string> allBkgs, allSigs;
    for (const auto &p : procSet) {
        if (tool.BkgDict.count(p)) allBkgs.push_back(p);
        else if (std::find(tool.SignalKeys.begin(), tool.SignalKeys.end(), p) != tool.SignalKeys.end()
                 || p.find("SMS") != std::string::npos || p.find("Cascades") != std::string::npos)
            allSigs.push_back(p);
        else
            allBkgs.push_back(p); // default unknown -> background
    }

    // --- 4) build yields[proc][binIndex] (binIndex: 0..nx-1) ---
    std::map<std::string, std::vector<double>> yields;
    for (const auto &p : procSet) yields[p] = std::vector<double>(nx, 0.0);

    auto BinContentSafe = [&](TH1* h, int bin)->double {
        if(!h) return 0.0;
        int nb = h->GetNbinsX();
        if(bin < 1 || bin > nb) return 0.0;
        return h->GetBinContent(bin);
    };

    for (int ib = 0; ib < nx; ++ib) {
        const auto &binMap = cutflowMap.at(bins[ib]);
        for (const auto &pp : binMap) {
            const std::string &proc = pp.first;
            TH1* h = pp.second;
            if(!h) continue;
    
            int nbins = h->GetNbinsX(); // number of regular bins
            double last = 0.0;
            if(nbins >= 1) {
                // safely get last regular bin using BinContentSafe
                last = BinContentSafe(h, nbins);
            } else {
                // defensive: histogram has no regular bins
                last = 0.0;
                std::cerr << "[Warning] Proc '" << proc << "' in bin '" << bins[ib]
                          << "' has zero bins; setting yield to 0\n";
            }
            yields[proc][ib] = last;
        }
    }

    // Ensure all bkgs/sigs exist in yields map (may be empty vectors already)
    for (const auto &b : allBkgs) if(!yields.count(b)) yields[b] = std::vector<double>(nx,0.0);
    for (const auto &s : allSigs) if(!yields.count(s)) yields[s] = std::vector<double>(nx,0.0);

    // --- 5) compute total background per bin ---
    std::vector<double> totalBkg(nx, 0.0);
    for (int ib=0; ib<nx; ++ib) {
        double sum = 0.0;
        for (const auto &b : allBkgs) {
            if(yields.count(b)) sum += yields[b][ib];
        }
        totalBkg[ib] = sum;
    }

    // --- 6) ordering: signals by first-bin yield, bkgs by first-bin yield,
    //                 bins ordered by totalBkg (descending) ---
    std::sort(allSigs.begin(), allSigs.end(),
        [&](const std::string &a, const std::string &b){
            double A = (yields.count(a) ? yields[a][0] : 0.0);
            double B = (yields.count(b) ? yields[b][0] : 0.0);
            return A > B;
        });
    std::sort(allBkgs.begin(), allBkgs.end(),
        [&](const std::string &a, const std::string &b){
            double A = (yields.count(a) ? yields[a][0] : 0.0);
            double B = (yields.count(b) ? yields[b][0] : 0.0);
            return A > B;
        });

    std::vector<int> binOrder(nx);
    for (int i=0;i<nx;i++) binOrder[i]=i;
    std::sort(binOrder.begin(), binOrder.end(),
        [&](int A, int B){ return totalBkg[A] > totalBkg[B]; });

    // --- 7) build Y-order depending on mode ---
    std::vector<std::string> yOrder;
    if(mode == "Zbi"){
        // Total Bkg at top, then signals (no backgrounds)
        yOrder.push_back("Total Bkg");
        for (const auto &s : allSigs) yOrder.push_back(s);
    } else {
        // signals, Total Bkg, backgrounds
        for (const auto &s : allSigs) yOrder.push_back(s);
        yOrder.push_back("Total Bkg");
        for (const auto &b : allBkgs) yOrder.push_back(b);
    }

    int ny = (int)yOrder.size();

    // --- 8) create TH2D with appropriate binning & labels ---
    std::string histName = groupKey + "_Cutflow2D_" + mode;
    for (auto &c : histName) if(c==' '||c=='/') c='_';
    TH2D *h2 = new TH2D(histName.c_str(), histName.c_str(), nx, 0.5, nx+0.5, ny, 0.5, ny+0.5);

    // X labels = bin names in the sorted order
    for (int ix=0; ix<nx; ++ix) {
        int old = binOrder[ix];
        h2->GetXaxis()->SetBinLabel(ix+1, bins[old].c_str());
    }
    // Y labels
    for (int iy=0; iy<ny; ++iy) {
        h2->GetYaxis()->SetBinLabel(iy+1, m_Title[yOrder[iy]].c_str());
    }

    // --- 9) fill contents. For Zbi collect signal Zbi values for z-range override ---
    std::vector<double> zbi_values_for_range;
    for (int ix=0; ix<nx; ++ix) {
        int oldBin = binOrder[ix];
        double B = totalBkg[oldBin];
        double sqrtB = (B>0.0 ? std::sqrt(B) : 0.0);
        for (int iy=0; iy<ny; ++iy) {
            const std::string &procName = yOrder[iy];
            double val = 0.0;
            if(mode == "Zbi") {
                if(procName == "Total Bkg") {
                    val = B;
                } else {
                    // signal Zbi for this bin
                    double S = (yields.count(procName) ? yields[procName][oldBin] : 0.0);
                    if(S > 0.0 && B >= 0.0) {
                        val = CalculateZbi(S, B, Zbi_unc);
                        if(val > 0.0) zbi_values_for_range.push_back(val);
                    } else {
                        val = 0.0;
                    }
                }
            } else {
                if(procName == "Total Bkg") {
                    if(mode == "yield") val = B;
                    else if(mode == "SoB") val = (B > 0.0 ? 1.0 : 0.0);
                    else if(mode == "SoverSqrtB") val = (sqrtB > 0.0 ? sqrtB : 0.0);
                    else val = B;
                } else {
                    double pY = (yields.count(procName) ? yields[procName][oldBin] : 0.0);
                    if(mode == "yield") val = pY;
                    else if(mode == "SoB") val = (B > 0.0 ? pY / B : 0.0);
                    else if(mode == "SoverSqrtB") val = (sqrtB > 0.0 ? pY / sqrtB : 0.0);
                    else val = pY;
                }
            }
            h2->SetBinContent(ix+1, iy+1, val);
        }
    }

    // --- 10) determine z-range override for Zbi (based only on signal Zbi values) ---
    double zmin_override = std::numeric_limits<double>::quiet_NaN();
    double zmax_override = std::numeric_limits<double>::quiet_NaN();
    if(mode == "Zbi" && !zbi_values_for_range.empty()) {
        double zmin = 1e300, zmax = -1e300;
        for(double v : zbi_values_for_range){
            if(v > 0.0) { if(v < zmin) zmin = v; if(v > zmax) zmax = v; }
        }
        if(zmax >= zmin && zmin < 1e299){
            zmin_override = std::max(0.0, 0.9 * zmin);
            zmax_override = 1.1 * zmax;
            if(zmax_override <= zmin_override) zmax_override = zmin_override + 1e-6;
        }
    }

    // --- 11) call plotting routine which handles drawing + special Zbi total-row masking ---
    Plot_EventCount2D(h2, mode, zmin_override, zmax_override);
    // Plot_EventCount2D deletes h2 internally as per your convention
}
