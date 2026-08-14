
#import matplotlib.pyplot as plt
#import mplhep as hep

'''
hep.style.use("CMS")
fig, ax = plt.subplots()
hep.cms.label("Preliminary", ax=ax, loc=0)
plt.show()
'''

NPS = True # whether or not to include new NPS results

import cmsstyle as CMS
CMS.SetExtraText("Preliminary")
if NPS:
    CMS.SetLumi(None, run=None)
    CMS.SetEnergy(None, unit="")
else: CMS.SetLumi(138)
canv = CMS.cmsCanvas('', 100, 425, 3, 85, 'm_{( #tilde{#chi}^{#pm}_{1}, #tilde{#chi}^{0}_{2})} [GeV]', '#Delta m(#tilde{#chi}^{#pm}_{1}, #tilde{#chi}^{0}_{1}) [GeV]', extraSpace=0.01, iPos=0)

import ROOT as rt
rt.gROOT.SetBatch(True)

combFile = rt.TFile.Open("HEPData-ins2755433-v1-Figure_12b_contours.root")

# ref on arxiv: https://arxiv.org/pdf/2402.01888
# 2/3l soft: [73] in above https://arxiv.org/pdf/2111.06296
# >= 3l: [74] in above https://arxiv.org/abs/2106.14246

tdirname = "Figure 12b contours"
graphnames = ["Graph1D_y1",
        "Graph1D_y2",
        "Graph1D_y3",
        "Graph1D_y4",
        "Graph1D_y5",
        "Graph1D_y6"]

colors = [rt.kGreen,rt.kGreen, rt.kRed,rt.kRed, rt.kBlack, rt.kBlack]
style = [7, 1 ,7 ,1 ,7, 1] 

graphs=[]
for idx, gname in enumerate(graphnames):
    tg = combFile.Get(tdirname+"/"+gname)
    tg.SetLineStyle( style[idx] )
    tg.SetLineColor( colors[idx] )
    tg.SetLineWidth(3)
    tg.Draw("same")
    graphs.append(tg)


oldRJRFile = rt.TFile.Open("B135_bugfix16_TChiWZSuper_xsec_smooth_canv.root")

exp_oldRJR = oldRJRFile.Get("gr_mid")
obs_oldRJR = oldRJRFile.Get("gr_obs")

exp_oldRJR.SetLineStyle( 7 )
exp_oldRJR.SetLineColor( rt.kBlue )
obs_oldRJR.SetLineStyle( 1 )
obs_oldRJR.SetLineColor( rt.kBlue+2)

exp_oldRJR.SetLineWidth(3)
obs_oldRJR.SetLineWidth(3)

exp_oldRJR.Draw("same")
obs_oldRJR.Draw("same")

newRJRFile = rt.TFile.Open("TChiWZ_Limits_Combined.root")

exp_newRJR = newRJRFile.Get("gr_mid")
exp_newRJR.SetLineStyle( 7 )
exp_newRJR.SetLineColor( rt.kMagenta )
exp_newRJR.SetLineWidth( 3 )
#exp_newRJR.RemovePoint(0)
if NPS: exp_newRJR.Draw("same")

#legendtitle = "#splitline{pp #rightarrow #tilde{#chi}_{2}^{0} #tilde{#chi}_{1}^{#pm}}{#tilde{#chi}_{2}^{0} #rightarrow Z*#tilde{#chi}_{1}^{0}, #tilde{#chi}_{1}^{#pm} #rightarrow W*#tilde{#chi}_{1}^{0}}
title2=  "#tilde{#chi}_{2}^{0} #tilde{#chi}_{1}^{#pm} #rightarrow WZ #tilde{#chi}_{1}^{0}#tilde{#chi}_{1}^{0}"
legend = rt.TLegend(0.21,0.67,0.4,0.85)
#legend.SetHeader(title2,"C")
legend.AddEntry(graphs[1],"SUS-18-004","l")
legend.AddEntry(graphs[3],"SUS-19-012","l")
legend.AddEntry(graphs[5],"SUS-21-008","l")
legend.AddEntry(obs_oldRJR,"SUS-23-003","l")
if NPS: legend.AddEntry(exp_newRJR,"NPS-26-001","l")
legend.Draw()

l = rt.TLatex()
l.SetTextFont(42)
l.SetNDC()
l.SetTextSize(0.035)
l.SetTextFont(42)
l.DrawLatex(0.18, 0.87,title2)


line = rt.TLine()
line.SetLineColor(rt.kBlack)
line.SetLineWidth(1)
line.SetLineStyle(1)
line.DrawLineNDC(0.74,0.87, 0.78, 0.87)


line.SetLineColor(rt.kBlack)
line.SetLineWidth(1)
line.SetLineStyle(2)
line.DrawLineNDC(0.74,0.83, 0.78, 0.83)
#line.SetLineStyle(3)
#line.SetLineColor(rt.kOrange)
l.DrawLatex(0.8,0.86,"observed")
l.DrawLatex(0.8,0.82,"expected")

if NPS: canv.SaveAs("limit_comp_TChiWZ_NPS.pdf")
else: canv.SaveAs("limit_comp_TChiWZ_noNPS.pdf")
