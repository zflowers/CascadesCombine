#ifndef BFTOOLS_H
#define BFTOOLS_H
#include <string>
#include <map>
#include <sstream> 
#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm> // For std::all_of
#include <cctype>    // For std::isdigit
#include <sstream>
#include <regex> // for regex matching
#include <memory>
#include <utility>

// ROOT includes
#include <TFile.h>
#include <TTree.h>
#include <TKey.h>
#include <TClass.h>
#include <TCollection.h>

typedef std::vector<std::string> stringlist;

class Process{
	
	public: 
	std::string procname{};
	long long unsigned int nevents{};
	double wnevents{};
	double staterror{};

	Process(std::string name, long long unsigned int n, double wn, double err) :procname(name), nevents(n), wnevents(wn), staterror(err){}
	//assume it is initialized from 0
	void Add(Process* p){
		nevents += p->nevents;
		wnevents += p->wnevents;
		staterror += p->staterror * p->staterror;
	}
	void FixError(){
		staterror = std::sqrt(staterror);
	}
};
class Bin{
	
	public:
	std::string binname{};
	std::map<std::string, Process*> bkgProcs{};
	std::map<std::string, Process*> combinedProcs{};
	std::map<std::string, Process*> signals{};
	std::pair<std::string, Process*> totalBkg{};

};

class BFTool{

	public:
	static std::vector<std::string> SplitString(const std::string& str,const std::string& delimiter);
	static std::string GetSignalTokensCascades(const std::string& input);
	static stringlist GetSignalTokensSMS(const std::string& input);
	static bool  ContainsAnySubstring(const std::string& mainString, const std::vector<std::string>& substrings);
        static stringlist filterSignalsSMS;
        static void SetFilterSignalsSMS(const stringlist& filters);

};
inline stringlist BFTool::filterSignalsSMS{};

inline void BFTool::SetFilterSignalsSMS(const stringlist& filters){
    filterSignalsSMS = filters;
}

inline std::vector<std::string> BFTool::SplitString(const std::string& str,const std::string& delimiter) {
    std::vector<std::string> tokens;
    size_t prev_pos = 0;
    size_t current_pos;

    while ((current_pos = str.find(delimiter, prev_pos)) != std::string::npos) {
        tokens.push_back(str.substr(prev_pos, current_pos - prev_pos));
        prev_pos = current_pos + delimiter.length();
    }
    tokens.push_back(str.substr(prev_pos)); // Add the last token

    return tokens;
}

inline stringlist BFTool::GetSignalTokensSMS(const std::string& input ){

    std::vector<std::string> tree_names;
    TFile *file = TFile::Open(input.c_str(), "READ");
    if (!file || file->IsZombie()) {
        std::cerr << "Error: could not open file " << input << std::endl;
        return tree_names;
    }

    // Regex pattern for tree names like SMS_123_456
    std::regex sms_pattern("^SMS_[0-9]+_[0-9]+$");

    // Iterate over keys in the file
    TIter nextkey(file->GetListOfKeys());
    TKey *key;
    while ((key = (TKey*)nextkey())) {
        if (std::string(key->GetClassName()) != "TTree")
            continue; // only process TTrees

        std::string tree_name = key->GetName();
        if (!std::regex_match(tree_name, sms_pattern))
            continue; // skip if not matching SMS_X_Y format
        if (!BFTool::filterSignalsSMS.empty())
            if (std::find(BFTool::filterSignalsSMS.begin(), BFTool::filterSignalsSMS.end(), tree_name) == BFTool::filterSignalsSMS.end())
                continue;
        tree_names.push_back(tree_name);
    }
    file->Close();
    return tree_names;

}

inline std::string BFTool::GetSignalTokensCascades(const std::string& input ){

    TFile *file = TFile::Open(input.c_str(), "READ");
    if (!file || file->IsZombie()) {
        std::cerr << "Error: could not open file " << input << std::endl;
        return "";
    }

    // Get the tree
    TTree *tree = nullptr;
    file->GetObject("KUAnalysis", tree);
    if (!tree) {
        std::cerr << "Error: could not find tree" << std::endl;
        file->Close();
        return "";
    }

    // Variables to hold final values
    int MP = 0, MSlepL = 0, MSneu = 0, MN2 = 0, MC1 = 0, MN1 = 0;
    
    // Temporary variables to read event-by-event
    int tMP, tMSlepL, tMSneu, tMN2, tMC1, tMN1;
    tree->SetBranchAddress("MP",     &tMP);
    tree->SetBranchAddress("MSlepL", &tMSlepL);
    tree->SetBranchAddress("MSneu",  &tMSneu);
    tree->SetBranchAddress("MN2",    &tMN2);
    tree->SetBranchAddress("MC1",    &tMC1);
    tree->SetBranchAddress("MN1",    &tMN1);
    
    Long64_t nEntries = tree->GetEntries();
    for (Long64_t i = 0; i < nEntries; ++i) {
        tree->GetEntry(i);
    
        if (!MP     && tMP)     MP     = tMP;
        if (!MSlepL && tMSlepL) MSlepL = tMSlepL;
        if (!MSneu  && tMSneu)  MSneu  = tMSneu;
        if (!MN2    && tMN2)    MN2    = tMN2;
        if (!MC1    && tMC1)    MC1    = tMC1;
        if (!MN1    && tMN1)    MN1    = tMN1;
    
        if (MP && MSlepL && MSneu && MN2 && MC1 && MN1) break;
    }

    file->Close();
    std::ostringstream oss;
    oss << "Cascades_"
    << MP << "_" << MSlepL << "_" << MSneu << "_" 
    << MN2 << "_" << MC1 << "_" << MN1;
    
    std::string combined = oss.str();
    return combined;
  
}

inline bool BFTool::ContainsAnySubstring(const std::string& mainString, const std::vector<std::string>& substrings) {
    for (const std::string& sub : substrings) {
	//std::cout<<"comparing string: "<< mainString <<", "<<sub<<"\n";
        if (mainString.find(sub) != std::string::npos) {
            // Substring found
            return true; 
        }
    }
    // No substring found
    return false; 
}
#endif
