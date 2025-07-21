
#include "JSONFactory.h"
#include "BuildFit.h"
#include <vector>
#include <string>
#include <filesystem> // Required for std::filesystem


int main(){

	std::string datacard_dir = "datacards";
	std::string input_json = "test.json";

	JSONFactory* j = new JSONFactory(input_json);
	BuildFit* BF = new BuildFit();
	
	std::vector<std::string> signals = j->GetSigProcs();
	//BF->BuildAsimovFit(j,"gogoG_2000_1000_500_10");
	
	//regenerate datacard directories
	fs::path dir_path = datacard_dir;
	fs::remove_all(dir_path);
	for( long unsigned int i=0; i<signals.size(); i++){
		std::filesystem::create_directories( datacard_dir+"/"+signals[i] );
		BF->BuildAsimovFit(j,signals[i], datacard_dir);
	}
}
