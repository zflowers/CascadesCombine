#include "BuildFit.h"

ch::Categories BuildFit::BuildCats(JSONFactory* j){
	ch::Categories cats{};
	int binNum=0;
	for (json::iterator it = j->j.begin(); it != j->j.end(); ++it) {
  		//std::cout << it.key() <<"\n";
		cats.push_back( {binNum, it.key()} );
		binNum++;
	}
	return cats;
}
std::map<std::string, float> BuildFit::BuildAsimovData(JSONFactory* j){

	std::map<std::string, float> obs_rates{};
	
	//outer loop bin iterator
	for (json::iterator it = j->j.begin(); it != j->j.end(); ++it){
		//inner loop process iterator
		std::string binname = it.key();
		float totalBkg = 0;
		for (json::iterator it2 = it.value().begin(); it2 != it.value().end(); ++it2){
			//std::cout<< it2.key()<<"\n";
			
			if( BFTool::ContainsAnySubstring( it2.key(), sigkeys)){
				continue;
			}
			else{
				//get the wnevents, index 1 of array
				json json_array = it2.value();
				//std::cout<< it2.key()<<" "<<json_array[1].get<float>()<<" "<<"\n";
				totalBkg+= json_array[1].get<float>();
			}
		}
		obs_rates[binname] = float(int(totalBkg));
		std::cout<<"adding totalbkg: "<<binname<<" "<< float(int(totalBkg))<<"\n";
	}
	return obs_rates;	
}
std::vector<std::string> BuildFit::GetBkgProcs(JSONFactory* j){
	std::vector<std::string> bkgprocs{};

	for (json::iterator it = j->j.begin(); it != j->j.end(); ++it){
                //inner loop process iterator
                std::string binname = it.key();
                for (json::iterator it2 = it.value().begin(); it2 != it.value().end(); ++it2){
                //      std::cout<< it2.key()<<"\n";
                        if( BFTool::ContainsAnySubstring( it2.key(), sigkeys)){
                                continue;
                        }
                        else{
				bkgprocs.push_back(it2.key());
			}
		}
	}
	return bkgprocs;
}
void BuildFit::BuildAsimovFit(JSONFactory* j){
	ch::Categories cats = BuildCats(j);
	std::cout<<"building obs rates \n";
	std::map<std::string, float> obs_rates = BuildAsimovData(j);
	std::cout<<"Getting process list\n";
	std::vector<std::string> bkgprocs = GetBkgProcs(j);
	std::cout<<"Build cb objects\n";
	cb.SetVerbosity(3);
	cb.AddObservations({"*"}, {""}, {"13.6TeV"}, {""}, cats);
	cb.AddProcesses(   {"*"}, {""}, {"13.6TeV"}, {""}, bkgprocs, cats, false);
	cb.ForEachObs([&](ch::Observation *x){
		x->set_rate(obs_rates[x->bin()]);
	});
	cb.ForEachProc([&j](ch::Process *x) {
	    std::cout<<x->bin()<<" "<<x->process()<<"\n";
	    json json_array = j->j[x->bin()][x->process()];
	    x->set_rate(json_array[1].get<float>());
	});

	cb.PrintAll();

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
