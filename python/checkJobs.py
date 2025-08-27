#!/usr/bin/env python3
"""
checkJobs.py

Checks Condor job outputs for a given submission (condor/<submit_name>),
and, if there are failed jobs, writes a single data-driven resubmit file
(with one `queue LogFile, Args from (...)` block) and optionally submits it.

Usage:
    call from python/submitJobs.py

"""
import argparse, os, re, subprocess, sys, glob, shutil
from typing import Dict, Tuple, List

CMS_ENV = "/cvmfs/cms.cern.ch/cmsset_default.sh"

def parse_args():
    p = argparse.ArgumentParser(description="Check and resubmit Condor jobs (single-queue resubmit).")
    p.add_argument("submit_name", help="Name of the submission folder (e.g. manualExample1).")
    p.add_argument("--root-dir", default="condor", help="Top-level condor directory (default: 'condor').")
    p.add_argument("--no-submit", action="store_true", help="Do not actually call condor_submit; just write the resubmit file.")
    p.add_argument("--check-json", action="store_true", help="Check for JSON output files")
    p.add_argument("--check-root", action="store_true", help="Check for ROOT output files")
    return p.parse_args()

# --------------------- job checks ---------------------
def job_paths(base_dir: str, job: str) -> Tuple[str, str, str, str]:
    json_path = os.path.join(base_dir, "json", f"{job}.json")
    root_path = os.path.join(base_dir, "root", f"{job}.root")
    out_path  = os.path.join(base_dir, "out",  f"{job}.out")
    err_path  = os.path.join(base_dir, "err",  f"{job}.err")
    return json_path, root_path, out_path, err_path

def check_job_ok(base_dir: str, job: str, check_json: bool, check_root: bool) -> bool:
    json_path, root_path, out_path, err_path = job_paths(base_dir, job)

    json_ok = os.path.exists(json_path) if check_json else False
    root_ok = os.path.exists(root_path) if check_root else False

    # require whichever flags were set
    file_ok = (json_ok if check_json else True) and (root_ok if check_root else True)
    if file_ok: return file_ok # if file ok just return

    # still enforce err/out consistency
    err_ok = os.path.exists(err_path) and os.path.getsize(err_path) <= 1
    out_ok = False
    if os.path.exists(out_path):
        try:
            with open(out_path, "r", errors="ignore") as fh:
                for line in fh:
                    if "Wrote output to: " in line:
                        out_ok = True
                        break
        except Exception as e:
            print(f"[checkJobs] Warning reading {out_path}: {e}", file=sys.stderr)

    return file_ok and err_ok and out_ok

# --------------------- parse original submit ---------------------
def parse_submit_for_mapping(submit_path: str) -> Tuple[str, Dict[str, str]]:
    """
    Parse the original submit file <submit_path>.

    Returns (header_text, mapping) where:
      - header_text is the content before the `queue LogFile, Args from (...)` block
        (or a default header if not found).
      - mapping is a dict LogFile -> Args (strings).
    """
    default_header = (
        "universe = vanilla\n"
        "executable = scripts/BFI.sh\n"
        "should_transfer_files = YES\n"
        "when_to_transfer_output = ON_EXIT\n"
        "transfer_input_files = BFI_condor.x\n"
        "request_cpus = 1\n"
        "request_memory = 1 GB\n\n"
    )

    if not os.path.exists(submit_path):
        # no submit file; return default header and empty mapping
        return default_header, {}

    content = open(submit_path, "r", errors="ignore").read()

    # Try to capture header and queue block (LogFile, Args style)
    m = re.search(r'(.*?)(?:\n|\r)+queue\s+LogFile\s*,\s*Args\s+from\s*\(\s*(.*?)\s*\)\s*',
                  content, flags=re.S | re.I)
    mapping = {}
    if m:
        header = m.group(1).rstrip() + "\n\n"
        queue_block = m.group(2)
        # Each line expected: <LogFileToken> "full args"
        for raw_line in queue_block.strip().splitlines():
            line = raw_line.strip()
            if not line or line.startswith("#"):
                continue
            # Try double-quoted args
            mm = re.match(r'([^\s"]+)\s+"(.+)"\s*$', line)
            if mm:
                mapping[mm.group(1)] = mm.group(2)
                continue
            # Try single-quoted args
            mm = re.match(r"([^\s']+)\s+'(.+)'\s*$", line)
            if mm:
                mapping[mm.group(1)] = mm.group(2)
                continue
            # Last-resort: split at first whitespace
            parts = line.split(None, 1)
            if len(parts) == 2:
                mapping[parts[0]] = parts[1].strip().strip('"').strip("'")
        return header, mapping

    # Fallback: try to find any lines like `LogFileName "Args"` anywhere
    lines = re.findall(r'^\s*([^\s"]+)\s+"([^"]+)"\s*$', content, flags=re.M)
    if lines:
        header = default_header
        for name, args in lines:
            mapping[name] = args
        return header, mapping

    # Nothing found: return default header and empty mapping
    return default_header, {}

# --------------------- resubmit file builder ---------------------
def write_single_queue_resubmit(resubmit_path: str, header: str, mapping: Dict[str, str], failed_jobs: List[str]) -> None:
    """
    Write a single resubmit submit file with a common header and one queue block listing failed jobs.
    Only jobs present in mapping will be included; others will be skipped (with a warning).
    """
    with open(resubmit_path, "w") as rf:
        rf.write("# AUTO-GENERATED resubmit file\n")
        rf.write("# header (copied from original submit if available)\n")
        rf.write(header.rstrip() + "\n\n")

        rf.write("queue LogFile, Args from (\n")
        any_written = False
        for j in failed_jobs:
            if j not in mapping:
                print(f"[checkJobs] Warning: original Args not found for job '{j}'; skipping it in resubmit file.", file=sys.stderr)
                continue
            argstr = mapping[j].replace('"', '\\"')
            rf.write(f"{j} \"{argstr}\"\n")
            any_written = True
        rf.write(")\n")

    if not any_written:
        # If no jobs were written, remove file and raise
        try:
            os.remove(resubmit_path)
        except Exception:
            pass
        raise RuntimeError("No failed jobs had an Args mapping; resubmit file not created.")

# --------------------- main ---------------------
def main():
    args = parse_args()
    if not (args.check_json or args.check_root):
        args.check_json = True
    submit_name = args.submit_name
    base_dir = os.path.join(args.root_dir, submit_name)

    if not os.path.isdir(base_dir):
        print(f"[checkJobs] ERROR: submission directory not found: {base_dir}", file=sys.stderr)
        sys.exit(2)

    json_dir = os.path.join(base_dir, "json")
    out_dir  = os.path.join(base_dir, "out")
    err_dir  = os.path.join(base_dir, "err")
    submit_path = os.path.join(base_dir, f"{submit_name}.sub")

    if not os.path.isdir(json_dir):
        print(f"[checkJobs] ERROR: json directory not found: {json_dir}", file=sys.stderr)
        sys.exit(2)

    header, mapping = parse_submit_for_mapping(submit_path)
    
    if mapping:
        # Use original queued LogFile tokens as canonical job list
        jobs = sorted(mapping.keys())
    else:
        # Fallback: union of names found in json/out/err if we couldn't parse the submit
        print("[checkJobs] Warning: could not extract LogFile->Args mapping from original submit; "
              "falling back to filesystem enumeration.", file=sys.stderr)
        names = set()
        for d, ext in ((json_dir, ".json"), (out_dir, ".out"), (err_dir, ".err")):
            if os.path.isdir(d):
                for fn in os.listdir(d):
                    if fn.endswith(ext):
                        names.add(os.path.splitext(fn)[0])
        jobs = sorted(names)
    
    if not jobs:
        print(f"[checkJobs] No jobs discovered (submit parsing and filesystem fallback both empty).", file=sys.stderr)
        sys.exit(1)

    successful = []
    failed = []

    for job in jobs:
        ok = check_job_ok(base_dir, job, args.check_json, args.check_root)
        if ok:
            successful.append(job)
        else:
            failed.append(job)

    # Print summary
    if len(failed) > 0:
        print(f"Failed jobs ({len(failed)}):")
        for fjob in failed:
            print("  ", fjob)

    if not failed:
        print("\nNo failed jobs to resubmit.")
        sys.exit(0)
    
    # Single-queue resubmit: just reuse the original .sub
    resubmit_name = f"resubmit_failed_{submit_name}.sub"
    resubmit_path = os.path.join(base_dir, resubmit_name)
    
    if not os.path.exists(submit_path):
        print(f"[checkJobs] ERROR: original submit file not found: {submit_path}", file=sys.stderr)
        sys.exit(1)
    
    # Copy original .sub to resubmit
    shutil.copy(submit_path, resubmit_path)
    print(f"\nResubmit file written: {resubmit_path}")
    
    if args.no_submit:
        print("[checkJobs] --no-submit specified: not calling condor_submit.")
        sys.exit(0)
    
    # Submit using CMS environment
    submit_cmd = f"source {CMS_ENV} && condor_submit {resubmit_path}"
    print(f"[checkJobs] Submitting resubmit file with:\n  {submit_cmd}\n")
    proc = subprocess.run(submit_cmd, shell=True, executable="/bin/bash", capture_output=True, text=True)
    
    if proc.returncode != 0:
        print(f"[checkJobs] condor_submit failed with exit code {proc.returncode}", file=sys.stderr)
        sys.exit(proc.returncode)
    
    print("Resubmit submitted successfully.")
    sys.exit(0)

if __name__ == "__main__":
    main()

