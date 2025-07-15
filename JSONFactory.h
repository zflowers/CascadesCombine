
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
	json j{};
	void WriteJSON(std::string filename);
};

#endif
