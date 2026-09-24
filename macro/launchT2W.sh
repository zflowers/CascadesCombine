#!/usr/bin/env bash
# macro/launchT2W.sh
source "macro/utils.sh"
dcdir="${1:-datacards_cascades}"
rundir="${2:-runs/latest}"
echo "[launchT2W] Using datacard directory: $dcdir"

for datacard in "${dcdir}"/*/*.txt; do
    name=$(basename "${datacard}" .txt)
    WSDIR="$(dirname "$datacard")"
    MASS=120   # fixed dummy mass -- real signal identity lives in $PROCESS now
    echo "[launchT2W] Running T2W for ${name}"
    combineTool.py \
      -M T2W \
      -m "$MASS" \
      -i "${datacard}" \
      -o "${name}_workspace.root"
    break
done
# v -10
