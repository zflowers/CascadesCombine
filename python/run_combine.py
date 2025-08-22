#!/usr/bin/env python3
import subprocess, time, glob, os, sys, time, argparse, json
from CondorJobCountMonitor import CondorJobCountMonitor

# ----- helper functions -----
def clean_binaries():
    """
    Run `make clean` to clean out binaries.
    """
    print("[run_all] Cleaning previous builds...", flush=True)
    subprocess.run(["make", "clean"], check=True)

def build_binaries():
    """
    Run `make all -j8` to build the latest binaries.
    """
    print("[run_all] Building all binaries...", flush=True)
    subprocess.run(["make", "all", "-j8"], check=True)

    print("[run_all] Build finished.", flush=True)

def wait_for_jobs():
    """
    Block until current user's Condor job count is below the monitor threshold.
    Uses CondorJobCountMonitor to check the current user's jobs.
    """
    monitor = CondorJobCountMonitor(threshold=1, verbose=False)
    monitor.wait_until_jobs_below()

def submit_jobs(stress_test, config, datasets, hist, make_json=False, make_root=False):
    """
    Runs submitJobs.py to generate Condor scripts and merge scripts.
    Must create `master_merge.sh` with all merge commands.
    """
    cmd = ["python3", "python/submitJobs.py", "--bins-cfg", config, "--datasets-cfg", datasets]

    if stress_test:
        cmd.append("--stress_test")
    if make_json:
        cmd.append("--make-json")
    if make_root:
        cmd.append("--make-root")
        if hist:
            cmd.append("--hist-yaml")
            cmd.append(hist)
    subprocess.run(cmd, check=True, stdout=sys.stdout, stderr=sys.stderr)

def get_flattened_json_path():
    """
    Return the path to the flattened JSON produced by the merge scripts.
    Picks the first file matching flattened_*.json in the json/ directory.
    """
    flattened_files = glob.glob("json/flattened_*.json")
    if not flattened_files:
        raise FileNotFoundError("No flattened JSON files found in json/")
    # Pick the first (or the most recently modified) file
    # Option: sort by mtime and pick the latest:
    flattened_files.sort(key=os.path.getmtime, reverse=True)
    return flattened_files[0]

def get_flattened_root_path():
    root_files = glob.glob("root/merged_*.root")
    if not root_files:
        raise FileNotFoundError("No merged ROOT files found in root/")
    root_files.sort(key=os.path.getmtime, reverse=True)
    return root_files[0]

def get_output_dir():
    """
    Return the directory where BF.x should write datacards.
    Derived from the flattened JSON filename.
    """
    flattened_json = get_flattened_json_path()
    base_name = os.path.basename(flattened_json)
    name_without_ext = os.path.splitext(base_name)[0]
    if name_without_ext.startswith("flattened_"):
        name_without_ext = name_without_ext[len("flattened_"):]  # remove prefix
    output_dir = f"datacards_{name_without_ext}"
    return output_dir

def print_events(json_file):
    with open(json_file, "r") as f:
        data = json.load(f)
    for bin_name, processes in data.items():
        print(f"Bin: {bin_name}")
        for proc_name, values in processes.items():
            if len(values) > 1:
                weighted_events = round(values[1], 2)
                print(f"  {proc_name}: {weighted_events}")

def get_work_dirs():
    """
    Return the directories from condor/.
    """
    return [name for name in os.listdir("condor/") if os.path.isdir(os.path.join("condor", name))]

def run_checkjobs_loop_parallel(no_resubmit=False, max_resubmits=3, check_json=False, check_root=False):
    """
    Check all work directories with checkJobs.py, resubmit failing jobs across
    all directories in one cycle, then wait once for all resubmitted jobs to finish.
    Returns True if no failed jobs remain (proceed), False on error or if max resubmits reached.
    """
    work_dirs = get_work_dirs()
    if not work_dirs:
        print("[run_all] No work directories found under condor/.", flush=True)
        return True

    attempt = 0
    check_marker_no_failed = "No failed jobs to resubmit."
    check_marker_resub_ok = "Resubmit submitted successfully."

    while attempt < max_resubmits:
        attempt += 1
        print(f"[run_all] Running checkJobs for all work_dirs (attempt {attempt}/{max_resubmits})...", flush=True)

        resubmitted_dirs = []
        any_unexpected = False

        for work_dir in work_dirs:
            check_cmd = ["python3", "python/checkJobs.py", work_dir]
            if check_json:
                check_cmd.append("--check-json")
            if check_root:
                check_cmd.append("--check-root")
            proc = subprocess.run(check_cmd, capture_output=True, text=True)

            # Print outputs (labeled)
            if proc.stderr:
                print(f"----- checkJobs stderr ({work_dir}) -----", file=sys.stderr, flush=True)
                print(proc.stderr, file=sys.stderr, flush=True)

            if proc.returncode != 0:
                print(f"[run_all] checkJobs.py returned non-zero ({proc.returncode}) for {work_dir}. Aborting.", file=sys.stderr, flush=True)
                return False

            stdout = proc.stdout or ""

            if check_marker_no_failed in stdout:
                # nothing to do for this work_dir
                continue
            elif check_marker_resub_ok in stdout:
                resubmitted_dirs.append(work_dir)
            else:
                # Unexpected output: be conservative and surface for inspection
                print(f"[run_all] Unexpected checkJobs output for {work_dir}. See printed stdout/stderr above.", file=sys.stderr, flush=True)
                any_unexpected = True

        if any_unexpected:
            return False

        if not resubmitted_dirs:
            print("[run_all] No failed jobs remaining in any work_dir. Proceeding.", flush=True)
            return True

        if no_resubmit:
            print(f"[run_all] Resubmissions would be performed in {resubmitted_dirs}, but no_resubmit=True. Stopping.", flush=True)
            return False

        # wait once for all resubmitted jobs across all dirs
        print(f"[run_all] Resubmitted jobs in {resubmitted_dirs}. Waiting for all resubmitted jobs to finish...", flush=True)
        wait_for_jobs()
        time.sleep(3) # buffer time for new outputs to transfer before recheck
        # after wait, loop again to re-run checkJobs across all dirs

    # reached max attempts
    print(f"[run_all] Reached max_resubmits ({max_resubmits}). Giving up.", file=sys.stderr, flush=True)
    return False

# ----- main workflow -----
def parse_args():
    p = argparse.ArgumentParser(description="Top-level workflow runner")
    p.add_argument("--max-resubmits", type=int, default=3,
                   help="Max resubmit cycles to attempt")
    p.add_argument("--bins-cfg", dest="bins_cfg", type=str, default="config/examples.yaml",
                   help="Path to YAML config file containing bin definitions")
    p.add_argument("--datasets-cfg", dest="datasets_cfg", type=str, default="config/datasets.yaml",
                   help="YAML config file containing dataset definitions")
    p.add_argument("--hist-cfg", dest="hist_cfg", type=str, default="config/hist.yaml",
                   help="YAML config file containing histogram definitions")
    p.add_argument("--stress_test", dest="stress_test", action="store_true",
                   help="Run stress test")
    p.add_argument("--make-json", action="store_true",
                   help="Generate JSON outputs")
    p.add_argument("--make-root", action="store_true",
                   help="Generate ROOT outputs")
    return p.parse_args()

def main():
    args = parse_args()

    start_time = time.time()

    # 1) Compile framework
    print("[run_all] Cleaning binaries...", flush=True)
    clean_binaries()
    print("[run_all] Building binaries...", flush=True)
    build_binaries()

    # 2) Submit jobs and generate master_merge.sh
    print("[run_all] Submitting jobs...", flush=True)
    submit_jobs(stress_test=args.stress_test, config=args.bins_cfg, datasets=args.datasets_cfg,
                hist=args.hist_cfg,make_json=args.make_json, make_root=args.make_root)

    # 3) Wait for jobs to finish
    print("[run_all] Waiting for condor jobs to finish...", flush=True)
    wait_for_jobs()

    # 4) Run checkJobs.py loop to find/resubmit failed jobs (if any)
    print("[run_all] Checking for failed jobs and resubmitting if necessary...", flush=True)
    ok = run_checkjobs_loop_parallel(no_resubmit=False, max_resubmits=args.max_resubmits, check_json=args.make_json, check_root=args.make_root)
    if not ok:
        print(f"[run_all] checkJobs step did not complete successfully. Aborting further steps.", file=sys.stderr)
        sys.exit(1)

    # 5) Run all merge scripts
    print("[run_all] Running master merge script", flush=True)
    if args.make_json:
        subprocess.run(["bash", "condor/master_merge.sh"], check=True, stdout=sys.stdout, stderr=sys.stderr)
    if args.make_root:
        subprocess.run(["bash", "condor/run_hadd_all.sh"], check=True, stdout=sys.stdout, stderr=sys.stderr)
    sys.exit(0) # debug

    if args.make_json:
        # 6) Run BF.x on the flattened JSON
        flattened_json = get_flattened_json_path()
        output_dir = get_output_dir()
        print(f"[run_all] Running BF.x with input {flattened_json} & output {output_dir}", flush=True)
        subprocess.run(["./BF.x", flattened_json, output_dir], check=True, stdout=sys.stdout, stderr=sys.stderr)

        # 7) Run combine
        print("[run_all] Launching combine jobs...", flush=True)
        subprocess.run(["bash", "macro/launchCombine.sh", output_dir], check=True, stdout=sys.stdout, stderr=sys.stderr)

        # 8) Print yields
        print(f"[run_all] Yields for {args.bins_cfg}")
        print_events(flattened_json)

        # 9) Collect significances
        print("[run_all] Collecting significances...", flush=True)
        subprocess.run(["python3", "-u", "macro/CollectSignificance.py", output_dir], check=True, stdout=sys.stdout, stderr=sys.stderr)

    print("[run_all] All steps completed.", flush=True)
    # end time
    end_time = time.time()

    # total time in seconds
    total_time_seconds = end_time - start_time
    # total time in minutes
    total_time_minutes = total_time_seconds / 60
    # total time in hours
    total_time_hours = total_time_minutes / 60

    print("Total time: {0:.2f} seconds = {1:.2f} minutes = {2:.2f} hours".format(
        total_time_seconds, total_time_minutes, total_time_hours))

if __name__ == "__main__":
    main()

