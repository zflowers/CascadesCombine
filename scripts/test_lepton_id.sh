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
VERSION="v0"
make clean
make all -j 8
echo "Submitting Regions..."

# need to edit ST to turn off all but dilept ttbar before running
# Call script to make regions
python3 python/make_Regions.py

# All Regions
rm -f config/bin_cfgs/Regions_Run3.yaml # Clean up yaml

cat config/bin_cfgs/Regions_Run3_2L_0J_lPTISR_Gold.yaml >> config/bin_cfgs/Regions_Run3.yaml
cat config/bin_cfgs/Regions_Run3_2L_0J_lPTISR_Silver.yaml >> config/bin_cfgs/Regions_Run3.yaml
cat config/bin_cfgs/Regions_Run3_2L_0J_lPTISR_Bronze.yaml >> config/bin_cfgs/Regions_Run3.yaml
cat config/bin_cfgs/Regions_Run3_2L_1J_lPTISR_Gold.yaml >> config/bin_cfgs/Regions_Run3.yaml
cat config/bin_cfgs/Regions_Run3_2L_1J_lPTISR_Silver.yaml >> config/bin_cfgs/Regions_Run3.yaml
cat config/bin_cfgs/Regions_Run3_2L_1J_lPTISR_Bronze.yaml >> config/bin_cfgs/Regions_Run3.yaml
cat config/bin_cfgs/Regions_Run3_2L_0J_hPTISR_Gold.yaml >> config/bin_cfgs/Regions_Run3.yaml
cat config/bin_cfgs/Regions_Run3_2L_0J_hPTISR_Silver.yaml >> config/bin_cfgs/Regions_Run3.yaml
cat config/bin_cfgs/Regions_Run3_2L_0J_hPTISR_Bronze.yaml >> config/bin_cfgs/Regions_Run3.yaml
cat config/bin_cfgs/Regions_Run3_2L_1J_hPTISR_Gold.yaml >> config/bin_cfgs/Regions_Run3.yaml
cat config/bin_cfgs/Regions_Run3_2L_1J_hPTISR_Silver.yaml >> config/bin_cfgs/Regions_Run3.yaml
cat config/bin_cfgs/Regions_Run3_2L_1J_hPTISR_Bronze.yaml >> config/bin_cfgs/Regions_Run3.yaml
cat config/bin_cfgs/Regions_Run3_3L_Jincl_lPTISR_Gold.yaml >> config/bin_cfgs/Regions_Run3.yaml
cat config/bin_cfgs/Regions_Run3_3L_Jincl_lPTISR_Silver.yaml >> config/bin_cfgs/Regions_Run3.yaml
cat config/bin_cfgs/Regions_Run3_3L_Jincl_lPTISR_Bronze.yaml >> config/bin_cfgs/Regions_Run3.yaml
cat config/bin_cfgs/Regions_Run3_3L_Jincl_hPTISR_Gold.yaml >> config/bin_cfgs/Regions_Run3.yaml
cat config/bin_cfgs/Regions_Run3_3L_Jincl_hPTISR_Silver.yaml >> config/bin_cfgs/Regions_Run3.yaml
cat config/bin_cfgs/Regions_Run3_3L_Jincl_hPTISR_Bronze.yaml >> config/bin_cfgs/Regions_Run3.yaml
cat config/bin_cfgs/Regions_Run3_4L_Gold.yaml >> config/bin_cfgs/Regions_Run3.yaml
cat config/bin_cfgs/Regions_Run3_4L_Silver.yaml >> config/bin_cfgs/Regions_Run3.yaml
cat config/bin_cfgs/Regions_Run3_4L_Bronze.yaml >> config/bin_cfgs/Regions_Run3.yaml
cat config/bin_cfgs/Regions_Run3_top_sideband_Gold.yaml >> config/bin_cfgs/Regions_Run3.yaml
cat config/bin_cfgs/Regions_Run3_top_sideband_Silver.yaml >> config/bin_cfgs/Regions_Run3.yaml
cat config/bin_cfgs/Regions_Run3_top_sideband_Bronze.yaml >> config/bin_cfgs/Regions_Run3.yaml

# Set BPJ to % of total
BIN_COUNT=$(grep -c 'Bin' config/bin_cfgs/Regions.yaml)
BINS_PER_JOB=$(awk -v n="$BIN_COUNT" 'BEGIN { printf "%d\n", n*0.05 + 0.5 }')

run_all --make-json --skip-compile --processes-cfg config/process_cfgs/processes_test_leptonid.yaml --bins-cfg config/bin_cfgs/Regions_Run3.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_Regions_234L_Run3_test_lepID_${VERSION}
sleep ${SLEEP}

sleep 30 # final sleep just to hold user from accidentally submitting something before last sub is out the door
echo "Submitted Regions!"
