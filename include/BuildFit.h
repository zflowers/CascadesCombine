#ifndef BUILDFIT_H
#define BUILDFIT_H

#include "JSONFactory.h"
#include "BuildFitTools.h"
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <filesystem>

#include <TFile.h>
#include <TH1F.h>

#include "CombineHarvester/CombineTools/interface/CombineHarvester.h"
#include "CombineHarvester/CombineTools/interface/Observation.h"
#include "CombineHarvester/CombineTools/interface/Process.h"
#include "CombineHarvester/CombineTools/interface/Utilities.h"
#include "CombineHarvester/CombineTools/interface/Systematics.h"
#include "CombineHarvester/CombineTools/interface/BinByBin.h"
using ch::syst::SystMap;
using ch::syst::SystMapFunc;
using ch::syst::bin;
using json = nlohmann::json;

class BuildFit{
    
    public:
    ch::CombineHarvester cb{};
    
    ch::Categories BuildCats(JSONFactory* j);
    std::string SanitizeName(const std::string &s);
    void WriteJsonAsFlatHists(JSONFactory* j, const std::string &outFile, std::map<std::string,float>* out_obs_rates = nullptr);
    std::vector<std::string> GetBkgProcs(JSONFactory* j);
    std::vector<std::string> ExtractSignalDetails( std::string signalPoint);
    std::vector<std::string> GetBinSet( JSONFactory* j);
    void AddFloatingNorms(stringlist bkgprocs);
    void BuildAsimovFit(JSONFactory* j, std::string signaPoint, std::string datacard_dir);
    void BuildAsimovData(std::map<std::string, float>& obs_rates, JSONFactory* j);
    void AddMCStatBinByBin(JSONFactory* j);
    void AddMCStatProcByProc(const std::string& bin, JSONFactory* j);
    void AddPTISRSys(const stringlist& binset, const stringlist& procs);
    void AddRaSys(const stringlist& binset, const stringlist& procs);
    void AddZSys(const stringlist& binset, const stringlist& procs);
    std::string GetMatchingBin(const std::string& bin, const std::string& token, const std::string& match_token);
    std::vector<std::string> sigkeys = { "Cascades", "SMS" };

};
#endif
