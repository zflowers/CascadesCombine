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
VERSION="v55"
make clean
make all -j 8

#run_all --make-json --make-impacts --skip-compile --bins-cfg config/bin_cfgs/Regions_2L.yaml --run-name Cascades_2L_Regions_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --make-impacts --skip-compile --bins-cfg config/bin_cfgs/Regions_3L.yaml --run-name Cascades_3L_Regions_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --make-impacts --skip-compile --bins-cfg config/bin_cfgs/Regions_4L.yaml --run-name Cascades_4L_Regions_${VERSION}
#sleep ${SLEEP}
#
#rm -f config/bin_cfgs/Regions.yaml # Clean up Regions.yaml
#cat config/bin_cfgs/Regions_2L.yaml >> config/bin_cfgs/Regions.yaml
#cat config/bin_cfgs/Regions_3L.yaml >> config/bin_cfgs/Regions.yaml
#cat config/bin_cfgs/Regions_4L.yaml >> config/bin_cfgs/Regions.yaml
#run_all --make-json --make-impacts --skip-compile --bins-cfg config/bin_cfgs/Regions.yaml --run-name Cascades_Regions_234L_${VERSION}
#sleep ${SLEEP}

# Plotting studies
VERSION="${VERSION}_Plots"
run_all --make-json --make-root --bins-cfg config/bin_cfgs/SuperBin2L.yaml --hist-cfg config/hist_cfgs/hist_SuperBin2L.yaml --run-name Cascades_2L_SuperBin_Plots_${VERSION}
sleep ${SLEEP}
run_all --make-json --make-root --bins-cfg config/bin_cfgs/SuperBin3L.yaml --hist-cfg config/hist_cfgs/hist_SuperBin3L.yaml --run-name Cascades_3L_SuperBin_Plots_${VERSION}
sleep ${SLEEP}
run_all --make-json --make-root --bins-cfg config/bin_cfgs/SuperBin4L.yaml --hist-cfg config/hist_cfgs/hist_SuperBin4L.yaml --run-name Cascades_4L_SuperBin_Plots_${VERSION}
sleep ${SLEEP}

sleep ${SLEEP} # final sleep just to hold user from accidentally submitting something before last sub is out the door
echo "Submitted Regions!"
