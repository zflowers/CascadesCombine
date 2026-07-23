#include <TFile.h>
#include <TKey.h>
#include <TH1.h>
#include <TF1.h>
#include <TCanvas.h>
#include <TPad.h>
#include <TLegend.h>
#include <TLatex.h>
#include <TGraphErrors.h>
#include <TLine.h>
#include <TStyle.h>
#include <TMath.h>
#include <TApplication.h>

#include <filesystem>

// Example syntax to run code:
// root -l -b 'macro/PlotZscore.C++("runs/run_Cascades_CRFit_Impacts_FD_234L_Run2Run3_v419_June30_2026_1144/datacards/SMS_TChiWZ_SMS_500_450/Analysis.root","runs/run_Cascades_CRFit_Impacts_FD_234L_Run2Run3_v419_June30_2026_1144/plots/pdfs")'

void PlotZscore(const char *inputFile, const char *outputDir)
{
    gStyle->SetOptStat(0);
    gStyle->SetOptTitle(0);

    std::filesystem::create_directories(outputDir);
    
    TFile *fin = TFile::Open(inputFile);
    
    std::string outRootName = std::string(outputDir) + "/PlotZscore.root";
    TFile *fout = TFile::Open(outRootName.c_str(), "RECREATE");

    TIter next(fin->GetListOfKeys());
    TKey *key;

    const double titleSize = 0.045;
    const double labelSize = 0.040;
    const double lowerSizeOffset = 0.045;

    while ((key = (TKey*)next()))
    {
        TObject *obj = key->ReadObj();

        if (!obj->InheritsFrom("TH1"))
            continue;

        TH1 *h = (TH1*)obj;

        const double mu     = h->GetMean();
        const double emu    = h->GetMeanError();
        const double sigma  = h->GetStdDev();
        const double esigma = h->GetStdDevError();

        const double bw = h->GetBinWidth(1);
        const double N  = h->Integral() * bw;

        TF1 *g = new TF1(Form("%s_gaus",h->GetName()),
                         "[0]*exp(-0.5*((x-[1])/[2])^2)",
                         h->GetXaxis()->GetXmin(),
                         h->GetXaxis()->GetXmax());

        double amp = N/(sigma*sqrt(2*TMath::Pi()));

        g->SetParameters(amp,mu,sigma);
        g->SetLineColor(kBlue);
        g->SetLineWidth(3);

        //----------------------------------------------------
        // Canvas
        //----------------------------------------------------

        TCanvas *c = new TCanvas(Form("c_%s",h->GetName()),h->GetTitle(),900,800);

        TPad *upper = new TPad("upper","",0,0.30,1,1);
        TPad *lower = new TPad("lower","",0,0,1,0.30);

        upper->SetBottomMargin(0.02);

        lower->SetTopMargin(0.04);
        lower->SetBottomMargin(0.30);

        upper->SetGrid();

        upper->Draw();
        lower->Draw();

        //----------------------------------------------------
        // Upper pad
        //----------------------------------------------------

        upper->cd();

        h->SetMarkerStyle(20);
        h->SetMarkerSize(1.0);
        h->SetLineColor(kBlack);

        h->GetXaxis()->SetTitleSize(0);
        h->GetXaxis()->SetLabelSize(0);
        h->GetXaxis()->CenterTitle();
        h->GetYaxis()->SetTitleSize(titleSize);
        h->GetYaxis()->SetLabelSize(labelSize);        
        h->GetYaxis()->CenterTitle();
        h->GetYaxis()->SetTitle("Fit bins / 0.25");
        h->GetYaxis()->SetTitleOffset(1.0);
        h->Draw("E1");

        double maxAbs = 0.0;

        for (int i=1; i<=h->GetNbinsX(); ++i) {
            if (h->GetBinContent(i) <= 0.) continue;
            double x = std::abs(h->GetBinCenter(i));
            if (x > maxAbs) maxAbs = x;
        }
        double xmax = 0.5 * std::ceil(2.0 * maxAbs);
        double xmin = -xmax;
        h->GetXaxis()->SetRangeUser(xmin, xmax);
        h->GetYaxis()->SetRangeUser(0., h->GetMaximum()*1.25);

        g->Draw("SAME");

        TLegend *leg = new TLegend(0.133,0.65,0.42,0.87);
        leg->SetBorderSize(0);
        leg->SetTextSize(0.033);

        leg->AddEntry(h,"Data (background-only fit)","lep");
        leg->AddEntry(g,"Gaussian model","l");
        leg->AddEntry((TObject*)0,Form("#mu = %.3f #pm %.3f", mu, emu), "");
        leg->AddEntry((TObject*)0,Form("#sigma = %.3f #pm %.3f", sigma, esigma), "");

        leg->Draw();

        TLatex cms;
        cms.SetNDC();
        cms.SetTextFont(42);
        cms.SetTextSize(0.050);
        
        double left = upper->GetLeftMargin();
        double top  = 1.0 - upper->GetTopMargin();
        
        cms.DrawLatex(left, top + 0.011, "#bf{CMS} #it{Preliminary}");

        //----------------------------------------------------
        // Lower pad
        //----------------------------------------------------

        lower->cd();
	
	TH1D *frame = new TH1D(Form("%s_frame", h->GetName()), "", 1, xmin, xmax);
	
        frame->GetXaxis()->SetLimits(xmin, xmax);
        frame->GetYaxis()->SetRangeUser(-5, 5);
	
	frame->GetXaxis()->SetTitle("Post-fit #it{Z}_{corr} scores");
	frame->GetYaxis()->SetTitle("#frac{Data - Model}{Error}");
	
	frame->GetXaxis()->SetTitleSize(titleSize+lowerSizeOffset);
	frame->GetYaxis()->SetTitleSize(titleSize+lowerSizeOffset);
	
	frame->GetXaxis()->SetLabelSize(labelSize+lowerSizeOffset);
	frame->GetYaxis()->SetLabelSize(labelSize+lowerSizeOffset);
	
	frame->GetYaxis()->SetTitleOffset(0.4);

        frame->GetXaxis()->CenterTitle();
        frame->GetYaxis()->CenterTitle();
	
	frame->Draw("");

        int nbins = h->GetNbinsX();

        TGraphErrors *pull = new TGraphErrors();

        for(int i=1;i<=nbins;i++)
        {
            double x = h->GetBinCenter(i);

            double data = h->GetBinContent(i);
            double err  = h->GetBinError(i);

            if(err<=0) continue;

            double model = g->Eval(x);

            double y = (data-model)/err;

            int p = pull->GetN();

            pull->SetPoint(p,x,y);
            pull->SetPointError(p,0.0,1.0);
        }

        pull->SetMarkerStyle(20);

        pull->Draw("P SAME");

        lower->SetGridx();
        lower->SetGridy();
        lower->RedrawAxis();

        std::string pdfName = std::string(outputDir) + "/" + h->GetName() + ".pdf";
        
        c->SaveAs(pdfName.c_str());
        
        fout->cd();
        c->Write(h->GetName());
    }

    fout->Write();
    fout->Close();
    
    fin->Close();
    gApplication->Terminate();
}
