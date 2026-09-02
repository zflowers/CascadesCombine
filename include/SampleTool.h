#ifndef SAMPLETOOL_H
#define SAMPLETOOL_H
#include <map>
#include <algorithm>
#include <iostream>
#include <vector>
#include <string>
#include "yaml-cpp/yaml.h"
#include "BuildFitTools.h"
#include <TFile.h>
#include <TTree.h>
#include <TSystem.h>
#include <limits>
#include <TTree.h>
#include <fstream>
#include <iomanip>
#include <regex>
#include <set>

using namespace std;

class SampleTool{

	public:
	std::map<std::string, std::map<std::string, double>> BkgDict{};
	std::map<std::string, std::map<std::string, double>> SigDict{};
        std::map<std::string, std::map<std::string, double>> DataDict{};
	std::map<std::string, std::map<std::string, double>> MasterDict{};
        map <string, double> LumiDict{};

	stringlist SignalKeys{};
		
	void LoadBkgs( const stringlist& bkglist );
	void LoadSigs( const stringlist& siglist );
        void LoadData( const stringlist& datalist );
	void LoadAllBkgs();
    	void LoadAllSigs();
        void LoadAllData();
	void LoadAllFromMaster();

	SampleTool();

        double GetFileFactor(const std::string &filePath) const;
	
        void PrintDict(std::map<std::string, std::map<std::string,double>>& d);
        
        void PrintKeys(stringlist sl);
        
        template <typename... Keys>
        std::map<std::string,double> mergeEntriesSafe(
            const std::map<std::string, std::map<std::string,double>>& dict,
            const Keys&... keys) const;
        
        std::map<std::string,double> mergeEntriesList(
            const std::map<std::string, std::map<std::string,double>>& dict,
            const stringlist& keys) const;
        
        stringlist loadPreferredGroupsFromYaml(const std::string &yamlPath);
        
        void WriteLatexTablesForGroups(
            const std::vector<std::string>& groups,
            const std::string& outdir
        ) const;

};

inline float UnquantizeMass(int massInt)
{
    return static_cast<float>(massInt) / 100;
}

#endif
