
#dcdir=datacards
#dcdir=datacards_22j
#dcdir=datacards_11j
dcdir=datacards_2GLLL

combineTool.py -M AsymptoticLimits  -d ${dcdir}/*/*.txt --there -n .limit --parallel 4
combineTool.py -M Significance -t -1 --expectSignal=1 -d ${dcdir}/*/*.txt --there --parallel 4

