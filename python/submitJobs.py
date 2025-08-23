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

def write_master_hadd_script(bin_names, condor_dir="condor", master_root_dir="root"):
    """
    Create a master hadd script that merges all per-bin ROOT files into one final ROOT
    file named like: hadded_BIN1_BIN2_..._BINn.root
    """
    master_script_path = os.path.join(condor_dir, "run_hadd_all.sh")
    os.makedirs(master_root_dir, exist_ok=True)

    joined_bins = "_".join(bin_names)
    final_root = os.path.join(master_root_dir, f"hadded_{joined_bins}.root")

    with open(master_script_path, "w") as f:
        f.write("#!/usr/bin/env bash\n")
        f.write("# Auto-generated master hadd script\n\n")

        # Call per-bin hadd scripts first
        for bin_name in bin_names:
            hadd_script = os.path.join(condor_dir, bin_name, "haddROOTs.sh")
            f.write(f"bash {hadd_script}\n")

        # Collect per-bin ROOT outputs
        per_bin_roots = [os.path.join(condor_dir, bin_name, f"{bin_name}.root") for bin_name in bin_names]
        f.write("existing_roots=()\n")
        f.write("for f in " + " ".join(per_bin_roots) + "; do\n")
        f.write("  if [ -f \"$f\" ]; then existing_roots+=(\"$f\"); fi\n")
        f.write("done\n")

        # Merge if at least one ROOT exists
        f.write("if [ ${#existing_roots[@]} -gt 0 ]; then\n")
        f.write(f"  hadd -f {final_root} ${{existing_roots[@]}}\n")
        f.write(f"  echo 'Final hadded ROOT -> {final_root}'\n")
        f.write("else\n")
        f.write("  echo 'No per-bin ROOT files found to hadd.'\n")
        f.write("fi\n")

    os.chmod(master_script_path, 0o755)
    print(f"[submitJobs] Generated master hadd script: {master_script_path}")
    return master_script_path

def setup_master_merge_script(
    bin_names,
    flatten_sh="run_flatten.sh",
    json_dir="json",
    hadd_sh="run_hadd_all.sh",
    root_dir="root",
    do_json=False,
    do_hadd=False,
    condor_dir="condor"
):
    """
    Create master_merge.sh to run all mergers (JSON flattening + ROOT hadd).
    Final output files include the spliced bin names.
    """
    os.makedirs(condor_dir, exist_ok=True)
    master_script_path = os.path.join(condor_dir, "master_merge.sh")

    joined_bins = "_".join(bin_names)

    with open(master_script_path, "w") as f:
        f.write("#!/usr/bin/env bash\n")
        f.write("# Auto-generated master merge script\n\n")

        # --- JSON merging ---
        if do_json:
            os.makedirs(json_dir, exist_ok=True)
            for bin_name in bin_names:
                merge_script = os.path.join(condor_dir, bin_name, "mergeJSONs.sh")
                f.write(f"bash {merge_script}\n")
            f.write(f"bash {os.path.join(condor_dir, flatten_sh)}\n")
            f.write(f"echo 'Final flattened JSON written to {json_dir}/flattened_{joined_bins}.json'\n\n")

        # --- ROOT hadd ---
        if do_hadd:
            os.makedirs(root_dir, exist_ok=True)
            # Call per-bin hadd scripts
            for bin_name in bin_names:
                hadd_script = os.path.join(condor_dir, bin_name, "haddROOTs.sh")
                f.write(f"bash {hadd_script}\n")
            # Call master hadd script
            f.write(f"bash {os.path.join(condor_dir, hadd_sh)}\n")
            f.write(f"echo 'Final hadd written to {root_dir}/hadded_{joined_bins}.root'\n")

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
    user_cuts = ""
    shorthands = ["=0Bronze","=2Pos","=2Gold",">=1OSSF","=1SSOF",">=2Mu",">=1Elec","<1SSSF",">=1Muon"]
    sides = ["","a","b"]
    generated = {}
    for shorthand, side in product(shorthands, sides):
        bin_name = build_bin_name(base_name, shorthand, side)
        lep_cut = build_lep_cuts(shorthand, side)
        generated[bin_name] = {"cuts": base_cuts, "lep-cuts": lep_cut, "predefined-cuts": predefined_cuts, "user-cuts": user_cuts}
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
        "--user-cuts", cfg.get("user-cuts",""),
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
    parser.add_argument("--bins-cfg", type=str, default="config/bin_cfgs/examples.yaml")
    parser.add_argument("--datasets-cfg", type=str, default="config/dataset_cfgs/datasets.yaml")
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
                v.setdefault("user-cuts","")
        bins.update(manual_bins)
    else:
        bins.update({"manualExample1":{"cuts":"Nlep>=2;MET>=150;PTISR>=200","lep-cuts":">=1OSSF","predefined-cuts":"Cleaning","user-cuts":""}})

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
        print(f"[BIN-DEF] bin={bin_name} cuts={cfg.get('cuts')} lep-cuts={cfg.get('lep-cuts')} predefined-cuts={cfg.get('predefined-cuts')} user-cuts={cfg.get('user-cuts')}")
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

