// PlotHistograms.cpp
#include <TFile.h>
#include <TH1.h>
#include <TH2.h>
#include <TKey.h>
#include <TCanvas.h>
#include <TSystem.h>
#include <TString.h>
#include <iostream>

// Output ROOT file pointer (global for simplicity)
TFile* outFile = nullptr;

// ----------------------
// 1D Histogram
// ----------------------
void Plot_Hist(TH1* h) {
    if (!h) return;

    TString histName = h->GetName();
    TCanvas* c = new TCanvas(Form("c_%s", histName.Data()), histName, 800, 600);

    h->Draw();

    // Save to output ROOT file
    outFile->cd();
    c->Write();

    // Save PDF
    TString pdfName = Form("plots/pdfs/%s.pdf", histName.Data());
    c->SaveAs(pdfName);

    delete c;
}

// ----------------------
// 2D Histogram
// ----------------------
void Plot_Hist(TH2* h) {
    if (!h) return;

    TString histName = h->GetName();
    TCanvas* c = new TCanvas(Form("c_%s", histName.Data()), histName, 800, 600);

    h->Draw("COLZ"); // default for 2D

    // Save to output ROOT file
    outFile->cd();
    c->Write();

    // Save PDF
    TString pdfName = Form("plots/pdfs/%s.pdf", histName.Data());
    c->SaveAs(pdfName);

    delete c;
}

// ----------------------
// Main
// ----------------------
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " input.root" << std::endl;
        return 1;
    }

    TString inputFileName = argv[1];
    TFile* inFile = TFile::Open(inputFileName, "READ");
    if (!inFile || inFile->IsZombie()) {
        std::cerr << "Error: cannot open input file " << inputFileName << std::endl;
        return 1;
    }

    // --- Make output dirs ---
    gSystem->mkdir("plots", kTRUE);
    gSystem->mkdir("plots/pdfs", kTRUE);

    // --- Build output root file name ---
    TString baseName = gSystem->BaseName(inputFileName);
    baseName.ReplaceAll(".root", "");
    TString outRootName = Form("plots/output_%s.root", baseName.Data());
    outFile = new TFile(outRootName, "RECREATE");

    // --- Loop over keys in input file ---
    TIter next(inFile->GetListOfKeys());
    TKey* key;
    while ((key = (TKey*)next())) {
        TObject* obj = key->ReadObj();
        if (!obj) continue;

        if (obj->InheritsFrom(TH1::Class())) {
            Plot_Hist((TH1*)obj);
        } else if (obj->InheritsFrom(TH2::Class())) {
            Plot_Hist((TH2*)obj);
        }
    }

    outFile->Close();
    inFile->Close();
    delete inFile;
    delete outFile;

    std::cout << "All histograms plotted and saved to " << outRootName 
              << " and plots/pdfs/" << std::endl;

    return 0;
}

