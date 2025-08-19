#!/usr/bin/env python3
import os, argparse, subprocess
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
lumi = str(1.)

# Maximum concurrent submissions
max_workers = 4

# Limit to first N jobs when submitting (None => no limit)
limit_submit = None  # e.g., 10

# -----------------------------
# TEMPLATING / HELPERS
# -----------------------------

def write_flatten_script(bin_names, flatten_exe="./flattenJSONs.x", json_dir="json"):
    """
    Create a run_flatten.sh script that merges all JSONs for the given bins.
    
    Args:
        bin_names (list of str): List of bin names.
        flatten_exe (str): Path to the flattenJSON executable.
        json_dir (str): Directory where final flattened JSON will be stored.
    """
    os.makedirs(json_dir, exist_ok=True)
    output_name = "_".join(bin_names)
    output_file = os.path.join(json_dir, f"flattened_{output_name}.json")
    script_path = "condor/run_flatten.sh"

    with open(script_path, "w") as f:
        f.write("#!/usr/bin/env bash\n")
        f.write("# Auto-generated flatten script\n")
        # build the command
        input_paths = [f"condor/{bin_name}" for bin_name in bin_names]
        f.write(f"{flatten_exe} {' '.join(input_paths)} {output_file}\n")
        f.write(f"echo 'Flattened JSON written to {output_file}'\n")

    os.chmod(script_path, 0o755)
    print(f"[submitJobs] Generated flatten script: {script_path}")

def setup_master_merge_script(bin_names, flatten_sh="run_flatten.sh", json_dir="json"):
    """
    Create master_merge.sh to run all per-bin merges and then flatten into a single JSON.
    """
    os.makedirs(json_dir, exist_ok=True)
    master_script_path = "condor/master_merge.sh"

    # Start fresh
    with open(master_script_path, "w") as f:
        f.write("#!/usr/bin/env bash\n")
        f.write("# Auto-generated master merge script\n\n")
    
        # Step 1: Run all per-bin merge scripts
        for bin_name in bin_names:
            merge_script = f"condor/{bin_name}/mergeJSONs.sh"
            f.write(f"bash {merge_script}\n")
        
        # Step 2: Call flatten on all merged bins
        input_bins = " ".join([f"condor/{bin_name}" for bin_name in bin_names])
        output_file = os.path.join(json_dir, f"flattened_{'_'.join(bin_names)}.json")
        f.write(f"bash condor/{flatten_sh}\n")
        f.write(f"echo 'Final flattened JSON written to {output_file}'\n")
    
    os.chmod(master_script_path, 0o755)
    print(f"[submitJobs] Master merge script generated: {master_script_path}")
    return master_script_path

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
        "=2Pos",
        "=2Gold",
        ">=1OSSF",
        "=1SSOF",
        ">=2Mu",
        ">=1Elec",
        "<1SSSF",
        ">=1Muon"
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
        "--memory", memory,
        "--lumi", lumi
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
    global dryrun, stress_test, lumi

    # CLI parsing
    parser = argparse.ArgumentParser(description="Submit BFI jobs with templated bins")
    parser.add_argument("--dryrun", dest="dryrun", action="store_true",
                        help="Enable dry-run mode")
    parser.add_argument("--stress_test", dest="stress_test", action="store_true",
                        help="Run stress test")
    parser.add_argument("--lumi", dest="lumi", type=str, default=lumi,
                        help="Lumi to scale events to")
    parser.set_defaults(dryrun=dryrun, stress_test=stress_test)
    args = parser.parse_args()

    dryrun = args.dryrun
    stress_test = args.stress_test
    lumi = args.lumi

    bins = {}
    
    # Manual bins
    manual_bins = {
        # Structure
        # Name of bin: {
        #     "cuts": cuts directly on saved branches,
        #     "lep-cuts": uses BuildFitInput::BuildLeptonCut to form cuts (see stress test above for examples),
        #     "predefined-cuts": predefined cuts in src/BuildFitInput.cpp,
        # },
        "manualExample1": {
            "cuts": "Nlep>=2,MET>=150,PTISR>=200",
            "lep-cuts": ">=1OSSF",
            "predefined-cuts": "Cleaning",
        },
        "manualExample2": {
            "cuts": "Nlep>=2,MET>=200,PTISR>=300",
            "lep-cuts": ">1OSSF",
            "predefined-cuts": "Cleaning",
        },
    }
    bins.update(manual_bins)
    
    # Templated bins
    if stress_test:
        gen = generate_bins_from_shorthands()
        for k, v in gen.items():
            if k in bins:
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
    print(f"Prepared {total_jobs} bin(s) for submission.")
    if dryrun:
        print("Dry-run mode enabled: no jobs will actually be submitted.")
    if limit_submit is not None:
        print(f"Limit enabled: will only submit first {limit_submit} job(s).")
    
    # Apply limit if requested
    if limit_submit is not None:
        jobs = jobs[:limit_submit]
    
    # Submit
    with ThreadPoolExecutor(max_workers=max_workers) as executor:
        executor.map(submit_job, jobs)
    
    if not dryrun:
        write_flatten_script(list(bins.keys()))
        setup_master_merge_script(list(bins.keys()))
        print("\nAll submissions dispatched.\n")

if __name__ == "__main__":
    main()

