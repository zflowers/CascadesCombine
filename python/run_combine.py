#!/usr/bin/env python3
import subprocess, time, glob, os, sys
from CondorJobCountMonitor import CondorJobCountMonitor

def build_binaries():
    """
    Run `make clean` and `make all -j8` to ensure the latest binaries.
    """
    print("[run_combine] Cleaning previous builds...")
    subprocess.run(["make", "clean"], check=True)
    
    print("[run_combine] Building all binaries...")
    subprocess.run(["make", "all", "-j8"], check=True)
    
    print("[run_combine] Build finished.")

def wait_for_jobs():
    """
    Return True if all jobs are finished.
    Uses CondorJobCountMonitor to check the current user's jobs.
    """
    monitor = CondorJobCountMonitor(threshold=1, verbose=False)
    monitor.wait_until_jobs_below()

def submit_jobs(stress_test):
    """
    Runs submitJobs.py to generate Condor scripts and merge scripts.
    Must create `master_merge.sh` with all merge commands.
    """
    cmd = ["python3", "python/submitJobs.py"]
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

def main():

    # 1) Compile framework
    print("Building binaries...", flush=True)
    build_binaries()

    # 2) Submit jobs and generate master_merge.sh
    print("Submitting jobs...", flush=True)
    submit_jobs(stress_test=True)

    # 3) Wait for jobs to finish
    print("Waiting for condor jobs", flush=True)
    wait_for_jobs()

    # 4) Run all merge scripts
    print("Running master merge script", flush=True)
    subprocess.run(["bash", "condor/master_merge.sh"], check=True, stdout=sys.stdout, stderr=sys.stderr)

    # 5) Run BF.x on the flattened JSON
    flattened_json = get_flattened_json_path()
    output_dir = get_output_dir()
    print(f"Running BF.x with input {flattened_json} & output {output_dir}", flush=True)
    subprocess.run(["./BF.x", flattened_json, output_dir], check=True, stdout=sys.stdout, stderr=sys.stderr)

    # 6) Run combine
    print("Launching combine jobs...", flush=True)
    subprocess.run(["bash", "macro/launchCombine.sh", output_dir], check=True, stdout=sys.stdout, stderr=sys.stderr)

    # 7) Collect significances
    print("Collecting significances...", flush=True)
    subprocess.run(["python3", "-u", "macro/CollectSignificance.py", output_dir], check=True, stdout=sys.stdout, stderr=sys.stderr)

    print("All steps completed.", flush=True)

if __name__ == "__main__":
    main()
