#include "JSONFactory.h"


JSONFactory::JSONFactory(std::map<std::string, Bin*> analysisbins){
	//loop and add bins 
	for(const auto& it: analysisbins ){
		std::string binname = it.first;
		//std::map<std::string, Process* > bkgprocs = it.second->bkgProcs;
		std::map<std::string, Process* > combinedprocs = it.second->combinedProcs;
		std::map<std::string, Process* > signals = it.second->signals;
		for(const auto& it2: combinedprocs ){
			std::string procname = it2.first;
			j[binname][procname] = { it2.second->nevents, it2.second->wnevents, it2.second->staterror };
		}
		for(const auto& it2: signals){
			std::string procname = it2.first;
			j[binname][procname] = { it2.second->nevents, it2.second->wnevents, it2.second->staterror };
		}
	}
}
JSONFactory::JSONFactory(std::string filename){
	std::ifstream ifs(filename);
	j = json::parse(ifs);

}
void JSONFactory::WriteJSON(std::string filename){
	std::cout<<"Writing json "<<filename<<" ... \n";
	std::ofstream outputFile(filename);
	if (outputFile.is_open()) {
    	outputFile << j.dump(4); // Writes with 4-space indentation
        // or outputFile << jsonData; // Writes in a compact format
        outputFile.close();
    } else {
        // Handle error if file cannot be opened
        std::cerr << "Error: Could not open file for writing." << std::endl;
    }
	    
}
