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

chmod +x BFI_condor.x

clean_arg() { echo "$1" | tr -d '\n' | tr -d '\r' | xargs; }

BIN=""
ROOTFILE=""
OUTPUT_JSON=""
OUTPUT_HIST=""
HIST_YAML=""
CUTS=""
LEP_CUTS=""
PREDEF_CUTS=""
USER_CUTS=""
SIG_TYPE=""
LUMI=""
SMS_FILTERS=""
JSON_FLAG=""
HIST_FLAG=""

# --- Parse arguments ---
while [[ $# -gt 0 ]]; do
    key="$1"
    case $key in
        --bin) BIN=$(clean_arg "$2"); shift 2;;
        --file) ROOTFILE=$(clean_arg "$2"); shift 2;;

        # Standalone flags
        --json) JSON_FLAG="--json"; shift;;
        --hist) HIST_FLAG="--hist"; shift;;

        # Output filenames
        --json-output) OUTPUT_JSON=$(clean_arg "$2"); shift 2;;
        --root-output) OUTPUT_HIST=$(clean_arg "$2"); shift 2;;
        --hist-yaml) HIST_YAML=$(clean_arg "$2"); shift 2;;

        # Cuts
        --cuts) CUTS=$(clean_arg "$2"); shift 2;;
        --lep-cuts) LEP_CUTS=$(clean_arg "$2"); shift 2;;
        --predefined-cuts) PREDEF_CUTS=$(clean_arg "$2"); shift 2;;
        --user-cuts) USER_CUTS=$(clean_arg "$2"); shift 2;;

        # Other options
        --sig-type) SIG_TYPE=$(clean_arg "$2"); shift 2;;
        --lumi) LUMI=$(clean_arg "$2"); shift 2;;

        # Multi-value argument
        --sms-filters)
            shift
            SMS_FILTERS=""
            while [[ $# -gt 0 && ! $1 == --* ]]; do
                SMS_FILTERS+="$1,"
                shift
            done
            SMS_FILTERS=${SMS_FILTERS%,}
            ;;

        *) echo "Unknown option $1"; shift;;
    esac
done

# --- Auto-generate output filenames if not provided ---
base_name=$(basename "$ROOTFILE" .root)
if [[ -z "$OUTPUT_JSON" ]]; then
    OUTPUT_JSON="${BIN}_${base_name}.json"
fi
if [[ -z "$OUTPUT_HIST" ]]; then
    OUTPUT_HIST="${BIN}_${base_name}.root"
fi

# Force basenames to stay local
OUTPUT_JSON=$(basename "$OUTPUT_JSON")
OUTPUT_HIST=$(basename "$OUTPUT_HIST")
[[ -n "$HIST_YAML" ]] && HIST_YAML=$(basename "$HIST_YAML")

# --- Build command as a single quoted string ---
CMD="./BFI_condor.x --bin \"$BIN\" --file \"$ROOTFILE\""

[[ -n "$JSON_FLAG" ]] && CMD="$CMD $JSON_FLAG"
[[ -n "$OUTPUT_JSON" ]] && CMD="$CMD --json-output \"$OUTPUT_JSON\""

[[ -n "$HIST_FLAG" ]] && CMD="$CMD $HIST_FLAG"
[[ -n "$OUTPUT_HIST" ]] && CMD="$CMD --root-output \"$OUTPUT_HIST\""
[[ -n "$HIST_YAML" ]] && CMD="$CMD --hist-yaml \"$HIST_YAML\""

[[ -n "$CUTS" ]] && CMD="$CMD --cuts \"$CUTS\""
[[ -n "$LEP_CUTS" ]] && CMD="$CMD --lep-cuts \"$LEP_CUTS\""
[[ -n "$PREDEF_CUTS" ]] && CMD="$CMD --predefined-cuts \"$PREDEF_CUTS\""
[[ -n "$USER_CUTS" ]] && CMD="$CMD --user-cuts \"$USER_CUTS\""

[[ -n "$SIG_TYPE" ]] && CMD="$CMD --sig-type \"$SIG_TYPE\""
[[ -n "$LUMI" ]] && CMD="$CMD --lumi \"$LUMI\""
[[ -n "$SMS_FILTERS" ]] && CMD="$CMD --sms-filters \"$SMS_FILTERS\""

# --- Echo and run ---
echo "Running BFI_condor.x with command:"
echo "$CMD"
eval "$CMD"

echo "[$(date)] Job finished."
