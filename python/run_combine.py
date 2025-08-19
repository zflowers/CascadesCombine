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

def submit_jobs():
    """
    Runs submitJobs.py to generate Condor scripts and merge scripts.
    Must create `master_merge.sh` with all merge commands.
    """
    subprocess.run(["python3", "python/submitJobs.py"], check=True)

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
    build_binaries()

    # 2) Submit jobs and generate master_merge.sh
    submit_jobs()

    # 3) Wait for jobs to finish
    print("Waiting for condor jobs")
    wait_for_jobs()

    # 4) Run all merge scripts
    print("Running master merge script")
    subprocess.run(["bash", "condor/master_merge.sh"], check=True)

    # 5) Run BF.x on the flattened JSON
    flattened_json = get_flattened_json_path()
    output_dir = get_output_dir()
    #if not os.path.isfile(flattened_json) or os.path.getsize(flattened_json) == 0:
    #    raise RuntimeError(f"Flattened JSON '{flattened_json}' is missing or empty!")
    print(f"Running BF.x with input {flattened_json} & output {output_dir}")
    subprocess.run(["./BF.x", flattened_json, output_dir], check=True)

    # 6) Run combine
    subprocess.run(["bash", "macro/launchCombine.sh", output_dir], check=True)

    #7) Collect significances
    subprocess.run(["python3", "macro/CollectSignificance.py", output_dir], check=True)

if __name__ == "__main__":
    main()

