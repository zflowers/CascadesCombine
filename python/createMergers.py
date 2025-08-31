#!/usr/bin/env python3
import os, sys, argparse
from pathlib import Path

# ---------------------------------
# HELPERS
# ---------------------------------
def extract_bin_names(run_dir):
    condor_dir = os.path.join(run_dir, "condor")
    return [d for d in os.listdir(condor_dir) if os.path.isdir(os.path.join(condor_dir, d))]

def write_flatten_script(run_dir):
    script_name = f"run_flatten.sh"
    script_path = os.path.join(run_dir, "condor", script_name)
    output_file = os.path.join(run_dir, "flattened.json")
    bin_names = extract_bin_names(run_dir)
    with open(script_path, "w") as f:
        f.write("#!/usr/bin/env bash\n")
        f.write("# Auto-generated flatten script\n\n")
        input_paths = [os.path.join(run_dir, "condor", bin_name) for bin_name in bin_names]
        f.write(f"{run_dir}/exe/flattenJSONs.x {' '.join(input_paths)} {output_file}\n")
    os.chmod(script_path, 0o755)
    print(f"[createMergers] Generated flatten script: {script_path}")
    return script_path

def write_master_hadd_script(run_dir):
    script_name = f"run_hadd_all.sh"
    master_script_path = os.path.join(run_dir, "condor", script_name)
    final_root = os.path.join(run_dir, "final_hadded.root")
    bin_names = extract_bin_names(run_dir)
    with open(master_script_path, "w") as f:
        f.write("#!/usr/bin/env bash\n")
        f.write("# Auto-generated master hadd script\n\n")
        # Call per-bin hadd scripts first
        for bin_name in bin_names:
            hadd_script = os.path.join(run_dir, "condor", bin_name, "haddROOTs.sh")
            f.write(f"bash {hadd_script}\n")
        # Collect per-bin ROOT outputs
        per_bin_roots = [os.path.join(run_dir, "condor", bin_name, f"{bin_name}.root") for bin_name in bin_names]
        f.write("\nexisting_roots=()\n")
        f.write("for f in " + " ".join(per_bin_roots) + "; do\n")
        f.write("  if [ -f \"$f\" ]; then existing_roots+=(\"$f\"); fi\n")
        f.write("done\n\n")
        # Merge if at least one ROOT exists
        f.write("if [ ${#existing_roots[@]} -gt 0 ]; then\n")
        f.write(f"  if ! hadd -f -n 10 {final_root} ${{existing_roots[@]}} > /dev/null 2>&1; then\n")
        f.write(f"    echo 'Error: hadd failed for {final_root}'\n")
        f.write("    exit 1\n")
        f.write("  fi\n")
        f.write(f"  echo 'Final hadded ROOT -> {final_root}'\n")
        f.write("else\n")
        f.write("  echo 'No per-bin ROOT files found to hadd.'\n")
        f.write("fi\n")
    os.chmod(master_script_path, 0o755)
    print(f"[createMergers] Generated master hadd script: {master_script_path}")
    return master_script_path

def setup_master_merge_script(
    run_dir,
    flatten_sh=None,
    hadd_sh=None,
    do_json=False,
    do_hadd=False,
):
    """
    Create master_merge.sh to run all mergers (JSON flattening + ROOT hadd).
    flatten_sh and hadd_sh should be script filenames (not full paths) living in condor dir for the given run,
    or None if not present.
    """
    master_script_name = f"master_merge.sh"
    master_script_path = os.path.join(run_dir, "condor", master_script_name)
    bin_names = extract_bin_names(run_dir)

    with open(master_script_path, "w") as f:
        f.write("#!/usr/bin/env bash\n")
        f.write("# Auto-generated master merge script\n\n")

        # --- JSON merging ---
        if do_json:
            for bin_name in bin_names:
                merge_script = os.path.join(run_dir, "condor", bin_name, "mergeJSONs.sh")
                f.write(f"bash {merge_script}\n")
            if flatten_sh:
                f.write(f'bash {os.path.join(run_dir, "condor", flatten_sh)}\n')
            else:
                f.write("echo 'Warning: flatten script not provided; skipping flatten step.'\n")

        # --- ROOT hadd ---
        if do_hadd:
            # Call per-bin hadd scripts
            for bin_name in bin_names:
                hadd_script = os.path.join(run_dir, "condor", bin_name, "haddROOTs.sh")
                f.write(f"bash {hadd_script}\n")
            # Call master hadd script
            if hadd_sh:
                f.write(f'bash {os.path.join(run_dir, "condor", hadd_sh)}\n')
            else:
                f.write("echo 'Warning: master hadd script not provided; skipping hadd step.'\n")

    os.chmod(master_script_path, 0o755)
    print(f"[createMergers] Master merge script generated: {master_script_path}")
    return master_script_path

# -----------------------------
# MAIN
# -----------------------------
def main():
    parser = argparse.ArgumentParser(description="Create merger scripts (flatten JSONs / hadd ROOTs / master_merge)")
    parser.add_argument("--do-json", action="store_true", help="Generate JSON merging (mergeJSONs + flatten script)")
    parser.add_argument("--do-hadd", action="store_true", help="Generate ROOT hadd scripts")
    parser.add_argument("--make-master", action="store_true", help="Also create master_merge script that calls everything")
    parser.add_argument("--run-dir", type=str, default=None, help="directory containing run to create mergers for")
    args = parser.parse_args()
    if not args.run_dir:
        print("[createMergers] No run directory provided!")
        sys.exit(1)
    # Create per-user-requested scripts (and return script filenames)
    flatten_path = None
    hadd_path = None
    if args.do_json:
        flatten_path = write_flatten_script(args.run_dir)
    if args.do_hadd:
        hadd_path = write_master_hadd_script(args.run_dir)
    # If user wants master, create it and ensure we pass the correct script basenames
    if args.make_master:
        flatten_sh_name = os.path.basename(flatten_path) if flatten_path else None
        hadd_sh_name = os.path.basename(hadd_path) if hadd_path else None
        setup_master_merge_script(
            flatten_sh=flatten_sh_name,
            hadd_sh=hadd_sh_name,
            do_json=args.do_json,
            do_hadd=args.do_hadd,
            run_dir=args.run_dir
        )
    print(f"[createMergers] Done. Generated scripts located under: {args.run_dir}/condor/")

if __name__ == "__main__":
    main()
