#!/usr/bin/env bash
# launchCollectLimits.sh
dcdir="${1:-datacards_cascades}"
rundir="${2:-runs/latest}"

echo "[launchCollectLimits] Using datacard directory: $dcdir"

# -------------------
# Collect all limits
# -------------------

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

