#!/usr/bin/env python3
"""
checkJobsBF.py
Checks Condor job outputs for a given submission (condor_BF/<submit_name>),
and, if there are failed jobs, writes a single data-driven resubmit file
Usage:
    call from python/submitJobsBF.py
"""
import argparse, os, re, subprocess, sys, glob, shutil
from typing import Dict, Tuple, List

CMS_ENV = "/cvmfs/cms.cern.ch/cmsset_default.sh"

def parse_args():
    p = argparse.ArgumentParser(description="Check and resubmit Condor jobs (single-queue resubmit).")
    p.add_argument("submit_name", help="Name of the submission folder (e.g. manualExample1).")
    p.add_argument("--root-dir", default="condor", help="Top-level condor directory (default: 'condor').")
    p.add_argument("--no-submit", action="store_true", help="Do not actually call condor_submit; just write the resubmit file.")
    return p.parse_args()

def _file_nonzero(path: str) -> bool:
    """Return True if file exists and has non-zero size."""
    try:
        return os.path.exists(path) and os.path.getsize(path) > 0
    except Exception:
        return False

# --------------------- job checks ---------------------
def job_paths(base_dir: str, job: str) -> Tuple[str, str, str]:
    datacard_path = os.path.join(base_dir, f"{job}.txt")
    out_path  = os.path.join(base_dir, "out",  f"{job}.out")
    err_path  = os.path.join(base_dir, "err",  f"{job}.err")
    return datacard_path, out_path, err_path

def check_job_ok(base_dir: str, job: str) -> bool:
    datacard_path, out_path, err_path = job_paths(base_dir, job)

    # Use non-zero-size checks to avoid counting partial/empty files as success
    file_ok = _file_nonzero(datacard_path)

    err_ok = os.path.exists(err_path) and os.path.getsize(err_path) <= 1
    if not err_ok and os.path.exists(err_path):
        # Use grep to check if err file is empty excluding common ignores
        bash = f"grep -v -e 'nvidia-drivers' -e 'CUDA_COMPATIBILITY' {err_path}"
        try:
            output = subprocess.check_output(['bash','-c',bash]).decode().splitlines()
            err_ok = len(output) == 0
            if not err_ok:
                err_log_print = "[checkJobsBF] "+err_path+" has error: "+"\n".join(output)
                print(err_log_print, flush=True)
        except subprocess.CalledProcessError:
            # grep returns non-zero if no lines found, i.e., err_ok is True
            err_ok = True

    out_ok = False
    if os.path.exists(out_path):
        bash = f"grep -q 'Wrote datacard to: ' {out_path}"
        try:
            subprocess.check_call(['bash', '-c', bash])
            out_ok = True
        except subprocess.CalledProcessError:
            pass

    return file_ok and err_ok and out_ok

# --------------------- parse original submit ---------------------
def parse_submit_for_mapping(submit_path: str) -> tuple[str, dict[str, str]]:
    """
    Parse the original submit file <submit_path>.

    Returns (header_text, mapping) where:
      - header_text is the content before the `queue LogFile, Args from (...)` block
        (or a default header if not found).
      - mapping is a dict LogFile -> Args (strings).

    Handles:
      - Multi-line queue blocks
      - Trailing semicolons
      - Single or double-quoted Args
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
        return default_header, {}

    with open(submit_path, "r", errors="ignore") as f:
        content = f.read()

    # Grab header and queue block
    m = re.search(
        r'(.*?)(?:\n|\r)+queue\s+LogFile\s*,\s*Args\s+from\s*\(\s*(.*?)\s*\)\s*$', 
        content, flags=re.S | re.I
    )
    mapping = {}
    if m:
        header = m.group(1).rstrip() + "\n\n"
        queue_block = m.group(2)

        for raw_line in queue_block.strip().splitlines():
            line = raw_line.strip().rstrip(";")  # remove trailing semicolon
            if not line or line.startswith("#"):
                continue

            # Try double-quoted args
            mm = re.match(r'([^\s"]+)\s+"(.+)"$', line)
            if mm:
                mapping[mm.group(1)] = mm.group(2)
                continue

            # Try single-quoted args
            mm = re.match(r"([^\s']+)\s+'(.+)'$", line)
            if mm:
                mapping[mm.group(1)] = mm.group(2)
                continue

            # Last-resort: split at first whitespace
            parts = line.split(None, 1)
            if len(parts) == 2:
                mapping[parts[0]] = parts[1].strip().strip('"').strip("'")

        return header, mapping

    # Fallback: find any lines like `LogFile "Args"` anywhere
    lines = re.findall(r'^\s*([^\s"]+)\s+"([^"]+)"\s*$', content, flags=re.M)
    if lines:
        header = default_header
        for name, args in lines:
            mapping[name] = args
        return header, mapping

    # Nothing found
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
                print(f"[checkJobsBF] Warning: original Args not found for job '{j}'; skipping it in resubmit file.", file=sys.stderr)
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
    submit_name = args.submit_name
    base_dir = os.path.join(args.root_dir, submit_name)

    if not os.path.isdir(base_dir):
        print(f"[checkJobsBF] ERROR: submission directory not found: {base_dir}", file=sys.stderr)
        sys.exit(2)

    out_dir  = os.path.join(base_dir, "out")
    err_dir  = os.path.join(base_dir, "err")
    submit_path = os.path.join(base_dir, f"{submit_name}.sub")

    header, mapping = parse_submit_for_mapping(submit_path)
    
    if mapping:
        # Use original queued LogFile tokens as canonical job list
        jobs = sorted(mapping.keys())
    else:
        # Fallback: union of names found in json/out/err if we couldn't parse the submit
        print("[checkJobsBF] Warning: could not extract LogFile->Args mapping from original submit; "
              "falling back to filesystem enumeration.", file=sys.stderr)
        names = set()
        for d, ext in ((json_dir, ".json"), (out_dir, ".out"), (err_dir, ".err")):
            if os.path.isdir(d):
                for fn in os.listdir(d):
                    if fn.endswith(ext):
                        names.add(os.path.splitext(fn)[0])
        jobs = sorted(names)
    
    if not jobs:
        print(f"[checkJobsBF] No jobs discovered (submit parsing and filesystem fallback both empty).", file=sys.stderr)
        sys.exit(1)

    print(f"[checkJobsBF] Inspecting {len(jobs)} jobs for submission '{submit_name}'", flush=True)

    successful = []
    failed = []

    for job in jobs:
        ok = check_job_ok(base_dir, job)
        if ok:
            successful.append(job)
        else:
            failed.append(job)

    if not failed:
        print("[checkJobsBF] No failed jobs to resubmit.")
        sys.exit(0)

    # Print summary
    if len(failed) > 0:
        print(f"[checkJobsBF] Failed jobs ({len(failed)}):", flush=True)
        for fjob in failed:
            print("  ", fjob, flush=True)
    
    # Build a resubmit submit file containing ONLY the failed jobs
    resubmit_name = f"resubmit_failed_{submit_name}.sub"
    resubmit_path = os.path.join(base_dir, resubmit_name)

    try:
        write_single_queue_resubmit(resubmit_path, header, mapping, failed)
        print(f"\n[checkJobsBF] Resubmit file written: {resubmit_path}", flush=True)
    except RuntimeError as e:
        print(f"[checkJobsBF] No failed jobs could be mapped for resubmit: {e}", flush=True)
        sys.exit(0)

    if args.no_submit:
        print("[checkJobsBF] --no-submit specified: not calling condor_submit.", flush=True)
        sys.exit(0)

    # Submit the resubmit file
    submit_cmd = f"source {CMS_ENV} && condor_submit {resubmit_path}"
    print(f"[checkJobsBF] Submitting resubmit file with:\n  {submit_cmd}\n", flush=True)
    proc = subprocess.run(submit_cmd, shell=True, executable="/bin/bash", capture_output=True, text=True)

    if proc.returncode != 0:
        print(f"[checkJobsBF] condor_submit failed with exit code {proc.returncode}", file=sys.stderr)
        print(proc.stdout, file=sys.stderr)
        print(proc.stderr, file=sys.stderr)
        sys.exit(proc.returncode)

    # Parse condor_submit stdout for schedd and cluster id
    stdout = proc.stdout or ""
    schedd = None
    cluster_id = None

    m_s = re.search(r"Attempting to submit jobs to\s+(\S+)", stdout)
    if m_s:
        schedd = m_s.group(1).strip()

    m_c = re.search(r"submitted to cluster\s+(\d+)", stdout)
    if m_c:
        cluster_id = m_c.group(1).strip()

    # Alternate: "cluster <id> on schedd <name>"
    if not cluster_id:
        m_c2 = re.search(r"cluster\s+(\d+)\s+on\s+schedd\s+(\S+)", stdout)
        if m_c2:
            cluster_id = m_c2.group(1).strip()
            if not schedd:
                schedd = m_c2.group(2).strip()

    if not cluster_id:
        print("[checkJobsBF] Warning: could not parse cluster id from condor_submit output. stdout:\n", stdout, file=sys.stderr)
    else:
        # Append to per-submission submitted_clusters.txt (condor/<submit_name>/submitted_clusters.txt)
        per_sub_file = os.path.join(base_dir, "submitted_clusters.txt")
        try:
            os.makedirs(os.path.dirname(per_sub_file), exist_ok=True)
            with open(per_sub_file, "a") as cf:
                if schedd:
                    cf.write(f"{cluster_id} {schedd}\n")
                else:
                    cf.write(f"{cluster_id}\n")
        except Exception as e:
            print(f"[checkJobsBF] Warning: failed to write {per_sub_file}: {e}", file=sys.stderr)

    print("[checkJobsBF] Resubmit submitted successfully.", flush=True)
    sys.exit(0)

if __name__ == "__main__":
    main()
