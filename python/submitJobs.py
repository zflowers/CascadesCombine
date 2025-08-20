#!/usr/bin/env python3
import os, sys, argparse, subprocess, yaml
from itertools import product
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

# -----------------------------
# USER CONFIGURATION SECTION
# -----------------------------

# Base arguments
cpus = "1"
memory = "1 GB"

# Defaults (can be overridden by CLI args)
dryrun = False
stress_test = False
lumi = str(400.)

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
    """
    os.makedirs("condor", exist_ok=True)
    os.makedirs(json_dir, exist_ok=True)
    output_name = "_".join(bin_names) if bin_names else "all"
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

def load_datasets(cfg_path):
    """Read dataset definitions from YAML."""
    with open(cfg_path, "r") as f:
        cfg = yaml.safe_load(f)
    bkg = cfg.get("datasets", {}).get("bkg", [])
    sig = cfg.get("datasets", {}).get("sig", [])
    return bkg, sig

def setup_master_merge_script(bin_names, flatten_sh="run_flatten.sh", json_dir="json"):
    """
    Create master_merge.sh to run all per-bin merges and then flatten into a single JSON.
    """
    os.makedirs("condor", exist_ok=True)
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
    base_cuts = "Nlep>=2;MET>=150"
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
        ">=1Muon",
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

def build_command(bin_name, cfg, bkg_datasets, sig_datasets, sms_filters):
    """Construct the subprocess command to call createJobs.py"""
    cuts = cfg.get("cuts", "")
    lep_cuts = cfg.get("lep-cuts", "")
    predefined = cfg.get("predefined-cuts", "")

    cmd = [
        "python3", "python/createJobs.py",
        "--bkg_datasets", *bkg_datasets,
        "--sig_datasets", *sig_datasets,
        "--bin", bin_name,
        "--cuts", cuts,
        "--lep-cuts", lep_cuts,
        "--predefined-cuts", predefined,
        "--cpus", cpus,
        "--memory", memory,
        "--lumi", lumi,
    ]

    # Add SMS filters if provided
    if sms_filters:
        cmd += ["--sms-filters", *sms_filters]

    if dryrun:
        cmd.append("--dryrun")
    return cmd

def submit_job(cmd):
    """Submit a single job via subprocess"""
    print("Submit command:", " ".join(cmd))
    if not dryrun:
        subprocess.run(cmd)

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
    parser.add_argument("--bins-cfg", dest="bins_cfg", type=str, default="config/examples.yaml",
                        help="YAML config file containing bin definitions")
    parser.add_argument("--datasets-cfg", dest="datasets_cfg", type=str,
                        default="config/datasets.yaml",
                        help="YAML config file containing dataset definitions")
    parser.set_defaults(dryrun=dryrun, stress_test=stress_test)
    args = parser.parse_args()

    dryrun = args.dryrun
    stress_test = args.stress_test
    lumi = args.lumi

    # Load dataset names from YAML
    bkg_datasets, sig_datasets = load_datasets(args.datasets_cfg)
    with open(args.datasets_cfg, "r") as f:
        yaml_cfg = yaml.safe_load(f)
    
    sms_filters = yaml_cfg.get("sms_filters", [])

    bins = {}
    bins_cfg_path = args.bins_cfg
    cfg_path = Path(bins_cfg_path)

    # built-in default manual_bins (used only as fallback if YAML missing)
    default_manual_bins = {
        "manualExample1": {
            "cuts": "Nlep>=2;MET>=150;PTISR>=200",
            "lep-cuts": ">=1OSSF",
            "predefined-cuts": "Cleaning",
        }
    }

    if cfg_path.exists():
        try:
            with open(cfg_path) as f:
                manual_bins = yaml.safe_load(f) or {}
            # Normalize/flatten each bin entry
            for name, cfg in list(manual_bins.items()):
                if not isinstance(cfg, dict):
                    print(f"[submitJobs] WARNING: bin '{name}' in YAML is not a mapping/dict; skipping.")
                    manual_bins.pop(name, None)
                    continue
                # Ensure keys exist
                cfg.setdefault("cuts", "")
                cfg.setdefault("lep-cuts", "")
                cfg.setdefault("predefined-cuts", "")

                # Process lep-cuts: remove commented lines and join remaining lines
                if isinstance(cfg["lep-cuts"], str):
                    kept_lines = []
                    for line in cfg["lep-cuts"].splitlines():
                        if line.strip().startswith("#"):
                            # skip commented lines
                            continue
                        kept_lines.append(line.rstrip())
                    # join without newline to produce the compact shorthand expected by createJobs
                    cfg["lep-cuts"] = "".join(kept_lines)
            bins.update(manual_bins)
            print(f"[submitJobs] Loaded {len(manual_bins)} bin(s) from YAML: {cfg_path}")
        except Exception as e:
            print(f"[submitJobs] ERROR loading YAML config: {e}")
            print("[submitJobs] Falling back to built-in manual bins.")
            bins.update(default_manual_bins)
    else:
        print(f"[submitJobs] WARNING: YAML config not found at {cfg_path}")
        print("[submitJobs] Using built-in manual bins as fallback (you can supply --bins-cfg).")
        bins.update(default_manual_bins)

    # -----------------------------
    # GENERATE STRESS-TEST TEMPLATED BINS
    # -----------------------------
    if stress_test:
        gen = generate_bins_from_shorthands()
        for k, v in gen.items():
            if k in bins:
                print(f"[TEMPLATE SKIP] Generated bin '{k}' collides with manual bin; skipping.")
                continue
            bins[k] = v

    # -----------------------------
    # Build jobs
    # -----------------------------
    jobs = []
    print("\n===== BEGIN BIN DEFINITIONS =====\n")
    for bin_name, cfg in bins.items():
        # ensure cfg has expected keys (defensive)
        if not isinstance(cfg, dict):
            print(f"[WARN] skipping malformed bin '{bin_name}'")
            continue

        cmd = build_command(bin_name, cfg, bkg_datasets, sig_datasets, sms_filters)
        jobs.append(cmd)

        print(f"[BIN-DEF] bin=\"{bin_name}\"")
        print(f"          lep-cuts=\"{cfg.get('lep-cuts', '')}\"")
        print(f"          cuts=\"{cfg.get('cuts', '')}\"")
        print(f"          predefined-cuts=\"{cfg.get('predefined-cuts', '')}\"\n")

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

