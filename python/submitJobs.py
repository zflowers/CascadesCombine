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

libs_dir = Path(__file__).parent.parent / "libs"  # go up one level
pySampleTool = load_pybind_module("pySampleTool", libs_dir)

# --- configuration ---
LOG_DIR = Path("condor/logs")
JSON_DIR = Path("json")
SUBMIT_DIR = Path("condor/submit")
LOG_DIR.mkdir(parents=True, exist_ok=True)
JSON_DIR.mkdir(parents=True, exist_ok=True)
SUBMIT_DIR.mkdir(parents=True, exist_ok=True)

CONDOR_TEMPLATE = """
universe                = vanilla
executable              = condor/BFI.sh
arguments               = {arguments}

log                     = {log_file}
output                  = {out_file}
error                   = {err_file}

should_transfer_files   = YES
when_to_transfer_output = ON_EXIT
transfer_input_files    = BFI_condor.x

transfer_output_files   = {transfer_outputs}
transfer_output_remaps  = "{transfer_remaps}"

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

queue
"""

# sanitize a string to be safe for filenames/condor names
def sanitize(s):
    s = re.sub(r'[^A-Za-z0-9_.-]', '_', s)
    return s[:200]  # avoid extremely long names

def write_and_submit(jobname, arguments, cpus="1", memory="8 GB", dryrun=False):
    jobname_safe = sanitize(jobname)
    log_file = LOG_DIR / f"{jobname_safe}.log"
    out_file = LOG_DIR / f"{jobname_safe}.out"
    err_file = LOG_DIR / f"{jobname_safe}.err"
    json_file = JSON_DIR / f"{jobname_safe}.json"

    transfer_outputs = f"BFI_condor.debug,{json_file.name}"
    # remap : remote file -> local path to store
    transfer_remaps = f"BFI_condor.debug = {LOG_DIR / 'BFI_condor.debug'}; {json_file.name} = {json_file}"

    submit_content = CONDOR_TEMPLATE.format(
        arguments=arguments,
        log_file=log_file,
        out_file=out_file,
        err_file=err_file,
        transfer_outputs=transfer_outputs,
        transfer_remaps=transfer_remaps,
        cpus=cpus,
        memory=memory
    )

    submit_path = SUBMIT_DIR / f"{jobname_safe}.sub"
    submit_path.write_text(submit_content)
    print(f"Wrote submit file: {submit_path}")
    # submit
    if not dryrun:
        proc = subprocess.run(["condor_submit", str(submit_path)], capture_output=True, text=True)
        if proc.returncode != 0:
            print("condor_submit failed:", proc.stderr)
        else:
            print("Submitted:", jobname_safe)

def main():
    bkglist = ["ttbar", "ST", "DY", "ZInv", "DBTB", "QCD", "Wjets"]
    siglist = ["Cascades"]  # or ["SMS_Gluinos"] for SMS

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--bkg_datasets",
        nargs="+",
        default=bkglist,
        help="Bkg dataset names to query via SampleTool (exact names known to SampleTool)"
    )
    parser.add_argument(
        "--sig_datasets",
        nargs="+",
        default=siglist,
        help="Sig dataset names to query via SampleTool (exact names known to SampleTool)"
    )
    parser.add_argument("--bin", default="TEST", help="--bin argument to pass to BFI")
    parser.add_argument("--cuts", default="Nlep>=2,MET>=150", help="--cuts")
    parser.add_argument("--lep-cuts", default=">=1OSSF", help="--lep-cuts")
    parser.add_argument("--predefined-cuts", default="Cleaning", help="--predefined-cuts")
    parser.add_argument("--cpus", default="1")
    parser.add_argument("--memory", default="8 GB")
    parser.add_argument("--dryrun", "--dry-run", action="store_true", help="Do not call condor_submit")
    args = parser.parse_args()

    tool = pySampleTool.SampleTool()
    tool.LoadBkgs(args.bkg_datasets)
    tool.LoadSigs(args.sig_datasets)
    for ds, files in tool.BkgDict.items():
        for fpath in files:
            jobname = f"{ds}_{os.path.basename(fpath)}"
            arguments = (
                f"--bin {args.bin} --file {fpath} "
                f"--cuts {args.cuts} --lep-cuts {args.lep_cuts} --predefined-cuts {args.predefined_cuts} "
            )
            write_and_submit(jobname, arguments, cpus=args.cpus, memory=args.memory, dryrun=args.dryrun)
            break #debug

    for ds, files in tool.SigDict.items():
        for fpath in files:
            jobname = f"{ds}_{os.path.basename(fpath)}"            
            # Base arguments
            arguments = (
                f"--bin {args.bin} --file {fpath} "
                f"--cuts {args.cuts} --lep-cuts {args.lep_cuts} --predefined-cuts {args.predefined_cuts} "
            )
            # Add sig-type depending on the dataset name
            if "Cascades" in jobname:
                arguments += "--sig-type cascades"
            elif "SMS" in jobname:
                arguments += "--sig-type sms"
            write_and_submit(jobname, arguments, cpus=args.cpus, memory=args.memory, dryrun=args.dryrun)
            break #debug

if __name__ == "__main__":
    main()
