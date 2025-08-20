#!/usr/bin/env python3
import subprocess, time, glob, os, sys, time, argparse
from CondorJobCountMonitor import CondorJobCountMonitor

# ----- helper functions -----
def build_binaries():
    """
    Run `make clean` and `make all -j8` to ensure the latest binaries.
    """
    print("[run_all] Cleaning previous builds...", flush=True)
    subprocess.run(["make", "clean"], check=True)

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

def submit_jobs(stress_test,config):
    """
    Runs submitJobs.py to generate Condor scripts and merge scripts.
    Must create `master_merge.sh` with all merge commands.
    """
    cmd = ["python3", "python/submitJobs.py", "--bins-cfg", config]
    if stress_test:
        cmd.append("--stress_test")
    subprocess.run(
        cmd,
        check=True,
        stdout=sys.stdout,
        stderr=sys.stderr
    )

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

def get_work_dirs():
    """
    Return the directories from condor/.
    """
    return [name for name in os.listdir("condor/") if os.path.isdir(os.path.join("condor", name))]

# ----- integration with checkJobs.py -----
def run_checkjobs_loop(submit_name="",
                       no_resubmit=False,
                       max_resubmits=3):
    """
    Call checkJobs.py in a loop:
      - If checkJobs finds no failed jobs -> returns (proceed).
      - If checkJobs resubmits jobs -> wait_for_jobs() and loop again,
        up to max_resubmits times.
    Returns True if the loop ended with no failed jobs, False if we hit max_resubmits.
    """
    attempt = 0
    check_cmd_base = ["python3", "python/checkJobs.py", submit_name]

    while True:
        attempt += 1
        print(f"[run_all] Running checkJobs (attempt {attempt}/{max_resubmits})...", flush=True)

        proc = subprocess.run(check_cmd_base,
                              capture_output=True,
                              text=True)

        # print outputs for debugging
        if proc.stdout:
            print("----- checkJobs stdout -----")
            print(proc.stdout)
        if proc.stderr:
            print("----- checkJobs stderr -----", file=sys.stderr)
            print(proc.stderr, file=sys.stderr)

        if proc.returncode != 0:
            # checkJobs failed for some reason; surface the error
            print(f"[run_all] checkJobs.py exited with code {proc.returncode}", file=sys.stderr)
            return False

        stdout = proc.stdout or ""

        # check for the explicit messages that checkJobs.py prints
        no_failed_marker = "No failed jobs to resubmit."
        resubmit_success_marker = "Resubmit submitted successfully."

        if no_failed_marker in stdout:
            print("[run_all] checkJobs reports no failed jobs. Proceeding.", flush=True)
            return True

        if resubmit_success_marker in stdout:
            # it resubmitted some jobs; we should wait for them to finish
            if attempt >= max_resubmits:
                print(f"[run_all] Reached max_resubmits ({max_resubmits}). Stopping here.", file=sys.stderr)
                return False

            print("[run_all] Resubmissions were submitted. Waiting for resubmitted jobs to finish...", flush=True)
            wait_for_jobs()
            # after the wait, loop again to re-run checkJobs.py
            continue

        # If neither known marker is present, assume checkJobs did something unexpected.
        # We'll treat this conservatively as failure.
        print("[run_all] Unexpected checkJobs.py output. please inspect the printed stdout/stderr.", file=sys.stderr)
        return False

# ----- main workflow -----
def parse_args():
    p = argparse.ArgumentParser(description="Top-level workflow runner")
    p.add_argument("--max-resubmits", type=int, default=3,
                   help="Max resubmit cycles to attempt")
    p.add_argument("--bins-cfg", dest="bins_cfg", type=str, default="config/bins.yaml",
                   help="Path to YAML config file containing bin definitions")
    return p.parse_args()

def main():
    args = parse_args()

    start_time = time.time()

    # 1) Compile framework
    print("[run_all] Building binaries...", flush=True)
    build_binaries()

    # 2) Submit jobs and generate master_merge.sh
    print("[run_all] Submitting jobs...", flush=True)
    submit_jobs(stress_test=False,config=args.bins_cfg)

    # 3) Wait for jobs to finish
    print("[run_all] Waiting for condor jobs to finish...", flush=True)
    wait_for_jobs()

    # 4) Run checkJobs.py loop to find/resubmit failed jobs (if any)
    print("[run_all] Checking for failed jobs and resubmitting if necessary...", flush=True)
    work_dirs = get_work_dirs()
    for work_dir in work_dirs:
        ok = run_checkjobs_loop(submit_name=work_dir,
                                max_resubmits=args.max_resubmits)
        if not ok:
            print(f"[run_all] checkJobs step did not complete successfully for {work_dir}. Aborting further steps.", file=sys.stderr)
            sys.exit(1)

    # 5) Run all merge scripts
    print("[run_all] Running master merge script", flush=True)
    subprocess.run(["bash", "condor/master_merge.sh"], check=True, stdout=sys.stdout, stderr=sys.stderr)

    # 6) Run BF.x on the flattened JSON
    flattened_json = get_flattened_json_path()
    output_dir = get_output_dir()
    print(f"[run_all] Running BF.x with input {flattened_json} & output {output_dir}", flush=True)
    subprocess.run(["./BF.x", flattened_json, output_dir], check=True, stdout=sys.stdout, stderr=sys.stderr)

    # 7) Run combine
    print("[run_all] Launching combine jobs...", flush=True)
    subprocess.run(["bash", "macro/launchCombine.sh", output_dir], check=True, stdout=sys.stdout, stderr=sys.stderr)

    # 8) Collect significances
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

