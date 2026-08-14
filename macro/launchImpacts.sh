#!/usr/bin/env bash
# macro/launchImpacts.sh
source "macro/utils.sh"

# First argument = datacard directory (default datacards_cascades)
dcdir="${1:-datacards_cascades}"
# Second argument = run_dir (default: runs/latest)
rundir="${2:-runs/latest}"

echo "[launchImpacts] Using datacard directory: $dcdir"

# Run Impacts initial fit for first workspace
for WS in "${dcdir}"/*/*_workspace.root; do
    WSDIR="$(dirname "$WS")"
    WSFILE="$(basename "$WS")"
    MASS=$(extract_mass "$(basename "$WSDIR")")
    echo "[launchImpacts] Running Impacts doInitialFit in $WSDIR for $WSFILE"
    pushd "$WSDIR" > /dev/null || exit 1
    combineTool.py \
      -M Impacts \
      -d "$WSFILE" \
      -m "120" \
      --doInitialFit
      #--robustFit 1 \
    popd > /dev/null || exit 1
    break
done

# Run Impacts fits for first workspace
for WS in "${dcdir}"/*/*_workspace.root; do
    WSDIR="$(dirname "$WS")"
    WSFILE="$(basename "$WS")"
    MASS=$(extract_mass "$(basename "$WSDIR")")
    echo "[launchImpacts] Running Impacts fits in $WSDIR for $WSFILE"
    pushd "$WSDIR" > /dev/null || exit 1
    combineTool.py \
      -M Impacts \
      -d "$WSFILE" \
      -m "120" \
      --doFits \
      --robustFit 1
    popd > /dev/null || exit 1
    break
done

# Make Impacts results for first workspace
for WS in "${dcdir}"/*/*_workspace.root; do
    WSDIR="$(dirname "$WS")"
    WSFILE="$(basename "$WS")"
    MASS=$(extract_mass "$(basename "$WSDIR")")
    echo "[launchImpacts] Making Impacts json in $WSDIR for $WSFILE"
    pushd "$WSDIR" > /dev/null || exit 1
    combineTool.py \
      -M Impacts \
      -d "$WSFILE" \
      -m "120" \
      -o impacts.json
      #--robustFit 1 \
    popd > /dev/null || exit 1
    break
done

# Plot Impacts results for first workspace
for WS in "${dcdir}"/*/*_workspace.root; do
    WSDIR="$(dirname "$WS")"
    WSFILE="$(basename "$WS")"
    echo "[launchImpacts] Making Impacts plot in $WSDIR for $WSFILE"
    pushd "$WSDIR" > /dev/null || exit 1
    python3 $CMSSW_BASE/src/HiggsAnalysis/CombinedLimit/scripts/plotImpacts.py \
      -i impacts.json \
      -o impacts
    popd > /dev/null || exit 1
    break
done

if python3 $CMSSW_BASE/src/HiggsAnalysis/CombinedLimit/scripts/plotImpacts.py --help 2>/dev/null | grep -q "alpha"; then
    for WS in "${dcdir}"/*/*_workspace.root; do
        WSDIR="$(dirname "$WS")"
        WSFILE="$(basename "$WS")"
        echo "[launchImpacts] Making Impacts plot in $WSDIR for $WSFILE"
        pushd "$WSDIR" > /dev/null || exit 1
        python3 $CMSSW_BASE/src/HiggsAnalysis/CombinedLimit/scripts/plotImpacts.py \
          --sort alpha \
          -i impacts.json \
          -o impacts_alpha
        popd > /dev/null || exit 1
        break
    done
fi
