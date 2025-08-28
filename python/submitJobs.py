#!/usr/bin/env python3
import os
import sys
import argparse
import subprocess
import yaml
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

# ---------------------------------
# DEFAULT CONFIGURATION PARAMETERS
# ---------------------------------
cpus = "1"
memory = "1 GB"
dryrun = False
max_workers = 4
limit_submit = None  # limit number of job submissions (None=no limit)

# ---------------------------------
# HELPERS
# ---------------------------------
def strip_inline_comments(s):
    if not isinstance(s, str):
        return s
    lines = []
    for line in s.splitlines():
        line = line.split("#", 1)[0].rstrip()
        if line.strip():
            lines.append(line)
    return "\n".join(lines)

def load_processes(cfg_path):
    with open(cfg_path, "r") as f:
        cfg = yaml.safe_load(f)
    bkg = cfg.get("processes", {}).get("bkg", [])
    sig = cfg.get("processes", {}).get("sig", [])
    sms_filters = cfg.get("sms_filters", [])
    return bkg, sig, sms_filters

def build_command(bin_name, cfg, bkg_processes, sig_processes, sms_filters, make_json, make_root, hist_yaml, lumi):
    cmd = [
        "python3", "python/createJobs.py",
        "--bkg_processes", *bkg_processes,
        "--sig_processes", *sig_processes,
        "--bin", bin_name,
        "--cuts", cfg.get("cuts","").replace('\n',''),
        "--lep-cuts", cfg.get("lep-cuts","").replace('\n',''),
        "--predefined-cuts", cfg.get("predefined-cuts","").replace('\n',''),
        "--user-cuts", cfg.get("user-cuts","").replace('\n',''),
        "--cpus", cpus,
        "--memory", memory,
        "--lumi", lumi
    ]
    if sms_filters:
        cmd += ["--sms-filters", *sms_filters]
    if dryrun:
        cmd.append("--dryrun")

    # Add histogram/ROOT options
    if make_json:
        cmd.append("--make-json")
    if make_root:
        cmd.append("--make-root")
    if hist_yaml:
        cmd += ["--hist-yaml", hist_yaml]

    return cmd

def submit_job(cmd):
    print("Submit command:", " ".join(cmd))
    if not dryrun:
        subprocess.run(cmd)

# -----------------------------
# MAIN
# -----------------------------
def main():
    global dryrun

    parser = argparse.ArgumentParser(description="Submit BFI jobs (only: call createJobs.py for each bin)")
    parser.add_argument("--dryrun", action="store_true")
    parser.add_argument("--lumi", type=str, default="1.")
    parser.add_argument("--bins-cfg", type=str, default="config/bin_cfgs/examples.yaml",
                        help="YAML file with bin definitions (same format as before)")
    parser.add_argument("--processes-cfg", type=str, default="config/process_cfgs/processes.yaml")
    parser.add_argument("--make-json", action="store_true", help="Pass --make-json to createJobs.py")
    parser.add_argument("--make-root", action="store_true", help="Pass --make-root to createJobs.py")
    parser.add_argument("--hist-yaml", type=str, default=None, help="YAML file for histogram configuration")
    parser.add_argument("--bins-list-out", type=str, default="condor/bins_list.txt",
                        help="Optional: write the list of bin names to this file for createMergers.py")
    args = parser.parse_args()

    # Default behavior: make JSON if neither specified
    make_json = args.make_json
    make_root = args.make_root
    if not (make_json or make_root):
        make_json = True

    dryrun = args.dryrun
    lumi = args.lumi

    # Load processes
    bkg_processes, sig_processes, sms_filters = load_processes(args.processes_cfg)

    # Load bins
    bins_cfg_path = Path(args.bins_cfg)
    bins = {}
    if bins_cfg_path.exists():
        with open(bins_cfg_path) as f:
            bins = yaml.safe_load(f) or {}
            for k, v in bins.items():
                if isinstance(v, dict):
                    for key in ("cuts", "lep-cuts", "predefined-cuts", "user-cuts"):
                        v.setdefault(key, "")
                        v[key] = strip_inline_comments(v[key])
    else:
        print(f"[submitJobs] ERROR: bins cfg not found: {args.bins_cfg}", file=sys.stderr)
        sys.exit(1)

    # Build commands
    jobs = []
    print("\n===== BEGIN BIN DEFINITIONS =====\n")
    for bin_name, cfg in bins.items():
        cmd = build_command(bin_name, cfg, bkg_processes, sig_processes, sms_filters, make_json, make_root, args.hist_yaml, lumi)
        jobs.append(cmd)
        print(f"[BIN-DEF] bin={bin_name} cuts={cfg.get('cuts')} lep-cuts={cfg.get('lep-cuts')} predefined-cuts={cfg.get('predefined-cuts')} user-cuts={cfg.get('user-cuts')}")
    print("===== END BIN DEFINITIONS =====\n")

    if limit_submit is not None:
        jobs = jobs[:limit_submit]

    # Submit jobs in parallel
    with ThreadPoolExecutor(max_workers=max_workers) as executor:
        executor.map(submit_job, jobs)

    # Optionally write bins-list for createMergers.py to consume
    if args.bins_list_out:
        os.makedirs(os.path.dirname(args.bins_list_out) or ".", exist_ok=True)
        with open(args.bins_list_out, "w") as f:
            for b in bins.keys():
                f.write(b + "\n")
        print(f"[submitJobs] Wrote bin list to: {args.bins_list_out}")

    print("\nAll submissions dispatched.\n")

if __name__ == "__main__":
    main()
