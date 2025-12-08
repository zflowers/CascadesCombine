#!/usr/bin/env bash
source "macro/utils.sh"

dcdir="${1:-datacards_cascades}"
rundir="${2:-runs/latest}"

echo "[launchCombine] Using datacard directory: $dcdir"

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
    echo "[launchCombine] Directory: $base  (mass = $mass)"
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
        --parallel 4 \
        --job-dir "${rundir}/combine/$base"

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

# ------------------------------
# Collect all limits after the loop
# ------------------------------
echo "------------------------------------------------------------"
echo "[launchCombine] CollectLimits"
echo "------------------------------------------------------------"

shopt -s nullglob
files=( "${dcdir}"/SMS_TChiWZ_SMS_*/higgsCombine*AsymptoticLimits*.root )
shopt -u nullglob

if (( ${#files[@]} )); then
    combineTool.py \
        -M CollectLimits \
        -d "${dcdir}"/SMS_TChiWZ_SMS_*/higgsCombine*AsymptoticLimits*.root \
        -o ${dcdir}_SMS_TChiWZ_SMS_Limits.json \
        --job-dir "${rundir}/combine"
fi

shopt -s nullglob
files=( "${dcdir}"/Cascades_*/higgsCombine*AsymptoticLimits*.root )
shopt -u nullglob

if (( ${#files[@]} )); then
    combineTool.py \
        -M CollectLimits \
        -d "${dcdir}"/Cascades_*/higgsCombine*AsymptoticLimits*.root \
        -o ${dcdir}_Cascades_Limits.json \
        --job-dir "${rundir}/combine"
fi

