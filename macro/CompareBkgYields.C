#include <fstream>
#include <iostream>
#include <vector>
#include <string>

#include "TCanvas.h"
#include "TGraph.h"
#include "TLine.h"
#include "TAxis.h"
#include "TApplication.h"

void CompareBkgYields(const std::string& fileA,
                     const std::string& fileB)
{
    std::ifstream finA(fileA);
    std::ifstream finB(fileB);

    if (!finA.is_open() || !finB.is_open()) {
        std::cerr << "Failed to open input files\n";
        return;
    }

    std::vector<double> xvals;
    std::vector<double> yvals;

    int binnumA, binnumB;
    std::string binnameA, binnameB;
    double eventsA, errA;
    double eventsB, errB;
    int dataA, dataB;
    int maxEvents = 5000;

    while (finA >> binnumA >> binnameA >> eventsA >> errA >> dataA &&
           finB >> binnumB >> binnameB >> eventsB >> errB >> dataB)
    {
        if (binnameA != binnameB) {
            std::cerr << "Bin mismatch:\n"
                      << "  " << binnameA << "\n"
                      << "  " << binnameB << "\n";
            return;
        }
        if(eventsA > maxEvents || eventsB > maxEvents) continue;

        //xvals.push_back(eventsA);
        //yvals.push_back(eventsB);
        //yvals.push_back((eventsB - eventsA)/eventsA*100.);

        xvals.push_back(errA);
        yvals.push_back(100.*(errB - errA)/errA);
    }

    TCanvas *c = new TCanvas("c", "Comparison", 800, 800);

    TGraph *g = new TGraph(xvals.size(),
                           xvals.data(),
                           yvals.data());

    //g->SetTitle("Background Yield Comparison;File A Yield;File B Yield");
    g->SetTitle("Background Yield Comparison;File A Yield;Percent Change");
    g->SetMarkerStyle(20);
    g->SetMarkerSize(0.8);

    g->Draw("AP");

    double xmin = std::min(g->GetXaxis()->GetXmin(),
                           g->GetYaxis()->GetXmin());
    double xmax = std::max(g->GetXaxis()->GetXmax(),
                           g->GetYaxis()->GetXmax());

    //TLine *line = new TLine(xmin, xmin, xmax, xmax);
    TLine *line = new TLine(xmin, 0., xmin, xmax);
    line->SetLineStyle(2);
    line->Draw();

    c->SaveAs("yield_comparison.pdf");
    gApplication->Terminate();
}
