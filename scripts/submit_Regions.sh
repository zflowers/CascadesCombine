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
echo "Submitting Regions..."
SLEEP=60
VERSION="v149"
make clean
make all -j 8

run_all --make-json --skip-compile --bins-cfg config/bin_cfgs/Regions_2L_0J_lPTISR_Gold.yaml --run-name Cascades_2L_0J_lPTISR_Regions_Gold_${VERSION}
sleep ${SLEEP}
run_all --make-json --skip-compile --bins-cfg config/bin_cfgs/Regions_2L_0J_hPTISR_Gold.yaml --run-name Cascades_2L_0J_hPTISR_Regions_Gold_${VERSION}
sleep ${SLEEP}
run_all --make-json --skip-compile --bins-cfg config/bin_cfgs/Regions_2L_1J_lPTISR_Gold.yaml --run-name Cascades_2L_1J_lPTISR_Regions_Gold_${VERSION}
sleep ${SLEEP}
run_all --make-json --skip-compile --bins-cfg config/bin_cfgs/Regions_2L_1J_hPTISR_Gold.yaml --run-name Cascades_2L_1J_hPTISR_Regions_Gold_${VERSION}
sleep ${SLEEP}
#rm -f config/bin_cfgs/Regions_2L_Gold.yaml # Clean up yaml
#cat config/bin_cfgs/Regions_2L_0J_lPTISR_Gold.yaml >> config/bin_cfgs/Regions_2L_Gold.yaml
#cat config/bin_cfgs/Regions_2L_1J_lPTISR_Gold.yaml >> config/bin_cfgs/Regions_2L_Gold.yaml
#cat config/bin_cfgs/Regions_2L_0J_hPTISR_Gold.yaml >> config/bin_cfgs/Regions_2L_Gold.yaml
#cat config/bin_cfgs/Regions_2L_1J_hPTISR_Gold.yaml >> config/bin_cfgs/Regions_2L_Gold.yaml
#run_all --make-json --skip-compile --bins-cfg config/bin_cfgs/Regions_2L_Gold.yaml --run-name Cascades_2L_Regions_Gold_${VERSION}
#sleep ${SLEEP}
run_all --make-json --skip-compile --bins-cfg config/bin_cfgs/Regions_3L_Gold.yaml --run-name Cascades_3L_Regions_Gold_${VERSION}
sleep ${SLEEP}
run_all --make-json --skip-compile --bins-cfg config/bin_cfgs/Regions_4L_Gold.yaml --run-name Cascades_4L_Regions_Gold_${VERSION}
sleep ${SLEEP}
#
#rm -f config/bin_cfgs/Regions_Gold.yaml # Clean up yaml
#cat config/bin_cfgs/Regions_2L_0J_lPTISR_Gold.yaml >> config/bin_cfgs/Regions_Gold.yaml
#cat config/bin_cfgs/Regions_2L_1J_lPTISR_Gold.yaml >> config/bin_cfgs/Regions_Gold.yaml
#cat config/bin_cfgs/Regions_2L_0J_hPTISR_Gold.yaml >> config/bin_cfgs/Regions_Gold.yaml
#cat config/bin_cfgs/Regions_2L_1J_hPTISR_Gold.yaml >> config/bin_cfgs/Regions_Gold.yaml
#cat config/bin_cfgs/Regions_3L_Gold.yaml >> config/bin_cfgs/Regions_Gold.yaml
#cat config/bin_cfgs/Regions_4L_Gold.yaml >> config/bin_cfgs/Regions_Gold.yaml
#run_all --make-json --skip-compile --bins-cfg config/bin_cfgs/Regions_Gold.yaml --run-name Cascades_Regions_234L_Gold_${VERSION}
#sleep ${SLEEP}

run_all --make-json --skip-compile --bins-cfg config/bin_cfgs/Regions_2L_0J_lPTISR_Silver.yaml --run-name Cascades_2L_0J_lPTISR_Regions_Silver_${VERSION}
sleep ${SLEEP}
run_all --make-json --skip-compile --bins-cfg config/bin_cfgs/Regions_2L_0J_hPTISR_Silver.yaml --run-name Cascades_2L_0J_hPTISR_Regions_Silver_${VERSION}
sleep ${SLEEP}
run_all --make-json --skip-compile --bins-cfg config/bin_cfgs/Regions_2L_1J_lPTISR_Silver.yaml --run-name Cascades_2L_1J_lPTISR_Regions_Silver_${VERSION}
sleep ${SLEEP}
run_all --make-json --skip-compile --bins-cfg config/bin_cfgs/Regions_2L_1J_hPTISR_Silver.yaml --run-name Cascades_2L_1J_hPTISR_Regions_Silver_${VERSION}
sleep ${SLEEP}
#rm -f config/bin_cfgs/Regions_2L_Silver.yaml # Clean up yaml
#cat config/bin_cfgs/Regions_2L_0J_lPTISR_Silver.yaml >> config/bin_cfgs/Regions_2L_Silver.yaml
#cat config/bin_cfgs/Regions_2L_1J_lPTISR_Silver.yaml >> config/bin_cfgs/Regions_2L_Silver.yaml
#cat config/bin_cfgs/Regions_2L_0J_hPTISR_Silver.yaml >> config/bin_cfgs/Regions_2L_Silver.yaml
#cat config/bin_cfgs/Regions_2L_1J_hPTISR_Silver.yaml >> config/bin_cfgs/Regions_2L_Silver.yaml
#run_all --make-json --skip-compile --bins-cfg config/bin_cfgs/Regions_2L_Silver.yaml --run-name Cascades_2L_Regions_Silver_${VERSION}
#sleep ${SLEEP}
run_all --make-json --skip-compile --bins-cfg config/bin_cfgs/Regions_3L_Silver.yaml --run-name Cascades_3L_Regions_Silver_${VERSION}
sleep ${SLEEP}
run_all --make-json --skip-compile --bins-cfg config/bin_cfgs/Regions_4L_Silver.yaml --run-name Cascades_4L_Regions_Silver_${VERSION}
sleep ${SLEEP}
#
#rm -f config/bin_cfgs/Regions_Silver.yaml # Clean up yaml
#cat config/bin_cfgs/Regions_2L_0J_lPTISR_Silver.yaml >> config/bin_cfgs/Regions_Silver.yaml
#cat config/bin_cfgs/Regions_2L_1J_lPTISR_Silver.yaml >> config/bin_cfgs/Regions_Silver.yaml
#cat config/bin_cfgs/Regions_2L_0J_hPTISR_Silver.yaml >> config/bin_cfgs/Regions_Silver.yaml
#cat config/bin_cfgs/Regions_2L_1J_hPTISR_Silver.yaml >> config/bin_cfgs/Regions_Silver.yaml
#cat config/bin_cfgs/Regions_3L_Silver.yaml >> config/bin_cfgs/Regions_Silver.yaml
#cat config/bin_cfgs/Regions_4L_Silver.yaml >> config/bin_cfgs/Regions_Silver.yaml
#run_all --make-json --skip-compile --bins-cfg config/bin_cfgs/Regions_Silver.yaml --run-name Cascades_Regions_234L_Silver_${VERSION}
#sleep ${SLEEP}
#
run_all --make-json --skip-compile --bins-cfg config/bin_cfgs/Regions_2L_0J_lPTISR_Bronze.yaml --run-name Cascades_2L_0J_lPTISR_Regions_Bronze_${VERSION}
sleep ${SLEEP}
run_all --make-json --skip-compile --bins-cfg config/bin_cfgs/Regions_2L_0J_hPTISR_Bronze.yaml --run-name Cascades_2L_0J_hPTISR_Regions_Bronze_${VERSION}
sleep ${SLEEP}
run_all --make-json --skip-compile --bins-cfg config/bin_cfgs/Regions_2L_1J_lPTISR_Bronze.yaml --run-name Cascades_2L_1J_lPTISR_Regions_Bronze_${VERSION}
sleep ${SLEEP}
run_all --make-json --skip-compile --bins-cfg config/bin_cfgs/Regions_2L_1J_hPTISR_Bronze.yaml --run-name Cascades_2L_1J_hPTISR_Regions_Bronze_${VERSION}
sleep ${SLEEP}
#rm -f config/bin_cfgs/Regions_2L_Bronze.yaml # Clean up yaml
#cat config/bin_cfgs/Regions_2L_0J_lPTISR_Bronze.yaml >> config/bin_cfgs/Regions_2L_Bronze.yaml
#cat config/bin_cfgs/Regions_2L_1J_lPTISR_Bronze.yaml >> config/bin_cfgs/Regions_2L_Bronze.yaml
#cat config/bin_cfgs/Regions_2L_0J_hPTISR_Bronze.yaml >> config/bin_cfgs/Regions_2L_Bronze.yaml
#cat config/bin_cfgs/Regions_2L_1J_hPTISR_Bronze.yaml >> config/bin_cfgs/Regions_2L_Bronze.yaml
#run_all --make-json --skip-compile --bins-cfg config/bin_cfgs/Regions_2L_Bronze.yaml --run-name Cascades_2L_Regions_Bronze_${VERSION}
#sleep ${SLEEP}
run_all --make-json --skip-compile --bins-cfg config/bin_cfgs/Regions_3L_Bronze.yaml --run-name Cascades_3L_Regions_Bronze_${VERSION}
sleep ${SLEEP}
run_all --make-json --skip-compile --bins-cfg config/bin_cfgs/Regions_4L_Bronze.yaml --run-name Cascades_4L_Regions_Bronze_${VERSION}
sleep ${SLEEP}
#
#rm -f config/bin_cfgs/Regions_Bronze.yaml # Clean up yaml
#cat config/bin_cfgs/Regions_2L_0J_lPTISR_Bronze.yaml >> config/bin_cfgs/Regions_Bronze.yaml
#cat config/bin_cfgs/Regions_2L_1J_lPTISR_Bronze.yaml >> config/bin_cfgs/Regions_Bronze.yaml
#cat config/bin_cfgs/Regions_2L_0J_hPTISR_Bronze.yaml >> config/bin_cfgs/Regions_Bronze.yaml
#cat config/bin_cfgs/Regions_2L_1J_hPTISR_Bronze.yaml >> config/bin_cfgs/Regions_Bronze.yaml
#cat config/bin_cfgs/Regions_3L_Bronze.yaml >> config/bin_cfgs/Regions_Bronze.yaml
#cat config/bin_cfgs/Regions_4L_Bronze.yaml >> config/bin_cfgs/Regions_Bronze.yaml
#run_all --make-json --skip-compile --bins-cfg config/bin_cfgs/Regions_Bronze.yaml --run-name Cascades_Regions_234L_Bronze_${VERSION}
#sleep ${SLEEP}
#
#rm -f config/bin_cfgs/Regions.yaml # Clean up yaml
#cat config/bin_cfgs/Regions_2L_0J_lPTISR_Gold.yaml >> config/bin_cfgs/Regions.yaml
#cat config/bin_cfgs/Regions_2L_0J_lPTISR_Silver.yaml >> config/bin_cfgs/Regions.yaml
#cat config/bin_cfgs/Regions_2L_0J_lPTISR_Bronze.yaml >> config/bin_cfgs/Regions.yaml
#cat config/bin_cfgs/Regions_2L_1J_lPTISR_Gold.yaml >> config/bin_cfgs/Regions.yaml
#cat config/bin_cfgs/Regions_2L_1J_lPTISR_Silver.yaml >> config/bin_cfgs/Regions.yaml
#cat config/bin_cfgs/Regions_2L_1J_lPTISR_Bronze.yaml >> config/bin_cfgs/Regions.yaml
#cat config/bin_cfgs/Regions_2L_0J_hPTISR_Gold.yaml >> config/bin_cfgs/Regions.yaml
#cat config/bin_cfgs/Regions_2L_0J_hPTISR_Silver.yaml >> config/bin_cfgs/Regions.yaml
#cat config/bin_cfgs/Regions_2L_0J_hPTISR_Bronze.yaml >> config/bin_cfgs/Regions.yaml
#cat config/bin_cfgs/Regions_2L_1J_hPTISR_Gold.yaml >> config/bin_cfgs/Regions.yaml
#cat config/bin_cfgs/Regions_2L_1J_hPTISR_Silver.yaml >> config/bin_cfgs/Regions.yaml
#cat config/bin_cfgs/Regions_2L_1J_hPTISR_Bronze.yaml >> config/bin_cfgs/Regions.yaml
#cat config/bin_cfgs/Regions_3L_Gold.yaml >> config/bin_cfgs/Regions.yaml
#cat config/bin_cfgs/Regions_3L_Silver.yaml >> config/bin_cfgs/Regions.yaml
#cat config/bin_cfgs/Regions_3L_Bronze.yaml >> config/bin_cfgs/Regions.yaml
#cat config/bin_cfgs/Regions_4L_Gold.yaml >> config/bin_cfgs/Regions.yaml
#cat config/bin_cfgs/Regions_4L_Silver.yaml >> config/bin_cfgs/Regions.yaml
#cat config/bin_cfgs/Regions_4L_Bronze.yaml >> config/bin_cfgs/Regions.yaml
#run_all --make-json --skip-compile --bins-cfg config/bin_cfgs/Regions.yaml --run-name Cascades_Regions_234L_${VERSION}
#sleep ${SLEEP}

# Plotting studies
#VERSION="${VERSION}_Plots_Mperp_Tuning"
#run_all --make-json --make-root --bins-cfg config/bin_cfgs/Regions_2L_NoMperp.yaml --hist-cfg config/hist_cfgs/hist_RISR_Mperp.yaml --run-name Cascades_2L_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --make-root --bins-cfg config/bin_cfgs/Regions_3L_NoMperp.yaml --hist-cfg config/hist_cfgs/hist_RISR_Mperp.yaml --run-name Cascades_3L_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --make-root --bins-cfg config/bin_cfgs/Regions_4L_NoMperp.yaml --hist-cfg config/hist_cfgs/hist_RISR_Mperp.yaml --run-name Cascades_4L_${VERSION}
#sleep ${SLEEP}

sleep ${SLEEP} # final sleep just to hold user from accidentally submitting something before last sub is out the door
echo "Submitted Regions!"
