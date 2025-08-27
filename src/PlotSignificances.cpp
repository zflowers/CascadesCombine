// plotSignificance.cpp
#include "PlottingHelpers.h"

int main(int argc, char** argv) {
    string inputFile, histCfg, processCfg, binsCfg;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-i" || arg == "--input") {
            if (i + 1 < argc) inputFile = argv[++i];
            else { std::cerr << "[ERROR] Missing " << arg << std::endl; return 1; }
        }
        else if (arg == "-h" || arg == "--hist") {
            if (i + 1 < argc) histCfg = argv[++i];
            else { std::cerr << "[ERROR] Missing " << arg << std::endl; return 1; }
        }
        else if (arg == "-d" || arg == "--process") {
            if (i + 1 < argc) processCfg = argv[++i];
            else { std::cerr << "[ERROR] Missing " << arg << std::endl; return 1; }
        }
        else if (arg == "-b" || arg == "--bins") {
            if (i + 1 < argc) binsCfg = argv[++i];
            else { std::cerr << "[ERROR] Missing " << arg << std::endl; return 1; }
        }
        else if (arg == "--help") {
            std::cout << "[plotSignificance] Usage: " << argv[0]
                      << " -i <txtfile> -h <hist.yaml> -d <process.yaml> -b <bins.yaml>\n";
            return 0;
        }
        else {
            std::cerr << "[ERROR] Unknown arg " << arg << std::endl;
            return 1;
        }
    }

    if (inputFile.empty()) {
        std::cerr << "[ERROR] No input text file provided.\n";
        return 1;
    }

    // --------------------------------------------------
    // Extract bin names from input filename
    // Example: output/Significance_datacards_Example1__Example2__SuperBin2L.txt
    // -> we want Example1, Example2, SuperBin2L
    // --------------------------------------------------
    std::string baseName = inputFile.substr(inputFile.find_last_of("/\\") + 1);
    std::string noExt = baseName.substr(0, baseName.find_last_of('.'));

    std::vector<std::string> binNames;
    {
        std::string prefix = "Significance_datacards_";
        size_t pos = noExt.find(prefix);
        if (pos != std::string::npos) {
            std::string binsPart = noExt.substr(pos + prefix.size()); // "Example1__Example2__SuperBin2L"
            std::stringstream ss(binsPart);
            std::string token;
            size_t start = 0;
            while (true) {
                size_t idx = binsPart.find("__", start);
                if (idx == std::string::npos) {
                    binNames.push_back(binsPart.substr(start));
                    break;
                }
                binNames.push_back(binsPart.substr(start, idx - start));
                start = idx + 2;
            }
        }
    }

    // Build a joined string for output naming
    std::string binTag;
    for (size_t i = 0; i < binNames.size(); ++i) {
        if (i > 0) binTag += "__";
        binTag += binNames[i];
    }
    if (binTag.empty()) binTag = "UnknownBins";

    // --------------------------------------------------
    // Read significance values from file
    // --------------------------------------------------
    std::ifstream ifs(inputFile);
    if (!ifs.is_open()) {
        std::cerr << "Failed to open " << inputFile << "\n";
        return 1;
    }

    std::vector<std::string> procs;
    std::vector<double> vals;
    std::string line;
    while (std::getline(ifs, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        std::string proc;
        double sig;
        if (!(iss >> proc >> sig)) continue; // skip malformed lines
        procs.push_back(proc);
        vals.push_back(sig);
    }
    ifs.close();

    if (procs.empty()) {
        std::cerr << "No data read from file.\n";
        return 1;
    }

    // --------------------------------------------------
    // Setup output directory
    // --------------------------------------------------
    if (outputDir.empty()) {
        outputDir = "output/";
    }
    if (outputDir.back() != '/') outputDir += '/';
    gSystem->mkdir(outputDir.c_str(), kTRUE);
    gSystem->mkdir((outputDir + "pdfs").c_str(), kTRUE);

    // --------------------------------------------------
    // Load pretty-name/color maps
    // --------------------------------------------------
    loadFormatMaps();

    // --------------------------------------------------
    // Draw graph
    // --------------------------------------------------
    int N = (int)procs.size();
    TCanvas *c = new TCanvas(("c_" + binTag).c_str(), ("Significances_" + binTag).c_str(), 1200, 700);
    c->SetLeftMargin(0.08);
    c->SetRightMargin(0.04);
    c->SetBottomMargin(0.12);
    c->SetTopMargin(0.08);
    c->SetGridx(); c->SetGridy();
    gStyle->SetOptStat(0);
    gStyle->SetOptTitle(0);

    TH1F *hFrame = new TH1F("hFrame", "Significance", N, 0.5, N + 0.5);
    hFrame->SetMinimum(0.0);
    hFrame->SetMaximum(*std::max_element(vals.begin(), vals.end()) * 1.3 + 1e-6);
    hFrame->GetXaxis()->SetLabelSize(0.035);
    for (int i = 0; i < N; ++i) {
        std::string label = procs[i];
        auto it = m_Title.find(procs[i]);
        if (it != m_Title.end()) label = it->second;
        hFrame->GetXaxis()->SetBinLabel(i + 1, label.c_str());
    }
    hFrame->GetXaxis()->SetTitle("Signal Process");
    hFrame->GetXaxis()->SetTitleOffset(1.12);
    hFrame->GetXaxis()->CenterTitle();
    hFrame->GetYaxis()->SetTitle("Exp. Significance");
    hFrame->GetYaxis()->SetTitleOffset(1.03);
    hFrame->GetYaxis()->CenterTitle();
    hFrame->Draw();

    TGraph *g = new TGraph(N);
    for (int i = 0; i < N; ++i) {
        g->SetPoint(i, i + 1, vals[i]);
    }
    g->SetMarkerStyle(20);
    g->SetMarkerSize(1.1);
    g->SetLineWidth(2);
    g->Draw("P SAME");

    TLatex tex;
    tex.SetNDC(false);
    tex.SetTextFont(42);
    tex.SetTextSize(0.032);
    tex.SetTextAlign(12);

    double xmin = hFrame->GetXaxis()->GetXmin();
    double xmax = hFrame->GetXaxis()->GetXmax();
    double dx = (xmax - xmin) * 0.02;
    for (int i = 0; i < N; ++i) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%.3f", vals[i]);
        double yOffset = (hFrame->GetMaximum() - hFrame->GetMinimum()) * 0.02;
        tex.DrawLatex(i + 1 + dx, vals[i] + yOffset, buf);
    }

    TLatex l; l.SetNDC(); l.SetTextFont(42);
    l.SetTextSize(0.04);
    l.DrawLatex(0.085, 0.938, "#bf{CMS} Simulation Preliminary");

    // --------------------------------------------------
    // Save outputs with binTag in names
    // --------------------------------------------------
    std::string rootName = outputDir + "Significance_" + binTag + ".root";
    std::string pdfName  = outputDir + "pdfs/Significance_" + binTag + ".pdf";

    TFile fout(rootName.c_str(), "RECREATE");
    hFrame->Write();
    g->Write("gSignificance");
    c->Write();
    fout.Close();

    gErrorIgnoreLevel = 1001;
    c->SaveAs(pdfName.c_str());
    gErrorIgnoreLevel = 0;

    std::cout << "[PlotSignificances] Wrote ROOT file: " << rootName << " and PDF: " << pdfName << "\n";
    return 0;
}
