source /cvmfs/cms.cern.ch/cmsset_default.sh
export CMSSW_GIT_REFERENCE=/cvmfs/cms.cern.ch/cmssw.git.daily
source /cvmfs/cms.cern.ch/crab3/crab.sh
alias proxy='voms-proxy-init --voms cms -valid 192:00'
alias setup_combine='export SCRAM_ARCH=el9_amd64_gcc12 && cd /uscms/home/z374f439/nobackup/CMSSW_14_1_0_pre4/src/CascadesCombine/ && cmsenv'
alias condor_rm_user='condor_rm $USER -n lpcschedd4; condor_rm $USER -n lpcschedd5; condor_rm $USER -n lpcschedd6'
alias condor='watch condor_q $USER -batch'
run_all() {
    mkdir -p runs/
    echo "[run_all] Launching ./scripts/run_all.sh with arguments: $*"
    # Start detached using setsid (stdout to /dev/null, stderr -> terminal)
    setsid ./scripts/run_all.sh "$@" > /dev/null &
    PID=$!
    echo "[run_all] Background PID: $PID"

    # Wait briefly to ensure a new run directory is created
    sleep 1
    RUN_DIR=$(ls -td runs/run_* 2>/dev/null | head -n1)
    if [[ -z "$RUN_DIR" ]]; then
        echo "[run_all] Warning: Could not find a run directory."
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
