import ROOT
import os

path1 = "runs/run_ttbar_lep_id_test_MEDIUM_v0_April01_2026_1120/plots/"
path2 = "runs/run_ttbar_lep_id_test_TIGHT_v0_April01_2026_1054/plots/"

f1 = ROOT.TFile.Open(path1+"output_2DYields.root")
f2 = ROOT.TFile.Open(path2+"output_2DYields.root")
os.system("cp "+path1+"../flattened.json runs/diff/")
fout = ROOT.TFile.Open("runs/diff/diff.root", "RECREATE")
ROOT.gROOT.SetBatch(True)
ROOT.gStyle.SetOptTitle(0)

def get_hist_from_canvas(can):
    for obj in can.GetListOfPrimitives():
        if obj.InheritsFrom("TH2"):
            return obj
    return None

for key in f1.GetListOfKeys():
    name = key.GetName()

    obj1 = f1.Get(name)
    obj2 = f2.Get(name)

    if not obj1 or not obj2:
        print(f"Skipping {name} (missing in one file)")
        continue

    if not obj1.InheritsFrom("TCanvas"):
        continue

    h1 = get_hist_from_canvas(obj1)
    h2 = get_hist_from_canvas(obj2)

    if not h1 or not h2:
        print(f"No histogram in {name}")
        continue

    # Clone and subtract
    hdiff = h1.Clone(f"{h1.GetName()}_diff")
    hdiff.Add(h2, -1)

    # Create new canvas
    #c = ROOT.TCanvas(name, name, 800, 600)
    c = obj1.Clone(name + "_diff_canvas")
    c.cd()
    hdiff.Draw("COLZ TEXT")

    fout.cd()
    c.Write()
    c.SaveAs("runs/diff/plots/pdfs/"+c.GetName().replace("can_","")+".pdf")

fout.Close()
