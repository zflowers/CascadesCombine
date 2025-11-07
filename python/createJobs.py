#!/usr/bin/env python3
import os, sys, subprocess, argparse, re, shutil, time, random, hashlib
from pathlib import Path
import importlib.util
from collections import defaultdict
from typing import Optional, Union
from CondorJobCountMonitor import CondorJobCountMonitor

# ----------------------------------------
# Module imports (pybind)
# ----------------------------------------
def load_pybind_module(module_name, folder):
    folder = Path(folder)
    so_files = list(folder.glob(f"{module_name}*.so"))
    if not so_files:
        raise ImportError(f"Cannot find {module_name} .so module in {folder}/")
    spec = importlib.util.spec_from_file_location(module_name, so_files[0])
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod

libs_dir = Path(__file__).parent.parent / "libs"
pySampleTool = load_pybind_module("pySampleTool", libs_dir)

# ----------------------------------------
# Utilities
# ----------------------------------------
def sanitize(s):
    s = re.sub(r'[^A-Za-z0-9_.-]', '_', s)
    return s[:200]

def sanitize_for_base(s: str, maxlen: int = 240) -> str:
    s = re.sub(r'[^A-Za-z0-9_.-]', '_', s)
    return s[:maxlen]

def make_condor_dir_name(bin_name: str, bins_cfg_path: Optional[Union[str, Path]] = None) -> str:
    """
    Produce a short stable condor directory name for the given bin group.
    Keeps final length bounded while remaining human-readable and unique.
    Preference order:
      - If bins_cfg_path exists and follows pattern bin_group_XXX__safefirst, use XXX and safefirst.
      - Otherwise include a short safe prefix and append an 8-hex sha1 digest of bin_name.
    """
    # safe-stem extraction
    stem = None
    try:
        if bins_cfg_path:
            p = Path(bins_cfg_path)
            stem = p.stem
    except Exception:
        stem = None

    if stem:
        stem_safe = re.sub(r'[^A-Za-z0-9_.-]', '_', stem)
        # if submitJobs used "bin_group_XXX__safeFirst" pattern, try to preserve that XXX
        m = re.match(r'bin_group_(\d{3})__(.+)', stem_safe)
        if m:
            idx = m.group(1)
            sf = m.group(2)[:50]
            digest = hashlib.sha1(bin_name.encode()).hexdigest()[:8]
            return f"group_{idx}__{sf}__{digest}"
        # otherwise use the stem + digest
        digest = hashlib.sha1(bin_name.encode()).hexdigest()[:8]
        return f"group__{stem_safe[:50]}__{digest}"

    # fallback: use a sanitized prefix from bin_name + digest
    digest = hashlib.sha1(bin_name.encode()).hexdigest()[:8]
    safe = sanitize(bin_name)[:120]
    return f"{safe}__{digest}"

def _flatten_field(value):
    if value is None:
        return ""
    s = str(value)
    parts = []
    for line in s.splitlines():
        line = line.split("#", 1)[0].strip()
        if line:
            parts.append(line)
    joined = " ".join(parts)
    joined = joined.replace('"', '\\"')
    return joined

def get_auto_THRESHOLD():
    result = subprocess.run(
        ["condor_config_val", "-dump"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        check=True
    )   
    for line in result.stdout.splitlines():
        if line.startswith("MAX_JOBS_PER_OWNER"):
            _, value = line.split("=")
            return int(int(value.strip()) * 0.95)
            break
    return 10000 # default fallback

# ----------------------------------------
# Write helper scripts (merge/hadd)
# ----------------------------------------
def write_merge_script(bin_name, condor_bin_dir: Path, json_dirname="json"):
    """Make a merge script that expects json files to be in condor_bin_dir/json

    The script calls the run-local exe `exe/mergeJSONs.x` with two quoted args:
      1) merged output JSON path (condor_bin_dir/<bin_safe>.json)
      2) input json directory (condor_bin_dir/json)
    """
    merge_script_path = condor_bin_dir / "mergeJSONs.sh"

    # Use the sanitized directory name as the canonical bin name (this is what
    # write_submit_file uses for the condor bin directory).
    bin_safe = condor_bin_dir.name

    # Determine paths (resolve exe relative to the condor bin dir so the
    # script works even if executed from elsewhere)
    exe_candidate = (condor_bin_dir / ".." / ".." / "exe" / "mergeJSONs.x").resolve()
    merged_out = (condor_bin_dir / f"{bin_safe}.json").resolve()
    json_dir = (condor_bin_dir / json_dirname).resolve()

    with open(merge_script_path, "w") as f:
        f.write("#!/usr/bin/env bash\n")
        f.write("set -euo pipefail\n")
        f.write("# Auto-generated merge script (safe quoting for grouped bins)\n")
        f.write(f'EXEC="{exe_candidate.as_posix()}"\n')
        f.write(f'OUT="{merged_out.as_posix()}"\n')
        f.write(f'INDIR="{json_dir.as_posix()}"\n')
        f.write('mkdir -p "$(dirname "$OUT")"\n')
        f.write('echo "[mergeJSONs] Running: $EXEC $OUT $INDIR"\n')
        f.write('"$EXEC" "$OUT" "$INDIR"\n')

    os.chmod(merge_script_path, 0o755)

def write_hadd_script(bin_name, condor_bin_dir: Path, root_dirname="root"):
    hadd_script_path = condor_bin_dir / "haddROOTs.sh"
    merged_root = condor_bin_dir / f"{condor_bin_dir.name}.root"
    with open(hadd_script_path, "w") as f:
        f.write("#!/usr/bin/env bash\n")
        f.write("# Auto-generated per-bin hadd script\n")
        f.write(f'hadd -f "{merged_root.as_posix()}" "{(condor_bin_dir / root_dirname / "*.root").as_posix()}" > /dev/null 2>&1 || ')
        f.write(f'hadd -f "{merged_root.as_posix()}" "{(condor_bin_dir / root_dirname / "*.root").as_posix()}"\n')
    os.chmod(hadd_script_path, 0o755)

# ----------------------------------------
# Condor submit template
# ----------------------------------------
CONDOR_HEADER = """
universe                = vanilla
executable              = scripts/BFI.sh

should_transfer_files   = YES
when_to_transfer_output = ON_EXIT

request_cpus            = {cpus}
request_memory          = {memory}

# Release jobs automatically from hold for common conditions
periodic_release = (HoldReasonCode == 12 && HoldReasonSubCode == 256) || \
                   (HoldReasonCode == 13 && HoldReasonSubCode == 2)   || \
                   (HoldReasonCode == 12 && HoldReasonSubCode == 2)   || \
                   (HoldReasonCode == 26 && HoldReasonSubCode == 120) || \
                   (HoldReasonCode == 3  && HoldReasonSubCode == 0) \\

use_x509userproxy       = True
getenv                  = True
"""

# ----------------------------------------
# Job building
# ----------------------------------------
def build_jobs(tool, bin_name, cuts, lep_cuts, predef_cuts, user_cuts, sms_filters, hist_yaml_file=None):
    jobs = []

    def make_base_job(ds, fpath):
        return {
            "process": ds,
            "filepath": fpath,
            "fname_stem": Path(fpath).stem,
            "cuts": cuts,
            "lep_cuts": lep_cuts,
            "predef_cuts": predef_cuts,
            "user_cuts": user_cuts,
            "hist_yaml": hist_yaml_file,
        }

    # Background jobs
    for ds, files in tool.BkgDict.items():
        for fpath in files:
            jobs.append(make_base_job(ds, fpath))

    # Signal jobs
    for ds, files in tool.SigDict.items():
        for fpath in files:
            base = make_base_job(ds, fpath)

            sig_type = None
            if "SMS" in fpath:
                sig_type = "sms"
            elif "Cascades" in fpath:
                sig_type = "cascades"

            # one job per SMS filter if applicable
            if sig_type == "sms" and sms_filters:
                for filt in sms_filters:
                    job = {
                        **base,
                        "sig_type": sig_type,
                        "sms_filters": [filt],
                    }
                    jobs.append(job)
            else:
                job = {
                    **base,
                    "sig_type": sig_type,
                }
                jobs.append(job)

    return jobs

# ----------------------------------------
# Condor submit file writing
# ----------------------------------------
def write_submit_file(
    bin_name,
    jobs,
    condor_base_dir: Path,
    cpus="1",
    memory="1 GB",
    lumi=1,
    make_json=True,
    make_root=True,
    dryrun=False,
    bins_cfg: str = ""
):
    """
    condor_base_dir is the directory that will hold per-bin subdirs (e.g. runs/.../condor)
    """
    # Prefer a short stable group-based condor dir when possible (avoids long concatenated names).
    bin_safe = make_condor_dir_name(bin_name, bins_cfg if bins_cfg else None)
    bin_dir = condor_base_dir / bin_safe
    if bin_dir.exists():
        shutil.rmtree(bin_dir)
    bin_dir.mkdir(parents=True, exist_ok=True)

    # Determine run root and run-local exe candidate
    run_root = condor_base_dir.parent
    exe_dir_candidate = run_root / "exe"

    # Prefer run-local BFI_condor.x if available, else use repo-root path if present.
    bfi_candidates = [exe_dir_candidate / "BFI_condor.x", Path("BFI_condor.x")]
    bfi_path = None
    for c in bfi_candidates:
        if c.exists():
            bfi_path = c.resolve()
            break

    if not bfi_path:
        print("[createJobs] WARNING: Could not find BFI_condor.x in run exe dir or repo root; submission may fail.", file=sys.stderr)

    # Subdirectories inside bin_dir
    log_dir = bin_dir / "log"
    out_dir = bin_dir / "out"
    err_dir = bin_dir / "err"
    json_dir = bin_dir / "json"
    root_dir = bin_dir / "root"
    for d in (log_dir, out_dir, err_dir, json_dir, root_dir):
        d.mkdir(parents=True, exist_ok=True)

    submit_path = bin_dir / f"{bin_safe}.sub"

    # Header
    header = CONDOR_HEADER.format(cpus=cpus, memory=memory)
    submit_lines = [header]

    # Logs
    submit_lines.append(f"log    = {log_dir}/$(LogFile).log")
    submit_lines.append(f"output = {out_dir}/$(LogFile).out")
    submit_lines.append(f"error  = {err_dir}/$(LogFile).err")

    # Build per-job inputs collection (global)
    all_inputs = set()
    if bfi_path:
        all_inputs.add(str(bfi_path))

    # Per-job outputs and transfer remaps
    per_job_outputs = []
    if make_json:
        per_job_outputs.append("$(LogFile).json")
    if make_root:
        per_job_outputs.append("$(LogFile).root")

    if per_job_outputs:
        submit_lines.append("transfer_output_files = " + ", ".join(per_job_outputs))

        remap_entries = []
        if make_json:
            remap_entries.append(f"$(LogFile).json = {json_dir.as_posix()}/$(LogFile).json")
        if make_root:
            remap_entries.append(f"$(LogFile).root = {root_dir.as_posix()}/$(LogFile).root")

        submit_lines.append('transfer_output_remaps = "' + "; ".join(remap_entries) + '"')

    # Build each job's args and per-job remap entries
    for job in jobs:
        ds = job["process"]
        fpath = job["filepath"]
        fname_stem = job["fname_stem"]
        sig_type = job.get("sig_type", None)
        sms_filters = job.get("sms_filters", [])

        #base = sanitize(f"{bin_name}_{ds}_{fname_stem}" + (f"_{sms_filters[0]}" if sms_filters else ""))
        sub_stem = Path(submit_path).stem
        base_raw = f"{sub_stem}_{ds}_{fname_stem}"
        if sms_filters:
            base_raw += f"_{sms_filters[0]}"
        # sanitize but keep informative length
        base = sanitize_for_base(base_raw, maxlen=240)
        job["base"] = base

        # Collect outputs/remaps for this job
        job["remap_outputs"] = job.get("remap_outputs", [])

        # JSON output
        if make_json:
            local_json = f"{base}.json"
            job["remap_outputs"].append(f"{local_json} = json/{local_json}")

        # ROOT / histograms
        if make_root:
            local_root = f"{base}.root"
            job["remap_outputs"].append(f"{local_root} = root/{local_root}")
            # If a hist_yaml was provided, prefer the staged copy in run_root/configs/
            hist_yaml = job.get("hist_yaml", "")
            if hist_yaml:
                candidate = (run_root / "configs" / Path(hist_yaml).name)
                if candidate.exists():
                    hist_yaml_path = candidate.resolve()
                else:
                    hist_yaml_path = Path(hist_yaml).resolve()
                job.setdefault("transfer_input_files", []).append(str(hist_yaml_path))
                all_inputs.add(str(hist_yaml_path))

        # Ensure run-local BFI_condor.x (if found) is transferred to the job's working dir
        if bfi_path:
            job.setdefault("transfer_input_files", []).append(str(bfi_path))
            all_inputs.add(str(bfi_path))

        # If bins YAML provided, stage it for transfer into each job CWD
        bins_cfg_map = {}
        if bins_cfg:
            bcfg_path = Path(bins_cfg)
            if bcfg_path.exists():
                import yaml
                bins_cfg_map = yaml.safe_load(bcfg_path.read_text()) or {}
                # Ensure the exact file is transferred to job CWD (use basename)
                all_inputs.add(str(bcfg_path.resolve()))
            else:
                print(f"[createJobs] WARNING: requested --bins-cfg not found: {bins_cfg}", file=sys.stderr)

        # Flatten fields
        cuts_flat = _flatten_field(job.get("cuts", ""))
        lep_cuts_flat = _flatten_field(job.get("lep_cuts", "")).replace(" ", "")
        predef_flat = _flatten_field(job.get("predef_cuts", ""))
        user_flat = _flatten_field(job.get("user_cuts", ""))

        args_list = [
            f"--lumi {lumi}",
            f"--bin {bin_name}",
            f"--file {fpath}",
        ]

        # If a bins-cfg was provided and bin_name is a group (semicolon-separated),
        # expand per-bin cut fields from the YAML and append them in the same order
        if bins_cfg_map and ";" in bin_name:
            # parse bins in the same order as provided
            bins_list = [b.strip() for b in bin_name.split(";") if b.strip()]
            for b in bins_list:
                bcfg = bins_cfg_map.get(b, {}) or {}
                # Append each per-bin field as its own repeated CLI flag (order preserved)
                if bcfg.get("cuts"):
                    args_list.append(f"--cuts {_flatten_field(bcfg.get('cuts'))}")
                if bcfg.get("lep-cuts"):
                    args_list.append(f"--lep-cuts {_flatten_field(bcfg.get('lep-cuts')).replace(' ', '')}")
                if bcfg.get("predefined-cuts"):
                    args_list.append(f"--predefined-cuts {_flatten_field(bcfg.get('predefined-cuts'))}")
                if bcfg.get("user-cuts"):
                    args_list.append(f"--user-cuts {_flatten_field(bcfg.get('user-cuts'))}")
            # Also pass the basename of the bins-cfg file so BFI_condor can read it locally if desired
            args_list.append(f"--bins-cfg {Path(bins_cfg).name}")
        else:
            # Original behavior: pass single/flattened cuts passed in job config (works for single-bin createJobs calls)
            if cuts_flat:
                args_list.append(f"--cuts {cuts_flat}")
            if lep_cuts_flat:
                args_list.append(f"--lep-cuts {lep_cuts_flat}")
            if predef_flat:
                args_list.append(f"--predefined-cuts {predef_flat}")
            if user_flat:
                args_list.append(f"--user-cuts {user_flat}")
            # if bins_cfg provided but bin_name is single, still pass the file in
            if bins_cfg:
                args_list.append(f"--bins-cfg {Path(bins_cfg).name}")

        # Make-json / make-root options
        if make_json:
            args_list.extend(["--json", f"--json-output {base}.json"])
        if make_root:
            args_list.extend(["--hist", f"--root-output {base}.root"])
            if job.get("hist_yaml"):
                # pass only the basename; condor will transfer the YAML into the job CWD
                args_list.append(f"--hist-yaml {Path(job['hist_yaml']).name}")

        # Add single-line cut fields
        if cuts_flat:
            args_list.append(f"--cuts {cuts_flat}")
        if lep_cuts_flat:
            args_list.append(f"--lep-cuts {lep_cuts_flat}")
        if predef_flat:
            args_list.append(f"--predefined-cuts {predef_flat}")
        if user_flat:
            args_list.append(f"--user-cuts {user_flat}")

        if sig_type:
            args_list.append(f"--sig-type {sig_type}")
        if sms_filters:
            args_list.extend(["--sms-filters", sms_filters[0]])

        args_str = " ".join(a for a in args_list if a and not a.isspace())
        job["args_str"] = args_str
        job["base"] = base

    # Write transfer_input_files (global)
    if all_inputs:
        submit_lines.append("transfer_input_files = " + ", ".join(sorted(all_inputs)))

    # Queue jobs with LogFile and Args
    submit_lines.append("# Queue jobs with LogFile (used for log/out/err) and Args")
    submit_lines.append("queue LogFile, Args from (")

    for job in jobs:
        submit_lines.append(f'{job["base"]} "{job["args_str"]}"')

    submit_lines.append(")")

    # Write submit file
    submit_content = "\n".join(submit_lines) + "\n"
    submit_path.write_text(submit_content)

    # --- config ---
    known_schedds = [
        "lpcschedd4.fnal.gov",
        "lpcschedd5.fnal.gov",
        "lpcschedd6.fnal.gov",
    ]
    max_retries = 8      # overall attempts (including forced schedd attempts)
    per_schedd_limit = 3 # don't try the same schedd more than this many times
    # ------------------
    
    if not dryrun:
        attempt = 0
        success = False
        last_proc = None
    
        # track how many times tried each schedd with -name
        schedd_tries = defaultdict(int)
    
        # start without forcing a schedd; let condor pick first
        force_schedd = None
    
        transient_signatures = [
            "Can't find address of local schedd",
            "Querying the CMS LPC pool",
            "Attempting to submit jobs to",
            "Unable to connect to",
            "Failed to connect",
        ]
    
        # Hold condor submissions if over max threshold
        condor_monitor = CondorJobCountMonitor(threshold=get_auto_THRESHOLD()*.95,verbose=False)
        condor_monitor.wait_until_jobs_below()

        while attempt < max_retries and not success:
            attempt += 1
    
            # build command: allow an initial attempt without -name, then use -name to force
            if force_schedd:
                cmd = f"source /cvmfs/cms.cern.ch/cmsset_default.sh && condor_submit -name {force_schedd} {submit_path.as_posix()}"
                printed_name = force_schedd
            else:
                cmd = f"source /cvmfs/cms.cern.ch/cmsset_default.sh && condor_submit {submit_path.as_posix()}"
                printed_name = "(auto)"
    
            proc = subprocess.run(
                cmd,
                shell=True,
                executable="/bin/bash",
                capture_output=True,
                text=True
            )
            last_proc = proc
    
            stdout = (proc.stdout or "").strip()
            stderr = (proc.stderr or "").strip()
    
            if proc.returncode == 0:
                success = True
                break
    
            # If no returncode 0, inspect output to decide whether to retry
            is_transient = any(sig in stdout or sig in stderr for sig in transient_signatures)
    
            # Try to detect the schedd name that condor tried to use (if present in stdout)
            match_schedd = re.search(r"Attempting to submit jobs to (\S+)", stdout)
            reported_schedd = match_schedd.group(1) if match_schedd else None
    
            # Print the captured outputs for diagnosis
            print("  condor_submit failed (returncode={}):".format(proc.returncode))
            if stdout:
                print("  STDOUT:", stdout.replace("\n", " | "))
            if stderr:
                print("  STDERR:", stderr.replace("\n", " | "))
    
            if not is_transient:
                # Non-transient error: bail out (likely a real submitfile problem)
                print("[createJobs] Non-transient condor_submit failure; not retrying.")
                break
    
            # If transient, decide which schedd to try next.
            # If told which schedd failed, prefer other schedds first.
            candidate_schedds = [s for s in known_schedds if schedd_tries[s] < per_schedd_limit]
    
            if reported_schedd and reported_schedd in candidate_schedds:
                # exclude the reported failing schedd for the immediate next try
                candidate_schedds = [s for s in candidate_schedds if s != reported_schedd]
    
            if not candidate_schedds:
                # all schedds exhausted per the per_schedd_limit -> stop retrying
                print("[createJobs] All schedds have reached the per-schedd attempt limit. Stopping retries.")
                break
    
            # pick the schedd tried the least (ties broken randomly)
            min_tries = min(schedd_tries[s] for s in candidate_schedds)
            least_tried = [s for s in candidate_schedds if schedd_tries[s] == min_tries]
            next_schedd = random.choice(least_tried)
    
            schedd_tries[next_schedd] += 1
            force_schedd = next_schedd
    
            wait_time = min(2 * attempt, 10)
            print(f"[warn] Transient schedd error; will retry using -name {next_schedd} (attempt count for this schedd: {schedd_tries[next_schedd]}). Sleeping {wait_time}s.")
            time.sleep(wait_time)
    
        if not success:
            if last_proc is not None:
                print(f"condor_submit failed after {attempt} attempt(s). Last returncode: {last_proc.returncode}")
                print("Last STDOUT:", last_proc.stdout)
                print("Last STDERR:", last_proc.stderr)
            else:
                print("condor_submit failed: no subprocess result available.")
        else:
            # parse cluster id and schedd for bookkeeping
            final_stdout = (proc.stdout or "").strip()
            cluster_id = None
            schedd = None
    
            match_cluster = re.search(r"submitted to cluster (\d+)", final_stdout)
            if match_cluster:
                cluster_id = match_cluster.group(1)
    
            # prefer explicitly reported schedd in stdout; if not present, use the forced schedd if any
            match_schedd = re.search(r"Attempting to submit jobs to (\S+)", final_stdout)
            if match_schedd:
                schedd = match_schedd.group(1)
            elif force_schedd:
                schedd = force_schedd
    
            if cluster_id:
                record_path = bin_dir / "submitted_clusters.txt"
                with open(record_path, "a") as f:
                    if schedd:
                        f.write(f"{cluster_id} {schedd}\n")
                    else:
                        f.write(f"{cluster_id}\n")
    
            print(f"[createJobs] Submitted bin {bin_name} ({len(jobs)} jobs) via schedd {schedd or '(unknown)'}")
            if make_json:
                write_merge_script(bin_name, bin_dir, json_dirname="json")
            if make_root:
                write_hadd_script(bin_name, bin_dir, root_dirname="root")

# ----------------------------------------
# Main
# ----------------------------------------
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--bkg_processes", nargs="+", default=[], help="List of background process names")
    parser.add_argument("--sig_processes", nargs="+", default=[], help="List of signal process names")
    parser.add_argument("--sms-filters", nargs="*", default=[], help="Optional list of SMS trees to filter")
    parser.add_argument("--bin", default="TEST")
    parser.add_argument("--bins-cfg", default="", help="Path to bins YAML used to fetch per-bin cuts when grouping bins")
    parser.add_argument("--cuts", default="Nlep>=2;MET>=150")
    parser.add_argument("--lep-cuts", default=">=1OSSF")
    parser.add_argument("--predefined-cuts", default="Cleaning")
    parser.add_argument("--user-cuts", default="")
    parser.add_argument("--cpus", default="1")
    parser.add_argument("--memory", default="1 GB")
    parser.add_argument("--lumi", type=float, default=1.)
    parser.add_argument("--make-json", action="store_true", help="Enable JSON output")
    parser.add_argument("--make-root", action="store_true", help="Enable ROOT/histogram output")
    parser.add_argument("--hist-yaml", default="", help="Path to histogram YAML config (used if --make-root)")
    parser.add_argument("--dryrun", "--dry-run", action="store_true")
    parser.add_argument("--run-dir", type=str, default="condor", help="Directory to hold condor outputs (per-run condor dir)")
    args = parser.parse_args()

    # Set condor base dir from argument (make absolute to be unambiguous in logs)
    condor_base = Path(args.run_dir).resolve()
    condor_base.mkdir(parents=True, exist_ok=True)

    # Determine run-local exe dir (prefer run_root/exe)
    run_root = condor_base.parent
    exe_dir = run_root / "exe"
    if not exe_dir.exists():
        exe_dir = Path(".")
        print(f"[createJobs] Run-local exe dir not found; falling back to repo root: {exe_dir}")

    # Parse job list via pySampleTool
    tool = pySampleTool.SampleTool()
    tool.LoadBkgs(args.bkg_processes)
    if args.sms_filters:
        sms_filters = args.sms_filters
        pySampleTool.BFTool.SetFilterSignalsSMS(sms_filters)
        tool.LoadSigs(args.sig_processes)
    else:
        tool.LoadSigs(args.sig_processes)
        sms_filters = pySampleTool.BFTool.GetFilterSignalsSMS()

    # Build jobs
    jobs = build_jobs(
        tool,
        args.bin,
        args.cuts,
        args.lep_cuts,
        args.predefined_cuts,
        args.user_cuts,
        sms_filters,
        hist_yaml_file=args.hist_yaml if args.hist_yaml else None
    )

    # If a hist YAML was provided, store it in each job (reference staged configs if available)
    if args.hist_yaml:
        for job in jobs:
            job["hist_yaml"] = args.hist_yaml

    write_submit_file(
        args.bin,
        jobs,
        condor_base_dir=condor_base,
        cpus=args.cpus,
        memory=args.memory,
        lumi=args.lumi,
        make_json=args.make_json,
        make_root=args.make_root,
        dryrun=args.dryrun,
        bins_cfg=args.bins_cfg
    )

if __name__ == "__main__":
    main()
