#!/usr/bin/env bash
# macro/launchFitDiagnostics.sh

# First argument = datacard directory (default datacards_cascades)
dcdir="${1:-datacards_cascades}"
# Second argument = run_dir (default: runs/latest)
rundir="${2:-runs/latest}"

echo "[launchFitDiagnostics] Using datacard directory: $dcdir"

# Run FitDiagnostics initial fit for each workspace
for WS in "${dcdir}"/*/*_workspace.root; do
    WSDIR="$(dirname "$WS")"
    WSFILE="$(basename "$WS")"
    echo "[launchFitDiagnostics] Running FitDiagnostics in $WSDIR for $WSFILE"
    pushd "$WSDIR" > /dev/null || exit 1
    combineTool.py \
      -M FitDiagnostics \
      -d "$WSFILE" \
      -m 125 \
      --saveShapes \
      --saveWithUncertainties
    popd > /dev/null || exit 1
done

