#include "BuildFit.h"

ch::Categories BuildFit::BuildCats(JSONFactory* j){
	ch::Categories cats{};
	int binNum=0;
	for (json::iterator it = j->j.begin(); it != j->j.end(); ++it) {
  		std::cout << it.key() <<"\n";
		cats.push_back( {binNum, it.key()} );
		binNum++;
	}
	return cats;
}
std::map<std::string, float> BuildAsimovData(JSONFactory* j){
	std::map<std::string, float> obs_rates{};
	return obs_rates;	

}
void BuildFit::BuildAsimovFit(JSONFactory* j){
	ch::Categories cats = BuildCats(j);
}	
/*
void BuildAsimovRegions(

  ch::Categories cats = {
      {0, "A"},
      {1, "B"},
      {2, "C"},
      {3, "D"}
    };
  std::map<std::string, float> obs_rates = {
    {"A", 10.},
    {"B", 50.},
    {"C", 100.},
    {"D", 500.}
  };
  
    ch::CombineHarvester cb;
  cb.SetVerbosity(3);
 
  cb.AddObservations({"*"}, {""}, {"13TeV"}, {""},          cats);
  cb.AddProcesses(   {"*"}, {""}, {"13TeV"}, {""}, {"bkg"}, cats, false);
 
  cb.ForEachObs([&](ch::Observation *x) {
    x->set_rate(obs_rates[x->bin()]);
  });
  cb.ForEachProc([](ch::Process *x) {
    x->set_rate(1);
  });
  
    using ch::syst::SystMap;
  using ch::syst::SystMapFunc;
  using ch::syst::bin;
 
  // Add a traditional lnN systematic
  cb.cp().bin({"D"}).AddSyst(cb, "DummySys", "lnN", SystMap<>::init(1.0001));
  
  // Create a unqiue floating parameter in each bin
  cb.cp().bin({"B", "C", "D"}).AddSyst(cb, "scale_$BIN", "rateParam", SystMap<bin>::init
          ({"B"}, obs_rates["B"])
          ({"C"}, obs_rates["C"])
          ({"D"}, obs_rates["D"])
      );
      
  cb.PrintAll();
  cb.WriteDatacard("example3.txt");
  
 */
