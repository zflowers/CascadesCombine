#ifndef SAMPLETOOL_H
#define SAMPLETOOL_H
#include <map>
#include <algorithm>
#include <iostream>
#include <vector>
#include <string>
#include "BuildFitTools.h"

using namespace std;

class SampleTool{

	public:
	map <string, stringlist> BkgDict{};
	map <string, stringlist> SigDict{};
        map<string, stringlist> DataDict{};
	map <string, stringlist> MasterDict{};
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
	
	void PrintDict( map<string, stringlist>& d );
	void PrintKeys( stringlist sl );
        template <typename... Keys>
        stringlist mergeEntriesSafe(
            const std::map<std::string, stringlist>& dict,
            const Keys&... keys) const;

        stringlist mergeEntriesList(
            const std::map<std::string, stringlist>& dict,
            const stringlist& keys) const;

};

#endif
