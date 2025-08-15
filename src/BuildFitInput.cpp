#include "BuildFitInput.h"


BuildFitInput::BuildFitInput(){
	ROOT::EnableImplicitMT();
	std::cout<<"Enabled MT \n";
}

void BuildFitInput::LoadBkg_KeyValue( std::string key, stringlist bkglist, const double& Lumi){
	for( unsigned int i=0; i< bkglist.size(); i++){
		std::string subkey = key+"_"+std::to_string(i);

		ROOT::RDataFrame df("KUAnalysis", bkglist[i]);
                 // Define scaled weight (w * Lumi)
        	auto df_scaled = df.Define("weight_scaled", [Lumi](double w) { return w * Lumi; }, {"weight"});
        	// Define squared scaled weight ((w * Lumi)^2)
        	df_scaled = df_scaled.Define("weight_sq_scaled", [Lumi](double w) { return (w * Lumi) * (w * Lumi); }, {"weight"});
		_base_rdf_BkgDict[subkey] = std::make_unique<RNode>(df_scaled);
                rdf_BkgDict[subkey] = std::make_unique<RNode>(df_scaled);
		
	}
}

void BuildFitInput::LoadSig_KeyValue( std::string key, stringlist siglist, const double& Lumi){
	for( unsigned int i=0; i< siglist.size(); i++){
		std::string tree_name = "KUAnalysis";
                stringlist subkeys;
		bool isSMS = false;
                if(siglist[i].find("X_SMS") != std::string::npos){
  		  isSMS = true;
		  subkeys = BFTool::GetSignalTokensSMS( siglist[i]);
                }
                else
		  subkeys.push_back( BFTool::GetSignalTokensCascades( siglist[i] ));
                for (const auto& subkey : subkeys){
                  if(isSMS) tree_name = subkey;
		  ROOT::RDataFrame df(tree_name, siglist[i]);
                  // Define scaled weight (w * Lumi)
                  auto df_scaled = df.Define("weight_scaled", [Lumi](double w) { return w * Lumi; }, {"weight"});
        	  // Define squared scaled weight ((w * Lumi)^2)
        	  df_scaled = df_scaled.Define("weight_sq_scaled", [Lumi](double w) { return (w * Lumi) * (w * Lumi); }, {"weight"});
		  _base_rdf_SigDict[subkey] = std::make_unique<RNode>(df_scaled);
                  rdf_SigDict[subkey] = std::make_unique<RNode>(df_scaled);
		}
	}
}
void BuildFitInput::LoadBkg_byMap( map< std::string, stringlist>& BkgDict, const double& Lumi){
	
	for (const auto& pair : BkgDict) {
		std::cout<<"Loading RDataFrame for: "<<pair.first<<"\n";
		LoadBkg_KeyValue( pair.first, pair.second, Lumi);
	}

}
void BuildFitInput::LoadSig_byMap( map< std::string, stringlist>& SigDict, const double& Lumi){
	
	for (const auto& pair : SigDict) {
		std::cout<<"Loading RDataFrame for: "<<pair.first<<"\n";
		LoadSig_KeyValue( pair.first, pair.second, Lumi);
	}
	
}
void BuildFitInput::BuildRVBranch(){
	
	for (const auto& dfkey :rdf_BkgDict){
		//std::cout<<"building Rv for key:"<< dfkey <<"\n";
		auto tempdf = rdf_BkgDict[dfkey.first]->Define("Rv", "2.0*(rjrMVSum[1])/rjrASMass[1]");
		rdf_BkgDict[dfkey.first] = std::make_unique<RNode>(tempdf);
	}
	for (const auto& dfkey :rdf_SigDict){
		//std::cout<<"building Rv for key:"<< dfkey <<"\n";
		auto tempdf = rdf_SigDict[dfkey.first]->Define("Rv", "2.0*(rjrMVSum[1])/rjrASMass[1]");
		rdf_SigDict[dfkey.first] = std::make_unique<RNode>(tempdf);
	}
}
void BuildFitInput::FilterRegions( std::string filterName, std::string filterCuts ){
	
	std::cout<<"Building bkg nodes with "<< filterName <<"\n";
	for (const auto& it : rdf_BkgDict){
		bkg_filtered_dataframes[ std::make_pair(it.first,filterName) ] = std::make_unique<RN> ( (it.second)->Filter(filterCuts, filterName) );
	}
	std::cout<<"Building sig nodes with "<< filterName <<"\n";
	for (const auto& it : rdf_SigDict){
		sig_filtered_dataframes[ std::make_pair(it.first,filterName) ] = std::make_unique<RN> ( (it.second)->Filter(filterCuts, filterName) );
	}
		
}
void BuildFitInput::ReportRegions(int verbosity){
	std::cout<<"Reporting bkg nodes ...  \n";
	for (const auto& it : _base_rdf_BkgDict){
		if( verbosity > 0 ){
			std::cout<<it.first<<": \n";
			(it.second)->Report()->Print();
			std::cout<<"\n";
		}else{
			(it.second)->Report();
		}
	}
	std::cout<<"Reporting sig nodes ... \n";
	for (const auto& it : _base_rdf_SigDict){
		if( verbosity > 0 ){
			std::cout<<it.first<<": \n";
			(it.second)->Report()->Print();
			std::cout<<"\n";
		}else{
			(it.second)->Report();
		}
	}
}
countmap BuildFitInput::CountRegions(nodemap& filtered_df){
	std::cout<<"Loading Count action ...  \n";
	countmap countResults{};
	for (const auto& it : filtered_df){
		ROOT::RDF::RResultPtr<long long unsigned int> count_result = (it.second)->Count();
		countResults[std::make_pair( it.first.first, it.first.second) ] = count_result;		
	}
	return countResults;
}

summap BuildFitInput::SumRegions(std::string branchname, nodemap& filtered_df){
	std::cout<<"Loading Sum action  ... \n";
	summap sumResults{};
	for ( const auto& it: filtered_df){
		ROOT::RDF::RResultPtr<double>  sum_result = (it.second)->Sum(branchname);
		sumResults[std::make_pair( it.first.first, it.first.second) ] = sum_result;
	}
	return sumResults;		
}

summap BuildFitInput::SumRegions(std::string colname, nodemap &nodes, const double& Lumi) {
    summap results{};
    for (auto& [key, node_ptr] : nodes) {
        RNode node = *node_ptr;

        if (colname == "weight") {
            // Scale the original weight by Lumi
            auto scaled_node = node.Define("weight_scaled", [Lumi](double w){ return w * Lumi; }, {"weight"});
            results[key] = scaled_node.Sum("weight_scaled");
        }
        else if (colname == "weight_sq") {
            // Compute (weight*Lumi)^2 on the fly
            auto sq_node = node.Define("weight_sq", [Lumi](double w){ return (w * Lumi) * (w * Lumi); }, {"weight"});
            // When w2 already exists in the tree, just scale by Lumi^2
            //auto scaled_node = node.Define("weight_sq_scaled", [Lumi](double w2){ return w2 * Lumi * Lumi; }, {"w2"});
            results[key] = sq_node.Sum("weight_sq");
        }
        else {
            // Sum any other column directly
            results[key] = node.Sum(colname);
        }
    }
    return results;
}

errormap BuildFitInput::ComputeStatErrorFromSumW2(summap &sumw2) {
    std::cout << "Computing stat error ... \n";
    errormap errorResults{};
    for (auto& [key, sum_ptr] : sumw2) {
        errorResults[key] = std::sqrt(*sum_ptr);
    }
    return errorResults;
}

void BuildFitInput::PrintCountReports( countmap countResults) {
	std::cout<<"Reporting counts ... \n";
	for (const auto& it : countResults){
	ROOT::RDF::RResultPtr<long long unsigned int> count_result = it.second;
		std::cout<<it.first.first<<" "<< it.first.second<<" "<<*count_result<<"\n";
	}
	std::cout<<"\n";
	
}
void BuildFitInput::PrintSumReports( summap sumResults){
	std::cout<<"Reporting sums ... \n";
	for (const auto& it : sumResults){
	ROOT::RDF::RResultPtr<double> sum_result = it.second;
		std::cout<<it.first.first<<" "<<it.first.second<<" "<<*sum_result<<"\n";
	}
	std::cout<<"\n";
}

errormap BuildFitInput::ComputeStatError(nodemap& filtered_df, const double& Lumi) {
    std::cout << "Computing statistical error ... \n";
    errormap errorResults{};

    for (auto& [key, node_ptr] : filtered_df) {
        RNode node = *node_ptr;

        // define a temporary column for (w*Lumi)^2
        auto node2 = node.Define("w2_scaled", [Lumi](double w){ return (w*Lumi)*(w*Lumi); }, {"weight"});

        // sum the new column
        auto sumw2 = node2.Sum("w2_scaled");

        errorResults[key] = std::sqrt(*sumw2);  // sqrt(sum_i (w_i * Lumi)^2)
    }

    return errorResults;
}

void BuildFitInput::FullReport( countmap countResults, summap sumResults, errormap errorResults ){

	std::cout<<"BkgKey RawEvt WtEvt Err\n";
    // Iterate as long as both iterators are valid (assuming same size and keys)
	for (const auto& it : countResults){
        // Access key and values from both maps
        proc_cut_pair key = it.first;
        ROOT::RDF::RResultPtr<long long unsigned int> count_result = it.second;
        int count = *(count_result);
        double sum = *(sumResults[key]);
        double err = errorResults[key];

        std::cout << key.first<<" "<<key.second<<" " << count<<" "<<sum<<" "<<err << std::endl;

    }

}

std::map<std::string, Process*> BuildFitInput::CombineBkgs( std::map<std::string, Process*>& bkgProcs ){
	//build the set of backgrounds
	std::map<std::string, Process*> combinedBkgProcs{};
	for( const auto& it : bkgProcs){
		std::string procname = it.first;
		
		//check for key, if it doesn't exist create it
		procname = BFTool::SplitString( procname, "_")[0] ;
		if( combinedBkgProcs.find(procname) == combinedBkgProcs.end()){
			//key was not found, make a new proc and put it in the map
			Process* newproc = new Process(procname, 0, 0 ,0);
			combinedBkgProcs[procname] = newproc;
		}
		
		//if it is in there already, add up the  pieces with the existing components
		combinedBkgProcs[procname]->Add(bkgProcs[it.first]);
		
	}
	for( const auto& it: combinedBkgProcs){//loop back through and sqrt the errors
		combinedBkgProcs[it.first]->FixError();
	}
	return combinedBkgProcs;
}

void BuildFitInput::CreateBin(std::string binname){
	Bin* bin = new Bin();
	bin->binname = binname;
	analysisbins[binname] = bin;
}
void BuildFitInput::ConstructBkgBinObjects( countmap countResults, summap sumResults, errormap errorResults ){
	for( const auto& it: countResults ){
		proc_cut_pair cutpairkey = it.first;
		std::string procname = it.first.first;
		std::string binname = cutpairkey.second;
		//ROOT::RDF::RResultPtr<long long unsigned int> count_result = it.second;
		
		Process* thisproc = new Process(procname, *countResults[cutpairkey], *sumResults[cutpairkey], errorResults[cutpairkey]);
		analysisbins[ binname ]->bkgProcs.insert({procname, thisproc} ); 
	}
	// get the combined dictionary now
	for(const auto& it: analysisbins ){
		std::map<std::string, Process*> combinedBkgProcs = CombineBkgs(  it.second->bkgProcs );
		it.second->combinedProcs = combinedBkgProcs;
	}	
	
}
void BuildFitInput::AddSigToBinObjects( countmap countResults, summap sumResults, errormap errorResults, std::map<std::string, Bin*>& analysisbins){
	for(const auto& it: analysisbins ){
		std::string binname = it.first;
		for( const auto& it2: countResults){

			proc_cut_pair cutpairkey = it2.first;
			if( binname != cutpairkey.second ) continue;
			std::string binname2 = it2.first.second;
			std::string procname = it2.first.first;
			
			Process* thisproc = new Process( procname, *countResults[cutpairkey], *sumResults[cutpairkey], errorResults[cutpairkey]);
			analysisbins[binname]->signals.insert({procname, thisproc} );
		}
	}
}
void BuildFitInput::PrintBins(int verbosity){
	for(const auto& it: analysisbins){
		std::cout<<"Bin: "<< it.second->binname << "\n";
		//loop over raw bkg procs
		if(verbosity >= 3){
			for(const auto& it2: it.second->bkgProcs ){
				std::cout<<"   "<<it2.second->procname<<" "<< it2.second->nevents <<" "<<it2.second->wnevents<<" "<<it2.second->staterror<<"\n";
			}
		}
		if(verbosity > 0){
			for(const auto& it2: it.second->combinedProcs){
				std::cout<<"   "<< it2.second->procname<<" "<<it2.second->nevents <<" "<<it2.second->wnevents<<" "<<it2.second->staterror<<"\n";
			}	
		}
		if(verbosity >= 1){
			for(const auto& it2: it.second->signals){
				std::cout<<"   "<< it2.second->procname<<" "<<it2.second->nevents <<" "<<it2.second->wnevents<<" "<<it2.second->staterror<<"\n";
			}	
		}
	}
}

