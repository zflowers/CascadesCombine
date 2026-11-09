run_all() {
    mkdir -p runs/
    echo "[run_all] Launching ./scripts/run_all.sh with arguments: $*"
    # Start detached using setsid (stdout to /dev/null, stderr -> terminal)
    setsid ./scripts/run_all.sh "$@" > /dev/null &
    PID=$!

    # Wait briefly to ensure a new run directory is created
    sleep 1
    RUN_DIR=$(ls -td runs/run_* 2>/dev/null | head -n1)
    if [[ -z "$RUN_DIR" ]]; then
        return
    fi

    DEBUG_LOG="$RUN_DIR/debug_run_combine.debug"
    if [[ -f "$DEBUG_LOG" ]]; then
        echo "[run_all] Debug log located at: $DEBUG_LOG"
        echo "[run_all] You can monitor it with:"
        echo "          tail -f $DEBUG_LOG"
    else
        echo "[run_all] Warning: Debug log does not exist yet at $DEBUG_LOG"
    fi
}

SLEEP=70
VERSION="v433"
make clean
make all -j 8
echo "Submitting Regions..."

# Generate all region and CR YAMLs from Derek's canonical master.
python3 python/make_Regions.py --master config/bin_cfgs/derek_master_Regions.yaml --outdir config/bin_cfgs || exit $?

# Set BPJ to % of total
BIN_COUNT=$(grep -c 'Bin' config/bin_cfgs/Regions.yaml)
BINS_PER_JOB=$(awk -v n="$BIN_COUNT" 'BEGIN { printf "%d\n", n*0.03 + 0.5 }')

# Run2+Run3 Sensitivity
# run_all --make-json --skip-compile --bins-cfg config/bin_cfgs/Regions.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_Regions_234L_Run2Run3_${VERSION}
# sleep ${SLEEP}

# Run2 Only Sensitivity
# run_all --make-json --skip-compile --processes-cfg config/process_cfgs/processes_Run2.yaml --bins-cfg config/bin_cfgs/Regions_Run2.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_Regions_234L_Run2_${VERSION}
# sleep ${SLEEP}

# all TChiWZ Run2 only
#run_all --skip-compile --skip-plot-yields --make-json --bins-cfg config/bin_cfgs/Regions_Run2.yaml --bins-per-job ${BINS_PER_JOB} --processes-cfg config/process_cfgs/processes_allTChiWZ_Run2.yaml --run-name Cascades_Regions_234L_allTChiWZ_Run2_${VERSION}
#sleep ${SLEEP}

# all preUL TChiWZ Run2 only
#run_all --skip-compile --skip-plot-yields --make-json --bins-cfg config/bin_cfgs/Regions_Run2.yaml --bins-per-job ${BINS_PER_JOB} --processes-cfg config/process_cfgs/processes_allpreULTChiWZ_Run2.yaml --run-name Cascades_Regions_234L_allpreULTChiWZ_Run2_${VERSION}
#sleep ${SLEEP}

# all TChiWZ Run2+Run3
#run_all --skip-compile --skip-plot-yields --make-json --bins-cfg config/bin_cfgs/Regions.yaml --bins-per-job ${BINS_PER_JOB} --processes-cfg config/process_cfgs/processes_allTChiWZ.yaml --run-name Cascades_Regions_234L_Run2_Run3_allTChiWZ_${VERSION}
#sleep ${SLEEP}

# all preULTSlepSlep Run2+Run3
#run_all --skip-compile --skip-plot-yields --make-json --bins-cfg config/bin_cfgs/Regions.yaml --bins-per-job ${BINS_PER_JOB} --processes-cfg config/process_cfgs/processes_allpreULTSlepSlep.yaml --run-name Cascades_Regions_234L_Run2_Run3_allpreULTSlepSlep_${VERSION}
#sleep ${SLEEP}

# all preULTChiWW Run2+Run3
#run_all --skip-compile --skip-plot-yields --make-json --bins-cfg config/bin_cfgs/Regions.yaml --bins-per-job ${BINS_PER_JOB} --processes-cfg config/process_cfgs/processes_allpreULTChiWW.yaml --run-name Cascades_Regions_234L_Run2_Run3_allpreULTChiWW_${VERSION}
#sleep ${SLEEP}

# all preULTChiWZ Run2+Run3
#run_all --skip-compile --skip-plot-yields --make-json --bins-cfg config/bin_cfgs/Regions.yaml --bins-per-job ${BINS_PER_JOB} --processes-cfg config/process_cfgs/processes_allpreULTChiWZ.yaml --run-name Cascades_Regions_234L_Run2_Run3_allpreULTChiWZ_${VERSION}
#sleep ${SLEEP}

# Run2 Only CR Fit
run_all --skip-plot-yields --make-FD --make-impacts --processes-cfg config/process_cfgs/data_processes_Run2.yaml --make-json --skip-compile --bins-cfg config/bin_cfgs/Regions_CR_Run2.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_CRFit_Impacts_FD_234L_Run2_${VERSION}
sleep ${SLEEP}

# Run3 Only CR Fit
#run_all --skip-plot-yields --make-FD --processes-cfg config/process_cfgs/data_processes_Run3.yaml --make-json --skip-compile --bins-cfg config/bin_cfgs/Regions_CR_Run3.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_CRFit_Impacts_FD_234L_Run3_${VERSION}
#sleep ${SLEEP}

# Run2 + Run3 CR Fit
# run_all --skip-plot-yields --make-impacts --make-FD --processes-cfg config/process_cfgs/data_processes.yaml --make-json --skip-compile --bins-cfg config/bin_cfgs/Regions_CR.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_CRFit_Impacts_FD_234L_Run2Run3_${VERSION}
# sleep ${SLEEP}

# --make-impacts to turn on impacts

sleep 30 # final sleep just to hold user from accidentally submitting something before last sub is out the door
echo "Submitted Regions!"
