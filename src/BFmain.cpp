
#include "JSONFactory.h"
#include "BuildFit.h"
#include <vector>
#include <string>
#include <filesystem> // Required for std::filesystem
namespace fs = std::filesystem;


int main(){

	std::string datacard_dir = "datacards_cascades";
	std::string input_json = "./json/test_cascades.json";
	
	JSONFactory* j = new JSONFactory(input_json);
//	BuildFit* BF = new BuildFit();
	
	std::vector<std::string> signals = j->GetSigProcs();
	//BF->BuildAsimovFit(j,"gogoG_2000_1000_500_10");

	//regenerate datacard directories
	fs::path dir_path = datacard_dir;
	fs::remove_all(dir_path);
	for( long unsigned int i=0; i<signals.size(); i++){
		/*std::vector<std::string> splitSignal = BFTool::SplitString(signals[i], "_");
		std::string mass = "";
      		for(long unsigned int i=1; i< splitSignal.size(); i++){
			mass += splitSignal[i];
		}*/
		BuildFit* BF = new BuildFit();
		std::filesystem::create_directories( datacard_dir+"/"+signals[i] );
		BF->BuildAsimovFit(j,signals[i], datacard_dir);
		//break;
	}
}
