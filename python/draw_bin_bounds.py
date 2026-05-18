#!/usr/bin/env python3

import os
import ROOT

ROOT.gROOT.SetBatch(True)

# ============================================================
# Configuration
# ============================================================

INPUT_ROOT = "runs/run_ANPlots_v15_May07_2026_1213/plots/output_final_hadded.root"
OUTPUT_DIR = "runs/run_ANPlots_v15_May07_2026_1213/plots/pdfs/plots_with_boundaries"

# ------------------------------------------------------------
# Bin boundary definitions
# ------------------------------------------------------------

BOUNDARIES = {

    # ========================================================
    # 2L : RISRLEP vs Mperp_LEP
    # ========================================================
    ("2L", "RISRLEP_vs_Mperp_LEP"): {
        "x_bins": [0.7, 0.75, 0.8, 0.85, 0.9, 0.95, 1.0],

        # y boundaries per x-bin
        # key = (xmin, xmax)
        "y_bins": {
            (0.7, 0.75): [25, 35],
            (0.75, 0.8): [20, 30],
            (0.8, 0.85): [15, 30],
            (0.85, 0.9): [15, 25],
            (0.9, 0.95): [10, 20],
            (0.95, 1.0): [5, 15],
        }
    },

    # ========================================================
    # 3L : RISRLEP vs Mperp_LEP
    # ========================================================
    ("3L", "RISRLEP_vs_Mperp_LEP"): {
        "x_bins": [0.7, 0.8, 0.9, 1.0],

        "y_bins": {
            (0.7, 0.8): [40],
            (0.8, 0.9): [35],
            (0.9, 1.0): [30],
        }
    },

    # ========================================================
    # 2L : RISRLEP vs PTISRLEP
    # ========================================================
    ("2L", "RISRLEP_vs_PTISRLEP"): {
        "x_bins": [0.7, 0.75, 0.8, 0.85, 0.9, 0.95, 1.0],

        "y_bins": {
            (0.7, 0.75): [250, 350],
            (0.75, 0.8): [250, 350],
            (0.8, 0.85): [250, 350],
            (0.85, 0.9): [250, 350],
            (0.9, 0.95): [250, 350],
            (0.95, 1.0): [250, 350],
        }
    },

    # ========================================================
    # 3L : RISRLEP vs PTISRLEP
    # ========================================================
    ("3L", "RISRLEP_vs_PTISRLEP"): {
        "x_bins": [0.7, 0.8, 0.9, 1.0],

        "y_bins": {
            (0.7, 0.8): [200, 300],
            (0.8, 0.9): [200, 300],
            (0.9, 1.0): [200, 300],
        }
    },
}

# ============================================================
# Helper functions
# ============================================================

def determine_channel(canvas_name):
    if "can_2L__" in canvas_name:
        return "2L"
    elif "can_3L__" in canvas_name:
        return "3L"
    return None


def determine_variable(canvas_name):
    if "RISRLEP_vs_Mperp_LEP" in canvas_name:
        return "RISRLEP_vs_Mperp_LEP"
    elif "RISRLEP_vs_PTISRLEP" in canvas_name:
        return "RISRLEP_vs_PTISRLEP"
    return None


def get_main_hist(canvas):
    """
    Find the first TH2 object in the canvas.
    """
    for obj in canvas.GetListOfPrimitives():
        if obj.InheritsFrom("TH2"):
            return obj

    return None


def draw_boundaries(canvas, hist, config):

    # color to shade regions not passing preselection
    #shaded_color = ROOT.kGray + 1
    shaded_color = ROOT.kRed
    shaded_percent = 0.3

    x_axis = hist.GetXaxis()
    y_axis = hist.GetYaxis()

    y_min = y_axis.GetXmin()
    y_max = y_axis.GetXmax()

    lines = []
    x_bins = config["x_bins"]
    x_bin_min = x_bins[0]

    # --------------------------------------------------------
    # Shade excluded region (x < x_bin_min)
    # --------------------------------------------------------
    if x_axis.GetXmin() < x_bin_min:
        xbox = ROOT.TBox(
            x_axis.GetXmin(),
            y_min,
            x_bin_min,
            y_max
        )
        # Gray shading
        xbox.SetFillColorAlpha(shaded_color, shaded_percent)

        xbox.SetLineWidth(0)
        xbox.SetFillStyle(1001)
        xbox.Draw("same")

        lines.append(xbox)

    # --------------------------------------------------------
    # Vertical boundaries (RISR)
    # --------------------------------------------------------

    for x in x_bins[1:-1]:
        line = ROOT.TLine(x, y_min, x, y_max)
        line.SetLineStyle(2)  # dashed
        line.SetLineWidth(2)
        line.Draw("same")
        lines.append(line)

    x_plot_min = x_axis.GetXmin()
    x_plot_max = x_axis.GetXmax()

    # --------------------------------------------------------
    # Shade excluded low-y region
    # --------------------------------------------------------

    y_thresholds = [
        yvals[0]
        for yvals in config["y_bins"].values()
    ]

    global_ymin = min(y_thresholds)

    if y_axis.GetXmin() < global_ymin:

        shade_xmin = x_bin_min if x_axis.GetXmin() < x_bin_min else min(
            xmin for (xmin, xmax) in config["y_bins"]
        )

        ybox = ROOT.TBox(
            shade_xmin,
            y_axis.GetXmin(),
            x_axis.GetXmax(),
            global_ymin
        )

        ybox.SetFillColorAlpha(shaded_color, shaded_percent)
        ybox.SetFillStyle(1001)
        ybox.SetLineWidth(0)
        ybox.Draw("same")

        lines.append(ybox)

    # --------------------------------------------------------
    # Horizontal boundaries
    # --------------------------------------------------------

    for (xmin, xmax), yvals in config["y_bins"].items():

        for y in yvals:
            draw_xmin = x_bin_min if x_plot_min < x_bin_min else xmin

            line = ROOT.TLine(draw_xmin, y, xmax, y)
            line.SetLineStyle(2)
            line.SetLineWidth(2)
            line.Draw("same")

            lines.append(line)

    # Prevent garbage collection
    canvas._boundary_lines = lines


# ============================================================
# Main
# ============================================================

os.makedirs(OUTPUT_DIR, exist_ok=True)

f = ROOT.TFile.Open(INPUT_ROOT)

if not f or f.IsZombie():
    raise RuntimeError(f"Could not open ROOT file: {INPUT_ROOT}")

keys = f.GetListOfKeys()

for key in keys:

    obj = key.ReadObj()

    if not obj.InheritsFrom("TCanvas"):
        continue

    canvas = obj
    canvas_name = canvas.GetName()

    channel = determine_channel(canvas_name)
    variable = determine_variable(canvas_name)

    if channel is None or variable is None:
        continue

    config_key = (channel, variable)

    if config_key not in BOUNDARIES:
        print(f"[WARNING] No boundary config for {canvas_name}")
        continue

    print(f"Processing: {canvas_name}")

    hist = get_main_hist(canvas)

    if hist is None:
        print(f"[WARNING] No TH2 found in {canvas_name}")
        continue

    canvas.cd()
    canvas.SetGridx(0)
    canvas.SetGridy(0)
    ROOT.gStyle.SetOptTitle(0)
    ROOT.gStyle.SetOptStat(0)
    draw_boundaries(canvas, hist, BOUNDARIES[config_key])

    out_dir = os.path.join(OUTPUT_DIR, variable)
    os.makedirs(out_dir, exist_ok=True)

    out_pdf = os.path.join(out_dir, f"{canvas_name}.pdf")

    canvas.SaveAs(out_pdf)

