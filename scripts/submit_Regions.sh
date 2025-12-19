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
BINS_PER_JOB=30
SLEEP=70
VERSION="v338"
make clean
make all -j 8
echo "Submitting Regions..."

#run_all --make-root --processes-cfg config/process_cfgs/LepEtaStudy_processes.yaml --hist-cfg config/hist_cfgs/LepEtaStudy.yaml --bins-cfg config/bin_cfgs/LepEtaStudy.yaml --run-name LepEtaStudy_v10
#sleep ${SLEEP}

# Call script to make regions
python3 python/make_Regions.py

# All Regions
rm -f config/bin_cfgs/Regions.yaml # Clean up yaml
cat config/bin_cfgs/Regions_2L_0J_lPTISR_Gold.yaml >> config/bin_cfgs/Regions.yaml
cat config/bin_cfgs/Regions_2L_0J_lPTISR_Silver.yaml >> config/bin_cfgs/Regions.yaml
cat config/bin_cfgs/Regions_2L_0J_lPTISR_Bronze.yaml >> config/bin_cfgs/Regions.yaml
cat config/bin_cfgs/Regions_2L_1J_lPTISR_Gold.yaml >> config/bin_cfgs/Regions.yaml
cat config/bin_cfgs/Regions_2L_1J_lPTISR_Silver.yaml >> config/bin_cfgs/Regions.yaml
cat config/bin_cfgs/Regions_2L_1J_lPTISR_Bronze.yaml >> config/bin_cfgs/Regions.yaml
cat config/bin_cfgs/Regions_2L_0J_hPTISR_Gold.yaml >> config/bin_cfgs/Regions.yaml
cat config/bin_cfgs/Regions_2L_0J_hPTISR_Silver.yaml >> config/bin_cfgs/Regions.yaml
cat config/bin_cfgs/Regions_2L_0J_hPTISR_Bronze.yaml >> config/bin_cfgs/Regions.yaml
cat config/bin_cfgs/Regions_2L_1J_hPTISR_Gold.yaml >> config/bin_cfgs/Regions.yaml
cat config/bin_cfgs/Regions_2L_1J_hPTISR_Silver.yaml >> config/bin_cfgs/Regions.yaml
cat config/bin_cfgs/Regions_2L_1J_hPTISR_Bronze.yaml >> config/bin_cfgs/Regions.yaml
cat config/bin_cfgs/Regions_3L_0J_lPTISR_Gold.yaml >> config/bin_cfgs/Regions.yaml
cat config/bin_cfgs/Regions_3L_1J_lPTISR_Gold.yaml >> config/bin_cfgs/Regions.yaml
cat config/bin_cfgs/Regions_3L_0J_lPTISR_Silver.yaml >> config/bin_cfgs/Regions.yaml
cat config/bin_cfgs/Regions_3L_1J_lPTISR_Silver.yaml >> config/bin_cfgs/Regions.yaml
cat config/bin_cfgs/Regions_3L_0J_lPTISR_Bronze.yaml >> config/bin_cfgs/Regions.yaml
cat config/bin_cfgs/Regions_3L_1J_lPTISR_Bronze.yaml >> config/bin_cfgs/Regions.yaml
cat config/bin_cfgs/Regions_3L_0J_hPTISR_Gold.yaml >> config/bin_cfgs/Regions.yaml
cat config/bin_cfgs/Regions_3L_1J_hPTISR_Gold.yaml >> config/bin_cfgs/Regions.yaml
cat config/bin_cfgs/Regions_3L_0J_hPTISR_Silver.yaml >> config/bin_cfgs/Regions.yaml
cat config/bin_cfgs/Regions_3L_1J_hPTISR_Silver.yaml >> config/bin_cfgs/Regions.yaml
cat config/bin_cfgs/Regions_3L_0J_hPTISR_Bronze.yaml >> config/bin_cfgs/Regions.yaml
cat config/bin_cfgs/Regions_3L_1J_hPTISR_Bronze.yaml >> config/bin_cfgs/Regions.yaml
cat config/bin_cfgs/Regions_4L_Gold.yaml >> config/bin_cfgs/Regions.yaml
cat config/bin_cfgs/Regions_4L_Silver.yaml >> config/bin_cfgs/Regions.yaml
cat config/bin_cfgs/Regions_4L_Bronze.yaml >> config/bin_cfgs/Regions.yaml
cat config/bin_cfgs/Regions_top_sideband_Gold.yaml >> config/bin_cfgs/Regions.yaml
cat config/bin_cfgs/Regions_top_sideband_Silver.yaml >> config/bin_cfgs/Regions.yaml
cat config/bin_cfgs/Regions_top_sideband_Bronze.yaml >> config/bin_cfgs/Regions.yaml
run_all --skip-plot-yields --make-json --skip-compile --bins-cfg config/bin_cfgs/Regions.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_Regions_234L_${VERSION}
sleep ${SLEEP}

# Alternative all bins runs
#run_all --skip-compile --skip-plot-yields --make-json --bins-cfg config/bin_cfgs/Regions.yaml --bins-per-job ${BINS_PER_JOB} --processes-cfg config/process_cfgs/processes_allTChiWZ.yaml --run-name Cascades_Regions_234L_allTChiWZ_${VERSION}
#sleep ${SLEEP}
#run_all --processes-cfg config/process_cfgs/processes_2018onlybkg.yaml --skip-plot-yields --make-json --skip-compile --bins-cfg config/bin_cfgs/Regions.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_Regions_234L_${VERSION}_2018onlybkg
#sleep ${SLEEP}

# CR Fit
rm -f config/bin_cfgs/Regions_Bronze_CR.yaml # Clean up yaml
cat config/bin_cfgs/Regions_2L_0J_lPTISR_Bronze.yaml >> config/bin_cfgs/Regions_Bronze_CR.yaml
cat config/bin_cfgs/Regions_2L_1J_lPTISR_Bronze.yaml >> config/bin_cfgs/Regions_Bronze_CR.yaml
cat config/bin_cfgs/Regions_2L_0J_hPTISR_Bronze.yaml >> config/bin_cfgs/Regions_Bronze_CR.yaml
cat config/bin_cfgs/Regions_2L_1J_hPTISR_Bronze.yaml >> config/bin_cfgs/Regions_Bronze_CR.yaml
cat config/bin_cfgs/Regions_3L_0J_lPTISR_Bronze.yaml >> config/bin_cfgs/Regions_Bronze_CR.yaml
cat config/bin_cfgs/Regions_3L_1J_lPTISR_Bronze.yaml >> config/bin_cfgs/Regions_Bronze_CR.yaml
cat config/bin_cfgs/Regions_3L_0J_hPTISR_Bronze.yaml >> config/bin_cfgs/Regions_Bronze_CR.yaml
cat config/bin_cfgs/Regions_3L_1J_hPTISR_Bronze.yaml >> config/bin_cfgs/Regions_Bronze_CR.yaml
cat config/bin_cfgs/Regions_4L_Bronze.yaml >> config/bin_cfgs/Regions_Bronze_CR.yaml
cat config/bin_cfgs/Regions_top_sideband_Bronze.yaml >> config/bin_cfgs/Regions_Bronze_CR.yaml
#run_all --skip-plot-yields --make-impacts --make-FD --processes-cfg config/process_cfgs/data_processes.yaml --make-json --skip-compile --bins-cfg config/bin_cfgs/Regions_Bronze_CR.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_CRs_Impacts_FD_234L_Bronze_${VERSION}
#sleep ${SLEEP}

#run_all --make-json --only-yields --skip-compile --bins-cfg config/bin_cfgs/Regions_2L_0J_lPTISR_Gold.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_2L_0J_lPTISR_Regions_Gold_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --only-yields --skip-compile --bins-cfg config/bin_cfgs/Regions_2L_0J_hPTISR_Gold.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_2L_0J_hPTISR_Regions_Gold_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --only-yields --skip-compile --bins-cfg config/bin_cfgs/Regions_2L_1J_lPTISR_Gold.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_2L_1J_lPTISR_Regions_Gold_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --only-yields --skip-compile --bins-cfg config/bin_cfgs/Regions_2L_1J_hPTISR_Gold.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_2L_1J_hPTISR_Regions_Gold_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --only-yields --skip-compile --bins-cfg config/bin_cfgs/Regions_3L_0J_lPTISR_Gold.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_3L_0J_lPTISR_Regions_Gold_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --only-yields --skip-compile --bins-cfg config/bin_cfgs/Regions_3L_1J_lPTISR_Gold.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_3L_1J_lPTISR_Regions_Gold_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --only-yields --skip-compile --bins-cfg config/bin_cfgs/Regions_3L_0J_hPTISR_Gold.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_3L_0J_hPTISR_Regions_Gold_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --only-yields --skip-compile --bins-cfg config/bin_cfgs/Regions_3L_1J_hPTISR_Gold.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_3L_1J_hPTISR_Regions_Gold_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --only-yields --skip-compile --bins-cfg config/bin_cfgs/Regions_4L_Gold.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_4L_Regions_Gold_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --only-yields --skip-compile --bins-cfg config/bin_cfgs/Regions_2L_0J_lPTISR_Silver.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_2L_0J_lPTISR_Regions_Silver_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --only-yields --skip-compile --bins-cfg config/bin_cfgs/Regions_2L_0J_hPTISR_Silver.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_2L_0J_hPTISR_Regions_Silver_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --only-yields --skip-compile --bins-cfg config/bin_cfgs/Regions_2L_1J_lPTISR_Silver.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_2L_1J_lPTISR_Regions_Silver_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --only-yields --skip-compile --bins-cfg config/bin_cfgs/Regions_2L_1J_hPTISR_Silver.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_2L_1J_hPTISR_Regions_Silver_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --only-yields --skip-compile --bins-cfg config/bin_cfgs/Regions_3L_0J_lPTISR_Silver.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_3L_0J_lPTISR_Regions_Silver_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --only-yields --skip-compile --bins-cfg config/bin_cfgs/Regions_3L_1J_lPTISR_Silver.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_3L_1J_lPTISR_Regions_Silver_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --only-yields --skip-compile --bins-cfg config/bin_cfgs/Regions_3L_0J_hPTISR_Silver.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_3L_0J_hPTISR_Regions_Silver_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --only-yields --skip-compile --bins-cfg config/bin_cfgs/Regions_3L_1J_hPTISR_Silver.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_3L_1J_hPTISR_Regions_Silver_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --only-yields --skip-compile --bins-cfg config/bin_cfgs/Regions_4L_Silver.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_4L_Regions_Silver_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --only-yields --skip-compile --bins-cfg config/bin_cfgs/Regions_2L_0J_lPTISR_Bronze.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_2L_0J_lPTISR_Regions_Bronze_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --only-yields --skip-compile --bins-cfg config/bin_cfgs/Regions_2L_0J_hPTISR_Bronze.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_2L_0J_hPTISR_Regions_Bronze_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --only-yields --skip-compile --bins-cfg config/bin_cfgs/Regions_2L_1J_lPTISR_Bronze.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_2L_1J_lPTISR_Regions_Bronze_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --only-yields --skip-compile --bins-cfg config/bin_cfgs/Regions_2L_1J_hPTISR_Bronze.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_2L_1J_hPTISR_Regions_Bronze_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --only-yields --skip-compile --bins-cfg config/bin_cfgs/Regions_3L_0J_lPTISR_Bronze.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_3L_0J_lPTISR_Regions_Bronze_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --only-yields --skip-compile --bins-cfg config/bin_cfgs/Regions_3L_1J_lPTISR_Bronze.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_3L_1J_lPTISR_Regions_Bronze_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --only-yields --skip-compile --bins-cfg config/bin_cfgs/Regions_3L_0J_hPTISR_Bronze.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_3L_0J_hPTISR_Regions_Bronze_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --only-yields --skip-compile --bins-cfg config/bin_cfgs/Regions_3L_1J_hPTISR_Bronze.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_3L_1J_hPTISR_Regions_Bronze_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --only-yields --skip-compile --bins-cfg config/bin_cfgs/Regions_4L_Bronze.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_4L_Regions_Bronze_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --only-yields --skip-compile --bins-cfg config/bin_cfgs/Regions_top_sideband_Bronze.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_Regions_top_sideband_Bronze_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --only-yields --skip-compile --bins-cfg config/bin_cfgs/Regions_top_sideband_Silver.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_Regions_top_sideband_Silver_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --only-yields --skip-compile --bins-cfg config/bin_cfgs/Regions_top_sideband_Gold.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_Regions_top_sideband_Gold_${VERSION}
#sleep ${SLEEP}

# For CR Yields
#run_all --make-json --only-yields --skip-compile --processes-cfg config/process_cfgs/data_processes.yaml --bins-cfg config/bin_cfgs/Regions_2L_0J_lPTISR_Bronze.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_2L_0J_lPTISR_Regions_Bronze_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --only-yields --skip-compile --processes-cfg config/process_cfgs/data_processes.yaml --bins-cfg config/bin_cfgs/Regions_2L_0J_hPTISR_Bronze.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_2L_0J_hPTISR_Regions_Bronze_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --only-yields --skip-compile --processes-cfg config/process_cfgs/data_processes.yaml --bins-cfg config/bin_cfgs/Regions_2L_1J_lPTISR_Bronze.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_2L_1J_lPTISR_Regions_Bronze_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --only-yields --skip-compile --processes-cfg config/process_cfgs/data_processes.yaml --bins-cfg config/bin_cfgs/Regions_2L_1J_hPTISR_Bronze.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_2L_1J_hPTISR_Regions_Bronze_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --only-yields --skip-compile --processes-cfg config/process_cfgs/data_processes.yaml --bins-cfg config/bin_cfgs/Regions_3L_0J_lPTISR_Bronze.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_3L_0J_lPTISR_Regions_Bronze_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --only-yields --skip-compile --processes-cfg config/process_cfgs/data_processes.yaml --bins-cfg config/bin_cfgs/Regions_3L_1J_lPTISR_Bronze.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_3L_1J_lPTISR_Regions_Bronze_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --only-yields --skip-compile --processes-cfg config/process_cfgs/data_processes.yaml --bins-cfg config/bin_cfgs/Regions_3L_0J_hPTISR_Bronze.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_3L_0J_hPTISR_Regions_Bronze_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --only-yields --skip-compile --processes-cfg config/process_cfgs/data_processes.yaml --bins-cfg config/bin_cfgs/Regions_3L_1J_hPTISR_Bronze.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_3L_1J_hPTISR_Regions_Bronze_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --only-yields --skip-compile --processes-cfg config/process_cfgs/data_processes.yaml --bins-cfg config/bin_cfgs/Regions_4L_Bronze.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_4L_Regions_Bronze_${VERSION}
#sleep ${SLEEP}

# Plotting studies
#VERSION="${VERSION}_Plots_Mperp_Tuning"
#run_all --make-json --make-root --bins-cfg config/bin_cfgs/Regions_2L_NoMperp.yaml --hist-cfg config/hist_cfgs/hist_RISR_Mperp.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_2L_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --make-root --bins-cfg config/bin_cfgs/Regions_3L_NoMperp.yaml --hist-cfg config/hist_cfgs/hist_RISR_Mperp.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_3L_${VERSION}
#sleep ${SLEEP}
#run_all --make-json --make-root --bins-cfg config/bin_cfgs/Regions_4L_NoMperp.yaml --hist-cfg config/hist_cfgs/hist_RISR_Mperp.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_4L_${VERSION}
#sleep ${SLEEP}

#run_all --make-root --processes-cfg config/process_cfgs/WjetsTune_processes.yaml --hist-cfg config/hist_cfgs/WjetsTune.yaml --bins-cfg config/bin_cfgs/WjetsTune.yaml --run-name Cascades_WjetsTune_${VERSION}
#sleep ${SLEEP}

# Combinations
#rm -f config/bin_cfgs/Regions_2L_Gold.yaml # Clean up yaml
#cat config/bin_cfgs/Regions_2L_0J_lPTISR_Gold.yaml >> config/bin_cfgs/Regions_2L_Gold.yaml
#cat config/bin_cfgs/Regions_2L_1J_lPTISR_Gold.yaml >> config/bin_cfgs/Regions_2L_Gold.yaml
#cat config/bin_cfgs/Regions_2L_0J_hPTISR_Gold.yaml >> config/bin_cfgs/Regions_2L_Gold.yaml
#cat config/bin_cfgs/Regions_2L_1J_hPTISR_Gold.yaml >> config/bin_cfgs/Regions_2L_Gold.yaml
#run_all --make-json --skip-compile --bins-cfg config/bin_cfgs/Regions_2L_Gold.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_2L_Regions_Gold_${VERSION}

#rm -f config/bin_cfgs/Regions_2L_Silver.yaml # Clean up yaml
#cat config/bin_cfgs/Regions_2L_0J_lPTISR_Silver.yaml >> config/bin_cfgs/Regions_2L_Silver.yaml
#cat config/bin_cfgs/Regions_2L_1J_lPTISR_Silver.yaml >> config/bin_cfgs/Regions_2L_Silver.yaml
#cat config/bin_cfgs/Regions_2L_0J_hPTISR_Silver.yaml >> config/bin_cfgs/Regions_2L_Silver.yaml
#cat config/bin_cfgs/Regions_2L_1J_hPTISR_Silver.yaml >> config/bin_cfgs/Regions_2L_Silver.yaml
#run_all --make-json --skip-compile --bins-cfg config/bin_cfgs/Regions_2L_Silver.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_2L_Regions_Silver_${VERSION}
#sleep ${SLEEP}

#rm -f config/bin_cfgs/Regions_2L_Bronze.yaml # Clean up yaml
#cat config/bin_cfgs/Regions_2L_0J_lPTISR_Bronze.yaml >> config/bin_cfgs/Regions_2L_Bronze.yaml
#cat config/bin_cfgs/Regions_2L_1J_lPTISR_Bronze.yaml >> config/bin_cfgs/Regions_2L_Bronze.yaml
#cat config/bin_cfgs/Regions_2L_0J_hPTISR_Bronze.yaml >> config/bin_cfgs/Regions_2L_Bronze.yaml
#cat config/bin_cfgs/Regions_2L_1J_hPTISR_Bronze.yaml >> config/bin_cfgs/Regions_2L_Bronze.yaml
#run_all --make-json --skip-compile --bins-cfg config/bin_cfgs/Regions_2L_Bronze.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_2L_Regions_Bronze_${VERSION}
#sleep ${SLEEP}

#rm -f config/bin_cfgs/Regions_Gold.yaml # Clean up yaml
#cat config/bin_cfgs/Regions_2L_0J_lPTISR_Gold.yaml >> config/bin_cfgs/Regions_Gold.yaml
#cat config/bin_cfgs/Regions_2L_1J_lPTISR_Gold.yaml >> config/bin_cfgs/Regions_Gold.yaml
#cat config/bin_cfgs/Regions_2L_0J_hPTISR_Gold.yaml >> config/bin_cfgs/Regions_Gold.yaml
#cat config/bin_cfgs/Regions_2L_1J_hPTISR_Gold.yaml >> config/bin_cfgs/Regions_Gold.yaml
#cat config/bin_cfgs/Regions_3L_0J_lPTISR_Gold.yaml >> config/bin_cfgs/Regions_Gold.yaml
#cat config/bin_cfgs/Regions_3L_1J_lPTISR_Gold.yaml >> config/bin_cfgs/Regions_Gold.yaml
#cat config/bin_cfgs/Regions_3L_0J_hPTISR_Gold.yaml >> config/bin_cfgs/Regions_Gold.yaml
#cat config/bin_cfgs/Regions_3L_1J_hPTISR_Gold.yaml >> config/bin_cfgs/Regions_Gold.yaml
#cat config/bin_cfgs/Regions_4L_Gold.yaml >> config/bin_cfgs/Regions_Gold.yaml
#run_all --make-json --skip-compile --bins-cfg config/bin_cfgs/Regions_Gold.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_Regions_234L_Gold_${VERSION}
#sleep ${SLEEP}

#rm -f config/bin_cfgs/Regions_Silver.yaml # Clean up yaml
#cat config/bin_cfgs/Regions_2L_0J_lPTISR_Silver.yaml >> config/bin_cfgs/Regions_Silver.yaml
#cat config/bin_cfgs/Regions_2L_1J_lPTISR_Silver.yaml >> config/bin_cfgs/Regions_Silver.yaml
#cat config/bin_cfgs/Regions_2L_0J_hPTISR_Silver.yaml >> config/bin_cfgs/Regions_Silver.yaml
#cat config/bin_cfgs/Regions_2L_1J_hPTISR_Silver.yaml >> config/bin_cfgs/Regions_Silver.yaml
#cat config/bin_cfgs/Regions_3L_0J_lPTISR_Silver.yaml >> config/bin_cfgs/Regions_Silver.yaml
#cat config/bin_cfgs/Regions_3L_1J_lPTISR_Silver.yaml >> config/bin_cfgs/Regions_Silver.yaml
#cat config/bin_cfgs/Regions_3L_0J_hPTISR_Silver.yaml >> config/bin_cfgs/Regions_Silver.yaml
#cat config/bin_cfgs/Regions_3L_1J_hPTISR_Silver.yaml >> config/bin_cfgs/Regions_Silver.yaml
#cat config/bin_cfgs/Regions_4L_Silver.yaml >> config/bin_cfgs/Regions_Silver.yaml
#run_all --make-json --skip-compile --bins-cfg config/bin_cfgs/Regions_Silver.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_Regions_234L_Silver_${VERSION}
#sleep ${SLEEP}

#rm -f config/bin_cfgs/Regions_Bronze.yaml # Clean up yaml
#cat config/bin_cfgs/Regions_2L_0J_lPTISR_Bronze.yaml >> config/bin_cfgs/Regions_Bronze.yaml
#cat config/bin_cfgs/Regions_2L_1J_lPTISR_Bronze.yaml >> config/bin_cfgs/Regions_Bronze.yaml
#cat config/bin_cfgs/Regions_2L_0J_hPTISR_Bronze.yaml >> config/bin_cfgs/Regions_Bronze.yaml
#cat config/bin_cfgs/Regions_2L_1J_hPTISR_Bronze.yaml >> config/bin_cfgs/Regions_Bronze.yaml
#cat config/bin_cfgs/Regions_3L_0J_lPTISR_Bronze.yaml >> config/bin_cfgs/Regions_Bronze.yaml
#cat config/bin_cfgs/Regions_3L_1J_lPTISR_Bronze.yaml >> config/bin_cfgs/Regions_Bronze.yaml
#cat config/bin_cfgs/Regions_3L_0J_hPTISR_Bronze.yaml >> config/bin_cfgs/Regions_Bronze.yaml
#cat config/bin_cfgs/Regions_3L_1J_hPTISR_Bronze.yaml >> config/bin_cfgs/Regions_Bronze.yaml
#cat config/bin_cfgs/Regions_4L_Bronze.yaml >> config/bin_cfgs/Regions_Bronze.yaml
#run_all --make-json --skip-compile --bins-cfg config/bin_cfgs/Regions_Bronze.yaml --bins-per-job ${BINS_PER_JOB} --run-name Cascades_Regions_234L_Bronze_${VERSION}
#sleep ${SLEEP}

sleep ${SLEEP} # final sleep just to hold user from accidentally submitting something before last sub is out the door
echo "Submitted Regions!"
