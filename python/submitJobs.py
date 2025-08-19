#!/usr/bin/env python3
import argparse
import subprocess
from itertools import product
from concurrent.futures import ThreadPoolExecutor

# -----------------------------
# USER CONFIGURATION SECTION
# -----------------------------

# Datasets
bkg_datasets = ["ttbar", "ST", "DY", "ZInv", "DBTB", "QCD", "Wjets"]
sig_datasets = ["Cascades"]

# Base arguments
cpus = "1"
memory = "1 GB"

# Defaults (can be overridden by CLI args)
dryrun = False
stress_test = False

# Use template-generated bins. Comment the generate call below to disable.
use_generate_from_shorthand = True

# Maximum concurrent submissions
max_workers = 4

# Limit to first N jobs when submitting (None => no limit)
limit_submit = None  # e.g., 10

# -----------------------------
# TEMPLATING / HELPERS
# -----------------------------

def build_bin_name(base, shorthand, side, extra=None):
    """Construct a unique bin name"""
    name = f"{base}_{shorthand}"
    if side:
        name += f"_{side}"
    if extra:
        name += f"_{extra}"
    # sanitize: replace characters that might be awkward in names (optional)
    name = name.replace(">", "gt").replace("<", "lt").replace("=", "eq").replace("+", "p")
    name = name.replace(",", "").replace(" ", "")
    return name

def build_lep_cuts(shorthand, side):
    """Build the lepton cut expression (simple expansion matching your C++ shorthand style)"""
    return shorthand + (f"_{side}" if side else "")

def generate_bins_from_shorthands():
    """
    Generate a dict of bins from cartesian product of shorthands x sides.
    Returns mapping: bin_name -> { "cuts": ..., "lep-cuts": ..., "predefined-cuts": ... }
    """
    base_name = "TEST"
    base_cuts = "Nlep>=2,MET>=150"
    predefined_cuts = "Cleaning"
    
    shorthands = [
        "=0Bronze",
        #"=2Pos",
        #"=2Gold",
        #">=1OSSF",
        #"=1SSOF",
        # ">=2Mu",
        #">=1Elec",
        #"<1SSSF",
        # ">=1Muon"
    ]
    
    sides = [
        "",
        "a",
        "b"
    ]
    
    generated = {}
    for shorthand, side in product(shorthands, sides):
        bin_name = build_bin_name(base_name, shorthand, side)
        lep_cut = build_lep_cuts(shorthand, side)
        generated[bin_name] = {
            "cuts": base_cuts,
            "lep-cuts": lep_cut,
            "predefined-cuts": predefined_cuts
        }
    return generated

def build_command(bin_name, cfg):
    """Construct the subprocess command to call createJobs.py"""
    cmd = [
        "python3", "python/createJobs.py",
        "--bkg_datasets", *bkg_datasets,
        "--sig_datasets", *sig_datasets,
        "--bin", bin_name,
        "--cuts", cfg["cuts"],
        "--lep-cuts", cfg["lep-cuts"],
        "--predefined-cuts", cfg["predefined-cuts"],
        "--cpus", cpus,
        "--memory", memory
    ]
    if dryrun:
        cmd.append("--dryrun")
    return cmd

def submit_job(cmd):
    """Submit a single job via subprocess"""
    print("Submit command:", " ".join(cmd))
    if not dryrun:
        subprocess.run(cmd)

# -----------------------------
# MAIN
# -----------------------------

def main():
    global dryrun, stress_test

    # CLI parsing
    parser = argparse.ArgumentParser(description="Submit BFI jobs with templated bins")
    parser.add_argument("--dryrun", dest="dryrun", action="store_true",
                        help="Enable dry-run mode")
    parser.add_argument("--stress_test", dest="stress_test", action="store_true",
                        help="Run stress test")
    parser.set_defaults(dryrun=dryrun, stress_test=stress_test)
    args = parser.parse_args()

    dryrun = args.dryrun
    stress_test = args.stress_test

    bins = {}
    
    # Manual bins
    manual_bins = {
        "TEST_manualExample": {
            "cuts": "Nlep>=2,MET>=150,PTISR>=200",
            "lep-cuts": ">=1OSSF",
            "predefined-cuts": "Cleaning",
        },
    }
    bins.update(manual_bins)
    
    # Templated bins
    if use_generate_from_shorthand:
        gen = generate_bins_from_shorthands()
        for k, v in gen.items():
            if k in bins:
                if stress_test:
                    print(f"[TEMPLATE SKIP] Generated bin '{k}' collides with manual bin; skipping.")
                continue
            bins[k] = v
    
    # Build jobs
    jobs = []
    print("\n===== BEGIN BIN DEFINITIONS =====\n")
    for bin_name, cfg in bins.items():
        cmd = build_command(bin_name, cfg)
        jobs.append(cmd)
    
        print(f"[BIN-DEF] bin=\"{bin_name}\"")
        print(f"          lep-cuts=\"{cfg['lep-cuts']}\"")
        print(f"          cuts=\"{cfg['cuts']}\"")
        print(f"          predefined-cuts=\"{cfg['predefined-cuts']}\"\n")
    
    print("===== END BIN DEFINITIONS =====\n")
    
    # Summary
    total_jobs = len(jobs)
    print(f"Prepared {total_jobs} job(s) for submission.")
    if dryrun:
        print("Dry-run mode enabled: no jobs will actually be submitted.")
    if limit_submit is not None:
        print(f"Limit enabled: will only submit first {limit_submit} job(s).")
    print()
    
    # Apply limit if requested
    if limit_submit is not None:
        jobs = jobs[:limit_submit]
    
    # Submit
    with ThreadPoolExecutor(max_workers=max_workers) as executor:
        executor.map(submit_job, jobs)
    
    if not dryrun:
        print("\nAll submissions dispatched.\n")

if __name__ == "__main__":
    main()

