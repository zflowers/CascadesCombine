//Code for Fitting Efficiency Curves

#include <TMinuit.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <sys/stat.h>
#include <algorithm>
#include <TMatrixDSym.h>
#include <TGraphErrors.h>
#include <TPaveStats.h>
#include <TLine.h>
#include <TFitResult.h>
#include <Math/ProbFunc.h>
#include <TF1.h>

using namespace std;

void Get_Fit(TGraphAsymmErrors*& gr, vector<TF1*> funcs, vector<int> colors, string outFile, string name);
void Fit_Graph_With_Funcs(TCanvas*& canv, TGraphAsymmErrors*& gr, vector<TF1*> funcs, const vector<int>& colors, string name);
TGraph* Get_Fit_Ratio(double x_min, double x_max, TF1* Bkg_Nominal, TF1* Data_Nominal);
TGraphErrors* Get_Bands(double x_min, double x_max, TF1* Data_Nominal, int N, double& a1, double& a2, double& b1, double& b2, double& c1, double& c2);
void Get_Bands_Ratio_Params(double& a1, double& a2, double& b1, double& b2, double& c1, double& c2, const int& year, const int& electron, const bool& data);
TGraphErrors* Get_Bands_Ratio(double x_min, double x_max, TGraphAsymmErrors* gr, TF1* Bkg_Nominal, TF1* Data_Nominal, double& a1, double& a2, double& b1, double& b2, double& c1, double& c2);
TGraphAsymmErrors* TGAE_Ratio(TGraphAsymmErrors* gr_bkg, TGraphAsymmErrors* gr_data);
std::string get_str_between_two_str(const std::string &s, const std::string &start_delim, const std::string &stop_delim);

string Derive_Output_Dir(const string& fname)
{
    filesystem::path input(fname);
    // Go up from the file -> plots/ -> run_XYZ/, then append trigger_sf/
    filesystem::path outdir = input.parent_path().parent_path() / "trigger_sf";

    if (!filesystem::exists(outdir)) {
        filesystem::create_directories(outdir);
        cout << "Created output directory: " << outdir << endl;
    } else {
        cout << "Output directory: " << outdir << endl;
    }

    return outdir.string() + "/";
}

Double_t Error_Func(Double_t *x, Double_t *par)
{
 return par[0]*TMath::Erf(x[0]-par[1]);
}

Double_t Gaussian_CDF_Func(Double_t *x, Double_t *par)
{
 return par[0]*ROOT::Math::normal_cdf(x[0],par[2],par[1]);
}

Double_t Double_Gaussian_CDF_Func_Add(Double_t *x, Double_t *par)
{
 return par[0]*((TMath::Cos(par[4])*TMath::Cos(par[4]))*ROOT::Math::normal_cdf(x[0],par[2],par[1])+(TMath::Sin(par[4])*TMath::Sin(par[4]))*ROOT::Math::normal_cdf(x[0],par[2]+par[3],par[1]));
}

Double_t Double_Gaussian_CDF_Func_Multi(Double_t *x, Double_t *par)
{
 return par[0]*((TMath::Cos(par[4])*TMath::Cos(par[4]))*ROOT::Math::normal_cdf(x[0],par[2],par[1])+(TMath::Sin(par[4])*TMath::Sin(par[4]))*ROOT::Math::normal_cdf(x[0],par[2]*par[3],par[1]));
}

bool invert_colors = false;

bool fileExists(const std::string& filename)
{
 struct stat buf;
 if (stat(filename.c_str(), &buf) != -1)
 {
  return true;
 }
 return false;
}

TGraphAsymmErrors* get_gr(string cut, string tag)
{
 TCanvas* can = new TCanvas(("temp"+cut+tag).c_str(),("temp"+cut+tag).c_str(),750,500);
 TFile *f = TFile::Open(("Eff_output_"+cut+".root").c_str(), "READ");
 TDirectoryFile* folder = nullptr;
 f->GetObject(tag.c_str(),folder);
 folder->cd();
 TEfficiency* eff = nullptr;
 folder->GetObject("METtrigger",eff);
 eff->Draw("AP");
 can->Update();
 TGraphAsymmErrors* gr = eff->GetPaintedGraph();
 delete can;
 f->Close();
 delete f;
 return gr;
}

TGraphAsymmErrors* TGAE_Ratio(TGraphAsymmErrors* gr_bkg, TGraphAsymmErrors* gr_data)
{
 TGraphAsymmErrors* mg = NULL;
 int N = gr_bkg->GetN();
 if(N != gr_data->GetN()) return mg;
 double xnew[N];
 double ynew[N];
 for(int i=0; i<N; i++)
 {
  double x_bkg,y_bkg,x_data,y_data;
  gr_bkg->GetPoint(i,x_bkg,y_bkg);
  gr_data->GetPoint(i,x_data,y_data);
  if(x_bkg != x_data) return mg;
  xnew[i]=x_bkg;
  ynew[i]=y_data/y_bkg;
 }
 mg = new TGraphAsymmErrors(N,xnew,ynew);
 for(int i=0; i<N; i++)
 {
  double bkg_ey_h = gr_bkg->GetErrorYhigh(i);
  double bkg_ey_l = gr_bkg->GetErrorYlow(i);
  double data_ey_h = gr_data->GetErrorYhigh(i);
  double data_ey_l = gr_data->GetErrorYlow(i);
  double x_bkg,y_bkg,x_data,y_data;
  gr_bkg->GetPoint(i,x_bkg,y_bkg);
  gr_data->GetPoint(i,x_data,y_data);
  mg->SetPointError(i,gr_bkg->GetErrorXlow(i),gr_data->GetErrorXhigh(i),sqrt((data_ey_l*data_ey_l)*((y_data/y_bkg)*(y_data/y_bkg))/(y_data*y_data)+(bkg_ey_l*bkg_ey_l)*((y_data/y_bkg)*(y_data/y_bkg))/(y_bkg*y_bkg)),sqrt((data_ey_h*data_ey_h)*((y_data/y_bkg)*(y_data/y_bkg))/(y_data*y_data)+(bkg_ey_h*bkg_ey_h)*((y_data/y_bkg)*(y_data/y_bkg))/(y_bkg*y_bkg)));
 }
 return mg;
}

TF1* Get_Func_Nominal(const int& year, const int& electron, const bool& data, const double& x_min, const double& x_max)
{
 if(year == 2016)
 {
  if(electron == 0)
  {
   if(data)
   {
    TF1* Nominal = new TF1("Nominal",Gaussian_CDF_Func,x_min,x_max,3);
    Nominal->SetParameter(0,0.99);
    Nominal->SetParameter(1,125.);
    Nominal->SetParameter(2,40.);
    return Nominal;
   }
   else
   {
    TF1* Nominal = new TF1("Nominal",Double_Gaussian_CDF_Func_Multi,x_min,x_max,5);
    Nominal->SetParameter(0,1.);
    Nominal->SetParameter(1,100.);
    Nominal->SetParameter(2,10.);
    Nominal->SetParameter(3,10.);
    Nominal->SetParameter(4,0.5);
    return Nominal;
   }
  }
 }
 std::cout << "[Get_Func_Nominal] returning default func and parameters!" << std::endl;
 TF1* Nominal = new TF1("Nominal",Gaussian_CDF_Func,x_min,x_max,3);
 Nominal->SetParameter(0,0.99);
 Nominal->SetParameter(1,125.);
 Nominal->SetParameter(2,40.);
 return Nominal;
}

void Get_Func_Params(const int& year, const int& electron, const bool& data, string cut, string tag, double& norm, double& mean, double& sigma, double& scale, double& weight)
{
 TGraphAsymmErrors* gr = get_gr(cut, tag);
 TCanvas* can = new TCanvas(("temp"+cut+tag+std::to_string(year)).c_str(),("temp"+cut+tag+std::to_string(year)).c_str(),750,500);
 can->cd();
 gr->Draw("A");
 TF1* func = Get_Func_Nominal(year,electron,data,gr->GetXaxis()->GetXmin(),gr->GetXaxis()->GetXmax()); 
 gr->Fit(func,"EMS+0");
 norm = func->GetParameter(0);
 mean = func->GetParameter(1);
 sigma = func->GetParameter(2);
 if(func->GetNpar() > 3)
 {
  scale = func->GetParameter(3);
  weight = func->GetParameter(4);
 }
 else
 {
std::cout << "year: " << year << " electron: " << electron << " data: " << data << " cut: " << cut << " tag: " << tag << std::endl;
  scale = 0.;
  weight = 0.;
 }
}

void Get_Bands_Ratio_Params(double& a1, double& a2, double& b1, double& b2, double& c1, double& c2, const int& year, const int& electron, const bool& data)
{
  if(year == 2016)
  {
   if(electron == 0)
   {
    a1 = .075e-4;
    a2 = -.15e-4;
    b1 = 225.;
    b2 = 225.;
    c1 = 1.005;
    c2 = 0.992;
   }
   else if(electron)
   {
    a1 = .66e-4;
    a2 = -.35e-4;
    b1 = 220.;
    b2 = 220.;
    c1 = 1.015;
    c2 = 0.985;
   }
   else
   {
    a1 = .4e-5;
    a2 = -.6e-5;
    b1 = 220.;
    b2 = 220.;
    c1 = 1.004;
    c2 = 0.996;
   }
  }
}

double Get_Value(string& line)
{
 size_t comma_pos = line.find(",");
 string value = line.substr(0,comma_pos);
 line.erase(0,comma_pos+1);
 return std::stod(value);
}

void Get_Band_Params_CSV(string name, double& a1, double& a2, double& b1, double& b2, double& c1, double& c2)
{
  std::ifstream ifile("Parameters.csv");
  if(!ifile.is_open()){
    std::cout << "can't open csv file " << std::endl;
    return;
  }
  string line;
  //discard header line
  getline(ifile,line);
  while(getline(ifile,line)){
   if(line.find(name) == std::string::npos) continue;
   size_t s = line.find(name);
   if(s != std::string::npos) { line.erase(s,(name+",").length()); }
   a1 = Get_Value(line);
   a2 = Get_Value(line);
   b1 = Get_Value(line);
   b2 = Get_Value(line);
   c1 = Get_Value(line);
   c2 = Get_Value(line);
   return;
  }
}

void Get_Fit_Params_CSV(string name, double& norm, double& mean, double& sigma, double& scale, double& weight)
{
  std::ifstream ifile("Parameters.csv");
  if(!ifile.is_open()){
    std::cout << "can't open csv file " << std::endl;
    return;
  }
  string line, values;
  //discard first line
  getline(ifile,line);
  while(getline(ifile,line)){
   if(line.find(name) == std::string::npos) continue;
   size_t s = line.find(name);
   if(s != std::string::npos) { line.erase(s,(name+",").length()); }
   double dum = 0.0; //dummy variable
   //skip variables that we don't need
   dum = Get_Value(line);
   dum = Get_Value(line);
   dum = Get_Value(line);
   dum = Get_Value(line);
   dum = Get_Value(line);
   dum = Get_Value(line);
   dum *= 0.0;
   norm = Get_Value(line);
   mean = Get_Value(line);
   sigma = Get_Value(line);
   scale = Get_Value(line);
   weight = Get_Value(line);
   return;
  }
}

TGraphErrors* Get_Bands_Ratio(double x_min, double x_max, TGraphAsymmErrors* gr, TF1* Bkg_Nominal, TF1* Data_Nominal, double& a1, double& a2, double& b1, double& b2, double& c1, double& c2)
{
 int N = 1000;
 TGraphErrors* gr_bands_ratio = new TGraphErrors(N);
 double x = 0.;
 double y_upper_i = 0.;
 double y_lower_i = 0.;
 double y = 0.;
 double x_err = ((x_max-x_min)/(N));
 double y_err = 0.;

 for(int i = 0; i < N; i++)
 {
  x = x_min+(((x_max-x_min)/(N))*i);
  y_upper_i = (a1*(x-b1)*(x-b1)+c1);
  y_lower_i = (a2*(x-b2)*(x-b2)+c2);
  if(x > b1)
  {
   y_upper_i = 1.015;
  }
  if(x > b2)
  {
   y_lower_i = 0.99;
  }
  y_upper_i *= (Data_Nominal->Eval(x)/Bkg_Nominal->Eval(x));
  y_lower_i *= (Data_Nominal->Eval(x)/Bkg_Nominal->Eval(x));
  y = (y_upper_i+y_lower_i)/2.;
  y_err = (y_upper_i-y_lower_i)/2.;
  gr_bands_ratio->SetPoint(i,x,y);
  gr_bands_ratio->SetPointError(i,x_err,y_err);
 }
 return gr_bands_ratio; 
}

TGraphErrors* Get_Bands(double x_min, double x_max, TF1* Data_Nominal, int N, double& a1, double& a2, double& b1, double& b2, double& c1, double& c2)
{
 TGraphErrors* gr_bands = new TGraphErrors(N);
 double x = 0.;
 double y = 0.;
 double x_err = ((x_max-x_min)/(N));
 double y_err_i = 0.;
 double y_upper_i = 0.;
 double y_lower_i = 0.;

 for(int i = 0; i < N; i++)
 {
  x = x_min+(((x_max-x_min)/(N))*i);
  y_upper_i = (a1*(x-b1)*(x-b1)+c1);
  y_lower_i = (a2*(x-b2)*(x-b2)+c2);
  if(x > b1)
  {
   y_upper_i = 1.01;
  }
  if(x > b2)
  {
   y_lower_i = 0.99;
  }
  y_upper_i = std::min(1.,Data_Nominal->Eval(x)*y_upper_i);
  y_lower_i = Data_Nominal->Eval(x)*y_lower_i;
  y = (y_upper_i+y_lower_i)/2.;
  y_err_i = (y_upper_i-y_lower_i)/2.;
  if(y > 1.) {y = 1.;}
  if(y_err_i > 1.) {y_err_i = 1.;}
  gr_bands->SetPoint(i,x,y);
  gr_bands->SetPointError(i,x_err,y_err_i);
 }
 return gr_bands; 
}

TGraph* Get_Fit_Ratio(double x_min, double x_max, TF1* Bkg_Nominal, TF1* Data_Nominal)
{
 int N = 1000;
 TGraph* gr = new TGraph(1000);
 double x = 0.;
 for(int i = 0; i < 1000; i++)
 {
  x = x_min+(((x_max-x_min)/(N))*i);
  gr->SetPoint(i,x,Data_Nominal->Eval(x)/Bkg_Nominal->Eval(x));
 }
 return gr;
}

void Output_Parameters(const int& year, const int& electron, const bool& data, string cut, string tag)
{
 double a1, a2, b1, b2, c1, c2, norm, mean, sigma;
 double scale = 0;
 double weight = 0;
 Get_Bands_Ratio_Params(a1,a2,b1,b2,c1,c2,year,electron,data);
 Get_Func_Params(year,electron,data,cut,tag,norm,mean,sigma,scale,weight);

 ofstream output;
 if(!fileExists("Parameters.csv"))
 {
  output.open("Parameters.csv",fstream::app);
  output << "Name" << "," << "a1" << "," << "a2" << "," << "b1" << "," << "b2" << "," << "c1" << "," << "c2" << "," << "norm" << "," << "mean" << "," << "sigma" << "," << "scale" << "," << "weight" << "," << endl;
 }
 else { output.open("Parameters.csv",fstream::app); }
 string name = cut+"_"+tag;
 if(cut.find("Electron0") != std::string::npos) { name+="_Electron0"; }
 else if(cut.find("Electron1") != std::string::npos) { name+="_Electron1"; }
 else if(cut.find("Electron2") != std::string::npos) { name+="_Electron2"; }
 output << name << "," << a1 << "," << a2 << "," << b1 << "," << b2 << "," << c1 << "," << c2 << "," << norm << "," << mean << "," << sigma << "," << scale << "," << weight << "," << endl;
 output.close();
}

void Fitter_Eff_Nano(TGraphAsymmErrors* gr_given, vector<int> colors, string name)
{
 if(name.find("triggerE") != std::string::npos) return;
 //TGraphAsymmErrors* gr = (TGraphAsymmErrors*)(gr_given->Clone());
 for(int j = 0; j < int(gr_given->GetN()); j++)
 {
  gr_given->SetPointEXlow(j,0.);
  gr_given->SetPointEXhigh(j,0.);
 }
 double x_min = gr_given->GetXaxis()->GetXmin();
 double x_max = gr_given->GetXaxis()->GetXmax();
 
 vector<TF1*> funcs;

 //Err Function
 double Norm_Erf=1.;
 double Mean_Erf=100.;

 TF1* func_Erf = new TF1("func_Erf",Error_Func,x_min,x_max,2);
 func_Erf->SetParameter(0,Norm_Erf);
 func_Erf->SetParameter(1,Mean_Erf);
 func_Erf->SetParName(0,"Norm_Erf");
 func_Erf->SetParName(1,"Mean_Erf");
 
 //funcs.push_back(func_Erf);

 //Gaussian CDF Function
 double Norm_Gauss_CDF=1.;
 double Mean_Gauss_CDF=100.;
 double Sigma_Gauss_CDF=10.;

 TF1* func_Gaussian_CDF0 = new TF1("func_Gaussian_CDF0",Gaussian_CDF_Func,x_min,x_max,3);
 func_Gaussian_CDF0->SetParameter(0,Norm_Gauss_CDF);
 func_Gaussian_CDF0->SetParameter(1,Mean_Gauss_CDF);
 func_Gaussian_CDF0->SetParameter(2,Sigma_Gauss_CDF);
 func_Gaussian_CDF0->SetParName(0,"Norm_Gauss_CDF");
 func_Gaussian_CDF0->SetParName(1,"Mean_Gauss_CDF");
 func_Gaussian_CDF0->SetParName(2,"Sigma_Gauss_CDF");
 
 funcs.push_back(func_Gaussian_CDF0);

 //Double Gaussian CDF Function
 double Norm_Double_Gauss_CDF=1.;
 double Mean_Double_Gauss_CDF=100.;
 double Sigma_Double_Gauss_CDF=10.;
 double SigmaScale_Double_Gauss_CDF=10.;
 double Weight_Double_Gauss_CDF=.5;

 TF1* func_Double_Gaussian_CDF_Add0 = new TF1("func_Double_Gaussian_CDF_Add0",Double_Gaussian_CDF_Func_Add,x_min,x_max,5);
 func_Double_Gaussian_CDF_Add0->SetParameter(0,Norm_Double_Gauss_CDF);
 func_Double_Gaussian_CDF_Add0->SetParameter(1,Mean_Double_Gauss_CDF);
 func_Double_Gaussian_CDF_Add0->SetParameter(2,Sigma_Double_Gauss_CDF);
 func_Double_Gaussian_CDF_Add0->SetParameter(3,SigmaScale_Double_Gauss_CDF);
 func_Double_Gaussian_CDF_Add0->SetParameter(4,Weight_Double_Gauss_CDF);
 //func_Double_Gaussian_CDF_Add0->SetParLimits(3,1.,100.);
 //func_Double_Gaussian_CDF_Add0->SetParLimits(4,-TMath::Pi()/2.,TMath::Pi()/2.);
 func_Double_Gaussian_CDF_Add0->SetParName(0,"Norm_Double_Gauss_CDF0");
 func_Double_Gaussian_CDF_Add0->SetParName(1,"Mean_Double_Gauss_CDF0");
 func_Double_Gaussian_CDF_Add0->SetParName(2,"Sigma_Double_Gauss_CDF0");
 func_Double_Gaussian_CDF_Add0->SetParName(3,"Scale_Double_Gauss_CDF0");
 func_Double_Gaussian_CDF_Add0->SetParName(4,"Weight_Double_Gauss_CDF0");
 
 funcs.push_back(func_Double_Gaussian_CDF_Add0);

 TF1* func_Double_Gaussian_CDF_Multi0 = new TF1("func_Double_Gaussian_CDF_Multi0",Double_Gaussian_CDF_Func_Multi,x_min,x_max,5);
 func_Double_Gaussian_CDF_Multi0->SetParameter(0,Norm_Double_Gauss_CDF);
 func_Double_Gaussian_CDF_Multi0->SetParameter(1,Mean_Double_Gauss_CDF);
 func_Double_Gaussian_CDF_Multi0->SetParameter(2,Sigma_Double_Gauss_CDF);
 func_Double_Gaussian_CDF_Multi0->SetParameter(3,SigmaScale_Double_Gauss_CDF);
 func_Double_Gaussian_CDF_Multi0->SetParameter(4,Weight_Double_Gauss_CDF);
 //func_Double_Gaussian_CDF_Multi0->SetParLimits(3,1.,100.);
 //func_Double_Gaussian_CDF_Multi0->SetParLimits(4,-TMath::Pi()/2.,TMath::Pi()/2.);
 func_Double_Gaussian_CDF_Multi0->SetParName(0,"Norm_Double_Gauss_CDF0");
 func_Double_Gaussian_CDF_Multi0->SetParName(1,"Mean_Double_Gauss_CDF0");
 func_Double_Gaussian_CDF_Multi0->SetParName(2,"Sigma_Double_Gauss_CDF0");
 func_Double_Gaussian_CDF_Multi0->SetParName(3,"Scale_Double_Gauss_CDF0");
 func_Double_Gaussian_CDF_Multi0->SetParName(4,"Weight_Double_Gauss_CDF0");
 
 //funcs.push_back(func_Double_Gaussian_CDF_Multi0);

 if(!fileExists("Fit_Parameters_Output.csv"))
 {
  ofstream output;
  output.open("Fit_Parameters_Output.csv",fstream::app);
  output << "Efficiency";
 
  for(int i = 0; i < int(funcs.size()); i++)
  {
   output << " " << ",";
   for(int j = 0; j < funcs[i]->GetNpar(); j++)
   {
    output << funcs[i]->GetParName(j) << ",";
   }
   output << "ChiSquared" << "," << "NDF" << "," << "Status" << endl;
  }
  output.close();
 }

 //Call Get_Fit and send it whatever functions we need
 Get_Fit(gr_given,funcs,colors,"output_Fits.root",name);
}

void Format_Graph(TMultiGraph*& gr)
{
 gr->GetXaxis()->CenterTitle(true);
 gr->GetXaxis()->SetTitleFont(132);
 gr->GetXaxis()->SetTitleSize(0.06);
 gr->GetXaxis()->SetTitleOffset(1.06);
 gr->GetXaxis()->SetLabelFont(132);
 gr->GetXaxis()->SetLabelSize(0.0);
 gr->GetYaxis()->CenterTitle(true);
 gr->GetYaxis()->SetTitleFont(132);
 gr->GetYaxis()->SetTitleSize(0.06);
 gr->GetYaxis()->SetTitleOffset(.6);
 gr->GetYaxis()->SetLabelFont(132);
 gr->GetYaxis()->SetLabelSize(0.05);

 if(invert_colors)
 {
  gr->GetXaxis()->SetAxisColor(kWhite);
  gr->GetYaxis()->SetAxisColor(kWhite);
  gr->GetXaxis()->SetTitleColor(kWhite);
  gr->GetYaxis()->SetTitleColor(kWhite);
  gr->GetXaxis()->SetLabelColor(kWhite);
  gr->GetYaxis()->SetLabelColor(kWhite);
 }
}

void Format_Graph_res(TMultiGraph*& gr)
{
  gr->GetXaxis()->CenterTitle(true);
  gr->GetXaxis()->SetTitleFont(132);
  gr->GetXaxis()->SetTitleSize(0.12);
  gr->GetXaxis()->SetTitleOffset(.71);
  gr->GetXaxis()->SetLabelFont(132);
  gr->GetXaxis()->SetLabelSize(0.1);
  gr->GetYaxis()->CenterTitle(true);
  gr->GetYaxis()->SetTitleFont(132);
  gr->GetYaxis()->SetTitleSize(0.12);
  gr->GetYaxis()->SetTitleOffset(.3);
  gr->GetYaxis()->SetLabelFont(132);
  gr->GetYaxis()->SetLabelSize(0.1);

  if(invert_colors)
  {
   gr->GetXaxis()->SetAxisColor(kWhite);
   gr->GetYaxis()->SetAxisColor(kWhite);
   gr->GetXaxis()->SetTitleColor(kWhite);
   gr->GetYaxis()->SetTitleColor(kWhite);
   gr->GetXaxis()->SetLabelColor(kWhite);
   gr->GetYaxis()->SetLabelColor(kWhite);
  }
}

void Format_Graph_res(TGraphErrors*& gr)
{
  gr->GetXaxis()->CenterTitle(true);
  gr->GetXaxis()->SetTitleFont(132);
  gr->GetXaxis()->SetTitleSize(0.115);
  gr->GetXaxis()->SetTitleOffset(.75);
  gr->GetXaxis()->SetLabelFont(132);
  gr->GetXaxis()->SetLabelSize(0.09);
  gr->GetYaxis()->CenterTitle(true);
  gr->GetYaxis()->SetTitleFont(132);
  gr->GetYaxis()->SetTitleSize(0.115);
  gr->GetYaxis()->SetTitleOffset(.33);
  gr->GetYaxis()->SetLabelFont(132);
  gr->GetYaxis()->SetLabelSize(0.09);

  if(invert_colors)
  {
   gr->GetXaxis()->SetAxisColor(kWhite);
   gr->GetYaxis()->SetAxisColor(kWhite);
   gr->GetXaxis()->SetTitleColor(kWhite);
   gr->GetYaxis()->SetTitleColor(kWhite);
   gr->GetXaxis()->SetLabelColor(kWhite);
   gr->GetYaxis()->SetLabelColor(kWhite);
  }
}

void Format_Graph(TGraphAsymmErrors*& gr)
{
 gr->GetXaxis()->CenterTitle(true);
 gr->GetXaxis()->SetTitleFont(132);
 gr->GetXaxis()->SetTitleSize(0.06);
 gr->GetXaxis()->SetTitleOffset(1.06);
 gr->GetXaxis()->SetLabelFont(132);
 gr->GetXaxis()->SetLabelSize(0.0);
 //gr->GetXaxis()->SetLabelSize(0.00000001);
 gr->GetYaxis()->CenterTitle(true);
 gr->GetYaxis()->SetTitleFont(132);
 gr->GetYaxis()->SetTitleSize(0.06);
 gr->GetYaxis()->SetTitleOffset(.6);
 gr->GetYaxis()->SetLabelFont(132);
 gr->GetYaxis()->SetLabelSize(0.05);

 if(invert_colors)
 {
  gr->GetXaxis()->SetAxisColor(kWhite);
  gr->GetYaxis()->SetAxisColor(kWhite);
  gr->GetXaxis()->SetTitleColor(kWhite);
  gr->GetYaxis()->SetTitleColor(kWhite);
  gr->GetXaxis()->SetLabelColor(kWhite);
  gr->GetYaxis()->SetLabelColor(kWhite);
 }
}

void Format_Graph(TGraphErrors*& gr)
{
 gr->GetXaxis()->CenterTitle(true);
 gr->GetXaxis()->SetTitleFont(132);
 gr->GetXaxis()->SetTitleSize(0.06);
 gr->GetXaxis()->SetTitleOffset(1.06);
 gr->GetXaxis()->SetLabelFont(132);
 gr->GetXaxis()->SetLabelSize(0.0);
 //gr->GetXaxis()->SetLabelSize(0.00000001);
 gr->GetYaxis()->CenterTitle(true);
 gr->GetYaxis()->SetTitleFont(132);
 gr->GetYaxis()->SetTitleSize(0.06);
 gr->GetYaxis()->SetTitleOffset(.6);
 gr->GetYaxis()->SetLabelFont(132);
 gr->GetYaxis()->SetLabelSize(0.05);

 if(invert_colors)
 {
  gr->GetXaxis()->SetAxisColor(kWhite);
  gr->GetYaxis()->SetAxisColor(kWhite);
  gr->GetXaxis()->SetTitleColor(kWhite);
  gr->GetYaxis()->SetTitleColor(kWhite);
  gr->GetXaxis()->SetLabelColor(kWhite);
  gr->GetYaxis()->SetLabelColor(kWhite);
 }
}

TGraphAsymmErrors* TGAE_TF1(TGraphAsymmErrors *gr, TF1 *fit_func)
{
 int N = gr->GetN();
 double xnew[N];
 double ynew[N];
 for(int i=0; i < N; i++)
 {
  double x,y;
  gr->GetPoint(i, x, y);
  xnew[i] = x;
  ynew[i] = y-fit_func->Eval(x);
 }
 TGraphAsymmErrors* res_gr = new TGraphAsymmErrors(N,xnew,ynew);
 for(int i=0; i < N; i++)
 {
  res_gr->SetPointError(i,gr->GetErrorXlow(i),gr->GetErrorXhigh(i),gr->GetErrorYlow(i),gr->GetErrorYhigh(i));
 }
 res_gr->SetTitle("");
 return res_gr;
}

void Get_Fit(TGraphAsymmErrors*& gr, vector<TF1*> funcs, vector<int> colors, string outFile, string name)
{
 if(invert_colors)
 {
  gStyle->SetFrameFillColor(kBlack);
  gStyle->SetFrameLineColor(kWhite);
 }
 
 TCanvas* can = new TCanvas(name.c_str(),"",864.,468.);
 can->SetGridx();
 can->SetGridy();
 can->Draw();
 can->cd();
 if(invert_colors)
 {
  can->SetFillColor(kBlack);
 }

 can->Modified();
 can->Update();
 //TGraph* gr = get_gr(tags,Triggers,inFile,colors,can);
 Fit_Graph_With_Funcs(can,gr,funcs,colors,name);

 TFile* output = TFile::Open(outFile.c_str(),"UPDATE");
 can->Write();
 output->Close();
 delete can;
 delete output;
}

//Code for rounding numbers
double toPrecision(double num, int n) 
{
 if(num == 0) {
   return 0.0;
 }

 double d = std::ceil(std::log10(num < 0 ? -num : num));
 int power = n - (int)d;
 double magnitude = std::pow(10., power);
 long shifted = std::round(num*magnitude);

 std::ostringstream oss;
 oss << shifted/magnitude;
 string num_str = oss.str();
 return atof(num_str.c_str());
}

int getPrecision(double num)
{
 string num_str = std::to_string(num);
 num_str.erase ( num_str.find_last_not_of('0') + 1, std::string::npos );
 int pre = 0;
 bool point = false;
 int num_str_length = num_str.length();
 for(int i=0; i<num_str_length; i++)
 {
  if(point)
  {
   pre++;
  }
  if(num_str.at(i) == '.')
  {
   point = true;
  }
 }
 return pre;
}

double Round(const double& num)
{
 //int precision = getPrecision(num);
 //return (toPrecision(num,precision));
 return (toPrecision(num,3));
}

void Output_Fit_ToCSV(TF1* func, string name, string status, TFitResultPtr result)
{
 ofstream output;
 output.open("Fit_Parameters_Output.csv",fstream::app);
 output << name << ",";
 for(int j = 0; j < func->GetNpar(); j++)
 {
  string ParName = func->GetParName(j);
  if(ParName.find("Weight") != std::string::npos)
  {
   output << Round(TMath::Cos(func->GetParameter(j))*TMath::Cos(func->GetParameter(j))) << " +/- " << Round(fabs(-TMath::Sin(2.*func->GetParameter(j))*func->GetParError(j)-TMath::Cos(2.*func->GetParameter(j))*func->GetParError(j)*func->GetParError(j))) << ",";
  }
  else if(ParName.find("Scale") != std::string::npos)
  {
   double scale = func->GetParameter(j);
   double sigma = func->GetParameter(j-1);
   double scale_err = func->GetParError(j);
   double sigma_err = func->GetParError(j-1);
   TMatrixDSym cov = result->GetCovarianceMatrix();
   double cov_sigmascale = cov(j,j-1);
   string func_name = func->GetName();
   if(func_name.find("Multi") != std::string::npos)
   {
    //for multiplying
    output << Round(scale*sigma) << " +/- " << Round(sqrt(sigma*scale_err*sigma*scale_err+sigma_err*scale*sigma_err*scale+2.*sigma*scale*cov_sigmascale*cov_sigmascale)) << ",";
   }
   else if(func_name.find("Add") != std::string::npos)
   {
    //for adding
    output << Round(scale+sigma) << " +/- " << Round(sqrt(scale_err*scale_err+sigma_err*sigma_err)) << ",";
   }
  }
  else
  {
   output << Round(func->GetParameter(j)) << " +/- " << Round(func->GetParError(j)) << ",";
  }
 }
 output << Round(func->GetChisquare()) << "," << func->GetNDF() << "," << status << endl;
 output.close();
}

void Fit_Graph_With_Funcs(TCanvas*& canv, TGraphAsymmErrors*& gr_given, vector<TF1*> funcs, const vector<int>& colors, string name)
{
 gStyle->SetOptFit(1111);
 canv->cd();
 TPad* pad_gr = new TPad("pad_gr","pad_gr",0,.3,1.,1.);
 pad_gr->SetGridx();
 pad_gr->SetGridy();
 pad_gr->Draw();
 pad_gr->cd();
 canv->Update();

 vector<TGraphAsymmErrors*> vect_gr;
 for(int i = 0; i < int(funcs.size()); i++)
 {
  TGraphAsymmErrors* gr = (TGraphAsymmErrors*)(gr_given->Clone());
  Format_Graph(gr);
  vect_gr.push_back(gr);
 }
 vector<TPaveStats*> vect_stats;
 vector<TLegend> vect_leg;

 string XTitle = vect_gr[0]->GetXaxis()->GetTitle();
 double y1 = 0.8;
 double y2 = 0.8-.06*(funcs[0]->GetNpar());

 TMultiGraph* mg = new TMultiGraph();

 for(int i = 0; i < int(funcs.size()); i++)
 {
  vect_gr[i]->SetMarkerColor(kWhite);
  vect_gr[i]->SetLineColor(kWhite);
  vect_gr[i]->GetXaxis()->SetTitle("");

  if(invert_colors)
  {
   canv->SetFillColor(kBlack);
   pad_gr->SetFillColor(kBlack);
  }
  funcs[i]->SetNpx(10000);
  funcs[i]->SetLineColor(colors[i]);
  cout << endl << "Fitting " << name << " with " << funcs[i]->GetName() << endl;
  TFitResultPtr result = vect_gr[i]->Fit(funcs[i],"EMS+");
  mg->Add(vect_gr[i]); //

  TLegend leg(0.4,y1,0.6,y2,"");
  leg.SetTextFont(42);
  leg.SetTextSize(0.04);
  leg.SetFillColor(kWhite);
  leg.SetTextColor(colors[i]);
  leg.SetLineColor(colors[i]);
  TString func_name = funcs[i]->GetName();
  leg.AddEntry((TObject*)0, func_name, "");
  TString status_func = gMinuit->fCstatu;
  TString status_func_leg = "Status = ";
  status_func_leg+=status_func;
  leg.AddEntry((TObject*)0, status_func_leg, "");
  if(invert_colors)
  {
   leg.SetFillColor(kBlack);
   leg.SetShadowColor(kBlack); 
  }
  vect_leg.push_back(leg);

  pad_gr->Update();
  
  vect_gr[i]->Draw("AP SAMES"); //
  pad_gr->Update();
  canv->Update();
  TPaveStats *stats = (TPaveStats*)vect_gr[i]->GetListOfFunctions()->FindObject("stats");
  string stats_name = funcs[i]->GetName();
  stats->SetName(("stats_"+stats_name).c_str());
  stats->SetTextColor(colors[i]);
  stats->SetLineColor(colors[i]);
  stats->SetY1NDC(y1);
  stats->SetY2NDC(y2);
  y1 = y2;
  y2 = y2-.06*(funcs[i]->GetNpar());
  stats->SetX1NDC(0.6);
  stats->SetX2NDC(0.9);
  vect_stats.push_back(stats);
  pad_gr->Update();
  
  if(invert_colors)
  {
   stats->SetFillColor(kBlack);
  }
  pad_gr->Clear();
  pad_gr->Update();
  canv->Update();

  Output_Fit_ToCSV(funcs[i],name,string(status_func),result);

 }
 mg->Draw("AP SAMES");
 Format_Graph(mg);
 mg->GetYaxis()->SetTitle("Efficiency");
 for(int i = 0; i < int(funcs.size()); i++)
 {
  vect_leg[i].Draw("");
  vect_stats[i]->Draw("SAMES");
  //funcs[i]->Draw("SAMES");
 }
 pad_gr->Update();
 pad_gr->Modified();
 canv->Update();
 TLatex l;
 if(invert_colors)
 {
  l.SetTextColor(kWhite);
 }
 
 l.SetTextFont(42);
 l.SetNDC();
 l.SetTextSize(0.04);
 l.SetTextFont(42);
 l.DrawLatex(0.55,0.93,name.c_str());
 l.DrawLatex(0.13,0.93,"#bf{#it{CMS}} Internal 13 TeV Simulation");

 pad_gr->Update();
 canv->Update();

 canv->cd();
 TPad *pad_res = new TPad("pad_res", "pad_res", 0, 0.03, 1, 0.3);
 pad_res->SetGridx();
 pad_res->SetGridy();
 pad_res->SetTopMargin(1.3);
 pad_res->SetBottomMargin(0.2);
 pad_res->Draw();
 pad_res->cd();
 pad_res->Update();
 canv->Update();

 TMultiGraph* mg_res = new TMultiGraph();
 TGraphAsymmErrors* res = NULL;
 for(int i = 0; i < int(funcs.size()); i++)
 {
  res=TGAE_TF1(vect_gr[i],funcs[i]);
  Format_Graph(res);
  res->SetMarkerColor(colors[i]);
  res->SetLineColor(colors[i]);

  if(invert_colors)
  {
   pad_res->SetFillColor(kBlack);
  }
  mg_res->Add(res);
 }

 mg_res->Draw("AP");
 mg_res->GetXaxis()->SetLimits(mg->GetXaxis()->GetXmin(),mg->GetXaxis()->GetXmax());
 mg_res->GetXaxis()->SetTitle(XTitle.c_str());
 mg_res->GetYaxis()->SetTitle("Eff - Fit");
 Format_Graph_res(mg_res);
 pad_res->Modified();
 pad_res->Update();

 TLine* line = new TLine(mg_res->GetXaxis()->GetXmin(),0.0,mg_res->GetXaxis()->GetXmax(),0.0);
 line->SetLineColor(kWhite);
 line->SetLineStyle(1);
 line->Draw("SAMES");
 pad_res->Update();
 pad_res->Modified();
 canv->Update();
}

TGraphAsymmErrors* get_gr(vector<string> tags, vector<string> Triggers, string fname, vector<int> colors, TCanvas*& can)
{
 TFile *f = TFile::Open(fname.c_str(), "READ");
 TGraphAsymmErrors* gr = NULL;
 if(!f || f->IsZombie())
 {
  cout << "Unable to open " << fname << " for reading..." << endl;
  return gr;
 }
 for(int i = 0; i < int(tags.size()); i++)
 {
  TDirectoryFile* folder = nullptr;
  f->GetObject(tags[i].c_str(),folder);
  if(folder == NULL) continue;
  folder->cd();
  for(int j = 0; j < int(Triggers.size()); j++)
  {
   TEfficiency* eff = nullptr;
   folder->GetObject(Triggers.at(j).c_str(),eff);
   eff->Draw("AP");
   can->Update();
   TGraphAsymmErrors* gr = eff->GetPaintedGraph();
   gr->SetMarkerStyle(20);
   gr->SetMarkerColor(colors[i+j]);
   gr->SetLineColor(colors[i+j]);
  }
 }
 f->Close();
 delete f;
 
 return gr;
}

void Find_Fit_Params(TGraphAsymmErrors* gr, string name, TF1* func)
{
 string gr_name = gr->GetName();
 string canv_name = "canv_"+gr_name;
 TCanvas* canv = new TCanvas(canv_name.c_str(),canv_name.c_str(),750,500);
 canv->cd();

 TFitResultPtr result = gr->Fit(func,"EMS+");
 TString status = gMinuit->fCstatu;
 canv->Update();
 
 Output_Fit_ToCSV(func,name,string(status),result);
 delete canv;
}

void eraseSubStr(std::string & mainStr, const std::string & toErase)
{
 size_t pos = mainStr.find(toErase);
 if (pos != std::string::npos)
 {
  mainStr.erase(pos, toErase.length());
 }
}

std::string get_str_between_two_str(const std::string &s, const std::string &start_delim, const std::string &stop_delim)
{
 unsigned first_delim_pos = s.find(start_delim);
 unsigned end_pos_of_first_delim = first_delim_pos + start_delim.length();
 unsigned last_delim_pos = s.find_first_of(stop_delim, end_pos_of_first_delim);
 return s.substr(end_pos_of_first_delim,last_delim_pos - end_pos_of_first_delim);
}
