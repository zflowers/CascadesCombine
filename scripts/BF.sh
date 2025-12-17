#!/bin/bash
set -euo pipefail

echo "[BF.sh] Starting BF job on $(hostname)"
echo "[BF.sh] Working directory: $(pwd)"

JSON_FILE="$1"
DATACARDS_DIR="$2"
SIGNAL="$3"

export SCRAM_ARCH=el9_amd64_gcc12
CMSSW_VERSION=CMSSW_14_1_0_pre4

# --- CMS system setup ---
set +u
source /cvmfs/cms.cern.ch/cmsset_default.sh
set -u

# --- Create a local CMSSW project area ---
if [ ! -d "$CMSSW_VERSION/src" ]; then
    echo "[BF.sh] Creating local CMSSW project area"
    scram project CMSSW "$CMSSW_VERSION"
fi

# --- Unpack CombineHarvester + libs at CMSSW root ---
echo "[BF.sh] Unpacking CombineHarvester and libraries"
tar xzf cmssw_runtime.tgz -C "$CMSSW_VERSION"

# --- Setup runtime ---
cd "$CMSSW_VERSION/src"
eval `scram runtime -sh`
cd "$OLDPWD"

# --- Run ---
echo "[BF.sh] Running: ./BF.x $JSON_FILE $DATACARDS_DIR $SIGNAL"
./BF.x "$JSON_FILE" "$DATACARDS_DIR" "$SIGNAL"

mv datacards/$SIGNAL/$SIGNAL.txt ./
mv datacards/$SIGNAL/json_shapes_flat.root ./

echo "[BF.sh] Job finished successfully"
