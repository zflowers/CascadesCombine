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

void BuildFit::BuildAsimovData(
        std::map<std::string, float>& obs_rates,
        JSONFactory* j
    ){
    //outer loop bin iterator
    for (json::iterator it = j->j.begin(); it != j->j.end(); ++it){
        //inner loop process iterator
        std::string binname = it.key();
        float totalBkg = 0;
        for (json::iterator it2 = it.value().begin(); it2 != it.value().end(); ++it2){
            
            if( BFTool::ContainsAnySubstring( it2.key(), sigkeys)){
                continue;
            }
            else{
                //get the wnevents, index 1 of array
                json json_array = it2.value();
                totalBkg += json_array[1].get<float>();
            }
        }
        obs_rates[binname] = float(int(totalBkg));
    }
}

std::vector<std::string> BuildFit::GetBkgProcs(JSONFactory* j){
	std::vector<std::string> bkgprocs{};

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
	}//make this set unique 
	std::set<std::string> my_bkg_set(bkgprocs.begin(), bkgprocs.end());
	std::vector<std::string> bkgprocsunique(my_bkg_set.begin(), my_bkg_set.end());

	return bkgprocsunique;
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
            .AddSyst(cb, "scale_"+proc, "rateParam", SystMap<>::init(1.0));
            //.AddSyst(cb, "scale_"+proc, "lnN", SystMap<>::init(1.1));
    }
    cb.SetFlag("filters-use-regex", false);
}

void BuildFit::AddMCStatProcByProc(const std::string& bin, JSONFactory* j) {
    // indices in JSON array
    constexpr int IDX_SUMW  = 1; // scaled sum (nominal yield)
    constexpr int IDX_ERR   = 2; // stat err on weighted
    constexpr int IDX_SUMG  = 3; // sum of gen weights (raw)
    constexpr int IDX_SUMG2 = 4; // sum of gen_weight^2 (raw)

    json& bin_data = j->j[bin];
    for (json::iterator it = bin_data.begin(); it != bin_data.end(); ++it) {
        // skip signal processes
        if (BFTool::ContainsAnySubstring(it.key(), sigkeys)) continue;

        const json& arr = it.value();
        if (!arr.is_array() || arr.size() <= IDX_SUMG2) continue;

        double sumW = arr[IDX_SUMW].get<double>();   // scaled nominal yield
        double sumG = arr[IDX_SUMG].get<double>();   // raw gen sum
        double sumG2 = arr[IDX_SUMG2].get<double>(); // raw gen sum squares

        // basic sanity
        if (sumG2 <= 0.0) continue; // no info to compute gen variance
        if (sumW == 0.0) continue;  // nothing to attach a multiplicative nuisance to

        double fracErrGen = std::sqrt(sumG2) / std::abs(sumG); // scale-invariant fractional error
        if (!std::isfinite(fracErrGen)) continue;

        std::string proc = it.key();
        std::string syst_name = "mcstat_proc_" + bin + "_" + proc;

        double err_scaled = arr.size() > IDX_ERR ? arr[IDX_ERR].get<double>() : 0.0;
        double N_eff_d = (sumW * sumW) / (err_scaled * err_scaled);
        int N_eff = std::max(0, static_cast<int>(std::lround(N_eff_d)));
        // decide lnN vs Gamma: use lnN for reasonably large N_eff or small Neff, else flag for Gamma
        //double k = 1.0 + fracErrGen;
        double k = 1.0 + err_scaled/fabs(sumW);
        double alpha = sumW / N_eff;
        //k = std::min(k, 1.5); // cap k for extremely large values
        if (N_eff >= 10.0 || N_eff <= 1. || alpha < 0.) {
            cb.cp().bin({bin}).process({proc})
              .AddSyst(cb, syst_name, "lnN", ch::syst::SystMap<>::init(k));
        }
        else {
            cb.cp().bin({bin}).process({proc})
              .AddSyst(cb, syst_name+"__gmN__"+std::to_string(N_eff), "lnN", ch::syst::SystMap<>::init(alpha));
        }
    }
}

// based on https://cms-analysis.github.io/HiggsAnalysis-CombinedLimit/part2/bin-wise-stats/?utm_source=chatgpt.com#description-of-the-algorithm
void BuildFit::AddMCStatBinByBin(JSONFactory* j) {
    constexpr int IDX_COUNT = 0; // raw events
    constexpr int IDX_SUMW  = 1; // weighted events
    constexpr int IDX_ERR   = 2; // stat err on weighted
    constexpr int IDX_SUMG  = 3; // gen weights
    constexpr int IDX_SUMG2 = 4; // gen weights squared

    for (json::iterator it = j->j.begin(); it != j->j.end(); ++it) {
        const std::string bin = it.key();

        double totalSumW = 0.0; // weighted
        int totalSum = 0; // raw
        double accumVar_gen = 0.0; // accumulate G2 (sum of gen_weight^2)
        double accumG = 0.0;      // accumulate G
        double totalErr2_scaled = 0.0; // err sq

        // Step 1:
        for (json::iterator it2 = it.value().begin(); it2 != it.value().end(); ++it2) {
            if (BFTool::ContainsAnySubstring(it2.key(), sigkeys)) continue; // skip signal
            const json& arr = it2.value();
            if (!arr.is_array() || arr.size() <= IDX_SUMG2) continue;

            double sum = arr[IDX_COUNT].get<double>();
            double sumW = arr[IDX_SUMW].get<double>();
            double sumG = arr[IDX_SUMG].get<double>();
            double sumG2 = arr[IDX_SUMG2].get<double>();
            double err_scaled = arr.size() > IDX_ERR ? arr[IDX_ERR].get<double>() : 0.0;

            totalSum += sum;
            totalSumW += sumW;
            accumG += sumG;
            accumVar_gen += sumG2;
            totalErr2_scaled += err_scaled * err_scaled;
        }

       //std::cout << accumVar_gen << " " << accumG << " " << totalSumW << " " << totalErr2_scaled << std::endl; 
        // Step 2: Skip empty bins
        if (accumVar_gen > 0.0 && accumG != 0.0 && totalSumW > 0. && totalErr2_scaled > 0.) {
            // Step 3: effective unweighted events (round to nearest int)
            double N_eff_d = (totalSumW * totalSumW) / totalErr2_scaled;
            int N_eff = std::max(0, static_cast<int>(std::lround(N_eff_d)));
        
            // fractional gen error for the bin (scale-invariant)
            double fracErrGen = std::sqrt(accumVar_gen) / std::abs(accumG);
            if (!std::isfinite(fracErrGen)) continue;
            // decide lnN vs Gamma per bin
            constexpr int N_EFF_THRESHOLD = 10;
            if (N_eff >= N_EFF_THRESHOLD) {
                // Use a single lnN that scales the total yield in the bin.
                //double k = 1.0 + fracErrGen;
                double k = 1.0 + sqrt(totalErr2_scaled)/totalSumW;
                if (k <= 0.0) continue; // defensive
                //k = std::min(k, 1.5); // cap k for extremely large values
                cb.cp().bin({bin})
                  .AddSyst(cb, "CMS_stat_" + bin + "_MCStat", "lnN", SystMap<>::init(k));
            } else {
                // fallback to per-process treatment
                AddMCStatProcByProc(bin, j);
            }
        }
    }
}

std::string BuildFit::GetMatchingBin(const std::string& bin, const std::string& token, const std::string& match_token){
    size_t pos = bin.find(token);
    std::string match_bin = bin;
    if (pos != std::string::npos)
        match_bin.replace(pos, token.length(), match_token);
    return match_bin;
}

void BuildFit::AddZSys(const stringlist& binset, const stringlist& procs){
    cb.SetFlag("filters-use-regex", true);
    cb.cp().process(procs).bin({".*2L.*0J.*OffZ"})
        .AddSyst(cb, "Z_2L_0J", "lnN", SystMap<>::init(1.10));
    cb.cp().process(procs).bin({".*2L.*1J.*OffZ"})
        .AddSyst(cb, "Z_2L_1J", "lnN", SystMap<>::init(1.10));
    cb.SetFlag("filters-use-regex", false);
}

void BuildFit::AddRaSys(const stringlist& binset, const stringlist& procs){
    cb.SetFlag("filters-use-regex", true);
    cb.cp().process(procs).bin({".*3L.*0J.*Ah.*"})
        .AddSyst(cb, "Ra_3L_0J", "lnN", SystMap<>::init(1.10));
    cb.cp().process(procs).bin({".*3L.*1J.*Ah.*"})
        .AddSyst(cb, "Ra_3L_1J", "lnN", SystMap<>::init(1.10));
    cb.SetFlag("filters-use-regex", false);
}

void BuildFit::AddPTISRSys(const stringlist& binset, const stringlist& procs){
    cb.SetFlag("filters-use-regex", true);
    cb.cp().process(procs).bin({".*2L.*0J.*Ph.*"})
        .AddSyst(cb, "PTISR_2L_0J", "lnN", SystMap<>::init(1.10));
    cb.cp().process(procs).bin({".*2L.*1J.*Ph.*"})
        .AddSyst(cb, "PTISR_2L_1J", "lnN", SystMap<>::init(1.10));
    //for (const auto& bin: binset){
    //  if(bin.find("_P") == std::string::npos) continue;
    //  if(bin.find("_Ph_") != std::string::npos){
    //      std::string match_bin = GetMatchingBin(bin, "_Ph_", "_Pl_"); 
    //      cb.cp().bin({bin, match_bin})
    //          .AddSyst(cb, "PTISR_"+GetMatchingBin(bin, "_Ph_", ""), "lnN", SystMap<>::init(1.20));
    //  }
    //}
    cb.SetFlag("filters-use-regex", false);
}

void BuildFit::BuildAsimovFit(JSONFactory* j, std::string signalPoint, std::string datacard_dir){
    ch::Categories cats = BuildCats(j);
    //std::cout<<"building obs rates \n";
    std::map<std::string, float> obs_rates;
    BuildAsimovData(obs_rates, j);
    //std::cout<<"Getting process list\n";
    stringlist bkgprocs = GetBkgProcs(j);
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
    //for (const auto& bin: binset){
    //    cb.cp().bin({bin}).AddSyst(cb, bin+"_DummySys", "lnN", SystMap<>::init(1.03)); // 3% over each bin
    //}
    AddMCStatBinByBin(j);
    AddFloatingNorms(bkgprocs);
    AddPTISRSys(binset, bkgprocs);
    AddRaSys(binset, bkgprocs);
    //AddZSys(binset, bkgprocs);
    //std::cout << "Printing systematics..." << std::endl; cb.PrintSysts();
    //cb.PrintAll();
    cb.WriteDatacard(datacard_dir+"/"+signalPoint+"/"+signalPoint+".txt");
}    
