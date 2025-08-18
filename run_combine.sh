make clean
make -j 8
#./BFI.x
condor_submit condor/BFI.sub
python3 python/CondorJobCountMonitor.py
# Run second time just to make sure jobs are done
python3 python/CondorJobCountMonitor.py
make cmssw -j 8
./BF.x
./macro/launchCombine.sh
python3 macro/CollectSignificance.py
