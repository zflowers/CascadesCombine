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
                      std::string proc = it2.key();
                      //std::cout<< proc<<"\n";
                        bool is_base = BFTool::ContainsAnySubstring(proc, fakes_skip_list);
                        bool has_fakes = BuildFit::HasProcFAKES(j, proc);
                        if(BFTool::ContainsAnySubstring(proc, sigkeys) || proc.find("data") != std::string::npos || proc.find("Data") != std::string::npos || proc.find("FAKES") != std::string::npos || (is_base && has_fakes)){
                                continue;
                        }
                        else{
				bkgprocs.push_back(proc);
			}
		}
	}//make this set unique 
	std::set<std::string> my_bkg_set(bkgprocs.begin(), bkgprocs.end());
	std::vector<std::string> bkgprocsunique(my_bkg_set.begin(), my_bkg_set.end());

	return bkgprocsunique;
}

std::vector<std::string> BuildFit::GetFakesProcs(JSONFactory* j){
	std::vector<std::string> fakeprocs{};

	for (json::iterator it = j->j.begin(); it != j->j.end(); ++it){
                //inner loop process iterator
                std::string binname = it.key();
                for (json::iterator it2 = it.value().begin(); it2 != it.value().end(); ++it2){
                        if(it2.key().find("FAKES") != std::string::npos){
				fakeprocs.push_back(it2.key());
			} else continue;
		}
	}//make this set unique 
	std::set<std::string> my_fake_set(fakeprocs.begin(), fakeprocs.end());
	std::vector<std::string> fakeprocsunique(my_fake_set.begin(), my_fake_set.end());

	return fakeprocsunique;
}

bool BuildFit::HasDataObs(JSONFactory* j) {
    for (auto &bin : j->j.items()) {
        for (auto &proc : bin.value().items()) {
            if (strcasecmp(proc.key().c_str(), "data_obs") == 0 || proc.key().find("data") != std::string::npos || proc.key().find("Data") != std::string::npos)
                return true;
        }
    }
    return false;
}

bool BuildFit::HasProcFAKES(JSONFactory* j, const std::string& check_proc) {
    for (auto &bin : j->j.items()) {
        for (auto &proc : bin.value().items()) {
            std::string key = proc.key();
            // Skip actual data keys
            if (strcasecmp(key.c_str(), "data_obs") == 0 || 
                key.find("data") != std::string::npos || 
                key.find("Data") != std::string::npos) 
                continue;
            if (key.find(check_proc + "_FAKES") != std::string::npos)
                return true;
        }
    }
    return false;
}

std::string BuildFit::GetSignalMass(const std::string &proc) {
    if (proc.find("SMS") != std::string::npos) {
        size_t pos = proc.rfind("SMS");
        std::string mass = proc.substr(pos + 3);
        if (!mass.empty() && mass[0] == '_') mass.erase(0,1);
        size_t uscore = mass.rfind('_');
        if (uscore != std::string::npos) mass[uscore] = '0';
        return mass;
    }
    if (proc.find("Cascades_") == 0) {
        // Extract first and last mass only
        std::string massStr = proc.substr(strlen("Cascades_"));  
        std::vector<std::string> tokens = BFTool::SplitString(massStr, "_");
        if (tokens.size() >= 2) {
            std::string mass = tokens.front() + tokens.back(); // first + last
            // replace any remaining underscores with zero just in case
            std::replace(mass.begin(), mass.end(), '_', '0');
            return mass;
        } else {
            // fallback: use whatever is present
            std::replace(massStr.begin(), massStr.end(), '_', '0');
            return massStr;
        }
    }
    // fallback for other signals
    std::vector<std::string> tokens = BFTool::SplitString(proc, "_");
    std::string mass;
    for (size_t i=1; i<tokens.size(); i++) mass += tokens[i];
    return mass;
}

std::string BuildFit::GetSignalProcName(const std::string &proc) {
    // SMS: keep the usual prefix
    if (proc.find("SMS") != std::string::npos) {
        size_t pos = proc.rfind("SMS");
        return proc.substr(0, pos + 3); // e.g. "SMS_TChiWZ_SMS"
    }

    // Cascades: keep full original mass info for datacard name
    if (proc.rfind("Cascades_", 0) == 0) {
        return proc; // e.g. "Cascades_220_220_209_200_190_180"
    }

    // fallback: use everything up to first '_' (or whole proc)
    size_t p = proc.find('_');
    if (p != std::string::npos) return proc.substr(0, p);
    return proc;
}

stringlist BuildFit::ExtractSignalDetails(std::string signalPoint)
{
    stringlist splitPoint = BFTool::SplitString(signalPoint, "_");

    std::string analysis = splitPoint[0];
    std::string channel = "dummy";

    std::string mass = GetSignalMass(signalPoint);

    return {analysis, channel, mass};
}

stringlist BuildFit::GetBinSet(JSONFactory* j){
    stringlist bins{};
        for (json::iterator it = j->j.begin(); it != j->j.end(); ++it) {
                //std::cout << it.key() <<"\n";
                bins.push_back(  it.key() );
        }
        return bins;
}

std::string BuildFit::SanitizeName(const std::string &s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (isalnum(c) || c == '_' || c == '-') out += c;
        else out += '_';
    }
    return out;
}

void BuildFit::WriteJsonAsFlatHists(JSONFactory* j, const std::string &outFile, std::map<std::string,float>* out_obs_rates, const std::string& sig) {
    constexpr int IDX_RAW  = 0;
    constexpr int IDX_SUMW = 1;
    constexpr int IDX_ERR  = 2;

    TFile *f = TFile::Open(outFile.c_str(), "RECREATE");
    if (!f || f->IsZombie()) {
        std::cerr << "[ERROR] Could not create ROOT file " << outFile << std::endl;
    }

    int nWritten = 0;
    bool hasData = HasDataObs(j);
    for (auto itBin = j->j.begin(); itBin != j->j.end(); ++itBin) {

        const std::string origBin = itBin.key();
        json &binJson = itBin.value();

        double binTotal = 0.0;
        int binRaw = 0;
        int binData = 0;

        for (auto itProc = binJson.begin(); itProc != binJson.end(); ++itProc) {

            const std::string procOrig = itProc.key();

            // make isData check case-insensitive on the original proc name
            std::string procLower = procOrig;
            std::transform(procLower.begin(), procLower.end(), procLower.begin(), ::tolower);
            bool isData = (procLower.find("data") != std::string::npos);

            const json &valsObj = itProc.value();
            if (!valsObj.contains("nominal")) continue;

            const json &vals = valsObj["nominal"];
            if (!vals.is_array() || vals.size() <= IDX_ERR) continue;

            double nRaw = vals[IDX_RAW];
            double sumW = vals[IDX_SUMW];
            double err  = vals[IDX_ERR];

            // -----------------------------
            // Build histogram process name
            // -----------------------------
            std::string procName = procOrig;

            if (BFTool::ContainsAnySubstring(procOrig, sigkeys)) {
                if (procOrig != sig) continue; // only write the requested signal

                // derive prefix and mass deterministically
                std::string prefix = GetSignalProcName(procOrig);
                std::string mass   = GetSignalMass(procOrig);

                procName = prefix + "_" + mass;
            }

            // Use the original bin and process names so Combine finds exact matches
            std::string hname = origBin + "__" + procName;

            if (!isData) {
                binTotal += sumW;
                binRaw   += (int)(nRaw + 0.5);
            } else {
                binData = (int)nRaw;
            }

            if (!isData) {
                TH1F *h = new TH1F(hname.c_str(), hname.c_str(), 1, 0, 1);
                h->Sumw2();
                h->SetBinContent(1, sumW);
                h->SetBinError(1, err);
                h->SetEntries(nRaw);
                h->Write();
                delete h;
                ++nWritten;

                // -----------------------------
                // Systematic variations
                // -----------------------------
                if (valsObj.contains("systematics")) {
                    const json &systs = valsObj["systematics"];
                    for (auto itS = systs.begin(); itS != systs.end(); ++itS) {

                        const std::string systNameOrig = itS.key();
                        const std::string systName = SanitizeName(systNameOrig);
                        const json &ud = itS.value();

                        for (std::string_view udKey : {"Up", "Down"}) {
                            if (!ud.contains(std::string(udKey))) continue;

                            const json &arr = ud[std::string(udKey)];
                            if (!arr.is_array() || arr.size() <= IDX_ERR) continue;

                            double nRaw_ud = arr[IDX_RAW];
                            double sumW_ud = arr[IDX_SUMW];
                            double err_ud  = arr[IDX_ERR];

                            std::string hsyst = origBin + "__" + procName + "__" + systName + std::string(udKey);

                            TH1F *hs = new TH1F(hsyst.c_str(), hsyst.c_str(), 1, 0, 1);
                            hs->Sumw2();
                            hs->SetBinContent(1, sumW_ud);
                            hs->SetBinError(1, err_ud);
                            hs->SetEntries(nRaw_ud);
                            hs->Write();
                            delete hs;
                            ++nWritten;
                        }
                    }
                }
            }
        }

        if (out_obs_rates) {
            if (hasData) (*out_obs_rates)[origBin] = binData;
            else         (*out_obs_rates)[origBin] = binTotal;
        }

        //---------------------------------------------------------
        // Skip bins that have ONLY signal (no data, no background)
        //---------------------------------------------------------
        bool hasBackground = (binTotal > 0.0);
        bool hasDataEvents = (binData > 0);
        
        if (!hasBackground && !hasDataEvents) {
            std::cerr << "[WARNING] Skipping bin '" << origBin
                      << "' (signal present but no background or data)" << std::endl;
            continue; 
        }

        double asimov_val = out_obs_rates ? (*out_obs_rates)[origBin] : binTotal;

        // Use the original bin name here as well so Combine finds the data_obs histogram
        std::string dataName = origBin + "__data_obs";

        TH1F *hdata = new TH1F(dataName.c_str(), dataName.c_str(), 1, 0, 1);
        hdata->Sumw2();
        hdata->SetBinContent(1, asimov_val);
        hdata->SetEntries(hasData ? binData : (int)(asimov_val + 0.5));
        hdata->Write();
        delete hdata;
    }

    f->Write();
    f->Close();
    delete f;

    std::cout << "[BuildFit] Wrote " << nWritten << " histograms to " << outFile << std::endl;
}

void BuildFit::AddFloatingNorms(stringlist procs, const std::string& type, double val){
    cb.SetFlag("filters-use-regex", true);
    for (const auto& proc: procs){
        cb.cp().process({proc})
            .AddSyst(cb, "scale_"+proc, type, SystMap<>::init(val));
    }
    cb.SetFlag("filters-use-regex", false);
}

void BuildFit::AddSharedFloatingNorm(const stringlist& procs, const std::string& nuis_name, const std::string& type, double val){
    cb.SetFlag("filters-use-regex", true);
    cb.cp()
      .process(procs)   // all processes get the SAME nuisance
      .AddSyst(cb, nuis_name, type, SystMap<>::init(val));
    cb.SetFlag("filters-use-regex", false);
}

void BuildFit::AddFloatingNormsGroupedByFakeType(const stringlist& fakesprocs, const std::string& type, double val){
    // Map suffix -> list of procs
    std::map<std::string, stringlist> groups;

    const std::string marker = "_FAKES_";
    for (const auto &fp : fakesprocs) {
        auto pos = fp.find(marker);
        if (pos == std::string::npos) continue; // not a fake-style name, skip
        std::string suffix = fp.substr(pos + marker.size()); // e.g. "Elec" or "Muon"
        groups[suffix].push_back(fp);
    }

    for (const auto &kv : groups) {
        const std::string &suffix = kv.first;
        const stringlist &procs = kv.second;
        if (procs.empty()) continue;

        // nuisance name e.g. "scale_FAKES_Elec"
        std::string nuis_name = "scale_FAKES_" + suffix;

        AddSharedFloatingNorm(procs, nuis_name, type, val);
    }
}

void BuildFit::AddFakeFamiliesAsSharedNorms(const std::vector<std::string>& truebkgprocs, const std::vector<std::string>& fakesprocs, const std::string& type, double val){
    for (const auto& base : truebkgprocs) {
        // Build the family starting with the base process
        std::vector<std::string> family{ base };
        // Attach all matching fake processes
        for (const auto& fp : fakesprocs) {
            // Prefix match: base + "_FAKES"
            if (fp.rfind(base + "_FAKES", 0) == 0) {
                family.push_back(fp);
            }
        }
        // Only add a shared nuisance if base has >=1 fake processes
        if (family.size() > 1) {
            const std::string nuis_name = "scale_" + base;
            AddSharedFloatingNorm(family, nuis_name, type, val);
        }
    }
}

void BuildFit::AddShapeSystsFromJSON(JSONFactory* j) {
    for (const auto& itBin : j->j.items()) {
        const std::string& bin = itBin.key();
        const json& binJson = itBin.value();

        for (const auto& itProc : binJson.items()) {
            const std::string proc = itProc.key();
            const json& valsObj = itProc.value();

            if (!valsObj.contains("systematics")) continue;
            const json& systs = valsObj["systematics"];

            for (const auto& itS : systs.items()) {
                const std::string systName = SanitizeName(itS.key());
                
                // REGISTER this systematic with CombineHarvester
                cb.cp().bin({bin}).process({proc})
                    .AddSyst(cb, systName, "shape", SystMap<>::init(1.0));
            }
        }
    }
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
        //if (sumW <= 0.0) continue;  // nothing to attach a multiplicative nuisance to

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
        double accumG = 0.0;       // accumulate G
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
    cb.cp().process(procs).bin({".*2L.*0J.*P350.*"})
        .AddSyst(cb, "PTISR_2L_0J", "lnN", SystMap<>::init(1.10));
    cb.cp().process(procs).bin({".*2L.*1J.*P350.*"})
        .AddSyst(cb, "PTISR_2L_1J", "lnN", SystMap<>::init(1.10));
    cb.cp().process(procs).bin({".*3L.*0J.*P300.*"})
        .AddSyst(cb, "PTISR_3L_0J", "lnN", SystMap<>::init(1.10));
    cb.cp().process(procs).bin({".*3L.*1J.*P300.*"})
        .AddSyst(cb, "PTISR_3L_1J", "lnN", SystMap<>::init(1.10));
    cb.SetFlag("filters-use-regex", false);
}

void BuildFit::AddSameSignSys(const stringlist& binset, const stringlist& procs){
    cb.SetFlag("filters-use-regex", true);
    cb.cp().process(procs).bin({".*2L.*_SS.*"})
        .AddSyst(cb, "SameSign_2L", "lnN", SystMap<>::init(1.10));
    cb.cp().process(procs).bin({".*3L.*_SS.*"})
        .AddSyst(cb, "SameSign_3L", "lnN", SystMap<>::init(1.10));
    cb.SetFlag("filters-use-regex", false);
}

void BuildFit::AddBtagSys(const stringlist& binset, const stringlist& procs){
    cb.SetFlag("filters-use-regex", true);
    cb.cp().process(procs).bin({".*2L.*0J.*P250.*.*Btag.*"})
        .AddSyst(cb, "Btag_2L_0J_lPTISR", "lnN", SystMap<>::init(1.20));
    cb.cp().process(procs).bin({".*2L.*0J.*P350.*.*Btag.*"})
        .AddSyst(cb, "Btag_2L_0J_hPTISR", "lnN", SystMap<>::init(1.20));
    cb.cp().process(procs).bin({".*2L.*1J.*P250.*.*Btag.*"})
        .AddSyst(cb, "Btag_2L_1J_lPTISR", "lnN", SystMap<>::init(1.20));
    cb.cp().process(procs).bin({".*2L.*1J.*P350.*.*Btag.*"})
        .AddSyst(cb, "Btag_2L_1J_hPTISR", "lnN", SystMap<>::init(1.20));
    cb.cp().process(procs).bin({".*3L.*0J.*P200.*.*Btag.*"})
        .AddSyst(cb, "Btag_3L_0J_lPTISR", "lnN", SystMap<>::init(1.20));
    cb.cp().process(procs).bin({".*3L.*0J.*P300.*.*Btag.*"})
        .AddSyst(cb, "Btag_3L_0J_hPTISR", "lnN", SystMap<>::init(1.20));
    cb.cp().process(procs).bin({".*3L.*1J.*P200.*.*Btag.*"})
        .AddSyst(cb, "Btag_3L_1J_lPTISR", "lnN", SystMap<>::init(1.20));
    cb.cp().process(procs).bin({".*3L.*1J.*P300.*.*Btag.*"})
        .AddSyst(cb, "Btag_3L_1J_hPTISR", "lnN", SystMap<>::init(1.20));
    cb.cp().process(procs).bin({".*4L.*Btag.*"})
        .AddSyst(cb, "Btag_4L", "lnN", SystMap<>::init(1.20));
    cb.SetFlag("filters-use-regex", false);
}

void BuildFit::BuildFitSkeleton(JSONFactory* j, const std::string& signalPoint, const std::string& datacard_dir){
    ch::Categories cats = BuildCats(j);
    std::map<std::string, float> obs_rates;
    stringlist truebkgprocs = GetBkgProcs(j);
    stringlist fakesprocs = GetFakesProcs(j);
    stringlist bkgprocs;
    bkgprocs.reserve(truebkgprocs.size() + fakesprocs.size());
    bkgprocs.insert(bkgprocs.end(), truebkgprocs.begin(), truebkgprocs.end());
    bkgprocs.insert(bkgprocs.end(), fakesprocs.begin(), fakesprocs.end());

    //cb.SetVerbosity(3);
    stringlist signalDetails = ExtractSignalDetails(signalPoint);
    cb.AddObservations({"*"}, {signalDetails[0]}, {"13.6TeV"}, {signalDetails[1]}, cats);
    cb.AddProcesses({"*"}, {signalDetails[0]}, {"13.6TeV"}, {signalDetails[1]}, bkgprocs, cats, false);
    std::string sigProcPrefix = GetSignalProcName(signalPoint);
    std::string sigMass = GetSignalMass(signalPoint);
    cb.AddProcesses({sigMass}, {signalDetails[0]}, {"13.6TeV"}, {signalDetails[1]}, {sigProcPrefix}, cats, true);

    std::string fullPathString = j->json_file_name;
    std::filesystem::path p(fullPathString);
    std::filesystem::path parentPath = p.parent_path();
    std::string json_to_root_file = std::string(parentPath)+"/datacards/"+signalPoint+"/json_shapes_flat.root";
    WriteJsonAsFlatHists(j, json_to_root_file, &obs_rates, signalPoint);
    TFile* json_root_file = TFile::Open(json_to_root_file.c_str(), "UPDATE");
    if (!json_root_file || json_root_file->IsZombie()) {
        throw std::runtime_error("Cannot open " + json_to_root_file);
    }
    // collect bins that actually contain the signal process in JSON
    std::vector<std::string> bins_with_signal;
    for (auto itBin = j->j.begin(); itBin != j->j.end(); ++itBin) {
        const std::string binname = itBin.key(); // raw bin name (CH expects raw names)
        const json &binJson = itBin.value();
        if (binJson.contains(signalPoint)) {
            bins_with_signal.push_back(binname);
        }
    }
    AddShapeSystsFromJSON(j); // needs to be before ExtractShapes
    cb.cp().backgrounds().ExtractShapes(json_to_root_file, "$BIN__$PROCESS", "$BIN__$PROCESS__$SYSTEMATIC");
    
    // Only extract signal shapes for bins where we actually have signal histograms
    if (!bins_with_signal.empty()) {
        cb.cp().signals().ExtractShapes(json_to_root_file, "$BIN__$PROCESS_$MASS", "$BIN__$PROCESS_$MASS__$SYSTEMATIC");
        std::cout << "[BuildFit] extracted signals for " << bins_with_signal.size() << " bins\n";
    } else {
        std::cerr << "[BuildFit WARN] No bins contain signal '" << signalPoint << "' in JSON - skipping signal ExtractShapes.\n";
    }

    cb.FilterProcs([](ch::Process const *p){ return p->rate() <= 0; });

    stringlist binset = GetBinSet(j);
    //AddMCStatBinByBin(j);
    cb.cp().SetAutoMCStats(cb, 0.); // 0.1 // Turns on autoMCstats
    AddFakeFamiliesAsSharedNorms(truebkgprocs, fakesprocs, "rateParam", 1.0);
    AddFloatingNormsGroupedByFakeType(fakesprocs, "lnN", 1.2);
    AddPTISRSys(binset, bkgprocs);
    AddSameSignSys(binset, bkgprocs);
    AddRaSys(binset, bkgprocs);
    AddBtagSys(binset, bkgprocs);
    //std::cout << "Printing systematics..." << std::endl; cb.PrintSysts();
    //cb.PrintAll();
    cb.FilterSysts([](ch::Systematic const *s){ return s->value_u() == 1.0 && s->value_d() == 1.0; });
    cb.WriteDatacard(datacard_dir+"/"+signalPoint+"/"+signalPoint+".txt", *json_root_file);
    json_root_file->Close();
    delete json_root_file;
}
