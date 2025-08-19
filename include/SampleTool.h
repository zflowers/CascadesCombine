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
	map <string, stringlist> MasterDict{};

	stringlist SignalKeys{};
		
	void LoadBkgs( const stringlist& bkglist );
	void LoadSigs( const stringlist& siglist );
	void LoadAllBkgs();
    	void LoadAllSigs();
	void LoadAllFromMaster();

	SampleTool();
	
	void PrintDict( map<string, stringlist>& d );
	void PrintKeys( stringlist sl );

};

#endif
