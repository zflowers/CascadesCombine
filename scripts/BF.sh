#!/bin/bash
set -euo pipefail

echo "[BF.sh] Starting BF job on $(hostname)"
echo "[BF.sh] Working directory: $(pwd)"

# --- Sanity: inputs ---
if [ "$#" -ne 3 ]; then
    echo "Usage: BF.sh flattened.json datacards/ SIGNAL"
    exit 1
fi

JSON_FILE="$1"
DATACARDS_DIR="$2"
SIGNAL="$3"

# --- Sanity: required files ---
if [ ! -f cmssw_runtime.tgz ]; then
    echo "ERROR: cmssw_runtime.tgz not found"
    exit 1
fi

if [ ! -x ./BF.x ]; then
    echo "ERROR: BF.x not found or not executable"
    ls -l
    exit 1
fi

# --- Setup CMSSW runtime from tarball ---
echo "[BF.sh] Unpacking CMSSW runtime"
tar xzf cmssw_runtime.tgz

export SCRAM_ARCH=el9_amd64_gcc12
export CMSSW_BASE="$(pwd)"

# --- CMS system environment ---
set +u
source /cvmfs/cms.cern.ch/cmsset_default.sh
set -u

# --- Add lib directories to LD_LIBRARY_PATH ---
LD_LIBRARY_PATH=""

# Standard lib and biglib directories
if [ -d "$CMSSW_BASE/lib/$SCRAM_ARCH" ]; then
    LD_LIBRARY_PATH="$CMSSW_BASE/lib/$SCRAM_ARCH:$LD_LIBRARY_PATH"
fi
if [ -d "$CMSSW_BASE/biglib/$SCRAM_ARCH" ]; then
    LD_LIBRARY_PATH="$CMSSW_BASE/biglib/$SCRAM_ARCH:$LD_LIBRARY_PATH"
fi

# Include any lib directories under src (e.g., CombineHarvester)
for libdir in $(find "$CMSSW_BASE/src" -type d -name "lib" 2>/dev/null); do
    LD_LIBRARY_PATH="$libdir:$LD_LIBRARY_PATH"
done

# Add extra_libs directories
for libdir in $(find "$CMSSW_BASE/extra_libs" -type d -name "lib" 2>/dev/null); do
    LD_LIBRARY_PATH="$libdir:$LD_LIBRARY_PATH"
done

export LD_LIBRARY_PATH

# --- Python / ROOT paths ---
export PYTHONPATH="$CMSSW_BASE/python:${PYTHONPATH:-}"
export ROOT_INCLUDE_PATH="$CMSSW_BASE/src:${ROOT_INCLUDE_PATH:-}"

# --- Debug: check BF.x linkage ---
echo "[BF.sh] Checking BF.x shared library dependencies:"
ldd ./BF.x | grep "not found" || echo "All libraries found"

# --- Run BF.x ---
echo "[BF.sh] Running: ./BF.x $JSON_FILE $DATACARDS_DIR $SIGNAL"
./BF.x "$JSON_FILE" "$DATACARDS_DIR" "$SIGNAL"

echo "[BF.sh] Job finished successfully"

