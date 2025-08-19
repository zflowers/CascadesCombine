#!/usr/bin/env bash
# macro/launchCombine.sh

# Accept first argument as datacard directory, default to datacards_cascades
dcdir="${1:-datacards_cascades}"

echo "[launchCombine] Using datacard directory: $dcdir"
# Run AsymptoticLimits
combineTool.py -M AsymptoticLimits -d "${dcdir}"/*/*.txt --there -n .limit --parallel 4
# Run Significance
combineTool.py -M Significance -t -1 --expectSignal=1 -d "${dcdir}"/*/*.txt --there --parallel 4
