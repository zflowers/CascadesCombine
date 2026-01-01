#!/usr/bin/env python3
# submitCombineJobs.py
from __future__ import annotations
import argparse
import os
import subprocess
import re
import random
import time
import shutil
from pathlib import Path
from collections import defaultdict
from textwrap import dedent
from typing import List, Optional

# ------------------------------------------------------------
# Retry defaults
# ------------------------------------------------------------
DEFAULT_KNOWN_SCHEDDS = [
    "lpcschedd4.fnal.gov",
    "lpcschedd5.fnal.gov",
    "lpcschedd6.fnal.gov",
]
DEFAULT_MAX_RETRIES = 8
DEFAULT_PER_SCHEDD_LIMIT = 3

# ----------------------
# CMSSW runtime tarball 
# ----------------------
def make_cmssw_runtime_tarball(cache_dir):
    """ 
    Create a fresh CMSSW runtime tarball in cache_dir.
    Returns the path to the tarball.
    """
    cmssw_base = os.environ.get("CMSSW_BASE")
    if not cmssw_base:
        raise RuntimeError("CMSSW_BASE is not set. Did you cmsenv?")
    tarball = Path(f"{cache_dir}/cmssw_runtime.tgz")
    tarball.parent.mkdir(parents=True, exist_ok=True)
    if tarball.exists():
        return tarball
    items = [
        "src/CombineHarvester",
        "src/HiggsAnalysis",
        "lib/",
        "bin/",
        "biglib/",
        "python/",
        ".SCRAM/",
    ]
    exclude_items = [
        "*.git",
        "*tutorials*"
    ]
    cmd = [
        "tar", "czf", str(tarball),
        "-C", cmssw_base,
    ]
    # Add --exclude flags
    for ex in exclude_items:
        cmd.append(f"--exclude={ex}")
    # Add items to archive
    cmd.extend(items)
    subprocess.check_call(cmd)
    return tarball

def extract_combine_name(extra_args: str) -> str:
    toks = extra_args.split()
    if "-n" in toks:
        i = toks.index("-n")
        if i + 1 < len(toks):
            return toks[i + 1]
    return ""

# ------------------------------------------
# Extract mass locally using macro/utils.sh
# ------------------------------------------
def extract_mass(signal: str) -> str:
    cmd = f"source macro/utils.sh && extract_mass {signal}"
    proc = subprocess.run(
        ["bash", "-c", cmd],
        capture_output=True,
        text=True,
    )
    mass = proc.stdout.strip()
    if not mass:
        raise RuntimeError(f"Failed to extract mass from signal '{signal}'")
    return mass

# ------------------------------------------------------------
# Submit file content
# ------------------------------------------------------------
def make_submit_content(
    *,
    signal: str,
    method: str,
    mass: str,
    cmssw_runtime: str,
    executable: str,
    card: str,
    shapes: str,
    output_dir: str,
    logs_dir: Path,
    request_cpus: int,
    request_memory: str,
    request_disk: str,
    extra_args: str,
) -> str:

# higgsCombine.limit.AsymptoticLimits.mH3.00027e+06.root
    dest_dir = Path(output_dir) / signal
    dest_dir.mkdir(parents=True, exist_ok=True)
    cmssw_runtime = Path(cmssw_runtime).resolve()

    combine_name = extract_combine_name(extra_args)    
    name_part = combine_name if combine_name else ""
    output_file = f"higgsCombine{name_part}.{method}.mH{float(mass):.5e}.root"

    remaps = [
        f"{output_file}={dest_dir}/{output_file}",
    ]

    return dedent(f"""\
        universe                = vanilla
        executable              = {executable}
        should_transfer_files   = YES
        when_to_transfer_output = ON_EXIT

        transfer_input_files    = {card},{shapes},{cmssw_runtime},{executable}
        arguments               = {card} {mass} {method} {extra_args}

        request_cpus            = {request_cpus}
        request_memory          = {request_memory}
        request_disk            = {request_disk}

        output                  = {signal}_<method>.out
        error                   = {signal}_<method>.err
        log                     = {signal}_<method>.log

        transfer_output_files   = {output_file}
        #transfer_output_remaps  = "{';'.join(remaps)}"
        priority                = 5

        queue 1
    """)


# ------------------------------------------------------------
# Create submit directory + submit file
# ------------------------------------------------------------
def make_submit_file(
    *,
    signal: str,
    output_dir: str,
    executable: str,
    method: str,
    extra_args: str,
    request_cpus: int,
    request_memory: str,
    request_disk: str,
) -> Path:

    submit_dir = Path(f"{output_dir}") / signal

    # locate inputs
    cards = list(submit_dir.glob("*.txt"))
    shapes = list(submit_dir.glob("*.root"))

    if len(cards) != 1 or len(shapes) != 1:
        raise RuntimeError(f"{signal}: expected exactly 1 .txt and 1 .root")

    card = cards[0].name
    shape = shapes[0].name

    # compute mass locally
    mass = extract_mass(signal)

    cmssw_tar = make_cmssw_runtime_tarball(Path(f"{output_dir}/../"))

    submit_text = make_submit_content(
        signal=signal,
        method=method,
        mass=mass,
        cmssw_runtime=cmssw_tar,
        executable=executable,
        card=card,
        shapes=shape,
        output_dir=output_dir,
        logs_dir=submit_dir,
        request_cpus=request_cpus,
        request_memory=request_memory,
        request_disk=request_disk,
        extra_args=extra_args,
    )

    submit_path = submit_dir / f"job_{signal}_{method}.sub"
    submit_path.write_text(submit_text)
    return submit_path.resolve()

# ---------------------------
# submit_condor_with_retries
# ---------------------------
def submit_condor_with_retries(
    submit_path: str,
    *,
    dryrun: bool = False,
    record_dir: Optional[str] = None,
    known_schedds: Optional[List[str]] = None,
    max_retries: int = DEFAULT_MAX_RETRIES,
    per_schedd_limit: int = DEFAULT_PER_SCHEDD_LIMIT,
    condor_monitor: Optional[object] = None,
) -> bool:
    """
    Submit the condor submit file with transient-retry and schedd rotation.
    Runs 'condor_submit' with cwd set to the submit file's directory so that
    relative transfer_input_files basenames resolve correctly.
    """
    record_dir = Path(record_dir) if record_dir else Path.cwd()
    record_dir.mkdir(parents=True, exist_ok=True)
    record_path = record_dir / "submitted_clusters.txt"

    submit_dir = Path(submit_path).parent

    if known_schedds is None:
        known_schedds = DEFAULT_KNOWN_SCHEDDS

    if condor_monitor is None:
        condor_monitor = None

    if dryrun:
        print("[submitCombineJobs] would run:", f"condor_submit {submit_path}", "cwd=", str(submit_dir), flush=True)
        return True

    attempt = 0
    success = False
    last_proc = None
    schedd_tries = defaultdict(int)
    force_schedd = None

    transient_signatures = [
        "Can't find address of local schedd",
        "Querying the CMS LPC pool",
        "Attempting to submit jobs to",
        "Unable to connect to",
        "Failed to connect",
    ]

    while attempt < max_retries and not success:
        attempt += 1
        if force_schedd:
            cmd = f"source /cvmfs/cms.cern.ch/cmsset_default.sh && condor_submit -name {force_schedd} {os.path.basename(submit_path)}"
            printed_name = force_schedd
        else:
            cmd = f"source /cvmfs/cms.cern.ch/cmsset_default.sh && condor_submit {os.path.basename(submit_path)}"
            printed_name = "(auto)"

        proc = subprocess.run(
            cmd,
            shell=True,
            executable="/bin/bash",
            cwd=str(submit_dir),
            capture_output=True,
            text=True,
        )
        last_proc = proc
        stdout = (proc.stdout or "").strip()
        stderr = (proc.stderr or "").strip()

        if proc.returncode == 0:
            success = True
            break

        is_transient = any(sig in stdout or sig in stderr for sig in transient_signatures)

        match_schedd = re.search(r"Attempting to submit jobs to (\S+)", stdout)
        reported_schedd = match_schedd.group(1) if match_schedd else None

        print(f"  condor_submit failed (rc={proc.returncode})", flush=True)
        if stdout:
            print("  STDOUT:", stdout.replace("\n", " | "))
        if stderr:
            print("  STDERR:", stderr.replace("\n", " | "))

        if not is_transient:
            print("[submitCombineJobs] Non-transient condor_submit failure; not retrying.", flush=True)
            break

        candidate_schedds = [s for s in known_schedds if schedd_tries[s] < per_schedd_limit]
        if reported_schedd and reported_schedd in candidate_schedds:
            candidate_schedds = [s for s in candidate_schedds if s != reported_schedd]

        if not candidate_schedds:
            print("[submitCombineJobs] All schedds have reached per-schedd attempt limit. Stopping retries.", flush=True)
            break

        min_tries = min(schedd_tries[s] for s in candidate_schedds)
        least_tried = [s for s in candidate_schedds if schedd_tries[s] == min_tries]
        next_schedd = random.choice(least_tried)

        schedd_tries[next_schedd] += 1
        force_schedd = next_schedd

        wait_time = min(2 * attempt, 10)
        print(f"[submitCombineJobs WARN] Transient schedd error; will retry using -name {next_schedd} (attempt count for this schedd: {schedd_tries[next_schedd]}). Sleeping {wait_time}s.")
        time.sleep(wait_time)

    if not success:
        if last_proc is not None:
            print(f"condor_submit failed after {attempt} attempt(s). Last returncode: {last_proc.returncode}")
            print("Last STDOUT:", last_proc.stdout)
            print("Last STDERR:", last_proc.stderr)
        else:
            print("condor_submit failed: no subprocess result available.")
        return False

    final_stdout = (proc.stdout or "").strip()
    cluster_id = None
    schedd = None

    match_cluster = re.search(r"submitted to cluster (\d+)", final_stdout)
    if match_cluster:
        cluster_id = match_cluster.group(1)
    else:
        match_cluster2 = re.search(r"(\d+) job\(s\) submitted to cluster (\d+)", final_stdout)
        if match_cluster2:
            cluster_id = match_cluster2.group(2)

    match_schedd = re.search(r"Attempting to submit jobs to (\S+)", final_stdout)
    if match_schedd:
        schedd = match_schedd.group(1)
    elif force_schedd:
        schedd = force_schedd

    if cluster_id:
        with open(record_path, "a") as f:
            if schedd:
                f.write(f"{cluster_id} {schedd}\n")
            else:
                f.write(f"{cluster_id}\n")
    else:
        print("[submitCombineJobs WARN] Submission succeeded but cluster id not parsed from condor output. Full stdout:")
        print(final_stdout)

    return True

# ------------------------------------------------------------
# CLI
# ------------------------------------------------------------
def _cli():
    ap = argparse.ArgumentParser()
    ap.add_argument("--signal", required=True)
    ap.add_argument("--output-dir", required=True)
    ap.add_argument("--executable", default=f'{os.environ.get("CMSSW_BASE")}/src/CascadesCombine/macro/run_combine.sh')
    ap.add_argument("--method", default="AsymptoticLimits")
    ap.add_argument("--extra-args", default="")
    ap.add_argument("--cpus", type=int, default=1)
    ap.add_argument("--memory", default="2GB")
    ap.add_argument("--disk", default="2GB")
    ap.add_argument("--dryrun", action="store_true")
    args = ap.parse_args()

    submit_path = make_submit_file(
        signal=args.signal,
        output_dir=args.output_dir,
        executable=args.executable,
        method=args.method,
        extra_args=args.extra_args,
        request_cpus=args.cpus,
        request_memory=args.memory,
        request_disk=args.disk,
    )

    ok = submit_condor_with_retries(submit_path, dryrun=args.dryrun, record_dir=args.output_dir)

if __name__ == "__main__":
    _cli()

