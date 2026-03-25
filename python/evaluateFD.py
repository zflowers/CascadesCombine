# based on: https://github.com/Jphsx/ChisqDiagnostic/blob/master/evaluateFitDiagnostic.py
import pandas as pd
import numpy as np
from scipy import stats
import glob
import math
import sys
import csv
import ROOT as rt
import fittools as ft
pd.set_option('display.max_colwidth',None)
pd.set_option('display.max_columns', None)
pd.set_option('expand_frame_repr', False)
pd.set_option('display.max_rows', None)
rt.gROOT.SetBatch(True)

##### Input FD csv file and important params#####
countThreshold = 5 #Chisq min stat criteria
filename = sys.argv[1]
outfile = sys.argv[3]
n_nuisances = float(sys.argv[2])
DO_POISSON = True
do_MyChiSq = False
#############################

df = pd.read_csv(filename, index_col=None, header=0, delimiter=" ")
#df manipulation for separation
df['RegionSplit'] = df['RegionName'].str.split('_')
df['Run']   = df['RegionSplit'].str[1]   # Run2 / Run3
df['NLep']  = df['RegionSplit'].str[2]   # 2L / 3L / 4L
df['Rank']  = df['RegionSplit'].str[3]   # Bronze / Silver / Gold
#print(df)

histFile = rt.TFile(outfile,"RECREATE")
rt.gStyle.SetOptFit(1)

print("------------------Evaluating all bins------------------")
ft.analyzedf(df,countThreshold,n_nuisances, histFile, "allJ", DO_POISSON, False)

#print("------------------Evaluating all bins------------------")
#ft.analyzedf(df,countThreshold,n_nuisances, histFile, "allZ", DO_POISSON, True)

Run2 = df.loc[ df['Run'] == 'Run2' ]
print("------------------Evaluating all Run2 bins------------------")
ft.analyzedf(Run2,countThreshold,n_nuisances, histFile, "Run2", DO_POISSON, do_MyChiSq)

Run3 = df.loc[ df['Run'] == 'Run3' ]
print("------------------Evaluating all Run3 bins------------------")
ft.analyzedf(Run3,countThreshold,n_nuisances, histFile, "Run3", DO_POISSON, do_MyChiSq)

DO_POISSON= False

print("------------------all 2L bins------------------")
L2 = df.loc[ df['NLep'] == '2L' ]
ft.analyzedf(L2,countThreshold,n_nuisances, histFile, "2L", DO_POISSON, do_MyChiSq)

print("------------------all 3L bins------------------", DO_POISSON, do_MyChiSq)
L3 = df.loc[ df['NLep'] == '3L' ]
ft.analyzedf(L3,countThreshold,n_nuisances, histFile, "3L", DO_POISSON, do_MyChiSq)

print("------------------all 4L bins------------------", DO_POISSON, do_MyChiSq)
L4 = df.loc[ df['NLep'] == '4L' ]
ft.analyzedf(L4,countThreshold,n_nuisances, histFile, "4L", DO_POISSON, do_MyChiSq)

print("------------------Evaluating all gold bins------------------")
dfgold = df.loc[ df['Rank'] == 'Gold' ]
ft.analyzedf(dfgold,countThreshold,n_nuisances, histFile,"Gold", DO_POISSON, do_MyChiSq)

print("------------------Evaluating all silver bins------------------")
dfslvr = df.loc[ df['Rank'] == 'Silver' ]
ft.analyzedf(dfslvr,countThreshold,n_nuisances, histFile, "Silver", DO_POISSON, do_MyChiSq)

print("------------------Evaluating all bronze bins------------------")
dfbron = df.loc[ df['Rank'] == 'Bronze' ]
pulldf = ft.analyzedf(dfbron,countThreshold,n_nuisances, histFile, "Bronze", DO_POISSON, do_MyChiSq)

#print(pull.shape)
#print(pull)
#histFile.Write()
histFile.Close()

