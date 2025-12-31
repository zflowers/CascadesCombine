#!/usr/bin/env bash
# launchLimits.sh
set -euo pipefail
source "macro/utils.sh"

dcdir="${1:-datacards_cascades}"
rundir="${2:-runs/latest}"

echo "[launchLimits] Using datacard directory: $dcdir"

# ------------------------------
# Loop through directories
# ------------------------------
for d in "$dcdir"/*/ ; do
    [[ -d "$d" ]] || continue

    base=$(basename "$d")
    mass=$(extract_mass "$base")

    if [[ -z "$mass" ]]; then
        echo "[WARN] Could not extract mass from: $base"
        continue
    fi

    echo "------------------------------------------------------------"
    echo "[launchLimits] Directory: $base  (mass = $mass)"
    echo "------------------------------------------------------------"

    # One txt per directory
    card_files=("$d"/*.txt)

    # --- Asymptotic Limits ---
    combineTool.py \
        -M AsymptoticLimits \
        -d "$d"/*.txt \
        --there \
        -n .limit \
        -m "$mass" \
        --job-dir "${rundir}/combine/$base"

done

