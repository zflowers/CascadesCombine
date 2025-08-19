#!/bin/bash
echo "[$(date)] Starting job on $(hostname)"
echo "[$(date)] Working directory: $(pwd)"

# --- CMS environment ---
source /cvmfs/cms.cern.ch/cmsset_default.sh
export SCRAM_ARCH=el9_amd64_gcc12

cmsenv_dir=/cvmfs/cms.cern.ch/el9_amd64_gcc12/cms/cmssw/CMSSW_14_1_0_pre4/src
if [ -d "$cmsenv_dir" ]; then
    cd "$cmsenv_dir"
    eval `scram runtime -sh`
    cd -
else
    echo "CMSSW release not found on CVMFS!"
    exit 1
fi

# --- X509 proxy ---
echo "Using proxy: $X509_USER_PROXY"
export X509_USER_PROXY=$X509_USER_PROXY

# --- Make sure executable is runnable ---
chmod +x BFI_condor.x

# --- Helper to clean arguments ---
clean_arg() {
    echo "$1" | tr -d '\n' | tr -d '\r' | xargs
}

# --- Parse arguments ---
BIN=""
ROOTFILE=""
OUTPUT_JSON=""
CUTS=""
LEP_CUTS=""
PREDEF_CUTS=""

while [[ $# -gt 0 ]]; do
    key="$1"
    case $key in
        --bin) BIN=$(clean_arg "$2"); shift; shift;;
        --file) ROOTFILE=$(clean_arg "$2"); shift; shift;;
        --output) OUTPUT_JSON=$(clean_arg "$2"); shift; shift;;
        --cuts) CUTS=$(clean_arg "$2"); shift; shift;;
        --lep-cuts) LEP_CUTS=$(clean_arg "$2"); shift; shift;;
        --predefined-cuts) PREDEF_CUTS=$(clean_arg "$2"); shift; shift;;
        --sig-type) SIG_TYPE=$(clean_arg "$2"); shift; shift;;
        *) echo "Unknown option $1"; shift;;
    esac
done

# --- Debug file derived from --output ---
debug_file="condor/logs/$(basename "${OUTPUT_JSON}" .json).debug"

# --- Build command ---
CMD="./BFI_condor.x --bin \"$BIN\" --file \"$ROOTFILE\" --output \"$OUTPUT_JSON\""
[[ -n "$CUTS" ]] && CMD="$CMD --cuts \"$CUTS\""
[[ -n "$LEP_CUTS" ]] && CMD="$CMD --lep-cuts \"$LEP_CUTS\""
[[ -n "$PREDEF_CUTS" ]] && CMD="$CMD --predefined-cuts \"$PREDEF_CUTS\""
[[ -n "$SIG_TYPE" ]] && CMD="$CMD --sig-type $SIG_TYPE"

# --- Ensure directories exist --- 
mkdir -p "$(dirname "$OUTPUT_JSON")"
mkdir -p "$(dirname "$debug_file")"

# --- Echo and run ---
echo "Running BFI_condor.x with arguments:"
echo "$CMD"

# Use eval with stdbuf to preserve live stdout and tee into debug file
eval stdbuf -oL -eL $CMD 2>&1 | tee "$debug_file"

echo "[$(date)] Job finished."
