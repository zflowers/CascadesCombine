#!/usr/bin/env python3
import os, sys, subprocess, argparse, re
from pathlib import Path
import importlib.util

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

LOG_DIR = Path("condor/logs")
JSON_DIR = Path("json")
SUBMIT_DIR = Path("condor/submit")
LOG_DIR.mkdir(parents=True, exist_ok=True)
JSON_DIR.mkdir(parents=True, exist_ok=True)
SUBMIT_DIR.mkdir(parents=True, exist_ok=True)

CONDOR_HEADER = """
universe                = vanilla
executable              = condor/BFI.sh

should_transfer_files   = YES
when_to_transfer_output = ON_EXIT
transfer_input_files    = BFI_condor.x

request_cpus            = {cpus}
request_memory          = {memory}

periodic_hold = (CpusUsage > RequestCpus || MemoryUsage > RequestMemory) && (JobStatus == 2) && (CurrentTime - EnteredCurrentStatus > 60)

+JobTransforms = "if HoldReasonSubCode == 42 set RequestCpus = MIN(RequestCpus + 1, 32); if HoldReasonCode == 34 set RequestMemory = MIN(RequestMemory + 1, 24)"

periodic_release = (HoldReasonCode == 12 && HoldReasonSubCode == 256) || \\
                   (HoldReasonCode == 13 && HoldReasonSubCode == 2)   || \\
                   (HoldReasonCode == 12 && HoldReasonSubCode == 2)   || \\
                   (HoldReasonCode == 26 && HoldReasonSubCode == 120) || \\
                   (HoldReasonCode == 3  && HoldReasonSubCode == 0)   || \\
                   (HoldReasonSubCode == 42)

use_x509userproxy       = True
getenv                  = True
"""

def sanitize(s):
    s = re.sub(r'[^A-Za-z0-9_.-]', '_', s)
    return s[:200]

def build_jobs(tool, bin_name, cuts, lep_cuts, predef_cuts):
    jobs = []
    for ds, files in tool.BkgDict.items():
        for fpath in files:
            fname_stem = Path(fpath).stem
            jobs.append({
                "dataset": ds,
                "filepath": fpath,
                "fname_stem": fname_stem,
                "cuts": cuts,
                "lep_cuts": lep_cuts,
                "predef_cuts": predef_cuts
            })
    for ds, files in tool.SigDict.items():
        for fpath in files:
            fname_stem = Path(fpath).stem
            sig_type = None
            if "Cascades" in fpath:
                sig_type = "cascades"
            elif "SMS" in fpath:
                sig_type = "sms"
            jobs.append({
                "dataset": ds,
                "filepath": fpath,
                "fname_stem": fname_stem,
                "cuts": cuts,
                "lep_cuts": lep_cuts,
                "predef_cuts": predef_cuts,
                "sig_type": sig_type
            })
    return jobs

def write_submit_file(bin_name, jobs, cpus="1", memory="8 GB", dryrun=False):
    bin_safe = sanitize(bin_name)
    submit_path = SUBMIT_DIR / f"{bin_safe}.sub"

    header = CONDOR_HEADER.format(cpus=cpus, memory=memory)

    transfer_outputs = []
    transfer_remaps = []

    # --- Ensure directories exist ---
    LOG_DIR.mkdir(parents=True, exist_ok=True)
    JSON_DIR.mkdir(parents=True, exist_ok=True)

    submit_lines = [header]

    # --- Build global transfer outputs/remaps ---
    for job in jobs:
        fname_stem = job["fname_stem"]
        ds = job["dataset"]
        jobname_safe = sanitize(f"{bin_name}_{ds}_{fname_stem}")

        remote_json = f"json/{bin_name}_{ds}_{fname_stem}.json"
        remote_debug = f"condor/logs/{jobname_safe}.debug"

        local_json = JSON_DIR / f"{bin_name}_{ds}_{fname_stem}.json"
        local_debug = LOG_DIR / f"{jobname_safe}.debug"

        transfer_outputs.extend([remote_json, remote_debug])
        transfer_remaps.append(f"{remote_json} = {local_json}")
        transfer_remaps.append(f"{remote_debug} = {local_debug}")

    submit_lines.append(f"transfer_output_files = {','.join(transfer_outputs)}")
    submit_lines.append(f'transfer_output_remaps = "{"; ".join(transfer_remaps)}"')

    # --- Inline job list ---
    submit_lines.append("# Inline job list with unique log/output/error/debug/json per job")
    submit_lines.append("queue LogFile, OutFile, ErrFile, Args from (")

    for job in jobs:
        ds = job["dataset"]
        fpath = job["filepath"]
        fname_stem = job["fname_stem"]
        sig_type = job.get("sig_type", None)
        jobname_safe = sanitize(f"{bin_name}_{ds}_{fname_stem}")

        # Use Condor variables $LogFile, $OutFile, $ErrFile
        # remote_json is already set above
        remote_json = f"json/{bin_name}_{ds}_{fname_stem}.json"

        args_list = [
            f"--bin {bin_name}",
            f"--file {fpath}",
            f"--output {remote_json}",
            f"--cuts {job.get('cuts','')}",
            f"--lep-cuts {job.get('lep_cuts','')}",
            f"--predefined-cuts {job.get('predef_cuts','')}"
        ]
        if sig_type:
            args_list.append(f"--sig-type {sig_type}")
        args_str = " ".join(args_list)

        # Queue line uses $LogFile, $OutFile, $ErrFile
        submit_lines.append(f'$LogFile $OutFile $ErrFile "{args_str}"')

    submit_lines.append(")")  # close queue

    submit_content = "\n".join(submit_lines) + "\n"
    submit_path.write_text(submit_content)
    print(f"Wrote submit file: {submit_path}")

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

def main():
    bkglist = ["ttbar", "ST", "DY", "ZInv", "DBTB", "QCD", "Wjets"]
    siglist = ["Cascades"]

    parser = argparse.ArgumentParser()
    parser.add_argument("--bkg_datasets", nargs="+", default=bkglist)
    parser.add_argument("--sig_datasets", nargs="+", default=siglist)
    parser.add_argument("--bin", default="TEST")
    parser.add_argument("--cuts", default="Nlep>=2,MET>=150")
    parser.add_argument("--lep-cuts", default=">=1OSSF")
    parser.add_argument("--predefined-cuts", default="Cleaning")
    parser.add_argument("--cpus", default="1")
    parser.add_argument("--memory", default="8 GB")
    parser.add_argument("--dryrun", "--dry-run", action="store_true")
    args = parser.parse_args()

    tool = pySampleTool.SampleTool()
    tool.LoadBkgs(args.bkg_datasets)
    tool.LoadSigs(args.sig_datasets)

    jobs = build_jobs(tool, args.bin, args.cuts, args.lep_cuts, args.predefined_cuts)
    write_submit_file(args.bin, jobs, cpus=args.cpus, memory=args.memory, dryrun=args.dryrun)

if __name__ == "__main__":
    main()

