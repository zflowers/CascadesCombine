#!/usr/bin/env bash
# macro/launchCombine.sh

# First argument = datacard directory (default datacards_cascades)
dcdir="${1:-datacards_cascades}"
# Second argument = run_dir (default: runs/latest)
rundir="${2:-runs/latest}"

mkdir -p "${rundir}/combine_jobs"

echo "[launchCombine] Using datacard directory: $dcdir"
echo "[launchCombine] Storing outputs under: ${rundir}/combine_jobs"

# Run AsymptoticLimits
combineTool.py \
  -M AsymptoticLimits \
  -d "${dcdir}"/*/*.txt \
  --there \
  -n .limit \
  --parallel 4 \
  --job-dir "${rundir}/combine"

# Run Significance
combineTool.py \
  -M Significance \
  -t -1 \
  --expectSignal=1 \
  -d "${dcdir}"/*/*.txt \
  --there \
  --parallel 4 \
  --job-dir "${rundir}/combine"
