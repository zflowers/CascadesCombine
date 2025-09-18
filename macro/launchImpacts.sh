#!/usr/bin/env bash
# macro/launchImpacts.sh

# First argument = datacard directory (default datacards_cascades)
dcdir="${1:-datacards_cascades}"
# Second argument = run_dir (default: runs/latest)
rundir="${2:-runs/latest}"

echo "[launchImpacts] Using datacard directory: $dcdir"

# Run Impacts initial fit for each workspace
for WS in "${dcdir}"/*/*_workspace.root; do
    WSDIR="$(dirname "$WS")"
    WSFILE="$(basename "$WS")"
    echo "[launchImpacts] Running Impacts doInitialFit in $WSDIR for $WSFILE"
    pushd "$WSDIR" > /dev/null || exit 1
    combineTool.py \
      -M Impacts \
      -d "$WSFILE" \
      -m 125 \
      --doInitialFit
      #--robustFit 1 \
    popd > /dev/null || exit 1
done

# Run Impacts fits for each workspace
for WS in "${dcdir}"/*/*_workspace.root; do
    WSDIR="$(dirname "$WS")"
    WSFILE="$(basename "$WS")"
    echo "[launchImpacts] Running Impacts fits in $WSDIR for $WSFILE"
    pushd "$WSDIR" > /dev/null || exit 1
    combineTool.py \
      -M Impacts \
      -d "$WSFILE" \
      -m 125 \
      --doFits
      #--robustFit 1 \
    popd > /dev/null || exit 1
done
