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
#include <unordered_map>
#include <functional>

// ROOT includes
#include <TFile.h>
#include <TTree.h>
#include <TKey.h>
#include <TClass.h>
#include <TCollection.h>
#include <TLorentzVector.h>
#include <ROOT/RDataFrame.hxx>
#include <ROOT/RVec.hxx>

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
        static stringlist GetFilterSignalsSMS();

};
inline stringlist BFTool::filterSignalsSMS{};

inline void BFTool::SetFilterSignalsSMS(const stringlist& filters){
    filterSignalsSMS = filters;
}

inline stringlist BFTool::GetFilterSignalsSMS(){
    return filterSignalsSMS;
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

// Validation Helpers

// ----------------------
// Derived variables
// ----------------------
struct DerivedVar {
    std::string name; // variable name to be created in the RDataFrame
    std::string expr; // expression to define it (ROOT/C++ expression)
};

// --------------------------------------------------
// Type-detection helper: container-like detection
// --------------------------------------------------
template <typename, typename = void>
struct has_value_type : std::false_type {};

template <typename T>
struct has_value_type<T, std::void_t<typename T::value_type>> : std::true_type {};

// --------------------------------------------------
// TryValidateType: attempt to Take<T> over the first nCheck events
// - node is the tmpNode (the Define was already done on it)
// - Recurses with larger nCheck when results are sparse (empty / all-NaN for floats)
// - Returns true if the type T is compatible (even if sparse); false only when Take<T> throws
// --------------------------------------------------
template <typename T>
inline bool TryValidateType(ROOT::RDF::RNode node,
                            const DerivedVar &dv,
                            unsigned nCheck = 50,
                            unsigned maxCheck = 5000) {
    try {
        // create a subset for this attempt (so recursion with larger nCheck works)
        auto subset = node.Range(0, static_cast<long>(nCheck));

        // This will throw if T is not compatible with the column type (caught below)
        auto vals = subset.Take<T>(dv.name + "_test").GetValue();

        // If we reached here, the column can be interpreted as T.
        // Now decide whether the data are "sparse" and we should retry with more events.

        // --- scalar floating point (e.g. double) ---
        if constexpr (std::is_floating_point_v<T>) {
            bool anyFinite = std::any_of(vals.begin(), vals.end(), [](auto v){ return std::isfinite(v); });
            if (!anyFinite) {
                if (nCheck < maxCheck) {
                    unsigned next = std::min(nCheck * 2u, maxCheck);
                    return TryValidateType<T>(node, dv, next, maxCheck);
                } else {
                    std::cerr << "[BFI_condor] WARNING: '" << dv.name
                              << "' evaluated as " << typeid(T).name()
                              << " but produced no finite values in first " << maxCheck << " events (sparse data).\n";
                }
            }
            return true; // type compatible
        }

        // --- container-like (e.g. ROOT::VecOps::RVec<Inner>) ---
        else if constexpr (has_value_type<T>::value) {
            using Inner = typename T::value_type;
            if constexpr (std::is_floating_point_v<Inner>) {
                bool anyFiniteInner = false;
                for (const auto &container : vals) {
                    for (const auto &inner : container) {
                        if (std::isfinite(inner)) { anyFiniteInner = true; break; }
                    }
                    if (anyFiniteInner) break;
                }
                if (!anyFiniteInner) {
                    if (nCheck < maxCheck) {
                        unsigned next = std::min(nCheck * 2u, maxCheck);
                        return TryValidateType<T>(node, dv, next, maxCheck);
                    } else {
                        std::cerr << "[BFI_condor] WARNING: '" << dv.name
                                  << "' evaluated as container of " << typeid(Inner).name()
                                  << " but produced no finite inner values in first " << maxCheck << " events (sparse data).\n";
                    }
                }
                return true;
            } else {
                // inner is not floating (int/bool etc.)
                if (vals.empty()) {
                    if (nCheck < maxCheck) {
                        unsigned next = std::min(nCheck * 2u, maxCheck);
                        return TryValidateType<T>(node, dv, next, maxCheck);
                    } else {
                        std::cerr << "[BFI_condor] WARNING: '" << dv.name
                                  << "' evaluated as container type " << typeid(T).name()
                                  << " but returned empty sequence in first " << maxCheck << " events (sparse data).\n";
                    }
                }
                return true;
            }
        }

        // --- non-floating scalar (int, bool, etc.) ---
        else {
            if (vals.empty()) {
                if (nCheck < maxCheck) {
                    unsigned next = std::min(nCheck * 2u, maxCheck);
                    return TryValidateType<T>(node, dv, next, maxCheck);
                } else {
                    std::cerr << "[BFI_condor] WARNING: '" << dv.name
                              << "' evaluated as " << typeid(T).name()
                              << " but returned empty vector in first " << maxCheck << " events (sparse data).\n";
                }
            }
            return true;
        }

    } catch (const std::exception &e) {
        // Take<T> threw — T is incompatible with the column type (or some other error)
        // Return false to allow caller to try the next candidate type.
        return false;
    } catch (...) {
        return false;
    }
}

// --------------------------------------------------
// ValidateDerivedVar: try a set of candidate scalar/vector types
// --------------------------------------------------
inline bool ValidateDerivedVar(ROOT::RDF::RNode node,
                               const DerivedVar &dv,
                               unsigned nCheck = 50,
                               unsigned maxCheck = 5000) {
    try {
        // Define temporary test column (tmpNode holds the Define)
        ROOT::RDF::RNode tmpNode = node.Define(dv.name + "_test", dv.expr);

        // Try candidate scalar types first (order matters a bit: prefer double/float)
        if (TryValidateType<double>(tmpNode, dv, nCheck, maxCheck)) return true;
        if (TryValidateType<float>(tmpNode, dv, nCheck, maxCheck))  return true;
        if (TryValidateType<int>(tmpNode, dv, nCheck, maxCheck))    return true;
        if (TryValidateType<unsigned int>(tmpNode, dv, nCheck, maxCheck)) return true;
        if (TryValidateType<long long>(tmpNode, dv, nCheck, maxCheck))    return true;
        if (TryValidateType<unsigned long long>(tmpNode, dv, nCheck, maxCheck)) return true;
        if (TryValidateType<bool>(tmpNode, dv, nCheck, maxCheck))   return true;

        // Try container types (ROOT::VecOps::RVec<T>)
        if (TryValidateType<ROOT::VecOps::RVec<double>>(tmpNode, dv, nCheck, maxCheck)) return true;
        if (TryValidateType<ROOT::VecOps::RVec<float>>(tmpNode, dv, nCheck, maxCheck))  return true;
        if (TryValidateType<ROOT::VecOps::RVec<int>>(tmpNode, dv, nCheck, maxCheck))    return true;
        if (TryValidateType<ROOT::VecOps::RVec<unsigned int>>(tmpNode, dv, nCheck, maxCheck)) return true;
        if (TryValidateType<ROOT::VecOps::RVec<long long>>(tmpNode, dv, nCheck, maxCheck))    return true;
        if (TryValidateType<ROOT::VecOps::RVec<unsigned long long>>(tmpNode, dv, nCheck, maxCheck)) return true;
        if (TryValidateType<ROOT::VecOps::RVec<bool>>(tmpNode, dv, nCheck, maxCheck))   return true;

        // Nothing matched: emit hints and fail
        std::cerr << "[BFI_condor] ERROR validating '" << dv.name
                  << "' from expression: " << dv.expr << "\n";
        if (dv.expr.find("/") != std::string::npos &&
            dv.expr.find("SafeDiv") == std::string::npos) {
            std::cerr << "  HINT: Expression contains '/', consider using SafeDiv(num, den, def)\n";
        }
        if (dv.expr.find("[") != std::string::npos &&
            dv.expr.find("SafeIndex") == std::string::npos) {
            std::cerr << "  HINT: Expression uses indexing '[]', consider using SafeIndex(vec, idx, defaultVal)\n";
        }

        return false;

    } catch (const std::exception &e) {
        std::cerr << "[BFI_condor] WARNING: Exception validating '" << dv.name
                  << "': " << e.what() << "\n";
        return false;
    } catch (...) {
        std::cerr << "[BFI_condor] WARNING: Unknown exception validating '" << dv.name << "'\n";
        return false;
    }
}

#endif
