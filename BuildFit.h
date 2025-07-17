
#ifndef BUILDFIT_H
#define BUILDFIT_H

#include "JSONFactory.h"

#include <iostream>
#include <vector>


#include "CombineHarvester/CombineTools/interface/CombineHarvester.h"
#include "CombineHarvester/CombineTools/interface/Observation.h"
#include "CombineHarvester/CombineTools/interface/Process.h"
#include "CombineHarvester/CombineTools/interface/Utilities.h"
#include "CombineHarvester/CombineTools/interface/Systematics.h"
#include "CombineHarvester/CombineTools/interface/BinByBin.h"
using ch::syst::SystMapFunc;
using json = nlohmann::json;

class BuildFit{
	
	public:
	ch::CombineHarvester cb{};
	
	void BuildAsimovRegions(JSONFactory* jfact);
	ch::Categories BuildCats(JSONFactory* j);
	std::map<std::string, float> BuildAsimovData(JSONFactory* j);
	

}
