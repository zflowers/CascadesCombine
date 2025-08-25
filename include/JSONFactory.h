#ifndef JSONFACT_H
#define JSONFACT_H

#include "nlohmann/json.hpp" // JSON lib
#include "BuildFitTools.h"
#include <vector>
#include <map>
#include <iostream>
#include <fstream>

using json = nlohmann::json;

class JSONFactory{

	public:
	JSONFactory(std::map<std::string, Bin*> analysisbins);
	JSONFactory(std::string filename);
	json j{};
	std::vector<std::string> GetSigProcs();
	void WriteJSON(std::string filename);

	std::vector<std::string> sigkeys = { "Cascades", "SMS" };

};

#endif
