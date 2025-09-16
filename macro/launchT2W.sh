#!/usr/bin/env bash
# macro/launchT2W.sh

# First argument = datacard directory (default datacards_cascades)
dcdir="${1:-datacards_cascades}"
# Second argument = run_dir (default: runs/latest)
rundir="${2:-runs/latest}"

echo "[launchT2W] Using datacard directory: $dcdir"

# Run T2W for each datacard
for datacard in "${dcdir}"/*/*.txt; do
    name=$(basename "${datacard}" .txt)
    echo "[launchT2W] Running T2W for ${name}"
    combineTool.py \
      -M T2W \
      -i "${datacard}" \
      -o "${name}_workspace.root"
done
