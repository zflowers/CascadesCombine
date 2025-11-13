# Cascades Combine 
An RDataframe interface to go from TTree skims to plots & combine datacards and limits
- Based on [Justin's original LLPCombine](https://github.com/Jphsx/LLPCombine)

### Build instructions
BFI/BF - designed for el9 on LPC due to constraints from Combine

Local BFI builds just clone and `make`

LPC build
- Use el9 .. so `ssh -Y <username>@cmslpc-el9.fnal.gov`
- We will use the most up to date architectures etc, so you should copy everything from scripts/example_bash_profile.sh into your ~/.bash_profile, log out and log back in

The combine version is v10
https://cms-analysis.github.io/HiggsAnalysis-CombinedLimit/latest/#combine-v10-recommended-version

This combine version will use CMSSW 14 and we set it up like usual:
```
cmsrel CMSSW_14_1_0_pre4
cd CMSSW_14_1_0_pre4/src/
cmsenv
git -c advice.detachedHead=false clone --depth 1 --branch v10.3.1 https://github.com/cms-analysis/HiggsAnalysis-CombinedLimit.git HiggsAnalysis/CombinedLimit
cd HiggsAnalysis/CombinedLimit
scramv1 b clean; scramv1 b # always make a clean build
```

It may complain about gcc architecture .. we will use the el9 one because it is recommended - this is fine, ignore the message

We also will use combineHarvester for it's datacard automation API,
https://cms-analysis.github.io/CombineHarvester/

Combine harvester also needs set up in the CMSSW src
```
cd $CMSSW_BASE/src/
git clone https://github.com/cms-analysis/CombineHarvester.git CombineHarvester
cd CombineHarvester/
git checkout v3.0.0-pre1
scram b
```
To load YAMLs with C++:
```
cd $CMSSW_BASE/src/
git clone https://github.com/jbeder/yaml-cpp.git
cd yaml-cpp
git checkout yaml-cpp-0.7.0
mkdir build && cd build
cmake .. -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_INSTALL_PREFIX=$CMSSW_BASE/lib -DCMAKE_BUILD_TYPE=Release
make -j8
make install
```
Also for auto making yaml files later useful to do:
```
python3 -m ensurepip --upgrade --user
python3 -m pip install --user ruamel.yaml
```

The RDataframe framework is here,
https://github.com/zflowers/CascadesCombine
Also clone this repo into the CMSSW src
```
cd $CMSSW_BASE/src/
git clone git@github.com:zflowers/CascadesCombine.git
```

When everything is cloned and built outside of CascadesCombine, go into the CascadesCombine directory and compile everything
```
make clean
make all -j 8
```

### Workflow Super TLDR;
- Setup bins using .yaml files in config/
- Initialize your proxy:
```
voms-proxy-init --voms cms -valid 192:00
```
- call run_all to do everything
- for supported args to pass you can do
```
python3 python/run_combine.py --help
```

### Workflow TLDR;
- JSON is the intermediate BFI format
  - the JSON mapping is dictionary-like BINNAME[ PROCESS[ YIELDS]]
  - the process are background or signal by name
  - the yields are a vector of 3 quantities, {base_events, weighted_events, statistical_error}
- config/: .yaml files are stored here with bin definitions
  - four different types of cut strings are possible
  - square cuts directly on branches in ntuples
  - lepton based cuts on flavor, charge, etc. (see example)
  - BuildFitInput also has a few handy functions of common cuts (Cleaning cuts in PTCM, dphiCMI)
  - user cuts defined in BuildFitInput::loadCutsUser()
- src/BFI_condor.cpp is what is used for the CASCADES
  - runs a BFI job to create the JSON for each file in SampleTool
  - the bin name is a user defined name that maps to various cuts
  - different types of cuts are loaded in using strings
- python/createJobs.py
  - creates a condor submission script and working directory folders in condor/
  - keyed off of the bin name
  - submits all jobs for each file for a given bin
- python/submitJobs.py
  - creates condor submission scripts
  - run to make calls to createJobs for each bin
- src/flattenJSONs.cpp & src/mergeJSONs.cpp
  - helpers to merge JSON outputs from BFI_condor.cpp
  - createJobs and submitJobs automatically creates .sh scripts with relevant commands for calling mergers
  - submitJobs also places a master_merge file in the condor/ dir for a one bash call script
- src/BFmain.cpp is what sets up datacards
  - define your input json and datacard output directory here or pass with command line
  - additional systematics can be constructed in BuildFit.cpp
  - This BF design avoids shapes templates and ROOT histograms -- we do everything by hand, which is way faster
- running combine macro
  - examples to run/collect limits and significance is in macros folder

### Further Documentation
Further documentation in the form of slides is provided in docs/

### Implementation details and expected conventions

**Expected file formats**
Each file in sample tool will have it's own dataframe, do not combine them. If it has a different cross-section (e.g. HT slice) then it needs it's own data frame

**Signal file format and naming conventions**
Both BFI and BF expect signals to be 1 file per grid point with the signal name (process name) and mass information in the file name. These get parsed and passed into JSON/datacards with the common tool header `BuildFitTools.h`.

**Event loop design philosophy**
The way we calculate the amount of weighted events in a bin is by creating the event weight branch in the dataframe and then summing the event weights in that bin. In principle SFs and corrections can be applied in the same way: just add a new branch that creates a new SF weight, multiply evtWt and SFevtwt into a new branch newEvtWt. Now the number of events in the bin will be the sum over newEvtWt branch.

**Obey RDataFrame lazy execution**
The Filters book operations (bin creation) on the dataframe but the result of that operation isn't acquired until an action on the dataframe is executed. Action happens when you dereference a result, triggering the event loop. After the event loop the result ptrs that hold what you want to filter (or count or sum etc.) will be populated. Don't trigger multiple event loops, the run time will blow up
