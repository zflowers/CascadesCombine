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
BINS_PER_JOB=50
SLEEP=70
VERSION="v4"
make clean
make all -j 8
echo "Submitting Regions..."

run_all --make-root --skip-compile --processes-cfg config/process_cfgs/processes_NSFProposal.yaml --hist-cfg config/hist_cfgs/hist_NSFProposal.yaml --bins-cfg config/bin_cfgs/NSFProposal.yaml --run-name NSFProposal_${VERSION}
sleep ${SLEEP}
run_all --make-root --skip-compile --processes-cfg config/process_cfgs/processes_NSFProposal_bkg.yaml --hist-cfg config/hist_cfgs/hist_NSFProposal.yaml --bins-cfg config/bin_cfgs/NSFProposal.yaml --run-name NSFProposal_${VERSION}_bkg
sleep ${SLEEP}
run_all --make-root --skip-compile --processes-cfg config/process_cfgs/processes_NSFProposal_sig.yaml --hist-cfg config/hist_cfgs/hist_NSFProposal.yaml --bins-cfg config/bin_cfgs/NSFProposal.yaml --run-name NSFProposal_${VERSION}_sig
sleep ${SLEEP}

sleep ${SLEEP} # final sleep just to hold user from accidentally submitting something before last sub is out the door
echo "Submitted Regions!"
