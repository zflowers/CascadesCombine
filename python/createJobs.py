#!/usr/bin/env python3
import os, sys, subprocess, argparse, re, shutil
from pathlib import Path
import importlib.util

# ----------------------------------------
# Module imports
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
# Condor setup
# ----------------------------------------
CONDOR_DIR = Path("condor")
CONDOR_DIR.mkdir(exist_ok=True)

# ----------------------------------------
# Utilities
# ----------------------------------------
def sanitize(s):
    s = re.sub(r'[^A-Za-z0-9_.-]', '_', s)
    return s[:200]

def write_merge_script(bin_name, json_dir="json"):
    merge_script_path = os.path.join(f'{CONDOR_DIR}/{bin_name}', "mergeJSONs.sh")
    with open(merge_script_path, "w") as f:
        f.write("#!/usr/bin/env bash\n")
        f.write("# Auto-generated merge script\n")
        f.write(f"./mergeJSONs.x {CONDOR_DIR}/{bin_name}/{bin_name} {json_dir}\n")
    os.chmod(merge_script_path, 0o755)
    print(f"[createJobs] Generated merge script: {merge_script_path}")

def write_hadd_script(bin_name, condor_dir="condor", root_dir="root"):
    hadd_script_path = os.path.join(condor_dir, bin_name, "haddROOTs.sh")
    os.makedirs(os.path.dirname(hadd_script_path), exist_ok=True)
    with open(hadd_script_path, "w") as f:
        f.write("#!/usr/bin/env bash\n")
        f.write("# Auto-generated per-bin hadd script\n")
        f.write(f"hadd -f {condor_dir}/{bin_name}/{bin_name}.root "
                f"{condor_dir}/{bin_name}/{root_dir}/*.root\n")
    os.chmod(hadd_script_path, 0o755)
    print(f"[createJobs] Generated hadd script: {hadd_script_path}")

CONDOR_HEADER = """
universe                = vanilla
executable              = scripts/BFI.sh

should_transfer_files   = YES
when_to_transfer_output = ON_EXIT
transfer_input_files    = BFI_condor.x

request_cpus            = {cpus}
request_memory          = {memory}

# Release jobs automatically from hold for common conditions
periodic_release = (HoldReasonCode == 12 && HoldReasonSubCode == 256) || \
                   (HoldReasonCode == 13 && HoldReasonSubCode == 2)   || \
                   (HoldReasonCode == 12 && HoldReasonSubCode == 2)   || \
                   (HoldReasonCode == 26 && HoldReasonSubCode == 120) || \
                   (HoldReasonCode == 3  && HoldReasonSubCode == 0) \

use_x509userproxy       = True
getenv                  = True
"""

# ----------------------------------------
# Job building
# ----------------------------------------
def build_jobs(tool, bin_name, cuts, lep_cuts, predef_cuts, user_cuts, sms_filters, hist_yaml_file=None):
    """
    Build the job list for Condor submission.
    
    Arguments:
        tool           : pySampleTool.SampleTool instance
        bin_name       : str, name of the bin
        cuts           : str, event selection cuts
        lep_cuts       : str, lepton-specific cuts
        predef_cuts    : str, predefined cuts
        user_cuts      : str, user cuts
        sms_filters    : list of SMS filters to apply
        hist_yaml_file : optional str, path to histogram YAML to pass to each job
    """
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
def write_submit_file(bin_name, jobs, cpus="1", memory="1 GB", lumi=1, make_json=True, make_root=True, dryrun=False):
    bin_safe = sanitize(bin_name)
    bin_dir = CONDOR_DIR / bin_safe
    if bin_dir.exists():
        print("[createJobs] Removing existing condor dir:", bin_dir.name)
        shutil.rmtree(bin_dir)
    bin_dir.mkdir(parents=True, exist_ok=True)

    # Subdirectories
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
    all_inputs = set(["BFI_condor.x"])
    all_remaps = []

    # helper: flatten multi-line YAML literal blocks into a single-line string
    def _flatten_field(value):
        if value is None:
            return ""
        s = str(value)
        parts = []
        for line in s.splitlines():
            # remove comments starting with '#'
            line = line.split("#", 1)[0].strip()
            if line:  # skip empty lines
                parts.append(line)
        joined = " ".join(parts)
        # escape any literal double-quotes inside the value so they don't break the submit file
        joined = joined.replace('"', '\\"')
        return joined

    # ----
    # Add per-job outputs (use $(LogFile) placeholders) once, globally,
    # so condor knows each job will produce these outputs.
    per_job_outputs = []
    if make_json:
        per_job_outputs.append("$(LogFile).json")
    if make_root:
        per_job_outputs.append("$(LogFile).root")

    if per_job_outputs:
        submit_lines.append("transfer_output_files = " + ", ".join(per_job_outputs))

        # Remap these per-job outputs into the desired subdirectories
        remap_entries = []
        if make_json:
            remap_entries.append(f"$(LogFile).json = {json_dir.as_posix()}/$(LogFile).json")
        if make_root:
            remap_entries.append(f"$(LogFile).root = {root_dir.as_posix()}/$(LogFile).root")

        # Single transfer_output_remaps line (avoid f-string brace pitfalls)
        submit_lines.append('transfer_output_remaps = "' + "; ".join(remap_entries) + '"')

    # ----
    # Build each job's args and per-job remap entries (kept for compatibility)
    for job in jobs:
        ds = job["process"]
        fpath = job["filepath"]
        fname_stem = job["fname_stem"]
        sig_type = job.get("sig_type", None)
        sms_filters = job.get("sms_filters", [])

        base = sanitize(f"{bin_name}_{ds}_{fname_stem}" + (f"_{sms_filters[0]}" if sms_filters else ""))

        outputs = []

        # JSON (per-job local json filename and remap entry kept)
        if make_json:
            local_json = f"{base}.json"
            outputs.append("--json")
            outputs.append(f"--json-output {local_json}")
            job["remap_outputs"] = job.get("remap_outputs", [])
            job["remap_outputs"].append(f"{local_json} = json/{local_json}")

        # ROOT / histograms (per-job local root filename and remap entry kept)
        if make_root:
            local_root = f"{base}.root"
            outputs.append("--hist")
            outputs.append(f"--root-output {local_root}")
            job["remap_outputs"] = job.get("remap_outputs", [])
            job["remap_outputs"].append(f"{local_root} = root/{local_root}")
            # Include YAML file if defined (transfer input)
            hist_yaml_file = job.get("hist_yaml", "")
            if hist_yaml_file:
                outputs.append(f"--hist-yaml {os.path.basename(hist_yaml_file)}")
                job.setdefault("transfer_input_files", []).append(hist_yaml_file)
                all_inputs.add(hist_yaml_file)

        # Ensure BFI_condor.x is in transfer_input_files
        job.setdefault("transfer_input_files", []).append("BFI_condor.x")

        # Collect remaps for bookkeeping (not used to produce transfer_output_files)
        all_remaps.extend(job.get("remap_outputs", []))

        # Flatten possible multi-line fields and build args_list
        cuts_flat = _flatten_field(job.get("cuts", ""))
        lep_cuts_flat = _flatten_field(job.get("lep_cuts", ""))
        lep_cuts_flat = lep_cuts_flat.replace(" ","")
        predef_flat = _flatten_field(job.get("predef_cuts", ""))
        user_flat = _flatten_field(job.get("user_cuts", ""))

        args_list = [
            f"--lumi {lumi}",
            f"--bin {bin_name}",
            f"--file {fpath}",
            *outputs,
        ]
        # Add the (now single-line) fields only if they're non-empty
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
            args_list.append("--sms-filters")
            args_list.append(sms_filters[0])

        # Join into single-line args_str
        args_str = " ".join(a for a in args_list if a and not a.isspace())
        job["args_str"] = args_str
        job["base"] = base

    # Write transfer_input_files (global)
    submit_lines.append("transfer_input_files = " + ", ".join(sorted(all_inputs)))

    # Queue jobs
    submit_lines.append("# Queue jobs with LogFile (used for log/out/err) and Args")
    submit_lines.append("queue LogFile, Args from (")

    for job in jobs:
        # job["args_str"] is guaranteed to be a single line now
        submit_lines.append(f'{job["base"]} "{job["args_str"]}"')

    submit_lines.append(")")

    # Write file
    submit_content = "\n".join(submit_lines) + "\n"
    submit_path.write_text(submit_content)
    print(f"[createJobs] Wrote submit file: {submit_path}")

    # Submit if not dryrun
    if not dryrun:
        proc = subprocess.run(
            f"source /cvmfs/cms.cern.ch/cmsset_default.sh && condor_submit {submit_path}",
            shell=True,
            executable="/bin/bash",
            capture_output=True,
            text=True
        )
        if proc.returncode != 0:
            print("condor_submit failed:", proc.stderr)
        else:
            print(f"Submitted bin {bin_name} ({len(jobs)} jobs)")
            if make_json:
                write_merge_script(bin_name,json_dir)
            if make_root:
                write_hadd_script(bin_name)

# ----------------------------------------
# Main
# ----------------------------------------
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--bkg_processes", nargs="+", default=[],
                        help="List of background process names")
    parser.add_argument("--sig_processes", nargs="+", default=[],
                        help="List of signal process names")
    parser.add_argument("--sms-filters", nargs="*", default=[],
                        help="Optional list of SMS trees to filter; empty means no filtering")
    parser.add_argument("--bin", default="TEST")
    parser.add_argument("--cuts", default="Nlep>=2;MET>=150")
    parser.add_argument("--lep-cuts", default=">=1OSSF")
    parser.add_argument("--predefined-cuts", default="Cleaning")
    parser.add_argument("--user-cuts", default="")
    parser.add_argument("--cpus", default="1")
    parser.add_argument("--memory", default="1 GB")
    parser.add_argument("--lumi", type=float, default=1.)
    parser.add_argument("--make-json", action="store_true",
                        help="Enable JSON output")
    parser.add_argument("--make-root", action="store_true",
                        help="Enable ROOT/histogram output")
    parser.add_argument("--hist-yaml", default="",
                        help="Path to histogram YAML config (used if --make-root)")
    parser.add_argument("--dryrun", "--dry-run", action="store_true")
    args = parser.parse_args()

    tool = pySampleTool.SampleTool()

    # Load processes
    tool.LoadBkgs(args.bkg_processes)
    sms_filters = []
    if args.sms_filters:
        sms_filters = args.sms_filters
        pySampleTool.BFTool.SetFilterSignalsSMS(sms_filters)
        tool.LoadSigs(args.sig_processes)
    else:
        tool.LoadSigs(args.sig_processes)
        sms_filters = pySampleTool.BFTool.GetFilterSignalsSMS()

    jobs = build_jobs(
        tool,
        args.bin,
        args.cuts,
        args.lep_cuts,
        args.predefined_cuts,
        args.user_cuts,
        sms_filters,
        hist_yaml_file=args.hist_yaml
    )

    # Inject YAML path into each job if provided
    if args.hist_yaml:
        for job in jobs:
            job["hist_yaml"] = args.hist_yaml

    write_submit_file(
        args.bin,
        jobs,
        cpus=args.cpus,
        memory=args.memory,
        lumi=args.lumi,
        make_json=args.make_json,
        make_root=args.make_root,
        dryrun=args.dryrun
    )

if __name__ == "__main__":
    main()
