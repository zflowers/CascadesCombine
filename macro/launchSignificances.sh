#!/usr/bin/env bash
# launchSignificances.sh
source "macro/utils.sh"

dcdir="${1:-datacards_cascades}"
rundir="${2:-runs/latest}"

echo "[launchSignificances] Using datacard directory: $dcdir"

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
    echo "[launchSignificances] Directory: $base  (mass = $mass)"
    echo "------------------------------------------------------------"

    # One txt per directory
    card_files=("$d"/*.txt)

    # --- Significance ---
    combineTool.py \
        -M Significance \
        -t -1 \
        --expectSignal=1 \
        -d "$d"/*.txt \
        --there \
        -m "$mass" \
        --parallel 4 \
        --job-dir "${rundir}/combine/$base"
done

