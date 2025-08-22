#!/usr/bin/env python3
import os, sys, argparse, subprocess, yaml
from itertools import product
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
from glob import glob
import stat

# -----------------------------
# USER CONFIGURATION SECTION
# -----------------------------
cpus = "1"
memory = "1 GB"
dryrun = False
stress_test = False
lumi = str(400.)
max_workers = 4
limit_submit = None  # limit number of job submissions (None=no limit)

# -----------------------------
# HELPERS
# -----------------------------
def write_flatten_script(bin_names, flatten_exe="./flattenJSONs.x", json_dir="json"):
    os.makedirs("condor", exist_ok=True)
    os.makedirs(json_dir, exist_ok=True)
    output_name = "_".join(bin_names) if bin_names else "all"
    output_file = os.path.join(json_dir, f"flattened_{output_name}.json")
    script_path = "condor/run_flatten.sh"

    with open(script_path, "w") as f:
        f.write("#!/usr/bin/env bash\n")
        f.write("# Auto-generated flatten script\n")
        input_paths = [f"condor/{bin_name}" for bin_name in bin_names]
        f.write(f"{flatten_exe} {' '.join(input_paths)} {output_file}\n")
        f.write(f"echo 'Flattened JSON written to {output_file}'\n")

    os.chmod(script_path, 0o755)
    print(f"[submitJobs] Generated flatten script: {script_path}")

def write_master_hadd_script(bin_names, master_root_dir="root"):
    master_script_path = Path("condor") / "run_hadd_all.sh"
    master_root_dir_path = Path(master_root_dir)
    master_root_dir_path.mkdir(parents=True, exist_ok=True)
    final_root = master_root_dir_path / "hadded_allBins.root"

    with open(master_script_path, "w") as f:
        f.write("#!/usr/bin/env bash\n")
        f.write("# Auto-generated master hadd script\n\n")
        
        # Call per-bin hadd scripts
        for bin_name in bin_names:
            f.write(f"bash condor/{bin_name}/run_hadd.sh\n")

        # Collect per-bin hadded ROOTs
        per_bin_roots = [f"condor/{bin_name}/{master_root_dir}/hadded_{bin_name}.root"
                         for bin_name in bin_names]
        f.write("existing_roots=()\n")
        f.write("for f in " + " ".join(per_bin_roots) + "; do\n")
        f.write("  if [ -f \"$f\" ]; then existing_roots+=(\"$f\"); fi\n")
        f.write("done\n")
        f.write("if [ ${#existing_roots[@]} -gt 0 ]; then\n")
        f.write(f"  hadd -f {final_root} ${{existing_roots[@]}}\n")
        f.write(f"  echo 'Final hadded ROOT -> {final_root}'\n")
        f.write("else\n")
        f.write("  echo 'No per-bin ROOT files found to hadd.'\n")
        f.write("fi\n")

    st = os.stat(master_script_path)
    os.chmod(master_script_path, st.st_mode | stat.S_IEXEC)
    print(f"[submitJobs] Generated master hadd script: {master_script_path}")
    return master_script_path

def setup_master_merge_script(bin_names, flatten_sh="run_flatten.sh", json_dir="json", hadd_sh="run_hadd_all.sh", root_dir="root", do_json=False, do_hadd=False):
    """
    Create master_merge.sh to run all mergers.
    """
    os.makedirs("condor", exist_ok=True)
    master_script_path = "condor/master_merge.sh"

    # Start fresh
    with open(master_script_path, "w") as f:
        f.write("#!/usr/bin/env bash\n")
        f.write("# Auto-generated master merge script\n\n")
        if do_json:
            os.makedirs(json_dir, exist_ok=True)
            for bin_name in bin_names:
                merge_script = f"condor/{bin_name}/mergeJSONs.sh"
                f.write(f"bash {merge_script}\n")
            output_file = os.path.join(json_dir, f"flattened_{'_'.join(bin_names)}.json")
            f.write(f"bash condor/{flatten_sh}\n")
            f.write(f"echo 'Final flattened JSON written to {output_file}'\n")
        if do_hadd:
            os.makedirs(root_dir, exist_ok=True)
            for bin_name in bin_names:
                merge_script = f"condor/{bin_name}/run_hadd.sh"
                f.write(f"bash {merge_script}\n")
            output_file = os.path.join(root_dir, f"hadd_output_{'_'.join(bin_names)}.root")
            f.write(f"bash condor/{hadd_sh}\n")
            f.write(f"echo 'Final hadd written to {output_file}\n")
    
    os.chmod(master_script_path, 0o755)
    print(f"[submitJobs] Master merge script generated: {master_script_path}")
    return master_script_path

def load_datasets(cfg_path):
    with open(cfg_path, "r") as f:
        cfg = yaml.safe_load(f)
    bkg = cfg.get("datasets", {}).get("bkg", [])
    sig = cfg.get("datasets", {}).get("sig", [])
    sms_filters = cfg.get("sms_filters", [])
    return bkg, sig, sms_filters

def build_bin_name(base, shorthand, side, extra=None):
    name = f"{base}_{shorthand}"
    if side:
        name += f"_{side}"
    if extra:
        name += f"_{extra}"
    return name.replace(">", "gt").replace("<", "lt").replace("=", "eq").replace("+", "p").replace(",", "").replace(" ", "")

def build_lep_cuts(shorthand, side):
    return shorthand + (f"_{side}" if side else "")

def generate_bins_from_shorthands():
    base_name = "TEST"
    base_cuts = "Nlep>=2;MET>=150"
    predefined_cuts = "Cleaning"
    shorthands = ["=0Bronze","=2Pos","=2Gold",">=1OSSF","=1SSOF",">=2Mu",">=1Elec","<1SSSF",">=1Muon"]
    sides = ["","a","b"]
    generated = {}
    for shorthand, side in product(shorthands, sides):
        bin_name = build_bin_name(base_name, shorthand, side)
        lep_cut = build_lep_cuts(shorthand, side)
        generated[bin_name] = {"cuts": base_cuts, "lep-cuts": lep_cut, "predefined-cuts": predefined_cuts}
    return generated

def build_command(bin_name, cfg, bkg_datasets, sig_datasets, sms_filters, make_json, make_root, hist_yaml):
    cmd = [
        "python3", "python/createJobs.py",
        "--bkg_datasets", *bkg_datasets,
        "--sig_datasets", *sig_datasets,
        "--bin", bin_name,
        "--cuts", cfg.get("cuts",""),
        "--lep-cuts", cfg.get("lep-cuts",""),
        "--predefined-cuts", cfg.get("predefined-cuts",""),
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
    global dryrun, stress_test, lumi

    parser = argparse.ArgumentParser(description="Submit BFI jobs with templated bins")
    parser.add_argument("--dryrun", action="store_true")
    parser.add_argument("--stress_test", action="store_true")
    parser.add_argument("--lumi", type=str, default=lumi)
    parser.add_argument("--bins-cfg", type=str, default="config/examples.yaml")
    parser.add_argument("--datasets-cfg", type=str, default="config/datasets.yaml")
    parser.add_argument("--make-json", action="store_true", help="Create JSON output")
    parser.add_argument("--make-root", action="store_true", help="Create ROOT output")
    parser.add_argument("--hist-yaml", type=str, default=None, help="YAML file for histogram configuration")
    args = parser.parse_args()

    # Default behavior: make JSON if neither specified
    make_json = args.make_json
    make_root = args.make_root
    if not (make_json or make_root):
        make_json = True

    dryrun = args.dryrun
    stress_test = args.stress_test
    lumi = args.lumi

    # Load datasets
    bkg_datasets, sig_datasets, sms_filters = load_datasets(args.datasets_cfg)

    # Load bins
    bins_cfg_path = Path(args.bins_cfg)
    bins = {}
    if bins_cfg_path.exists():
        with open(bins_cfg_path) as f:
            manual_bins = yaml.safe_load(f) or {}
        for k,v in manual_bins.items():
            if isinstance(v, dict):
                v.setdefault("cuts","")
                v.setdefault("lep-cuts","")
                v.setdefault("predefined-cuts","")
        bins.update(manual_bins)
    else:
        bins.update({"manualExample1":{"cuts":"Nlep>=2;MET>=150;PTISR>=200","lep-cuts":">=1OSSF","predefined-cuts":"Cleaning"}})

    if stress_test:
        gen = generate_bins_from_shorthands()
        for k,v in gen.items():
            if k not in bins:
                bins[k]=v

    # Submit createJobs.py for each bin
    jobs = []
    print("\n===== BEGIN BIN DEFINITIONS =====\n")
    for bin_name, cfg in bins.items():
        cmd = build_command(bin_name, cfg, bkg_datasets, sig_datasets, sms_filters, make_json, make_root, args.hist_yaml)
        jobs.append(cmd)
        print(f"[BIN-DEF] bin={bin_name} cuts={cfg.get('cuts')} lep-cuts={cfg.get('lep-cuts')} predefined-cuts={cfg.get('predefined-cuts')}")
    print("===== END BIN DEFINITIONS =====\n")

    if limit_submit is not None:
        jobs = jobs[:limit_submit]

    with ThreadPoolExecutor(max_workers=max_workers) as executor:
        executor.map(submit_job, jobs)

    if not dryrun:
        if args.make_json:
            # Generate flatten JSON
            write_flatten_script(list(bins.keys()))
        if args.make_root:
            # Generate master hadd script
            write_master_hadd_script(list(bins.keys()))
        if args.make_json or args.make_root:
            # Generate final merge script
            setup_master_merge_script(list(bins.keys()), do_json=args.make_json, do_hadd=args.make_root)
        print("\nAll submissions dispatched.\n")

if __name__ == "__main__":
    main()

