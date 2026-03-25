#!/usr/bin/env bash
# launchCollectLimits.sh
dcdir="${1:-datacards_cascades}"
rundir="${2:-runs/latest}"

echo "[launchCollectLimits] Using datacard directory: $dcdir"

# -------------------
# Collect all limits
# -------------------

shopt -s nullglob
files=( "${dcdir}"/*/higgsCombine*AsymptoticLimits*.root )
shopt -u nullglob

if (( ${#files[@]} )); then
    combineTool.py \
        -M CollectLimits \
        -d "${dcdir}"/*/higgsCombine*AsymptoticLimits*.root \
        -o "${dcdir}"_Limits.json \
        --job-dir "${rundir}/combine"
fi

if (( ${#files[@]} >= 50 )); then
    root -l -b -q "macro/PlotLimitJSON.C++(\"${dcdir}_Limits.json\", false, kTChiWZ, \"${dcdir}_Limits.root\")"
fi
