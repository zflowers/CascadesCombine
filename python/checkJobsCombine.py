#!/usr/bin/env python3
"""
checkJobsCombine.py
Checks Condor outputs for combine jobs (limits / significances),
and writes resubmit files for failed jobs.
"""
from __future__ import annotations
import argparse
import os
import subprocess
import sys
import shutil
from typing import List, Tuple, Optional

CMS_ENV = "/cvmfs/cms.cern.ch/cmsset_default.sh"

# -------------------------
# Argument parsing
# -------------------------

def parse_args():
    p = argparse.ArgumentParser(description="Check and resubmit Combine jobs.")
    p.add_argument("submit_name", help="Submission folder name (e.g. signalName).")
    p.add_argument("--root-dir", default="condor", help="Top-level condor directory.")
    p.add_argument("--method", choices=["AsymptoticLimits", "Significance"], default=None, help="Combine method (affects success checks).")
    p.add_argument("--no-submit", action="store_true", help="Do not actually submit resubmits.")
    return p.parse_args()

# -------------------------
# File checks
# -------------------------

def _file_nonzero(path: str) -> bool:
    return os.path.exists(path) and os.path.getsize(path) > 0

def _err_file_ok(err_path: str) -> Tuple[bool, List[str]]:
    if not os.path.exists(err_path):
        return True, []

    try:
        with open(err_path, "r", errors="ignore") as f:
            lines = [ln.rstrip("\n") for ln in f]
    except Exception:
        return True, []

    ignore_patterns = [
        "WARNING",
        "TClass::Init",
        "cling::",
    ]
    filtered = [
        ln for ln in lines
        if ln.strip() and not any(pat in ln for pat in ignore_patterns)
    ]
    return len(filtered) == 0, filtered

def _combine_out_ok(out_path: str, method: Optional[str]) -> bool:
    if not os.path.exists(out_path):
        return False

    try:
        with open(out_path, "r", errors="ignore") as f:
            data = f.read()
    except Exception:
        return False

    if method == "AsymptoticLimits":
        return " -- AsymptoticLimits ( CLs ) --" in data
    elif method == "Significance":
        return "Significance:" in data
    else:
        # fallback / backward-compatible
        return "Finished running combine" in data

def _combine_root_ok(work_dir: str, method: str) -> bool:
    return any(
        f.startswith("higgsCombine")
        and method in f
        and f.endswith(".root")
        and _file_nonzero(os.path.join(work_dir, f))
        for f in os.listdir(work_dir)
    )

# -------------------------
# Per-job check
# -------------------------

def check_job_ok(base_dir: str, job: str, method: Optional[str]) -> bool:
    out_path = os.path.join(base_dir, f"{job}.out")
    err_path = os.path.join(base_dir, f"{job}.err")

    err_ok, err_lines = _err_file_ok(err_path)
    if not err_ok:
        print(f"[checkJobsCombine] {err_path} has errors:", flush=True)
        for l in err_lines:
            print("   ", l, flush=True)

    out_ok = _combine_out_ok(out_path, method)
    root_ok = _combine_root_ok(base_dir, method)

    return err_ok and (out_ok or root_ok)

# -------------------------
# Main
# -------------------------

def main():
    args = parse_args()
    base_dir = os.path.join(args.root_dir, args.submit_name)

    if not os.path.isdir(base_dir):
        print(f"[checkJobsCombine] ERROR: {base_dir} not found.", file=sys.stderr)
        sys.exit(2)

    submit_files = [
        f for f in os.listdir(base_dir)
        if f.endswith(".sub") and args.method in f
    ]

    if not submit_files:
        print(
            f"[checkJobsCombine] No .sub files found in {base_dir} "
            f"for method '{args.method}'.",
            file=sys.stderr,
        )
        sys.exit(1)

    failed_jobs: List[str] = []

    for submit_file in submit_files:
        job_name = os.path.splitext(submit_file)[0]
        if not check_job_ok(base_dir, job_name, args.method):
            failed_jobs.append(job_name)

    if not failed_jobs:
        sys.exit(0)

    print(f"[checkJobsCombine] Failed jobs ({len(failed_jobs)}):", flush=True)
    for j in failed_jobs:
        print("  ", j, flush=True)

    for job in failed_jobs:
        orig_submit = os.path.join(base_dir, f"{job}.sub")
        resubmit = os.path.join(base_dir, f"resubmit_{job}.sub")

        shutil.copyfile(orig_submit, resubmit)
        print(f"[checkJobsCombine] Resubmit file written: {resubmit}", flush=True)

        if not args.no_submit:
            cmd = f"source {CMS_ENV} && condor_submit {resubmit}"
            proc = subprocess.run(
                cmd,
                shell=True,
                executable="/bin/bash",
                capture_output=True,
                text=True,
            )
            if proc.returncode != 0:
                print(f"[checkJobsCombine] condor_submit failed for {job}", file=sys.stderr)
                print(proc.stdout, file=sys.stderr)
                print(proc.stderr, file=sys.stderr)
            else:
                print(f"[checkJobsCombine] Resubmit for {job} submitted successfully.", flush=True)

if __name__ == "__main__":
    main()

