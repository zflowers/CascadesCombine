
#import matplotlib.pyplot as plt
#import mplhep as hep

'''
hep.style.use("CMS")
fig, ax = plt.subplots()
hep.cms.label("Preliminary", ax=ax, loc=0)
plt.show()
'''

import cmsstyle as CMS
CMS.SetExtraText("Preliminary")
#CMS.SetLumi(138)
CMS.SetLumi(None, run=None)
CMS.SetEnergy(None, unit="")
canv = CMS.cmsCanvas('', 100, 300, 5, 85, 'm_{( #tilde{#chi}^{#pm}_{1})} [GeV]', '#Delta m(#tilde{#chi}^{#pm}_{1}, #tilde{#chi}^{0}_{1}) [GeV]', extraSpace=0.01, iPos=0)

import ROOT as rt
rt.gROOT.SetBatch(True)

oldRJRFile = rt.TFile.Open("B135_bugfix16_TChipmWWSuper_xsec_smooth_canv.root")

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

newRJRFile = rt.TFile.Open("TChiWW_Limits_Combined.root")

exp_newRJR = newRJRFile.Get("gr_mid")
exp_newRJR.SetLineStyle( 7 )
exp_newRJR.SetLineColor( rt.kMagenta )
exp_newRJR.SetLineWidth( 3 )
exp_newRJR.Draw("same")

title2=  "#tilde{#chi}_{1}^{+} #tilde{#chi}_{1}^{-} #rightarrow WW #tilde{#chi}_{1}^{0}#tilde{#chi}_{1}^{0}"
legend = rt.TLegend(0.65,0.73,0.9,0.83)
#legend.SetHeader(title2,"C")
legend.AddEntry(obs_oldRJR,"SUS-23-003","l")
legend.AddEntry(exp_newRJR,"NPS-26-001","l")
legend.Draw()

l = rt.TLatex()
l.SetTextFont(42)
l.SetNDC()
l.SetTextSize(0.035)
l.SetTextFont(42)
l.DrawLatex(0.67, 0.86,title2)

line = rt.TLine()
line.SetLineColor(rt.kBlack)
line.SetLineWidth(1)
line.SetLineStyle(1)
line.DrawLineNDC(0.66,0.71, 0.7, 0.71)

line.SetLineColor(rt.kBlack)
line.SetLineWidth(1)
line.SetLineStyle(2)
line.DrawLineNDC(0.66,0.67, 0.7, 0.67)
#line.SetLineStyle(3)
#line.SetLineColor(rt.kOrange)
l.DrawLatex(0.71,0.7,"observed")
l.DrawLatex(0.71,0.66,"expected")

canv.SaveAs("limit_comp_TChiWW_NPS.pdf")
