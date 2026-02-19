#!/usr/bin/env python3
"""
cleanPlotImpacts.py

A minimal, modern impacts plotting script (ROOT) for combineTool JSON outputs.

Examples:
  python cleanPlotImpacts.py -j impacts.json -s Btag Run2_PTISR --output myImpacts
  python cleanPlotImpacts.py -j impacts.json --all --sort impact_r
"""

import argparse
import json
import numpy as np
import os
from math import fabs
from ROOT import (
    TCanvas, TGraphAsymmErrors, TH1F, TLatex, TLine,
    gStyle, TLegend
)
import ROOT

def shorten(label, n=40):
    if len(label) <= n:
        return label
    return label[:n-3] + "..."

def load_json(path):
    with open(path, 'r') as f:
        return json.load(f)

def select_params(data, substrings, use_all=False):
    sel = []
    for p in data.get('params', []):
        name = p.get('name','')
        if use_all:
            sel.append(p)
        else:
            for s in substrings:
                if s in name:
                    sel.append(p)
                    break
    return sel

def compute_values(p):
    # Expect p['fit'] to be [low, center, high]
    fit = p.get('fit', [])
    if len(fit) >= 3:
        low, cen, high = fit[0], fit[1], fit[2]
    elif len(fit) == 2:
        low, cen = fit[0], fit[1]
        high = cen
    elif len(fit) == 1:
        low = cen = high = fit[0]
    else:
        low = cen = high = 0.0
    err_lo = cen - low
    err_hi = high - cen
    # protect against tiny negative rounding
    if err_lo < 0: err_lo = 0.0
    if err_hi < 0: err_hi = 0.0
    return cen, err_lo, err_hi

def make_graph(params, label_maxlen=40, marker_style=20):
    N = len(params)
    if N == 0:
        raise ValueError("No parameters to plot")

    xs = np.arange(1, N+1, dtype=np.float64)  # 1..N for TH1 bin labels
    ys = np.zeros(N, dtype=np.float64)
    xlo = np.zeros(N, dtype=np.float64)
    xhi = np.zeros(N, dtype=np.float64)
    ylo = np.zeros(N, dtype=np.float64)
    yhi = np.zeros(N, dtype=np.float64)
    labels = []

    for i, p in enumerate(params):
        cen, err_lo, err_hi = compute_values(p)
        ys[i] = float(cen)
        ylo[i] = float(err_lo)
        yhi[i] = float(err_hi)
        labels.append(shorten(p.get('name',''), label_maxlen))

    # Graph expects numpy arrays (double)
    gr = TGraphAsymmErrors(N,
                           xs,
                           ys,
                           xlo, xhi,
                           ylo, yhi)
    gr.SetMarkerStyle(marker_style)
    gr.SetMarkerSize(0.9)
    gr.SetLineWidth(1)
    return gr, labels

def draw_plot(gr, labels, params, outbase, title="", ytitle="postfit value #pm error"):
    N = len(labels)
    # Canvas & style
    gStyle.SetOptStat(0)
    cv = TCanvas(outbase, outbase, 1400, 600)
    cv.SetLeftMargin(0.14)
    cv.SetRightMargin(0.06)
    cv.SetBottomMargin(0.28)
    cv.SetTopMargin(0.06)

    # create an empty histogram to draw axis and set bin labels
    h = TH1F("h_axis", "", N, 0.5, N + 0.5)
    h.GetYaxis().SetTitle(ytitle)
    h.GetYaxis().SetTitleSize(0.045)
    h.GetYaxis().SetTitleOffset(1.05)
    h.GetYaxis().SetLabelSize(0.04)
    # X labels
    for i, lab in enumerate(labels):
        h.GetXaxis().SetBinLabel(i+1, lab)
    h.GetXaxis().LabelsOption("v")  # vertical labels
    h.GetXaxis().SetLabelSize(0.035)
    h.GetXaxis().SetTitle("")


    # Find y-range from graph
    # we can compute min and max from data
    ys = [compute_values(p)[0] for p in params]
    ylo = [compute_values(p)[1] for p in params]
    yhi = [compute_values(p)[2] for p in params]
    ymins = [y - dlo for y, dlo in zip(ys, ylo)]
    ymaxs = [y + dhi for y, dhi in zip(ys, yhi)]
    ymin = min(ymins) if ymins else -1.0
    ymax = max(ymaxs) if ymaxs else 1.0
    yrange = ymax - ymin
    pad = 0.12 * (yrange if yrange != 0 else 1.0)
    h.SetMinimum(ymin - pad)
    h.SetMaximum(ymax + pad)

    h.Draw("HIST")  # draw axes and labels
    gr.Draw("P same")

    # Draw reference lines: 0 for Gaussian, 1 for Unconstrained if present
    any_gauss = any(p.get('type','').lower() == 'gaussian' for p in params)
    any_uncon = any(p.get('type','').lower() == 'unconstrained' for p in params)
    if any_gauss:
        l0 = TLine(0.5, 0.0, N + 0.5, 0.0)
        l0.SetLineStyle(7)
        l0.Draw("same")
    if any_uncon:
        l1 = TLine(0.5, 1.0, N + 0.5, 1.0)
        l1.SetLineStyle(7)
        l1.Draw("same")

    # legend (optional)
    leg = TLegend(0.78, 0.7, 0.95, 0.88)
    leg.SetFillStyle(0)
    leg.SetBorderSize(0)
    leg.AddEntry(gr, "postfit value #pm error", "p")
    leg.Draw("same")

    # CMS label / info
    l = TLatex()
    l.SetNDC()
    l.SetTextFont(42)
    l.SetTextSize(0.035)
    l.DrawLatex(0.14, 0.96, "#bf{#it{CMS}} work-in-progress")
    if title:
        l.SetTextSize(0.032)
        l.DrawLatex(0.14, 0.92, title)

    cv.Update()
    # Save
    pdfname = outbase + ".pdf"
    rootname = outbase + ".root"
    cv.Print(pdfname)
    cv.Print(rootname)
    print("Saved:", pdfname, rootname)


def main():
    parser = argparse.ArgumentParser(description="Clean ROOT impacts plotter")
    parser.add_argument('-j','--json', required=True, help="input impacts json")
    parser.add_argument('-s','--sys', nargs='+', help="substring(s) to match nuisance names")
    parser.add_argument('--all', action='store_true', help="plot all nuisances")
    parser.add_argument('-o','--output', default="Impacts", help="output base name (no ext)")
    parser.add_argument('--label-len', type=int, default=40, help="max x-axis label length")
    parser.add_argument('--sort', choices=['name','impact_r'], default=None, help="sort nuisances")
    parser.add_argument('--reverse', action='store_true', help="reverse sort order")
    args = parser.parse_args()

    ROOT.gROOT.SetBatch(True)

    data = load_json(args.json)
    if not args.all and not args.sys:
        parser.error("Provide --sys substrings or use --all to plot everything.")

    params = select_params(data, args.sys or [], use_all=args.all)
    if len(params) == 0:
        print("No matching parameters found.")
        return

    # optionally sort
    if args.sort == 'impact_r':
        params = sorted(params, key=lambda p: p.get('impact_r', 0.0), reverse=args.reverse)
    elif args.sort == 'name':
        params = sorted(params, key=lambda p: p.get('name',''), reverse=args.reverse)

    gr, labels = make_graph(params, label_maxlen=args.label_len)
    draw_plot(gr, labels, params, args.output, title=os.path.basename(args.json))

if __name__ == "__main__":
    main()

