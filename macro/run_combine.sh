#!/usr/bin/env bash
# run_combine.sh
set -euo pipefail
: "${PYTHONPATH:=}"

datacard="$1"
mass="$2"
method="$3"
shift 3
extra_args="$@"

export SCRAM_ARCH=el9_amd64_gcc12
CMSSW_VERSION=CMSSW_14_1_0_pre4

# --- CMS system setup ---
set +u
source /cvmfs/cms.cern.ch/cmsset_default.sh
set -u

# --- Create a local CMSSW project area ---
if [ ! -d "$CMSSW_VERSION/src" ]; then
    echo "[run_combine.sh] Creating local CMSSW project area"
    scram project CMSSW "$CMSSW_VERSION"
fi

# --- Unpack CMSSW tar at CMSSW root ---
echo "[run_combine.sh] Unpacking cmssw tar"
tar -xzf cmssw_runtime.tgz -C "$CMSSW_VERSION/"

# --- Stage files to src ---
cp *.root *.txt "$CMSSW_VERSION/src"

# --- Setup runtime ---
cd "$CMSSW_VERSION/src"
scram b ProjectRename
eval `scram runtime -sh`

# --- Add CMSSW bin to PATH so 'combine' is found ---
export PATH="$CMSSW_BASE/bin/$SCRAM_ARCH:$PATH"

# --- Make CombinedLimit python modules visible as HiggsAnalysis.CombinedLimit.<module>
CL_DIR="$CMSSW_BASE/src/HiggsAnalysis/CombinedLimit"
PY_DIR="$CL_DIR/python"

if [[ -d "$PY_DIR" ]]; then
  echo "[run_combine.sh] Creating symlinks for CombinedLimit python modules..."
  pushd "$CL_DIR" >/dev/null || true
  for entry in "$PY_DIR"/* "$PY_DIR"/.*; do
    base="$(basename "$entry")"
    [[ "$base" == "." || "$base" == ".." ]] && continue
    [[ -e "$base" ]] && continue
    ln -s "python/$base" "$base"
  done
  popd >/dev/null || true
fi

export PYTHONPATH="$CMSSW_BASE/src:${PYTHONPATH:-}"

# --- Run combine ---
out_suffix=".${method,,}"   # e.g. .asymptoticlimits
combine_cmd=( combineTool.py -M "$method" -d "$datacard" --there -m "$mass" )
if [[ -n "$extra_args" ]]; then
    combine_cmd+=( $extra_args )
fi

echo "[run_combine.sh] COMMAND: ${combine_cmd[*]}"
"${combine_cmd[@]}"
cd /srv/

# --- Collect outputs ---
shopt -s nullglob
cp -a $CMSSW_VERSION/src/higgsCombine*.root . || true
cp -a $CMSSW_VERSION/src/*.txt . || true
shopt -u nullglob

echo "[run_combine.sh] Finished running combine"
