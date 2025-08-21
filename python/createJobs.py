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
    """
    Create a mergeJSONs.sh script in condor/{bin_name} that merges all JSONs in json_dir.
    Args:
        bin_name (str): The bin name (used to create condor/{bin_name}/mergeJSONs.sh)
        json_dir (str): Directory containing JSONs (default 'json')
    """

    merge_script_path = os.path.join(f'{CONDOR_DIR}/{bin_name}', "mergeJSONs.sh")
    with open(merge_script_path, "w") as f:
        f.write("#!/usr/bin/env bash\n")
        f.write("# Auto-generated merge script\n")
        f.write(f"./mergeJSONs.x {CONDOR_DIR}/{bin_name}/{bin_name} {json_dir}\n")

    # Make the script executable
    os.chmod(merge_script_path, 0o755)
    print(f"[createJobs] Generated merge script: {merge_script_path}")

CONDOR_HEADER = """
universe                = vanilla
executable              = scripts/BFI.sh

should_transfer_files   = YES
when_to_transfer_output = ON_EXIT
transfer_input_files    = BFI_condor.x

request_cpus            = {cpus}
request_memory          = {memory}

periodic_hold = (CpusUsage > RequestCpus || MemoryUsage > RequestMemory) && (JobStatus == 2) && (CurrentTime - EnteredCurrentStatus > 60)

+JobTransforms = "if HoldReasonSubCode == 42 set RequestCpus = MIN(RequestCpus + 1, 32); if HoldReasonCode == 34 set RequestMemory = MIN(RequestMemory + 1, 24)"

periodic_release = (HoldReasonCode == 12 && HoldReasonSubCode == 256) || \
                   (HoldReasonCode == 13 && HoldReasonSubCode == 2)   || \
                   (HoldReasonCode == 12 && HoldReasonSubCode == 2)   || \
                   (HoldReasonCode == 26 && HoldReasonSubCode == 120) || \
                   (HoldReasonCode == 3  && HoldReasonSubCode == 0)   || \
                   (HoldReasonSubCode == 42)

use_x509userproxy       = True
getenv                  = True
"""

# ----------------------------------------
# Job building
# ----------------------------------------
def build_jobs(tool, bin_name, cuts, lep_cuts, predef_cuts, sms_filters):
    jobs = []

    def make_base_job(ds, fpath):
        return {
            "dataset": ds,
            "filepath": fpath,
            "fname_stem": Path(fpath).stem,
            "cuts": cuts,
            "lep_cuts": lep_cuts,
            "predef_cuts": predef_cuts
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
    
            # one job per filter
            if sig_type == "sms" and sms_filters:
                for filt in sms_filters:
                    job = {
                        **base,
                        "sig_type": sig_type,
                        "sms_filters": [filt],
                    }
                    jobs.append(job)
            else:
                # cascades or sms with no filters
                job = {
                    **base,
                    "sig_type": sig_type,
                }
                jobs.append(job)
    
    return jobs

# ----------------------------------------
# Condor submit file writing
# ----------------------------------------
def write_submit_file(bin_name, jobs, cpus="1", memory="1 GB", lumi=1, dryrun=False):
    bin_safe = sanitize(bin_name)
    bin_dir = CONDOR_DIR / bin_safe
    if bin_dir.exists():
        print("Removing dir:",bin_dir.name)
        shutil.rmtree(bin_dir)
    bin_dir.mkdir(parents=True, exist_ok=True)

    # Subdirectories
    log_dir = bin_dir / "log"
    out_dir = bin_dir / "out"
    err_dir = bin_dir / "err"
    json_dir = bin_dir / "json"
    for d in (log_dir, out_dir, err_dir, json_dir):
        d.mkdir(parents=True, exist_ok=True)

    submit_path = bin_dir / f"{bin_safe}.sub"

    # Header
    header = CONDOR_HEADER.format(cpus=cpus, memory=memory)
    submit_lines = [header]

    # Logs go here (shared config, uses $(LogFile))
    submit_lines.append(f"log    = {log_dir}/$(LogFile).log")
    submit_lines.append(f"output = {out_dir}/$(LogFile).out")
    submit_lines.append(f"error  = {err_dir}/$(LogFile).err")

    submit_lines.append("transfer_output_files = $(LogFile).json")
    submit_lines.append(f'transfer_output_remaps = "$(LogFile).json = {json_dir.as_posix()}/$(LogFile).json"')

    # Inline queue
    submit_lines.append("# Queue jobs with LogFile (used for log/out/err) and Args")
    submit_lines.append("queue LogFile, Args from (")

    for job in jobs:
        ds = job["dataset"]
        fpath = job["filepath"]
        fname_stem = job["fname_stem"]
        sig_type = job.get("sig_type", None)
        sms_filters = job.get("sms_filters", [])
    
        # include sms filter in base if present
        base = sanitize(f"{bin_name}_{ds}_{fname_stem}" + (f"_{sms_filters[0]}" if sms_filters else ""))
    
        # Pass plain filename (no leading ./) so Condor matches it to transfer_output_files
        remote_json_arg = f"{base}.json"
    
        args_list = [
            f"--lumi {lumi}",
            f"--bin {bin_name}",
            f"--file {fpath}",
            f"--output {remote_json_arg}",
            f"--cuts {job.get('cuts','')}",
            f"--lep-cuts {job.get('lep_cuts','')}",
            f"--predefined-cuts {job.get('predef_cuts','')}"
        ]
        if sig_type:
            args_list.append(f"--sig-type {sig_type}")
        if sms_filters:
            args_list.append("--sms-filters")
            args_list.append(sms_filters[0])
    
        args_str = " ".join(a for a in args_list if a and not a.isspace())
        submit_lines.append(f'{base} "{args_str}"')

    submit_lines.append(")")

    # Write .sub
    submit_content = "\n".join(submit_lines) + "\n"
    submit_path.write_text(submit_content)
    print(f"Wrote submit file: {submit_path}")

    # Optional submit
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
            write_merge_script(bin_name,json_dir)

# ----------------------------------------
# Main
# ----------------------------------------
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--bkg_datasets", nargs="+", default=[],
                        help="List of background dataset names")
    parser.add_argument("--sig_datasets", nargs="+", default=[],
                        help="List of signal dataset names")
    parser.add_argument("--sms-filters", nargs="*", default=[],
                        help="Optional list of SMS trees to filter; empty means no filtering")
    parser.add_argument("--bin", default="TEST")
    parser.add_argument("--cuts", default="Nlep>=2;MET>=150")
    parser.add_argument("--lep-cuts", default=">=1OSSF")
    parser.add_argument("--predefined-cuts", default="Cleaning")
    parser.add_argument("--cpus", default="1")
    parser.add_argument("--memory", default="1 GB")
    parser.add_argument("--lumi", default=1.)
    parser.add_argument("--dryrun", "--dry-run", action="store_true")
    args = parser.parse_args()

    tool = pySampleTool.SampleTool()

    # Load datasets directly from args
    if args.sms_filters:
        pySampleTool.BFTool.SetFilterSignalsSMS(args.sms_filters)
    tool.LoadBkgs(args.bkg_datasets)
    tool.LoadSigs(args.sig_datasets)

    jobs = build_jobs(
        tool,
        args.bin,
        args.cuts,
        args.lep_cuts,
        args.predefined_cuts,
        args.sms_filters
    )
    write_submit_file(
        args.bin,
        jobs,
        cpus=args.cpus,
        memory=args.memory,
        lumi=args.lumi,
        dryrun=args.dryrun
    )

if __name__ == "__main__":
    main()
