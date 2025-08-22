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
SIG_TYPE=""
LUMI=""
SMS_FILTERS=""

while [[ $# -gt 0 ]]; do
    key="$1"
    case $key in
        --bin) BIN=$(clean_arg "$2"); shift 2;;
        --file) ROOTFILE=$(clean_arg "$2"); shift 2;;
        --output-json) OUTPUT_JSON=$(clean_arg "$2"); shift 2;;
        --output-hist) OUTPUT_HIST=$(clean_arg "$2"); shift 2;;
        --hist-yaml) HIST_YAML=$(clean_arg "$2"); shift 2;;
        --cuts) CUTS=$(clean_arg "$2"); shift 2;;
        --lep-cuts) LEP_CUTS=$(clean_arg "$2"); shift 2;;
        --predefined-cuts) PREDEF_CUTS=$(clean_arg "$2"); shift 2;;
        --sig-type) SIG_TYPE=$(clean_arg "$2"); shift 2;;
        --lumi) LUMI=$(clean_arg "$2"); shift 2;;
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

# --- Build command ---
CMD=(./BFI_condor.x --bin "$BIN" --file "$ROOTFILE")

# JSON output
[[ -n "$OUTPUT_JSON" ]] && CMD+=(--output "$OUTPUT_JSON" --json)

# Hist output + yaml
[[ -n "$OUTPUT_HIST" ]] && CMD+=(--hist --hist-output "$OUTPUT_HIST")
[[ -n "$HIST_YAML" ]] && CMD+=(--hist-yaml "$HIST_YAML")

# Other args
[[ -n "$CUTS" ]] && CMD+=(--cuts "$CUTS")
[[ -n "$LEP_CUTS" ]] && CMD+=(--lep-cuts "$LEP_CUTS")
[[ -n "$PREDEF_CUTS" ]] && CMD+=(--predefined-cuts "$PREDEF_CUTS")
[[ -n "$SIG_TYPE" ]] && CMD+=(--t "$SIG_TYPE")
[[ -n "$LUMI" ]] && CMD+=(--lumi "$LUMI")
[[ -n "$SMS_FILTERS" ]] && CMD+=(--m "$SMS_FILTERS")

# --- Echo and run ---
echo "Running BFI_condor.x with arguments:"
printf ' %q' "${CMD[@]}"
echo
stdbuf -oL -eL "${CMD[@]}"

echo "[$(date)] Job finished."

