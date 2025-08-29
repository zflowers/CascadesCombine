#!/usr/bin/env python3
import os, sys, argparse, yaml
from pathlib import Path

# ---------------------------------
# HELPERS
# ---------------------------------
def join_bins(bin_names, sep="__"):
    if not bin_names:
        return "all"
    return sep.join(bin_names)

def write_flatten_script(bin_names, flatten_exe="./flattenJSONs.x", json_dir="json", condor_dir="condor"):
    os.makedirs(condor_dir, exist_ok=True)
    os.makedirs(json_dir, exist_ok=True)
    joined = join_bins(bin_names)
    script_name = f"run_flatten_{joined}.sh"
    script_path = os.path.join(condor_dir, script_name)
    output_file = os.path.join(json_dir, f"flattened_{joined}.json")

    with open(script_path, "w") as f:
        f.write("#!/usr/bin/env bash\n")
        f.write("# Auto-generated flatten script\n\n")
        input_paths = [os.path.join(condor_dir, bin_name) for bin_name in bin_names]
        # join input paths safely (they should not contain whitespace normally)
        f.write(f"{flatten_exe} {' '.join(input_paths)} {output_file}\n")
        f.write(f"echo 'Flattened JSON written to {output_file}'\n")

    os.chmod(script_path, 0o755)
    print(f"[createMergers] Generated flatten script: {script_path}")
    return script_path

def write_master_hadd_script(bin_names, condor_dir="condor", master_root_dir="root"):
    os.makedirs(condor_dir, exist_ok=True)
    os.makedirs(master_root_dir, exist_ok=True)
    joined = join_bins(bin_names)
    script_name = f"run_hadd_all_{joined}.sh"
    master_script_path = os.path.join(condor_dir, script_name)
    final_root = os.path.join(master_root_dir, f"hadded_{joined}.root")

    with open(master_script_path, "w") as f:
        f.write("#!/usr/bin/env bash\n")
        f.write("# Auto-generated master hadd script\n\n")

        # Call per-bin hadd scripts first
        for bin_name in bin_names:
            hadd_script = os.path.join(condor_dir, bin_name, "haddROOTs.sh")
            f.write(f"bash {hadd_script}\n")

        # Collect per-bin ROOT outputs
        per_bin_roots = [os.path.join(condor_dir, bin_name, f"{bin_name}.root") for bin_name in bin_names]
        f.write("\nexisting_roots=()\n")
        f.write("for f in " + " ".join(per_bin_roots) + "; do\n")
        f.write("  if [ -f \"$f\" ]; then existing_roots+=(\"$f\"); fi\n")
        f.write("done\n\n")

        # Merge if at least one ROOT exists
        f.write("if [ ${#existing_roots[@]} -gt 0 ]; then\n")
        f.write(f"  hadd -f {final_root} ${{existing_roots[@]}}\n")
        f.write(f"  echo 'Final hadded ROOT -> {final_root}'\n")
        f.write("else\n")
        f.write("  echo 'No per-bin ROOT files found to hadd.'\n")
        f.write("fi\n")

    os.chmod(master_script_path, 0o755)
    print(f"[createMergers] Generated master hadd script: {master_script_path}")
    return master_script_path

def setup_master_merge_script(
    bin_names,
    flatten_sh=None,
    json_dir="json",
    hadd_sh=None,
    root_dir="root",
    do_json=False,
    do_hadd=False,
    condor_dir="condor"
):
    """
    Create master_merge_<joined_bins>.sh to run all mergers (JSON flattening + ROOT hadd).
    flatten_sh and hadd_sh should be script filenames (not full paths) living in condor_dir,
    or None if not present.
    """
    os.makedirs(condor_dir, exist_ok=True)
    joined = join_bins(bin_names)
    master_script_name = f"master_merge_{joined}.sh"
    master_script_path = os.path.join(condor_dir, master_script_name)

    with open(master_script_path, "w") as f:
        f.write("#!/usr/bin/env bash\n")
        f.write("# Auto-generated master merge script\n\n")

        # --- JSON merging ---
        if do_json:
            os.makedirs(json_dir, exist_ok=True)
            for bin_name in bin_names:
                merge_script = os.path.join(condor_dir, bin_name, "mergeJSONs.sh")
                f.write(f"bash {merge_script}\n")
            if flatten_sh:
                f.write(f"bash {os.path.join(condor_dir, flatten_sh)}\n")
            else:
                f.write("echo 'Warning: flatten script not provided; skipping flatten step.'\n")
            f.write(f"echo 'Final flattened JSON written to {json_dir}/flattened_{joined}.json'\n\n")

        # --- ROOT hadd ---
        if do_hadd:
            os.makedirs(root_dir, exist_ok=True)
            # Call per-bin hadd scripts
            for bin_name in bin_names:
                hadd_script = os.path.join(condor_dir, bin_name, "haddROOTs.sh")
                f.write(f"bash {hadd_script}\n")
            # Call master hadd script
            if hadd_sh:
                f.write(f"bash {os.path.join(condor_dir, hadd_sh)}\n")
            else:
                f.write("echo 'Warning: master hadd script not provided; skipping hadd step.'\n")
            f.write(f"echo 'Final hadd written to {root_dir}/hadded_{joined}.root'\n")

    os.chmod(master_script_path, 0o755)
    print(f"[createMergers] Master merge script generated: {master_script_path}")
    return master_script_path

def strip_inline_comments(s):
    if not isinstance(s, str):
        return s
    lines = []
    for line in s.splitlines():
        line = line.split("#", 1)[0].rstrip()
        if line.strip():
            lines.append(line)
    return "\n".join(lines)

# -----------------------------
# MAIN
# -----------------------------
def main():
    parser = argparse.ArgumentParser(description="Create merger scripts (flatten JSONs / hadd ROOTs / master_merge)")
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--bins-cfg", type=str, help="YAML file with bin definitions")
    group.add_argument("--bins-list", type=str, help="Plain text file with one bin name per line (e.g. condor/bins_list.txt)")
    parser.add_argument("--condor-dir", type=str, default="condor", help="Base condor directory where per-bin subdirs live")
    parser.add_argument("--json-dir", type=str, default="json", help="Directory to place flattened JSONs")
    parser.add_argument("--root-dir", type=str, default="root", help="Directory to place final hadded ROOT")
    parser.add_argument("--flatten-exe", type=str, default="./flattenJSONs.x", help="Executable to flatten per-bin JSONs")
    parser.add_argument("--do-json", action="store_true", help="Generate JSON merging (mergeJSONs + flatten script)")
    parser.add_argument("--do-hadd", action="store_true", help="Generate ROOT hadd scripts")
    parser.add_argument("--make-master", action="store_true", help="Also create master_merge script that calls everything")
    args = parser.parse_args()

    # Gather bin names
    bin_names = []
    if args.bins_list:
        p = Path(args.bins_list)
        if not p.exists():
            print(f"[createMergers] ERROR: bins-list file not found: {args.bins_list}", file=sys.stderr)
            sys.exit(1)
        with open(p) as f:
            for line in f:
                name = line.strip()
                if name:
                    bin_names.append(name)
    else:
        p = Path(args.bins_cfg)
        if not p.exists():
            print(f"[createMergers] ERROR: bins-cfg not found: {args.bins_cfg}", file=sys.stderr)
            sys.exit(1)
        with open(p) as f:
            bins = yaml.safe_load(f) or {}
            for k, v in bins.items():
                if isinstance(v, dict):
                    for key in ("cuts", "lep-cuts", "predefined-cuts", "user-cuts"):
                        v.setdefault(key, "")
                        v[key] = strip_inline_comments(v[key])
                bin_names.append(k)

    if not bin_names:
        print("[createMergers] No bins found to create mergers for.", file=sys.stderr)
        sys.exit(1)

    # Create per-user-requested scripts (and return script filenames)
    flatten_path = None
    hadd_path = None

    if args.do_json:
        flatten_path = write_flatten_script(bin_names, flatten_exe=args.flatten_exe, json_dir=args.json_dir, condor_dir=args.condor_dir)

    if args.do_hadd:
        hadd_path = write_master_hadd_script(bin_names, condor_dir=args.condor_dir, master_root_dir=args.root_dir)

    # If user wants master, create it and ensure we pass the correct script basenames
    if args.make_master:
        flatten_sh_name = os.path.basename(flatten_path) if flatten_path else None
        hadd_sh_name = os.path.basename(hadd_path) if hadd_path else None

        setup_master_merge_script(
            bin_names,
            flatten_sh=flatten_sh_name,
            json_dir=args.json_dir,
            hadd_sh=hadd_sh_name,
            root_dir=args.root_dir,
            do_json=args.do_json,
            do_hadd=args.do_hadd,
            condor_dir=args.condor_dir
        )

    print(f"[createMergers] Done. Generated scripts located under: {args.condor_dir}/")

if __name__ == "__main__":
    main()
