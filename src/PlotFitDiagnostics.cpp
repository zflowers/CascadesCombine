// PlotFitDiagnostics.cpp
#include "PlottingTools.h"

// ----------------------
// Main
// ----------------------
int main(int argc, char* argv[]) {

    string inputFile;
    string treeName = "shapes_prefit";
    vector<string> userBinPatterns; 
    string patternFile;
    loadFormatMaps();

    // ----------------------
    // Parse CLI args
    // ----------------------
    for(int i=1;i<argc;++i){
        string arg = argv[i];

        if(arg=="-i"||arg=="--input") {
            if(i+1 < argc) inputFile = argv[++i];
            else { cerr<<"[ERROR PlotFitDiagnostics] Missing value for "<<arg<<endl; return 1; }
        }
        else if(arg=="-o"||arg=="--output") {
            if(i+1 < argc) outputDir = argv[++i];
            else { cerr<<"[ERROR PlotFitDiagnostics] Missing value for "<<arg<<endl; return 1; }
        }
        else if(arg=="-t"||arg=="--tree") {
            if(i+1 < argc) treeName = argv[++i]; // shapes_prefit or shapes_fit_b
            else { cerr<<"[ERROR PlotFitDiagnostics] Missing value for "<<arg<<endl; return 1; }
        }
        else if(arg=="-b"||arg=="--bin") {
            if(i+1 < argc) userBinPatterns.push_back(argv[++i]);
            else { cerr<<"[ERROR PlotFitDiagnostics] Missing value for "<<arg<<endl; return 1; }
        }
        else if(arg=="--patternFile") {
            if(i+1<argc) patternFile = argv[++i];
            else { cerr<<"[ERROR PlotFitDiagnostics] Missing value for "<<arg<<endl; return 1; }
        }
        else if(arg=="--help"){
            cout << "Usage: " << argv[0] << "\n"
                 << "  -i  <fitDiagnostics.root>\n"
                 << "  -o  <outputDir>\n"
                 << "  -t  <tree: shapes_prefit or shapes_fit_b>\n"
                 << "  -b  <bin pattern, can be wildcard> (repeatable)\n"
                 << "  --patternFile <file with bin patterns>\n"
                 << "\n";
            return 0;
        }
        else {
            cerr<<"[ERROR PlotFitDiagnostics] Unknown arg "<<arg<<endl;
            return 1;
        }
    }

    if(inputFile.empty()){
        cerr<<"[ERROR PlotFitDiagnostics] No input file provided.\n";
        return 1;
    }

    // ----------------------
    // Open ROOT file
    // ----------------------
    TFile* fIn = TFile::Open(inputFile.c_str(), "READ");
    if(!fIn || fIn->IsZombie()){
        cerr<<"[ERROR PlotFitDiagnostics] Cannot open file: "<<inputFile<<endl;
        return 1;
    }

    // ----------------------
    // Get the requested tree
    // ----------------------
    TDirectory* fitDir = (TDirectory*)fIn->Get(treeName.c_str());
    if(!fitDir){
        cerr<<"[ERROR PlotFitDiagnostics] Tree "<<treeName<<" not found in file.\n";
        return 1;
    }

    // Ensure output directory exists
    if(outputDir.back() != '/') outputDir += '/';
    gSystem->mkdir(outputDir.c_str(), kTRUE);
    gSystem->mkdir((outputDir+"pdfs").c_str(), kTRUE);

    gStyle->SetOptStat(0);
    gStyle->SetOptTitle(0);

    // ----------------------
    // STEP 1 Scan all bins inside the tree
    // ----------------------
    vector<string> allBinNames;
    {
        TIter next(fitDir->GetListOfKeys());
        TKey* key;
        while((key = (TKey*)next())){
            TObject* obj = key->ReadObj();
            if(obj->InheritsFrom(TDirectoryFile::Class())) {
                allBinNames.push_back(obj->GetName());
            }
        }
    }

    // ----------------------
    // STEP 2 Determine which bins the user wants to merge
    // ----------------------
    map<string, vector<string>> mergedBinGroups;
    if(!userBinPatterns.empty() || !patternFile.empty()){
        vector<MergedBinGroup> groups = BuildMergedBinGroups(allBinNames, userBinPatterns, patternFile);
        for (const auto& g : groups){
            mergedBinGroups[g.group_name] = g.bin_names;
            gSystem->mkdir((outputDir+"pdfs/"+SanitizeString(g.group_name)).c_str(), kTRUE);
        }
    }
    else {
        // Default: one group per bin
        for(const auto& b : allBinNames){
            mergedBinGroups[b] = {b};
            gSystem->mkdir((outputDir+"pdfs/"+SanitizeString(b)).c_str(), kTRUE);
        }
    }

    // ----------------------
    // STEP 3 Loop over each merged group
    // ----------------------
    for(const auto& grp : mergedBinGroups){

        const string& mergedName = grp.first;
        const vector<string>& binsToCombine = grp.second;

        // ----------------------
        // STEP 4 Retrieve per-process hist for each bin
        // ----------------------
        CombinedBinHists mergedHists = LoadAndCombineBinHists(fitDir, mergedName, binsToCombine);

        // ----------------------
        // STEP 5 Produce stack plot for this merged group
        // ----------------------
        PlotMergedStack(treeName+"_"+mergedName, mergedHists);
    }

    cout << "[PlotFitDiagnostics] All plots saved in: " << outputDir << endl;
    return 0;
}
