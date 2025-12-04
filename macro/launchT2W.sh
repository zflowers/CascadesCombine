#!/usr/bin/env bash
# macro/launchT2W.sh
source "macro/utils.sh"

# First argument = datacard directory (default datacards_cascades)
dcdir="${1:-datacards_cascades}"
# Second argument = run_dir (default: runs/latest)
rundir="${2:-runs/latest}"

echo "[launchT2W] Using datacard directory: $dcdir"

# Run T2W for first datacard
for datacard in "${dcdir}"/*/*.txt; do
    name=$(basename "${datacard}" .txt)
    WSDIR="$(dirname "$datacard")"
    MASS=$(extract_mass "$(basename "$WSDIR")")
    echo "[launchT2W] Running T2W for ${name}"
    combineTool.py \
      -M T2W \
      -m "$MASS" \
      -i "${datacard}" \
      -o "${name}_workspace.root"
    break
done
