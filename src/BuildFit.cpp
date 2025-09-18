#include "BuildFit.h"

ch::Categories BuildFit::BuildCats(JSONFactory* j){
    ch::Categories cats{};
    int binNum=0;
    for (json::iterator it = j->j.begin(); it != j->j.end(); ++it) {
          //std::cout << it.key() <<"\n";
        cats.push_back( {binNum, it.key()} );
        binNum++;
    }
    return cats;
}

std::map<std::string, float> BuildFit::BuildAsimovData(JSONFactory* j){

    std::map<std::string, float> obs_rates{};
    
    //outer loop bin iterator
    for (json::iterator it = j->j.begin(); it != j->j.end(); ++it){
        //inner loop process iterator
        std::string binname = it.key();
        float totalBkg = 0;
        for (json::iterator it2 = it.value().begin(); it2 != it.value().end(); ++it2){
            //std::cout<< it2.key()<<"\n";
            
            if( BFTool::ContainsAnySubstring( it2.key(), sigkeys)){
                continue;
            }
            else{
                //get the wnevents, index 1 of array
                json json_array = it2.value();
                //std::cout<< it2.key()<<" "<<json_array[1].get<float>()<<" "<<"\n";
                totalBkg+= json_array[1].get<float>();
            }
        }
        obs_rates[binname] = float(int(totalBkg));
        //std::cout<<"adding totalbkg: "<<binname<<" "<< float(int(totalBkg))<<"\n";
    }
    return obs_rates;    
}

stringlist BuildFit::GetBkgProcs(JSONFactory* j){
    stringlist bkgprocs{};

    for (json::iterator it = j->j.begin(); it != j->j.end(); ++it){
                //inner loop process iterator
                std::string binname = it.key();
                for (json::iterator it2 = it.value().begin(); it2 != it.value().end(); ++it2){
                //      std::cout<< it2.key()<<"\n";
                        if( BFTool::ContainsAnySubstring( it2.key(), sigkeys)){
                                continue;
                        }
                        else{
                bkgprocs.push_back(it2.key());
            }
        }
    }
    return bkgprocs;
}

stringlist BuildFit::ExtractSignalDetails( std::string signalPoint){

    stringlist splitPoint = BFTool::SplitString( signalPoint, "_");
    std::string analysis = splitPoint[0];
    std::string channel = "gamma";    
    //pad for mass?
    std::string mass = "";
    for( long unsigned int i=1; i< splitPoint.size(); i++){
        mass += splitPoint[i];
    }

    stringlist signalDetails = {analysis, channel, mass};
    return signalDetails;

}

stringlist BuildFit::GetBinSet( JSONFactory* j){
    stringlist bins{};
        for (json::iterator it = j->j.begin(); it != j->j.end(); ++it) {
                //std::cout << it.key() <<"\n";
                bins.push_back(  it.key() );
        }
        return bins;

}

void BuildFit::AddFloatingNorms(stringlist bkgprocs){
    cb.SetFlag("filters-use-regex", true);
    for (const auto& proc: bkgprocs){
        cb.cp().process({proc})
            //.AddSyst(cb, "scale_"+proc, "rateParam", SystMap<>::init(1.0));
            .AddSyst(cb, "scale_"+proc, "lnN", SystMap<>::init(1.1));
    }
    cb.SetFlag("filters-use-regex", false);
}

void BuildFit::BuildAsimovFit(JSONFactory* j, std::string signalPoint, std::string datacard_dir){
    ch::Categories cats = BuildCats(j);
    //std::cout<<"building obs rates \n";
    std::map<std::string, float> obs_rates = BuildAsimovData(j);
    //std::cout<<"Getting process list\n";
    stringlist bkgprocs = GetBkgProcs(j);
    std::sort(bkgprocs.begin(), bkgprocs.end());
    bkgprocs.erase(std::unique(bkgprocs.begin(), bkgprocs.end()), bkgprocs.end());
    //std::cout<<"Parse Signal point\n";
    stringlist signalDetails = ExtractSignalDetails( signalPoint);
    //std::cout<<"Build cb objects\n";
    //cb.SetVerbosity(3);
    cb.AddObservations({"*"}, {signalDetails[0]}, {"13.6TeV"}, {signalDetails[1]}, cats);
    cb.AddProcesses(   {"*"}, {signalDetails[0]}, {"13.6TeV"}, {signalDetails[1]}, bkgprocs, cats, false);
    cb.AddProcesses(   {signalDetails[2]}, {signalDetails[0]}, {"13.6TeV"}, {signalDetails[1]}, {signalPoint}, cats, true);
    cb.ForEachObs([&](ch::Observation *x){
        x->set_rate(obs_rates[x->bin()]);
    });
    cb.ForEachProc([&j](ch::Process *x) {
        //std::cout<<x->bin()<<" "<<x->process()<<"\n";
        json json_array = j->j[x->bin()][x->process()];
        x->set_rate(json_array[1].get<float>());
    });

    stringlist binset = GetBinSet(j);
    //AddFloatingNorms(bkgprocs);
    cb.cp().bin(binset).AddSyst(cb, "DummySys", "lnN", SystMap<>::init(1.01)); // 1% over all bins
    //for (const auto& bin: binset){
    //    cb.cp().bin({bin}).AddSyst(cb, bin+"_DummySys", "lnN", SystMap<>::init(1.03)); // 3% over each bin
    //}
      
    //cb.PrintAll();
    cb.WriteDatacard(datacard_dir+"/"+signalPoint+"/"+signalPoint+".txt");
}    
