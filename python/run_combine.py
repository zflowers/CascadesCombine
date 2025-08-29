#!/usr/bin/env python3
import subprocess, time, glob, os, sys, time, argparse, json, yaml, re
from CondorJobCountMonitor import CondorJobCountMonitor

# ----- helper functions -----
def _read_condor_bins_list(condor_dir="condor"):
    """
    Reads the automatically generated bins list file from condor/
    Returns list of bin names. Picks the most recent file matching bins_list_*.txt
    """
    pattern = os.path.join(condor_dir, "bins_list_*.txt")
    files = glob.glob(pattern)
    if not files:
        return []
    # pick the latest file by modification time
    files.sort(key=os.path.getmtime, reverse=True)
    latest_file = files[0]
    with open(latest_file) as f:
        return [ln.strip() for ln in f if ln.strip()]

def _joined_bins_from_condor_list(condor_dir="condor"):
    """
    Return joined bin names (with '__') from the auto-generated bins list file
    """
    bins = _read_condor_bins_list(condor_dir=condor_dir)
    return "__".join(bins) if bins else None

def _joined_bins_from_yaml(bins_cfg):
    """
    Return joined bin names (with '__') directly from the YAML file
    """
    try:
        with open(bins_cfg, "r") as f:
            bins_data = yaml.safe_load(f) or {}
        names = list(bins_data.keys())
        if names:
            # sanitize names to remove any characters unsafe for filenames
            import re
            safe_names = [re.sub(r"[^A-Za-z0-9_]", "__", n) for n in names]
            return "__".join(safe_names)
    except Exception:
        pass
    return None

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

def submit_jobs(config, processes, hist, make_json=False, make_root=False, lumi="1."):
    """
    Runs submitJobs.py to generate Condor scripts.
    """
    cmd = ["python3", "python/submitJobs.py", "--bins-cfg", config, "--processes-cfg", processes, "--lumi", lumi]

    if make_json:
        cmd.append("--make-json")
    if make_root:
        cmd.append("--make-root")
        if hist:
            cmd.append("--hist-yaml")
            cmd.append(hist)
    subprocess.run(cmd, check=True, stdout=sys.stdout, stderr=sys.stderr)

def create_mergers(config, make_json=False, make_root=False):
    """
    Runs createMergers.py to generate merger scripts.
    """
    cmd = ["python3", "python/createMergers.py", "--bins-cfg", config, "--make-master"]

    if make_json:
        cmd.append("--do-json")
    if make_root:
        cmd.append("--do-hadd")
    subprocess.run(cmd, check=True, stdout=sys.stdout, stderr=sys.stderr)

def get_flattened_json_path(condor_dir="condor", json_dir="json"):
    """
    Return the path to the flattened JSON produced by the merge scripts.

    Preferred behavior:
      1) Use the auto-generated bins_list_*.txt to get joined bin names
      2) Otherwise fallback to latest json/flattened_*.json
    """
    joined = _joined_bins_from_condor_list(condor_dir=condor_dir)
    if joined:
        candidate = os.path.join(json_dir, f"flattened_{joined}.json")
        if os.path.exists(candidate):
            return candidate

    # fallback
    flattened_files = glob.glob(os.path.join(json_dir, "flattened_*.json"))
    if not flattened_files:
        raise FileNotFoundError(f"No flattened JSON files found in {json_dir}/")
    flattened_files.sort(key=os.path.getmtime, reverse=True)
    return flattened_files[0]


def get_flattened_root_path(condor_dir="condor", root_dir="root"):
    """
    Return the path to the hadded ROOT produced by the merge scripts.

    Preferred behavior:
      1) Use the auto-generated bins_list_*.txt to get joined bin names
      2) Otherwise fallback to latest root/hadded_*.root
    """
    joined = _joined_bins_from_condor_list(condor_dir=condor_dir)
    if joined:
        candidate = os.path.join(root_dir, f"hadded_{joined}.root")
        if os.path.exists(candidate):
            return candidate

    root_files = glob.glob(os.path.join(root_dir, "hadded_*.root"))
    if not root_files:
        raise FileNotFoundError(f"No hadded ROOT files found in {root_dir}/")
    root_files.sort(key=os.path.getmtime, reverse=True)
    return root_files[0]

def get_significances_path():
    sig_files = glob.glob("output/Significance_datacards_*.txt")
    if not sig_files:
        raise FileNotFoundError("No Significance files found in output/")
    sig_files.sort(key=os.path.getmtime, reverse=True)
    return sig_files[0]

def get_output_dir(flattened_json):
    """
    Return the directory where BF.x should write datacards.
    Derived from the provided flattened JSON filename.
    """
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

def get_all_work_dirs():
    """
    Return the directories from condor/.
    """
    return [name for name in os.listdir("condor/") if os.path.isdir(os.path.join("condor", name))]

def load_bins(bins_cfg):
    """
    Return the bin names (condor work dirs) from the input YAML.
    Sanitizes names so they are safe for filesystem/condor.
    If it fails, fall back to get_all_work_dirs() above.
    """
    try:
        with open(bins_cfg, "r") as f:
            bins_data = yaml.safe_load(f) or {}
        # The bin names are just the top-level keys
        raw_bin_names = list(bins_data.keys())
        if len(raw_bin_names) > 0:
            # Sanitize: replace bad chars with "__"
            safe_bin_names = [
                re.sub(r"[^A-Za-z0-9_]", "__", name) for name in raw_bin_names
            ]
            return safe_bin_names
    except Exception as e:
        # if YAML can't be read, fall through to fallback
        print(f"[run_all] Warning reading bins cfg ({bins_cfg}): {e}", file=sys.stderr)
    return get_all_work_dirs()

def load_submitted_clusters(condor_dir):
    clusters = []
    path = condor_dir / "submitted_clusters.txt"
    if path.exists():
        with open(path) as f:
            for line in f:
                parts = line.strip().split()
                if len(parts) == 2:
                    clusters.append((parts[0], parts[1]))  # (cluster_id, schedd)
                elif parts:
                    clusters.append((parts[0], None))
    return clusters

def wait_for_jobs(work_dirs = None):
    """
    Block until current user's Condor job count is below the monitor threshold.
    Uses CondorJobCountMonitor to check the current user's jobs.
    """
    idle_time_start = time.time()
    monitor = CondorJobCountMonitor(threshold=1, verbose=False)
    clusters = None
    if work_dirs:
        clusters = CondorJobCountMonitor.load_clusters_for_dirs(work_dirs, condor_root="condor")
    monitor.wait_until_no_idle_jobs(clusters=clusters)
    idle_time_end = time.time()
    monitor.wait_until_jobs_below(clusters=clusters)
    return idle_time_end - idle_time_start

def run_checkjobs_loop_parallel(work_dirs=None, no_resubmit=False, max_resubmits=3, check_json=False, check_root=False):
    """
    Check all work directories with checkJobs.py, resubmit failing jobs across
    all directories in one cycle, then wait once for all resubmitted jobs to finish.
    Returns True if no failed jobs remain (proceed), False on error or if max resubmits reached.
    """
    if not work_dirs:
        work_dirs = get_all_work_dirs()
    if not work_dirs:
        print("[run_all] No work directories found under condor/.", flush=True)
        return True

    attempt = 0
    check_marker_no_failed = "[checkJobs] No failed jobs to resubmit."
    check_marker_resub_ok = "[checkJobs] Resubmit submitted successfully."

    while attempt < max_resubmits:
        attempt += 1
        print(f"[run_all] Running checkJobs for {work_dirs} (attempt {attempt}/{max_resubmits})...", flush=True)

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

            stdout_lines = [line.strip() for line in (proc.stdout or "").splitlines()]
            if any(check_marker_no_failed == line for line in stdout_lines):
                continue
            elif any(check_marker_resub_ok == line for line in stdout_lines):
                resubmitted_dirs.append(work_dir)
            else:
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
        wait_for_jobs(work_dirs)
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
    p.add_argument("--bins-cfg", dest="bins_cfg", type=str, default="config/bin_cfgs/bin_examples.yaml",
                   help="Path to YAML config file containing bin definitions")
    p.add_argument("--processes-cfg", dest="processes_cfg", type=str, default="config/process_cfgs/processes.yaml",
                   help="YAML config file containing process definitions")
    p.add_argument("--hist-cfg", dest="hist_cfg", type=str, default="config/hist_cfgs/hist_examples.yaml",
                   help="YAML config file containing histogram definitions")
    p.add_argument("--stress_test", dest="stress_test", action="store_true",
                   help="Run stress test")
    p.add_argument("--make-json", action="store_true",
                   help="Generate JSON outputs")
    p.add_argument("--make-root", action="store_true",
                   help="Generate ROOT outputs")
    p.add_argument("--lumi", dest="lumi", type=str, default="400.0",
                   help="Lumi to scale everything to (default is 400.0)")
    return p.parse_args()

def main():
    args = parse_args()
    bins_cfg = args.bins_cfg
    hist_cfg = args.hist_cfg
    processes_cfg = args.processes_cfg
    if args.stress_test:
        print("Running stress test. Using stress yamls instead of loaded arg yamls")
        bins_cfg = "config/bin_cfgs/bin_stress.yaml"
        hist_cfg = "config/hist_cfgs/hist_stress.yaml"
        processes_cfg = "config/process_cfgs/processes_stress.yaml"

    start_time = time.time()

    # 1) Compile framework
    print("[run_all] Cleaning binaries...", flush=True)
    clean_binaries()
    print("[run_all] Building binaries...", flush=True)
    build_binaries()

    # 2) Submit jobs
    print("[run_all] Submitting jobs...", flush=True)
    submit_jobs(config=bins_cfg, processes=processes_cfg,
                hist=hist_cfg, make_json=args.make_json, make_root=args.make_root, lumi=args.lumi)

    # 3) Create merge scripts
    print("[run_all] Creating merger scripts...", flush=True)
    create_mergers(config=bins_cfg, make_json=args.make_json, make_root=args.make_root)

    condor_time_start = time.time()
    # 4) Wait for jobs to finish
    print("[run_all] Waiting for condor jobs to finish...", flush=True)
    idle_time_seconds = wait_for_jobs(work_dirs=load_bins(args.bins_cfg))

    # 5) Run checkJobs.py loop to find/resubmit failed jobs (if any)
    print("[run_all] Checking for failed jobs and resubmitting if necessary...", flush=True)
    ok = run_checkjobs_loop_parallel(work_dirs=load_bins(args.bins_cfg), no_resubmit=False, max_resubmits=args.max_resubmits, check_json=args.make_json, check_root=args.make_root)
    if not ok:
        print(f"[run_all] checkJobs step did not complete successfully. Aborting further steps.", file=sys.stderr)
        sys.exit(1)
    condor_time_end = time.time()
    condor_time_seconds = condor_time_end - condor_time_start

    # 6) Run all merge scripts
    joined_bins = _joined_bins_from_yaml(args.bins_cfg) or _joined_bins_from_condor_list()
    if not joined_bins:
        print("[run_all] WARNING: Could not determine joined bin names for master merge script.", file=sys.stderr)
        sys.exit(1)
    master_merge_sh = os.path.join("condor", f"master_merge_{joined_bins}.sh")
    if not os.path.exists(master_merge_sh):
        print(f"[run_all] ERROR: master merge script not found: {master_merge_sh}", file=sys.stderr)
        sys.exit(1)
    print(f"[run_all] Running master merge script: {master_merge_sh}", flush=True)
    subprocess.run(["bash", master_merge_sh], check=True, stdout=sys.stdout, stderr=sys.stderr)

    if args.make_root:
        # 7) Plot histograms
        hadd_file = get_flattened_root_path(condor_dir="condor", root_dir="root")
        plot_cmd = [
              "./PlotHistograms.x", 
              "-i", hadd_file,
              "-h", args.hist_cfg,
              "-d", args.processes_cfg,
              "-b", args.bins_cfg,
              "-l", args.lumi 
            ]
        print("[run_all] Plotting histograms with command:"," ".join(plot_cmd), flush=True)
        subprocess.run(plot_cmd, check=True, stdout=sys.stdout, stderr=sys.stderr)

    if args.make_json:
        # 8) Run BF.x on the flattened JSON
        flattened_json = get_flattened_json_path(condor_dir="condor", json_dir="json")
        output_dir = get_output_dir(flattened_json=flattened_json)
        print(f"[run_all] Running BF.x with input {flattened_json} & output {output_dir}", flush=True)
        subprocess.run(["./BF.x", flattened_json, output_dir], check=True, stdout=sys.stdout, stderr=sys.stderr)

        # 9) Run combine
        print("[run_all] Launching combine jobs...", flush=True)
        subprocess.run(["bash", "macro/launchCombine.sh", output_dir], check=True, stdout=sys.stdout, stderr=sys.stderr)

        # 10) Print yields
        print(f"[run_all] Yields for {args.bins_cfg}")
        print_events(flattened_json)

        # 11) Collect significances
        print("[run_all] Collecting significances...", flush=True)
        subprocess.run(["python3", "-u", "macro/CollectSignificance.py", output_dir], check=True, stdout=sys.stdout, stderr=sys.stderr)

        # 12) Plot significances
        plot_cmd = [
              "./PlotSignificances.x", 
              "-i", get_significances_path(),
              "-h", args.hist_cfg,
              "-d", args.processes_cfg,
              "-b", args.bins_cfg
            ]
        print("[run_all] Plotting significances with command:"," ".join(plot_cmd), flush=True)
        subprocess.run(plot_cmd, check=True, stdout=sys.stdout, stderr=sys.stderr)

    print("[run_all] All steps completed.", flush=True)
    end_time = time.time()

    # total time
    total_time_seconds = end_time - start_time
    condor_time_end = time.time()
    condor_time_seconds = condor_time_end - condor_time_start
    print("Time for all condor jobs to start running: {:.2f} seconds = {:.2f} minutes = {:.2f} hours".format(
        idle_time_seconds, idle_time_seconds/60, idle_time_seconds/3600), flush=True)
    print("Time for condor processing: {:.2f} seconds = {:.2f} minutes = {:.2f} hours".format(
        condor_time_seconds, condor_time_seconds/60, condor_time_seconds/3600), flush=True)
    print("Total time: {0:.2f} seconds = {1:.2f} minutes = {2:.2f} hours".format(
        total_time_seconds, total_time_seconds/60, total_time_seconds/3600), flush=True)

if __name__ == "__main__":
    main()
