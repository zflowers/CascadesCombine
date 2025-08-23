#ifndef BFI_H
#define BFI_H
#include "BuildFitTools.h"//Bin and process source
#include <ROOT/RDataFrame.hxx>
#include <ROOT/RVec.hxx>
#include "Math/Vector4Dfwd.h"

using namespace std;
using ROOT::RDF::RNode;

typedef ROOT::RDataFrame RDF;
typedef ROOT::RDF::RNode RN;
typedef std::vector<std::string> stringlist;
typedef std::pair<std::string,std::string> proc_cut_pair;
typedef std::map< proc_cut_pair, std::unique_ptr<RN> > nodemap;
//typedef std::map< proc_cut_pair, ROOT::RDF::RResultPtr<long long unsigned int> > countmap;
//typedef std::map< proc_cut_pair, ROOT::RDF::RResultPtr<double> > summap;
typedef std::map<proc_cut_pair, double> errormap;
typedef std::map<proc_cut_pair, double> countmap;
typedef std::map<proc_cut_pair, double> summap;

class BuildFitInput{
	
	public:
	map< std::string, std::unique_ptr<RNode> > _base_rdf_BkgDict{};//register actions on this guy, but build filters etc on operation/filter branching
	map< std::string, std::unique_ptr<RNode> > rdf_BkgDict{};//add branches and update this guy
	
	//same structures for signal, to be built in parallel
	map< std::string, std::unique_ptr<RNode> > _base_rdf_SigDict{};
	map< std::string, std::unique_ptr<RNode> > rdf_SigDict{};
	
	//also keep the evtwts for error propagation later
	map< std::string, double > bkg_evtwt{};
	map< std::string, double > sig_evtwt{};
	
	//nodemap := (sig/bkg keyname, cut region keyname), resultptr
	nodemap bkg_filtered_dataframes;//these are the analysis bins constructed from filters
	nodemap sig_filtered_dataframes;

	BuildFitInput();
	
	//load helpers
	void LoadBkg_KeyValue( const std::string& key, const stringlist& bkglist, const double& Lumi);
	void LoadSig_KeyValue( const std::string& key, const stringlist& siglist, const double& Lumi);
	
	void LoadSig_byMap( map< std::string, stringlist>& SigDict, const double& Lumi);
	void LoadBkg_byMap( map< std::string, stringlist>& BkgDict, const double& Lumi);
	
	void FilterRegions(const std::string& filterName, const stringlist& filterCuts );
	countmap CountRegions(nodemap& filtered_df);
	summap SumRegions(std::string branchname, nodemap& filtered_df);
	
	//bin objects
	std::map<std::string, Bin*> analysisbins{};
	
	//helpers print and debug df datastructures
	void ReportRegions(int verbosity=1);//report on base frame, initates action
        void ReportRegions(int verbosity, countmap &countResults, summap &sumResults, errormap &errorResults, bool DoSig);
	void PrintCountReports( const countmap& resultmap);
	void PrintSumReports( const summap& sumResults);
	void FullReport( const countmap& countResults, const summap& sumResults, const errormap& errorResults);
	
	//helpers bin objects
	void CreateBin(const std::string& binname);
	void CreateBin(const std::string& binName, const std::vector<std::string>& cuts);
	std::map<std::string, Process*> CombineBkgs( std::map<std::string, Process*>& bkgProcs );
	void ConstructBkgBinObjects( countmap countResults, summap sumResults, errormap errorResults );
	void AddSigToBinObjects( countmap countResults, summap sumResults, errormap errorResults, std::map<std::string, Bin*>& analysisbins);
	void PrintBins(int verbosity=1);

        //helpers cut objects
        using CutFn = std::function<std::string(BuildFitInput*)>;
        static const std::unordered_map<std::string, CutFn>& GetCutMap() { return cutMap_; }
        bool GetCutByName(const std::string& name, std::string& out);
        std::string GetCleaningCut();
	std::string GetZstarCut();
	std::string GetnoZstarCut();

    	std::map<std::string,std::string> userMacros = {
            {"MAX",       "ROOT::VecOps::Max"},       // Maximum value in a vector
            {"MIN",       "ROOT::VecOps::Min"},       // Minimum value in a vector
            {"SUM",       "ROOT::VecOps::Sum"},       // Sum of all elements
            {"MEAN",      "ROOT::VecOps::Mean"},      // Mean (average) of elements
            {"STDDEV",    "ROOT::VecOps::StdDev"},    // Standard deviation
            {"SIZE",      "ROOT::VecOps::Size"},      // Number of elements
            {"EMPTY",     "ROOT::VecOps::Empty"},     // Check if vector is empty
            {"NONEMPTY",  "!ROOT::VecOps::Empty"},    // Check if vector is not empty
            {"FRONT",     "ROOT::VecOps::Front"},     // First element
            {"BACK",      "ROOT::VecOps::Back"},      // Last element
            {"SORT",      "ROOT::VecOps::Sort"},      // Sorted version of the vector
            {"REVERSE",   "ROOT::VecOps::Reverse"},   // Reversed vector
            {"FILTER",    "ROOT::VecOps::Filter"},    // Filter elements based on a condition
            {"MAP",       "ROOT::VecOps::Map"},       // Apply a function to each element
            {"DELTA_PHI", "ROOT::VecOps::DeltaPhi"},  // Compute difference in phi angles
            {"DELTA_R",   "ROOT::VecOps::DeltaR"}     // Compute DeltaR between particles (eta, phi space)
    	};

    	// Register a new macro
    	void RegisterMacro(const std::string& name, const std::string& expansion);
        std::string ExpandMacros(const std::string& expr);
	std::string BuildLeptonCut(const std::string& shorthand, const std::string& side = "");
	ROOT::RDF::RNode DefineLeptonPairCounts(ROOT::RDF::RNode rdf, const std::string& side = "");
	ROOT::RDF::RNode DefinePairKinematics(ROOT::RDF::RNode rdf, const std::string& side = "");
        struct Registrar {
                Registrar(const std::string& name, CutFn fn) {
                    cutMap_[name] = fn;
                }
            };
        struct BuildFitInputCutDef {
            std::string name;                   // user-defined name for the cut
            std::vector<std::string> columns;   // columns needed to compute/apply the cut
            std::string expression;             // string to be used in node.Filter(...)
        };
        static std::map<string, BuildFitInputCutDef> loadCutsUser(ROOT::RDF::RNode &node);

    private:
        static std::unordered_map<std::string, CutFn> cutMap_;
};
#define REGISTER_CUT(classname, funcname, cutname) \
    static BuildFitInput::Registrar _registrar_##funcname( \
        cutname, [](BuildFitInput* obj){ return obj->funcname(); } \
    )
#endif
using CutDef = BuildFitInput::BuildFitInputCutDef;
