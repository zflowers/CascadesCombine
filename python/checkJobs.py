#!/usr/bin/env python3
#Example: python checkJobs.py manualExample1

import os
import re
import argparse
import subprocess
import sys

def parse_args():
    p = argparse.ArgumentParser(description="Check Condor jobs and resubmit failed ones (single .sub for all failed).")
    p.add_argument("submit_name", help="Submission name (folder name and submit file prefix), e.g. manualExample1")
    p.add_argument("--root-dir", default="condor", help="Top-level condor directory (default: 'condor')")
    p.add_argument("--no-submit", action="store_true", help="Do not actually call condor_submit; only write the resubmit .sub file")
    p.add_argument("--clean-json", action="store_true", help="Remove existing JSON outputs for failed jobs before resubmitting")
    p.add_argument("--cms-env", default="/cvmfs/cms.cern.ch/cmsset_default.sh", help="Path to cmsset_default.sh for environment setup")
    return p.parse_args()

def load_logfile_args(submit_path):
    """
    Parse queue LogFile, Args block from submit file and return dict LogFile -> Args
    """
    mapping = {}
    with open(submit_path, "r") as fh:
        content = fh.read()

    # Attempt to find the block: queue LogFile, Args from ( ... )
    m = re.search(r'queue\s+LogFile\s*,\s*Args\s+from\s*\(\s*(.*?)\s*\)', content, re.S)
    if not m:
        # fallback: try to find any lines like `name "args"`
        lines = re.findall(r'^\s*([^\s"]+)\s+"([^"]+)"\s*$', content, re.M)
        for name, args in lines:
            mapping[name] = args
        return mapping

    block = m.group(1)
    for raw_line in block.strip().splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        # Expect: LogFileName "full args"
        mm = re.match(r'([^\s"]+)\s+"(.+)"\s*$', line)
        if mm:
            mapping[mm.group(1)] = mm.group(2)
        else:
            # If pattern doesn't match, try to parse single-quoted args or space-separated
            mm2 = re.match(r"([^\s]+)\s+'(.+)'$", line)
            if mm2:
                mapping[mm2.group(1)] = mm2.group(2)
            else:
                # Last resort: attempt to split at first space, treat remainder as args (without quotes)
                parts = line.split(None, 1)
                if len(parts) == 2:
                    mapping[parts[0]] = parts[1].strip().strip('"').strip("'")
    return mapping

def check_job_ok(base_dirs, jobname):
    json_path = os.path.join(base_dirs["json"], f"{jobname}.json")
    out_path  = os.path.join(base_dirs["out"], f"{jobname}.out")
    err_path  = os.path.join(base_dirs["err"], f"{jobname}.err")

    # Condition 1: json exists
    json_ok = os.path.exists(json_path)

    # Condition 2: err exists and is empty
    err_ok = os.path.exists(err_path) and os.path.getsize(err_path) == 0

    # Condition 3: out contains "Wrote output to: "
    out_ok = False
    if os.path.exists(out_path):
        try:
            with open(out_path, "r", errors="ignore") as fh:
                for line in fh:
                    if "Wrote output to: " in line:
                        out_ok = True
                        break
        except Exception:
            out_ok = False

    return json_ok and err_ok and out_ok

def main():
    args = parse_args()

    submit_name = args.submit_name
    root_dir = args.root_dir
    base_dir = os.path.join(root_dir, submit_name)

    log_dir  = os.path.join(base_dir, "log")
    out_dir  = os.path.join(base_dir, "out")
    err_dir  = os.path.join(base_dir, "err")
    json_dir = os.path.join(base_dir, "json")
    submit_path = os.path.join(base_dir, f"{submit_name}.sub")

    # sanity checks
    if not os.path.isdir(base_dir):
        print(f"ERROR: submission directory does not exist: {base_dir}", file=sys.stderr)
        sys.exit(2)
    if not os.path.exists(submit_path):
        print(f"ERROR: submit file not found: {submit_path}", file=sys.stderr)
        sys.exit(2)
    if not os.path.isdir(json_dir):
        print(f"ERROR: json directory not found: {json_dir}", file=sys.stderr)
        sys.exit(2)

    logfile_to_args = load_logfile_args(submit_path)
    if not logfile_to_args:
        print("WARNING: couldn't parse LogFile->Args mapping from submit file. Resubmits requiring exact args may be skipped.", file=sys.stderr)

    # find all jobs based on json files
    jobs = sorted([os.path.splitext(f)[0] for f in os.listdir(json_dir) if f.endswith(".json")])

    successful = []
    failed = []

    base_dirs = {"log": log_dir, "out": out_dir, "err": err_dir, "json": json_dir}

    for job in jobs:
        ok = check_job_ok(base_dirs, job)
        if ok:
            successful.append(job)
        else:
            failed.append(job)

    print(f"Successful jobs ({len(successful)}):")
    for j in successful:
        print("  ", j)
    print()
    print(f"Failed jobs ({len(failed)}):")
    for j in failed:
        print("  ", j)

    if not failed:
        print("\nNo failed jobs to resubmit.")
        return

    # Optionally clean partial json files for failed jobs
    if args.clean_json:
        for job in failed:
            jp = os.path.join(json_dir, f"{job}.json")
            if os.path.exists(jp):
                try:
                    os.remove(jp)
                    print(f"Removed existing JSON: {jp}")
                except Exception as e:
                    print(f"Warning: could not remove {jp}: {e}", file=sys.stderr)

    # Build single resubmit file
    resubmit_name = f"resubmit_failed_{submit_name}.sub"
    resubmit_path = os.path.join(base_dir, resubmit_name)
    print(f"\nWriting resubmit file: {resubmit_path}")

    with open(resubmit_path, "w") as rf:
        rf.write("universe = vanilla\n")
        rf.write("executable = scripts/BFI.sh\n")
        rf.write("should_transfer_files = YES\n")
        rf.write("when_to_transfer_output = ON_EXIT\n")
        rf.write("transfer_input_files = BFI_condor.x\n")
        rf.write("request_cpus = 1\n")
        rf.write("request_memory = 1 GB\n\n")

        for job in failed:
            if job not in logfile_to_args:

