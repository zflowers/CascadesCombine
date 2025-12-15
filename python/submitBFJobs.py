#!/usr/bin/env python3
"""
Generate a Condor submit file for BF.x and optionally submit it with
transient-retry + schedd rotation logic. Designed to be called from
another script or run from the command line.

Key behavior matching your requirements:
 - No wrapper support (you requested to remove it).
 - The job's transfer_output_files is a single file named "<signal>.txt".
 - Automatically create the destination output directory: output_dir/<signal>/
 - Attempts to import CondorJobCountMonitor and use monitor.get_auto_THRESHOLD()
   if the module is available. You can also pass a condor_monitor instance to
   submit_condor_with_retries().
"""

from __future__ import annotations
import argparse
import os
import subprocess
import re
import random
import time
from pathlib import Path
from collections import defaultdict
from textwrap import dedent
from typing import List, Optional

# Retry defaults
DEFAULT_KNOWN_SCHEDDS = [
    "lpcschedd4.fnal.gov",
    "lpcschedd5.fnal.gov",
    "lpcschedd6.fnal.gov",
]
DEFAULT_MAX_RETRIES = 8
DEFAULT_PER_SCHEDD_LIMIT = 3


def make_submit_content(
    executable: str,
    signal: str,
    output_dir: str,
    transfer_output_files: Optional[List[str]] = None,
    submit_filename: Optional[str] = None,
    input_json: str = "flattened.json",
    logs_dir: str = "BF_condor_logs",
    request_cpus: int = 1,
    request_memory: str = "2GB",
    request_disk: str = "2GB",
    universe: str = "vanilla",
) -> str:
    """
    Build the text content for a Condor .sub file.

    - executable: path used for 'executable' in submit (e.g. BF.x or /path/to/BF.x)
    - signal: used for arguments and as the output filename (<signal>.txt)
    - output_dir: local directory on submit host where outputs should be remapped
    - transfer_output_files: list of filenames to transfer back; if None, defaults to [f"{signal}.txt"]
    """
    if transfer_output_files is None:
        transfer_output_files = [f"{signal}.txt"]

    formatted_files = [f.format(signal=signal) for f in transfer_output_files]

    submit_basename = submit_filename or f"job_{signal}.sub"
    job_log = os.path.join(logs_dir, f"{signal}.log")
    job_out = os.path.join(logs_dir, f"{signal}.out")
    job_err = os.path.join(logs_dir, f"{signal}.err")

    # Destination directory on submit host: output_dir/<signal>/
    dest_dir = os.path.join(output_dir, signal)
    # Ensure dest_dir exists on the submit host (we create it from the submission script).
    # Condor will remap transferred files into the absolute paths we provide below.
    os.makedirs(dest_dir, exist_ok=True)

    # Build transfer_output_remaps entries like: "name = /abs/path/to/dest"
    remaps = [f'"{fname} = {os.path.join(dest_dir, fname)}"' for fname in formatted_files]
    remaps_str = ", ".join(remaps)

    submit = dedent(
        f"""\
        universe                = {universe}
        executable              = {executable}
        should_transfer_files   = YES
        when_to_transfer_output = ON_EXIT
        transfer_input_files    = {input_json}
        arguments               = {input_json} datacards/ {signal}

        request_cpus            = {request_cpus}
        request_memory          = {request_memory}
        request_disk            = {request_disk}

        output                  = {job_out}
        error                   = {job_err}
        log                     = {job_log}

        transfer_output_files   = {', '.join(formatted_files)}
        transfer_output_remaps  = {remaps_str}

        queue 1
        """
    )
    return submit


def make_submit_file(
    signal: str,
    output_dir: str,
    executable: str = "BF.x",
    submit_filename: Optional[str] = None,
    input_json: str = "flattened.json",
    logs_dir: str = "BF_condor_logs",
    transfer_output_files: Optional[List[str]] = None,
    request_cpus: int = 1,
    request_memory: str = "2GB",
    request_disk: str = "2GB",
) -> str:
    """
    Write the submit file to the current directory and return its absolute path.

    - auto-creates logs_dir and dest output_dir/<signal> (the latter inside make_submit_content).
    """
    os.makedirs(logs_dir, exist_ok=True)

    submit_basename = submit_filename or f"job_{signal}.sub"
    submit_content = make_submit_content(
        executable=executable,
        signal=signal,
        output_dir=output_dir,
        transfer_output_files=transfer_output_files,
        submit_filename=submit_basename,
        input_json=input_json,
        logs_dir=logs_dir,
        request_cpus=request_cpus,
        request_memory=request_memory,
        request_disk=request_disk,
    )

    with open(submit_basename, "w") as f:
        f.write(submit_content)

    return os.path.abspath(submit_basename)


def submit_condor_with_retries(
    submit_path: str,
    *,
    dryrun: bool = False,
    record_dir: Optional[str] = None,
    known_schedds: Optional[List[str]] = None,
    max_retries: int = DEFAULT_MAX_RETRIES,
    per_schedd_limit: int = DEFAULT_PER_SCHEDD_LIMIT,
    condor_monitor: Optional[object] = None,
) -> bool:
    """
    Submit the condor submit file with transient-retry and schedd rotation.

    If successful, append "cluster_id schedd" (schedd optional) to record_dir/submitted_clusters.txt.

    Returns True if submission succeeded, False otherwise.
    """
    record_dir = Path(record_dir) if record_dir else Path.cwd()
    record_dir.mkdir(parents=True, exist_ok=True)
    record_path = record_dir / "submitted_clusters.txt"

    if known_schedds is None:
        known_schedds = DEFAULT_KNOWN_SCHEDDS

    # If a condor_monitor is not provided, try to import/instantiate one as you requested.
    if condor_monitor is None:
        try:
            # Attempt to import the class you mentioned; this is optional.
            from CondorJobCountMonitor import CondorJobCountMonitor  # type: ignore

            # instantiate a monitor with a safe default if possible
            try:
                tmp = CondorJobCountMonitor()  # try default constructor
                # If the instance has get_auto_THRESHOLD, compute an automatic threshold and re-create monitor
                if hasattr(tmp, "get_auto_THRESHOLD"):
                    auto = tmp.get_auto_THRESHOLD()
                    # Recreate monitor with a threshold scaled like your previous code
                    try:
                        condor_monitor = CondorJobCountMonitor(threshold=auto * 0.95, verbose=False)
                    except Exception:
                        # If constructor doesn't accept threshold, just use tmp
                        condor_monitor = tmp
                else:
                    condor_monitor = tmp
            except Exception:
                # As a last resort, try to instantiate without args ignoring failure
                condor_monitor = None
        except Exception:
            condor_monitor = None

    # If provided/created, wait for jobs below threshold (this matches your previous
    # pattern: condor_monitor.wait_until_jobs_below())
    if condor_monitor is not None:
        try:
            condor_monitor.wait_until_jobs_below()
        except Exception as e:
            print("[warn] condor_monitor.wait_until_jobs_below() raised:", e)

    if dryrun:
        print("[dryrun] would run:", f"condor_submit {submit_path}")
        return True

    attempt = 0
    success = False
    last_proc = None
    schedd_tries = defaultdict(int)
    force_schedd = None

    transient_signatures = [
        "Can't find address of local schedd",
        "Querying the CMS LPC pool",
        "Attempting to submit jobs to",
        "Unable to connect to",
        "Failed to connect",
    ]

    while attempt < max_retries and not success:
        attempt += 1
        if force_schedd:
            cmd = f"source /cvmfs/cms.cern.ch/cmsset_default.sh && condor_submit -name {force_schedd} {submit_path}"
            printed_name = force_schedd
        else:
            cmd = f"source /cvmfs/cms.cern.ch/cmsset_default.sh && condor_submit {submit_path}"
            printed_name = "(auto)"

        print(f"[info] Running attempt {attempt} submit (schedd={printed_name})...")
        proc = subprocess.run(
            cmd, shell=True, executable="/bin/bash", capture_output=True, text=True
        )
        last_proc = proc
        stdout = (proc.stdout or "").strip()
        stderr = (proc.stderr or "").strip()

        if proc.returncode == 0:
            success = True
            break

        is_transient = any(sig in stdout or sig in stderr for sig in transient_signatures)

        # Try to discover which schedd condor tried
        match_schedd = re.search(r"Attempting to submit jobs to (\S+)", stdout)
        reported_schedd = match_schedd.group(1) if match_schedd else None

        print(f"  condor_submit failed (rc={proc.returncode})")
        if stdout:
            print("  STDOUT:", stdout.replace("\n", " | "))
        if stderr:
            print("  STDERR:", stderr.replace("\n", " | "))

        if not is_transient:
            print("[createJobs] Non-transient condor_submit failure; not retrying.")
            break

        candidate_schedds = [s for s in known_schedds if schedd_tries[s] < per_schedd_limit]
        if reported_schedd and reported_schedd in candidate_schedds:
            candidate_schedds = [s for s in candidate_schedds if s != reported_schedd]

        if not candidate_schedds:
            print("[createJobs] All schedds have reached per-schedd attempt limit. Stopping retries.")
            break

        min_tries = min(schedd_tries[s] for s in candidate_schedds)
        least_tried = [s for s in candidate_schedds if schedd_tries[s] == min_tries]
        next_schedd = random.choice(least_tried)

        schedd_tries[next_schedd] += 1
        force_schedd = next_schedd

        wait_time = min(2 * attempt, 10)
        print(f"[warn] Transient schedd error; will retry using -name {next_schedd} (attempt count for this schedd: {schedd_tries[next_schedd]}). Sleeping {wait_time}s.")
        time.sleep(wait_time)

    if not success:
        if last_proc is not None:
            print(f"condor_submit failed after {attempt} attempt(s). Last returncode: {last_proc.returncode}")
            print("Last STDOUT:", last_proc.stdout)
            print("Last STDERR:", last_proc.stderr)
        else:
            print("condor_submit failed: no subprocess result available.")
        return False

    # Success: parse cluster id and schedd and record
    final_stdout = (proc.stdout or "").strip()
    cluster_id = None
    schedd = None

    match_cluster = re.search(r"submitted to cluster (\d+)", final_stdout)
    if match_cluster:
        cluster_id = match_cluster.group(1)
    else:
        match_cluster2 = re.search(r"(\d+) job\(s\) submitted to cluster (\d+)", final_stdout)
        if match_cluster2:
            cluster_id = match_cluster2.group(2)

    match_schedd = re.search(r"Attempting to submit jobs to (\S+)", final_stdout)
    if match_schedd:
        schedd = match_schedd.group(1)
    elif force_schedd:
        schedd = force_schedd

    if cluster_id:
        with open(record_path, "a") as f:
            if schedd:
                f.write(f"{cluster_id} {schedd}\n")
            else:
                f.write(f"{cluster_id}\n")
        print(f"[ok] Submitted cluster {cluster_id} (schedd={schedd}). Recorded to {record_path}")
    else:
        print("[warn] Submission succeeded but cluster id not parsed from condor output. Full stdout:")
        print(final_stdout)

    return True


# ----------- CLI -----------
def _cli():
    parser = argparse.ArgumentParser(description="Create Condor submit file for BF.x and submit with retries")
    parser.add_argument("--signal", required=True, help="Signal label to pass to BF.x and to tag outputs.")
    parser.add_argument("--output-dir", required=True, help="Local dest directory for outputs (on submit host).")
    parser.add_argument("--executable", default="BF.x", help="Path to executable (default BF.x)")
    parser.add_argument("--submit-file", help="Name of the submit file to write. Default job_<signal>.sub")
    parser.add_argument("--logs-dir", default="BF_condor_logs", help="Directory to store condor stdout/err/log files")
    parser.add_argument("--json", default="flattened.json", help="input flattened json file")
    parser.add_argument("--cpus", type=int, default=1)
    parser.add_argument("--memory", default="2GB")
    parser.add_argument("--disk", default="2GB")
    parser.add_argument("--dryrun", action="store_true", help="Don't actually condor_submit; just write the submit file")
    parser.add_argument("--record-dir", default=None, help="Where to record submitted_clusters.txt (defaults to cwd)")
    args = parser.parse_args()

    submit_path = make_submit_file(
        signal=args.signal,
        output_dir=args.output_dir,
        executable=args.executable,
        submit_filename=args.submit_file,
        input_json=args.json,
        logs_dir=args.logs_dir,
        request_cpus=args.cpus,
        request_memory=args.memory,
        request_disk=args.disk,
    )
    print(f"Wrote submit file: {submit_path}")

    ok = submit_condor_with_retries(
        submit_path,
        dryrun=args.dryrun,
        record_dir=args.record_dir,
    )
    if ok:
        print("condor_submit succeeded.")
    else:
        print("condor_submit failed after retries.")


if __name__ == "__main__":
    _cli()

