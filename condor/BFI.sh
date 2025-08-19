#!/bin/bash
echo "[$(date)] Starting job on $(hostname)"
echo "[$(date)] Working directory: $(pwd)"

# Source CMS environment
source /cvmfs/cms.cern.ch/cmsset_default.sh

# Set architecture
export SCRAM_ARCH=el9_amd64_gcc12

# Load CMSSW release from CVMFS
cmsenv_dir=/cvmfs/cms.cern.ch/el9_amd64_gcc12/cms/cmssw/CMSSW_14_1_0_pre4/src
if [ -d "$cmsenv_dir" ]; then
    cd $cmsenv_dir
    eval `scram runtime -sh`
    cd -
else
    echo "CMSSW release not found on CVMFS!"
    exit 1
fi

# Use the forwarded X509 proxy
echo "Using proxy: $X509_USER_PROXY"
export X509_USER_PROXY=$X509_USER_PROXY

# Make sure executable is runnable
chmod +x BFI_condor.x
mkdir -p json/

# --- Helper to clean arguments ---
clean_arg() {
    # Remove newlines, carriage returns, and leading/trailing whitespace
    echo "$1" | tr -d '\n' | tr -d '\r' | xargs
}

# Parse arguments from "$@" manually to handle Condor quoting
BIN=""
ROOTFILE=""
OUTPUT_JSON=""
CUTS=""
LEP_CUTS=""
PREDEF_CUTS=""

while [[ $# -gt 0 ]]; do
    key="$1"
    case $key in
        --bin)
            BIN=$(clean_arg "$2"); shift; shift;;
        --file)
            ROOTFILE=$(clean_arg "$2"); shift; shift;;
        --output)
            OUTPUT_JSON=$(clean_arg "$2"); shift; shift;;
        --cuts)
            CUTS=$(clean_arg "$2"); shift; shift;;
        --lep-cuts)
            LEP_CUTS=$(clean_arg "$2"); shift; shift;;
        --predefined-cuts)
            PREDEF_CUTS=$(clean_arg "$2"); shift; shift;;
        *)
            echo "Unknown option $1"; shift;;
    esac
done

# Ensure json/ exists
mkdir -p json
debug_file="$(basename "${OUTPUT_JSON}" .json).debug"

CMD="./BFI_condor.x --bin \"$BIN\" --file \"$ROOTFILE\" --output \"$OUTPUT_JSON\""
[[ -n "$CUTS" ]] && CMD="$CMD --cuts \"$CUTS\""
[[ -n "$LEP_CUTS" ]] && CMD="$CMD --lep-cuts \"$LEP_CUTS\""
[[ -n "$PREDEF_CUTS" ]] && CMD="$CMD --predefined-cuts \"$PREDEF_CUTS\""

# Echo and run
echo "Running BFI_condor.x with arguments:"
echo "$CMD"

# Use eval to preserve the quotes
eval stdbuf -oL -eL $CMD 2>&1 | tee "$debug_file"

echo "[$(date)] Job finished."

