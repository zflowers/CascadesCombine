
#ifndef "BFTOOLS.h"
#define "BFTOOLS.h"


class Process{
	
	public: 
	std::string procname{};
	long long unsigned int nevents{};
	double wnevents{};
	double staterror{};
	Process(std::string name, long long unsigned int n, double wn, double err);
	Process::Process(std::string name, long long unsigned int n, double wn, double err){
		procname = name;
		nevents =n;
		wnevents = wn;
		staterror = err;
	}
};
class Bin{
	
	public:
	std::string binname{};
	satistd::map<std::string, Process*> bkgProcs{};
	std::map<std::string, Process*> signals{};

};


class BFTool{

	public:
	static std::vector<std::string> SplitString(const std::string& input, char delimiter);
	static std::string GetSignalKeys(const std::string& input);
};

inline std::vector<std::string> BFTool::SplitString(const std::string& input, char delimiter) {
    std::vector<std::string> tokens;
    std::istringstream stream(input); // Create an input string stream from the input string
    std::string token;

    // Read tokens from the string stream separated by the delimiter
    while (std::getline(stream, token, delimiter)) {
        tokens.push_back(token); // Add the extracted token to the vector
    }

    return tokens;
}
inline std::string BFTool::GetSignalKeys(const std::string& input ){
	std::string mode = "x";
	std::string mgo = "0";
	std::string mn2 = "0";
	std::string mn1 = "0";
	std::string ctau = "10";
	std::vector<std::string> siglist_toks = SplitString(input, "/");
	std::string sig = siglist_toks[ siglist_toks.size()-1];
	std::vector<std::string> sig_toks = SplitString(sig, "_");
	
	mode = sig_toks[0];
	mgo = SplitString(sig_toks[6], "-")[1];
	mn2 = SplitString(sig_toks[7], "-")[1];
	mn1 = SplitString(sig_toks[8], "-")[1];
	
	std::string signalKeys = mode+"_"+mgo+"_"+mn2+"_"+mn1+"_"+ctau;
	return singalKeys;
	
	
}

#endif
