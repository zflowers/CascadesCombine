# based on https://github.com/Jphsx/ChisqDiagnostic/blob/master/fittools.py
import pandas as pd
import numpy as np
from scipy import stats
import glob
import math
import sys
import ROOT as rt
from ROOT import TMath as tm
import csv


def getStatus(df,countThreshold,n_nuisances):
	totalMCpre=0
	totalMCpost=0
	totalD=0
	totalMCpre = df['bprefit'].sum()
	totalMCpost = df['bpostfit'].sum()
	totalD = df['data'].sum()
	print('{0: <12}'.format(" "),'{0: <12}'.format("Expected"), '{0: <12}'.format("Observed"), '{0: <12}'.format("O-E"), '{0: <12}'.format("Nuisances") )
	print('{0: <12}'.format("Prefit"), '{0: <12}'.format(round(totalMCpre)),'{0: <12}'.format(totalD), '{0: <12}'.format(round(totalD-totalMCpre)),'{0: <12}'.format("0"))
	print('{0: <12}'.format("Postfit"),'{0: <12}'.format(round(totalMCpost)), '{0: <12}'.format(totalD),'{0: <12}'.format(round(totalD-totalMCpost)), '{0: <12}'.format(n_nuisances))
    
def getChisq(df, countThreshold,n_nuisances):
	chidf = pd.DataFrame()
	chidf['observed'] = df['data']
	chidf['expected_pre'] = df['bprefit']
	chidf['expected_post'] = df['bpostfit']
	chidf['variance_pre'] = df['bprefit_err']
	chidf['variance_post'] = df['bpostfit_err']
	chidf['variance_pre'] = chidf['variance_pre'].apply(lambda x:x*x)
	chidf['variance_post'] = chidf['variance_post'].apply(lambda x:x*x)

	chidf = chidf.loc[ chidf['expected_pre'] > (10e-5) ]
	TotalBins = chidf.shape[0]
	chipre = chidf.loc[ chidf['expected_pre'] > countThreshold]
	chipost = chidf.loc[ chidf['expected_post'] > countThreshold]
	SelectedBins_pre = chipre.shape[0]
	SelectedBins_post = chipost.shape[0]
    
    #calculate columns for pre chisq
	chipre['diff'] = chipre['observed'] - chipre['expected_pre']
	#chipre['sqdiff'] = chipre.apply(lambda x:(x*x))
	chipre['sqdiff'] = chipre['diff']*chipre['diff']
	#chipre['ratio'] = chipre['sqdiff']/chipre['variance_pre']
	chipre['ratio'] = chipre['sqdiff']/chipre['expected_pre']
	chi2pre = chipre['ratio'].sum()
	chi2preNDF = chi2pre/(float(SelectedBins_pre-n_nuisances))
	pvalue_pre = 1- stats.chi2.cdf(chi2pre,SelectedBins_pre)
    
    #calculate columns for post chisq
	chipost['diff'] = chipost['observed'] - chipost['expected_post']
	#chipost['sqdiff'] = chipost.apply(lambda x:(x*x))
	chipost['sqdiff'] = chipost['diff']*chipost['diff']
	#chipost['ratio'] = chipost['sqdiff']/(chidf['expected_post'] - chidf['variance_post'] )
	chipost['ratio'] = chipost['sqdiff']/chipost['expected_post']
	#chipost['ratio'] = chipost['sqdiff']/chidf['variance_post']
	chi2post = chipost['ratio'].sum()
	chi2postNDF = chi2post/(float(SelectedBins_post-n_nuisances))
	pvalue_post = 1- stats.chi2.cdf(chi2post,SelectedBins_post)
    
	print("Chi2  (O-E)^2 / E")
	print('{0: <12}'.format(" "),'{0: <12}'.format("Chisq"), '{0: <12}'.format("Chisq/NDF"), '{0: <12}'.format("P-val"),end='')
	print('{0: <12}'.format("Selected Bins"), '{0: <12}'.format("Total Bins"), '{0: <12}'.format("Threshold"))
	print('{0: <12}'.format("Prefit"),'{0: <12}'.format(round(chi2pre,1)), '{0: <12}'.format(round(chi2preNDF,1)), '{0: <12}'.format(round(pvalue_pre,2)), '{0: <12}'.format(SelectedBins_pre), '{0: <12}'.format(TotalBins), '{0: <12}'.format(countThreshold))
	print('{0: <12}'.format("Postfit"),'{0: <12}'.format(round(chi2post,1)), '{0: <12}'.format(round(chi2postNDF,1)), '{0: <12}'.format(round(pvalue_post,2)), '{0: <12}'.format(SelectedBins_post), '{0: <12}'.format(TotalBins), '{0: <12}'.format(countThreshold))

def getMyChisq(df, countThreshold,n_nuisances):
    chidf = pd.DataFrame()
    chidf['observed'] = df['data']
    chidf['expected_pre'] = df['bprefit']
    chidf['expected_post'] = df['bpostfit']
    chidf['variance_pre'] = df['bprefit_err']
    chidf['variance_post'] = df['bpostfit_err']
    chidf['variance_pre'] = chidf['variance_pre'].apply(lambda x:x*x)
    chidf['variance_post'] = chidf['variance_post'].apply(lambda x:x*x)
    chidf = chidf.loc[ chidf['expected_pre'] > (10e-5) ]
    TotalBins = chidf.shape[0]
    chipre = chidf.loc[ chidf['expected_pre'] > countThreshold]
    chipost = chidf.loc[ chidf['expected_post'] > countThreshold]
    SelectedBins_pre = chipre.shape[0]
    SelectedBins_post = chipost.shape[0]
    
    #calculate columns for pre chisq
    chipre['diff'] = chipre['observed'] - chipre['expected_pre']
	#chipre['sqdiff'] = chipre.apply(lambda x:(x*x))
    chipre['sqdiff'] = chipre['diff']*chipre['diff']
	#chipre['ratio'] = chipre['sqdiff']/chipre['variance_pre']
    #chipre['ratio'] = chipre['sqdiff']/chipre['expected_pre']
    chipre['ratio'] = chipre['sqdiff'] / (chipre['expected_pre'] + df['bprefit_err']**2)
    chi2pre = chipre['ratio'].sum()
	#chi2preNDF = chi2pre/(float(SelectedBins_pre-n_nuisances))
    chi2preNDF = chi2pre/(float(SelectedBins_pre))
    pvalue_pre = 1- stats.chi2.cdf(chi2pre,SelectedBins_pre)
    
    #calculate columns for post chisq
    chipost['diff'] = chipost['observed'] - chipost['expected_post']
	#chipost['sqdiff'] = chipost.apply(lambda x:(x*x))
    chipost['sqdiff'] = chipost['diff']*chipost['diff']
	#chipost['ratio'] = chipost['sqdiff']/(chidf['expected_post'] - chidf['variance_post'] )
	#chipost['ratio'] = chipost['sqdiff']/chipost['expected_post']
	#chipost['ratio'] = chipost['sqdiff']/chidf['variance_post']
    chipost['ratio'] = chipost['sqdiff'] / (chipost['expected_post'] + df['bpostfit_err']**2)
    chi2post = chipost['ratio'].sum()
    chi2postNDF = chi2post/(float(SelectedBins_post-n_nuisances))
    pvalue_post = 1- stats.chi2.cdf(chi2post,SelectedBins_post - n_nuisances)
    
    print("Chi2  (O-E)^2 / E")
    print('{0: <12}'.format(" "),'{0: <12}'.format("Chisq"), '{0: <12}'.format("Chisq/NDF"), '{0: <12}'.format("P-val"),end='')
    print('{0: <12}'.format("Selected Bins"), '{0: <12}'.format("Total Bins"), '{0: <12}'.format("Threshold"))
    print('{0: <12}'.format("Prefit"),'{0: <12}'.format(round(chi2pre,1)), '{0: <12}'.format(round(chi2preNDF,1)), '{0: <12}'.format(round(pvalue_pre,2)), '{0: <12}'.format(SelectedBins_pre), '{0: <12}'.format(TotalBins), '{0: <12}'.format(countThreshold))
    print('{0: <12}'.format("Postfit"),'{0: <12}'.format(round(chi2post,1)), '{0: <12}'.format(round(chi2postNDF,1)), '{0: <12}'.format(round(pvalue_post,2)), '{0: <12}'.format(SelectedBins_post), '{0: <12}'.format(TotalBins), '{0: <12}'.format(countThreshold))

def logXFactorial( x ):
	SUM=0.
	for xj in range (1,int(x)+1):
		SUM = SUM + np.log(xj)
	return SUM

def ComputeCol( col ):
	d = col.to_numpy()
	c = []
	for x in d:
		c.append(logXFactorial(x))
		
	return np.array(c)
	
def getPLike(df):

	pf = pd.DataFrame()
	pf['data'] = df['data']
	pf['bprefit'] = df['bprefit']
	pf['bpostfit'] = df['bpostfit']
	pf = pf.loc[ pf['bprefit'] > 10e-5 ]

	pf['A1'] = pf['data']*np.log( pf['bprefit'] )
	pf['C1'] = ComputeCol( pf['data'] ).tolist()
	A1 = pf['A1'].sum()
	B1 = pf['bprefit'].sum()
	C1 = pf['C1'].sum()
	
	PrefitLike = A1-B1-C1
	
	pf['A2'] = pf['data']*np.log( pf['bpostfit'] )
	A2 = pf['A2'].sum()
	B2 = pf['bpostfit'].sum()
	
	PostfitLike = A2-B2-C1
	
	print("Poisson LogL")
	print('{0: <12}'.format(" "),'{0: <12}'.format("-LogL"))
	print('{0: <12}'.format("Prefit"),'{0: <12}'.format(PrefitLike))
	
	print('{0: <12}'.format("Postfit"),'{0: <12}'.format(PostfitLike))



def getPoissonPull(MUB,NOBS,SIGMAB,FRACERROR, rg, NGENERATED):
	nuppertail = 0;
	nlowertail = 0;
	for i in range(1,NGENERATED):
		mu = MUB;
		if(FRACERROR > 1.0e-4):
			mu = rg.Gaus(MUB, SIGMAB)  # Choose Poisson mean, mu, from Gaussian with mean and rms of MUB and SIGMAB
        
			n = rg.Poisson(mu)         # Generate Poisson distributed random number, n, based on Poisson mean of mu  
		if(n >= NOBS):
			nuppertail =nuppertail+1;       # Count toys with n exceeding or equal to the observed counts
		if(n <= NOBS):
			nlowertail =nlowertail+1;       # Count toys with n less than or equalt to the observed counts
     
	ppull=0
	err = 0
	if(NOBS >= MUB):
		pvalue = float(nuppertail)/float(NGENERATED)
		ppull = tm.NormQuantile(1.0-pvalue)
		err= math.sqrt(pvalue*(1.0-pvalue)/float(NGENERATED)); 
	else:
  		pvalue = float(nlowertail)/float(NGENERATED)
  		ppull = -tm.NormQuantile(1.0-pvalue)
  		err= math.sqrt(pvalue*(1.0-pvalue)/float(NGENERATED));
   
	return [ppull,err]
    
def getPoissonCol(df):

  # res = df
  # res["MUB"] = df['bpostfit']
  # res["NOBS"] = df['data']
  # res["SIGMAB"] = df['bpostfit_err']
  # res["FRACERROR"] = res["SIGMAB"]/res["MUB"]
	seed = 4359
	rg = rt.TRandom3(seed)
	#NGENERATED = 10000000
	NGENERATED = 1000000
  
	MUBcol = df["bpostfit"].to_numpy()
	NOBScol = df["data"].to_numpy()
	SIGMABcol = df['bpostfit_err'].to_numpy()
	FRACERRORcol = (df['bpostfit_err']/df['bpostfit']).to_numpy()
	c = []
	err = []
	for MUB,NOBS,SIGMAB,FRACERROR in zip(MUBcol,NOBScol,SIGMABcol,FRACERRORcol):
		temp = getPoissonPull(MUB,NOBS,SIGMAB,FRACERROR,rg,NGENERATED)
		c.append(temp[0] )
		err.append(temp[1] )
		
	return [np.array(c), np.array(err)]

def getPull(df, DO_POISSON):

	#adding Poisson treatment, should be able to toggle this

	res = df
	res = res.loc[ res['bprefit'] > (10e-5) ]#remove extraneous bins created by diagnostic 
	res['Pre_O-E/sqrt(E)'] = (res['data'] - res['bprefit'])/ res['bprefit']**(1./2.)
	res['Pull_O-E/sqrt(E+VF)'] = (res['data'] - res['bpostfit'])/ ( res['bpostfit'] + res['bpostfit_err']**2 )**(1./2.)
	res['Post_O-E/sqrt(E)'] = (res['data'] - res['bpostfit'])/  res['bpostfit']**(1./2.)
	
	if DO_POISSON == True:
		temp = getPoissonCol(res)
		res['PoissonZscore'] = (temp[0]).tolist()
		res['PoissonErr'] = (temp[1]).tolist()
		res = res[['RegionName','BinNumber','Pre_O-E/sqrt(E)','Pull_O-E/sqrt(E+VF)','PoissonZscore','PoissonErr','data','bprefit','bpostfit']]
	else:
		res = res[['RegionName','BinNumber','Pre_O-E/sqrt(E)','Pull_O-E/sqrt(E+VF)','Post_O-E/sqrt(E)','data','bprefit','bpostfit']]
    
	print('Top 10 Leading normalized residuals sorted by pull O-E/sqrt(E+VF)') 
	res = res.sort_values(by='Pull_O-E/sqrt(E+VF)', key=pd.Series.abs, ascending=False)
	print(res[:10])
	#print(res)
	if DO_POISSON == True:
		print('Top 10 Leading Poisson Pulls')
		res = res.sort_values(by='PoissonZscore', key=pd.Series.abs, ascending=False)
		print(res[:10])
	else:
		print('Top 10 Leading normalized residuals sorted by pre O-E/sqrt(E)')    
		res = res.sort_values(by='Pre_O-E/sqrt(E)',key=pd.Series.abs, ascending=False)
		print(res[:10])
	
	
	return res
    
def getPlots(df,tfile,tag,DO_POISSON):
	rt.gStyle.SetOptFit(1)
	hpre = rt.TH1D("hpre"+tag, "Prefit Pull; O-E/#sqrt{E};N bins",40,-10,10)
	hpost = rt.TH1D("hpost"+tag, "Postfit Pull; O-E/#sqrt{E+VF};N bins",40, -10,10)
	hpoisson = rt.TH1D("hpois"+tag," Postfit Poisson Pull; P;N bins",40,-10,10)
	
	
	res = df	

	#res = res.loc[ res['bprefit'] > (10e-5) ]
	#res['Pull_O-E/sqrt(E+VF)'] = (res['data'] - res['bpostfit'])/ ( res['bpostfit'] + res['bpostfit_err']**2 )**(1./2.)
	#res['Pre_O-E/sqrt(E)'] = (res['data'] - res['bprefit'])/ res['bprefit']**(1./2.)
	#res = res.dropna()
	for pull in res['Pull_O-E/sqrt(E+VF)'].to_numpy():
		hpost.Fill(pull)
	for pull in res['Pre_O-E/sqrt(E)'].to_numpy():
		hpre.Fill(pull)
	if DO_POISSON == True:
		for pull in res['PoissonZscore'].to_numpy():
			hpoisson.Fill(pull)
		
	hpost.Fit('gaus',"Q")
	hpre.Fit('gaus',"Q")
	hpoisson.Fit('gaus',"Q")
	tfile.WriteTObject(hpost)
	tfile.WriteTObject(hpre)
	if DO_POISSON == True:
		tfile.WriteTObject(hpoisson)
	
	
def doGaussCounts(sigmaCuts, res):
	NTRIALS = 100
	ensembledf = pd.DataFrame(columns = ['Trial_Num', 'Sigma Cut', 'Average True Counts '+str(NTRIALS)+' Trials'])
	for trial in range(0,NTRIALS):
		gaus = np.random.normal(0, 1.0, size=res.shape[0])
		gaus = np.absolute(gaus)
		#print(trial,gaus)
		for cut in sigmaCuts:
			ensembledf = ensembledf.append({'Trial_Num' : trial, 'Sigma Cut' : cut, 'Average True Counts '+str(NTRIALS)+' Trials' : gaus[gaus >cut].size }, ignore_index = True)
			
	ensembledf = ensembledf.groupby(['Sigma Cut']).mean()
	ensembledf = ensembledf.reset_index()
	ensembledf = ensembledf.drop(columns='Trial_Num')
	#print(ensembledf)
	return(ensembledf)
	
def getSigmaCounts(df, DO_POISSON):
	res = df
	#res = res.loc[ res['bprefit'] > (10e-5) ]#remove extraneous bins created by diagnostic 
	#res['Pre_O-E/sqrt(E)'] = (res['data'] - res['bprefit'])/ res['bprefit']**(1./2.)
	#res['Pull_O-E/sqrt(E+VF)'] = (res['data'] - res['bpostfit'])/ ( res['bpostfit'] + res['bpostfit_err']**2 )**(1./2.)
	#res['Post_O-E/sqrt(E)'] = (res['data'] - res['bpostfit'])/  res['bpostfit']**(1./2.)
	#res = res[['RegionName','BinNumber','Pre_O-E/sqrt(E)','Pull_O-E/sqrt(E+VF)','Post_O-E/sqrt(E)','data','bprefit','bpostfit']]
	#res = res.dropna()
	
	#count pulls
	#[0,2] [>2, >2.5, >3, >3.5, >4.0, >5.0, >6.0]
	sigmaCuts=[0,1,1.5,2,2.5,3,3.5,4,5,6]
	sigmaCounts = []
	
	for cut in sigmaCuts:
		if DO_POISSON == True:
			sigmaCounts.append(res.loc[ res['PoissonZscore'].abs() > cut ].shape[0])
		else:
			sigmaCounts.append( res.loc[ res['Pull_O-E/sqrt(E+VF)'].abs() > cut ].shape[0] )
	
	pstring = ""
	if DO_POISSON == True:
		pstring = "Poisson Corrected"
	print(pstring+" Pull Counts")
	#print('{0: <12}'.format("Sigma Cut"), '{0: <12}'.format("N bins > cut") )
	#for cut,count in zip(sigmaCuts, sigmaCounts):
	#	print('{0: <12}'.format(cut), '{0: <12}'.format(count) )
		
	ensembledf = doGaussCounts(sigmaCuts, res)
	ensembledf['N bins > cut'] = sigmaCounts
	print(ensembledf)

def analyzedf(df, countThreshold, n_nuisances, tfile, tag, DO_POISSON, do_myChiSq):
    getStatus(df,countThreshold,n_nuisances)
    if do_myChiSq:
	    getMyChisq(df,countThreshold,n_nuisances)
    else:
	    getChisq(df,countThreshold,n_nuisances)
    getPLike(df)
    pulldf = getPull(df,DO_POISSON)
    getSigmaCounts(pulldf,DO_POISSON)
    getPlots(pulldf,tfile,tag,DO_POISSON)
    #getSigmaCounts(df,DO_POISSON)
    #getPlots(df, tfile, tag)
    return pulldf
    
