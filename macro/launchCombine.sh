dcdir=datacards_cascades
pushd ../
combineTool.py -M AsymptoticLimits  -d ${dcdir}/*/*.txt --there -n .limit --parallel 4
combineTool.py -M Significance -t -1 --expectSignal=1 -d ${dcdir}/*/*.txt --there --parallel 4
popd
