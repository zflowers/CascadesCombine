#!/usr/bin/env bash
# macro/launchImpacts.sh

# First argument = datacard directory (default datacards_cascades)
dcdir="${1:-datacards_cascades}"
# Second argument = run_dir (default: runs/latest)
rundir="${2:-runs/latest}"

echo "[launchImpacts] Using datacard directory: $dcdir"

# Run Impacts for each workspace
for WS in "${dcdir}"/*/*_workspace.root; do
    WSDIR="$(dirname "$WS")"
    WSFILE="$(basename "$WS")"
    echo "[launchImpacts] Running Impacts in $WSDIR for $WSFILE"
    pushd "$WSDIR" > /dev/null || exit 1
    combineTool.py \
      -M Impacts \
      -d "$WSFILE" \
      -m 125 \
      --doInitialFit
      #--robustFit 1 \
    popd > /dev/null || exit 1
done
#combineTool.py -M Impacts -d workspace.root -m 5000325 --input-file ../ --job-mode connect --sub-opts='+ProjectName="cms.org.ku" \n request_memory = 8 GB \n' 
