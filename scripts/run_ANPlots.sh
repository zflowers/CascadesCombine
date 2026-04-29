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
VERSION="v14"
make clean
make all -j 8

run_all --skip-compile --make-root --processes-cfg config/process_cfgs/processes_ANPlots.yaml --hist-cfg config/hist_cfgs/hist_ANPlots.yaml --bins-cfg config/bin_cfgs/ANPlots.yaml --bins-per-job 25 --run-name ANPlots_${VERSION}
sleep ${SLEEP} # sleep just to hold user from accidentally submitting something before sub is out the door
