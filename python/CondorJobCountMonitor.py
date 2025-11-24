import time
import subprocess
import os
import re
import shlex
from pathlib import Path
from typing import Iterable, Tuple, Optional, List

# Example usage snippets:
# 1) Read the global clusters file and wait only on those:
# clusters = CondorJobCountMonitor.load_submitted_clusters("condor")
# monitor = CondorJobCountMonitor(threshold=1)
# monitor.wait_until_jobs_below(clusters=clusters)

# 2) After resubmitting only some work_dirs:
# clusters = CondorJobCountMonitor.load_clusters_for_dirs(resubmitted_dirs, condor_root="condor")
# monitor.wait_until_jobs_below(clusters=clusters)

class CondorJobCountMonitor:
    """
    Condor job-count monitor with integrated cluster-list utilities.

    Usage:
        monitor = CondorJobCountMonitor(threshold=1, verbose=True)
        clusters = CondorJobCountMonitor.load_submitted_clusters("condor")
        monitor.wait_until_jobs_below(clusters=clusters)
    """
    def __init__(self, threshold: int = 10000, verbose: bool = False):
        self.verbose = verbose
        self.set_threshold(threshold)
        self.username = os.environ.get("USER", "") 

    def set_threshold(self, u_threshold: int):
        threshold = u_threshold
        if threshold < 0: # use auto threshold
            threshold = self.get_auto_THRESHOLD()
        self.threshold = max(1, int(threshold))

    # ---------------------------
    # File utilities (class helpers)
    # ---------------------------
    @classmethod
    def load_submitted_clusters(cls, condor_dir: str = "condor", filename: str = "submitted_clusters.txt") -> List[Tuple[str, Optional[str]]]:
        """
        Read condor/<filename> and return list of (cluster_id, schedd_or_None).
        Each line may be:
            76596545 lpcschedd3.fnal.gov
        or
            76596545
        """
        clusters: List[Tuple[str, Optional[str]]] = []
        p = Path(condor_dir) / filename
        if not p.exists():
            return clusters
        with p.open() as fh:
            for ln in fh:
                parts = ln.strip().split()
                if not parts:
                    continue
                if len(parts) >= 2:
                    clusters.append((parts[0], parts[1]))
                else:
                    clusters.append((parts[0], None))
        return clusters

    @classmethod
    def load_clusters_for_dirs(cls, work_dirs: Iterable[str], condor_root: str = "condor", filename: str = "submitted_clusters.txt") -> List[Tuple[str, Optional[str]]]:
        """
        For each work_dir (e.g. 'my_bin'), look for condor/<work_dir>/<filename>.
        If found, read cluster lines inside (same format as submitted_clusters.txt).
        Returns the union (deduped) of clusters found. If none found for any of the provided work_dirs,
        falls back to global submitted_clusters.txt.
        """
        collected = []
        seen = set()
        root = Path(condor_root)
        for wd in work_dirs:
            p = root / wd / filename
            if not p.exists():
                continue
            with p.open() as fh:
                for ln in fh:
                    parts = ln.strip().split()
                    if not parts:
                        continue
                    tup = (parts[0], parts[1] if len(parts) >= 2 else None)
                    key = (tup[0], tup[1] or "")
                    if key not in seen:
                        collected.append(tup)
                        seen.add(key)
        if not collected:
            # fallback to global
            return cls.load_submitted_clusters(condor_dir=condor_root, filename="submitted_clusters.txt")
        return collected

    @classmethod
    def record_cluster(cls, cluster_id: str, schedd: Optional[str] = None, condor_dir: str = "condor", filename: str = "submitted_clusters.txt"):
        """
        Append a cluster record to condor/<filename>. Creates the condor dir if needed.
        Format written: "<cluster_id> <schedd>\n" or "<cluster_id>\n"
        """
        p = Path(condor_dir)
        p.mkdir(parents=True, exist_ok=True)
        outp = p / filename
        with outp.open("a") as fh:
            if schedd:
                fh.write(f"{cluster_id} {schedd}\n")
            else:
                fh.write(f"{cluster_id}\n")
        if cls.__name__ == "CondorJobCountMonitor":
            pass

    # ---------------------------
    # Internal condor_q builders / parsers
    # ---------------------------
    def _run_cmd(self, cmd: str, check: bool = False, text: bool = True) -> Optional[str]:
        """Run a shell command, return stdout or None on failure."""
        try:
            out = subprocess.check_output(cmd, shell=True, text=text, stderr=subprocess.STDOUT)
            return out
        except subprocess.CalledProcessError as e:
            if self.verbose:
                print(f"[CondorJobCountMonitor] Command failed: {cmd}")
                print("  Output:", (e.output or "").replace("\n", " | "))
            if check:
                raise
            return None

    def _condor_q_cmd(self, cluster_id: str, schedd: Optional[str] = None, total: bool = False) -> str:
        if schedd:
            base = f"condor_q -name {schedd} {cluster_id}"
        else:
            base = f"condor_q {cluster_id}"
        if total:
            base = base + " -total"
        return base

    def _condor_q_user_cmd(self, total: bool = False) -> str:
        cmd = "condor_q $USER"
        if total:
            cmd = cmd + " -total"
        return cmd

    def _run_condor_q(self, cluster_id, schedd=None, total=False,
                      max_retries: int = 5, backoff: int = 2):
        """Run condor_q with retries if schedd is unreachable."""
        cmd = self._condor_q_cmd(cluster_id, schedd, total)
        attempt = 0
    
        transient_errors = [
            "Can't find address of local schedd",
            "Querying the CMS LPC pool",
            "Attempting to submit jobs to",
            "Unable to connect to",
            "Failed to connect",
            "Read failure during security negotiation",
        ]
    
        while attempt < max_retries:
            attempt += 1
            try:
                return subprocess.check_output(
                    cmd,
                    shell=True,
                    text=True,
                    stderr=subprocess.STDOUT
                )
            except subprocess.CalledProcessError as e:
                output = (e.output or "").strip()
                if any(sig in output for sig in transient_errors):
                    wait_time = min(backoff * attempt, 60)  # cap at 60s
                    print(f"[warn] condor_q transient schedd error "
                          f"(cluster={cluster_id}, schedd={schedd}, attempt={attempt}/{max_retries}). "
                          f"Retrying in {wait_time}s...")
                    print("  Output:", output.replace("\n", " | "))
                    time.sleep(wait_time)
                    continue
                else:
                    # not transient, re-raise
                    raise
    
        # if exit the loop, all retries failed
        print(f"[error] condor_q failed after {max_retries} retries "
              f"(cluster={cluster_id}, schedd={schedd})")
        return None

    def _count_jobs_from_output(self, output: str) -> int:
        """
        Return the total jobs for the condor_q output (cluster-specific).
        Only looks at 'Total for query:' line.
        """
        for line in output.splitlines():
            if line.startswith("Total for query:"):
                # Example line: "Total for query: 108 jobs; 0 completed, 0 removed, 0 idle, 108 running, 0 held, 0 suspended"
                parts = line.split()
                try:
                    return int(parts[3])  # the number after 'Total for query:'
                except ValueError:
                    return 0
        return 0

    def _count_idle_jobs_from_output(self, output: str) -> int:
        idle = 0
        for line in output.splitlines():
            line = line.strip()
            if not line or line.startswith("--") or "ID" in line:
                continue
            fields = line.split()
            if len(fields) > 5:
                status = fields[5]
                if status == "I":
                    idle += 1
        return idle

    # ---------------------------
    # Public API (with clusters support)
    # ---------------------------
    def get_total_jobs(self, clusters: Optional[Iterable[Tuple[str, Optional[str]]]] = None) -> int:
        """
        Returns the total number of jobs.
        If clusters is None: behave as before and query `condor_q $USER -total`.
        If clusters is provided: sum job totals for each (cluster_id, schedd).
        Returns -1 on error.
        """
        try:
            if clusters is None:
                output = subprocess.check_output(self._condor_q_user_cmd(total=True), shell=True, text=True)
                total = 0
                for line in output.splitlines():
                    if "Total for query" in line:
                        parts = [p for p in line.split() if p.isdigit()]
                        if parts:
                            total += int(parts[-1])
                return total
            else:
                total = 0
                for cluster_id, schedd in clusters:
                    output = self._run_condor_q(cluster_id, schedd, total=False)
                    if output is not None:
                        total += self._count_jobs_from_output(output)
                return total
        except Exception as e:
            print(f"Error retrieving job count: {e}")
        return -1

    def wait_until_no_idle_jobs(self, clusters: Optional[Iterable[Tuple[str, Optional[str]]]] = None):
        check_count = 0
        active_clusters = list(clusters) if clusters is not None else None
    
        while True:
            try:
                if active_clusters is None:
                    output = subprocess.check_output(self._condor_q_user_cmd(total=False), shell=True, text=True)
                    idle_jobs = self._count_idle_jobs_from_output(output)
                    if idle_jobs == 0:
                        if self.verbose:
                            print("[CondorJobCountMonitor] All jobs have moved out of idle (global check).")
                        break
                    if check_count % 10 == 0 and self.verbose:
                        print(f"[CondorJobCountMonitor] {idle_jobs} job(s) still idle (global). Waiting...")
                else:
                    idle_jobs = 0
                    new_active = []
                    for cluster_id, schedd in active_clusters:
                        output = self._run_condor_q(cluster_id, schedd, total=False)
                        if output is None:
                            if self.verbose:
                                print(f"[CondorJobCountMonitor] condor_q failed for {cluster_id} on {schedd}; assuming finished.")
                            continue
                    
                        total_jobs_cluster = self._count_jobs_from_output(output)
                        cluster_idle = self._count_idle_jobs_from_output(output)
                    
                        if total_jobs_cluster == 0:
                            if self.verbose:
                                print(f"[CondorJobCountMonitor] cluster {cluster_id} on {schedd} has no jobs; assuming finished.")
                            continue
                    
                        idle_jobs += cluster_idle
                        if cluster_idle > 0:
                            new_active.append((cluster_id, schedd))
    
                    active_clusters = new_active
                    if not active_clusters:
                        if self.verbose:
                            print("[CondorJobCountMonitor] No active clusters remaining; exiting idle wait.")
                        break
                    if check_count % 10 == 0 and self.verbose:
                        print(f"[CondorJobCountMonitor] {idle_jobs} idle job(s) remaining across {len(active_clusters)} cluster(s). Waiting...")
    
            except Exception as e:
                print(f"[CondorJobCountMonitor] Error retrieving job statuses: {e}", flush=True)
            check_count += 1
            time.sleep(15)
    
    def _parse_request_memory(self, raw: str) -> int:
        """
        Parse RequestMemory from a condor -long output fragment.
        Returns memory in MB (int). If not found, default to 2048 MB.
        Accepts values like: 4096, "4096", "4GB", 4G, 4096MB, etc.
        """
        # raw is the full condor -long output; search for RequestMemory line.
        m = re.search(r"RequestMemory\s*=\s*\"?([\d\.]+)\s*([KkMmGgTt]B?|[KkMmGgTt])?\"?", raw)
        if not m:
            # fallback default
            return 2048
        val = float(m.group(1))
        unit = (m.group(2) or "").upper()
        # normalize to MB
        if unit.startswith("T"):
            return int(val * 1024 * 1024)
        if unit.startswith("G"):
            return int(val * 1024)
        if unit.startswith("K"):
            return int(val / 1024)
        # no unit or 'M' assumed
        return int(val)

    def _parse_request_cpus(self, raw: str) -> int:
        """Parse RequestCpus from condor -long output. Default to 1 if absent."""
        m = re.search(r"RequestCpus\s*=\s*\"?(\d+)\"?", raw)
        if not m:
            return 1
        return int(m.group(1))

    def _check_and_fix_held_jobs(self,
                                 scale_cpu: int = 1,
                                 scale_mem_gb: int = 2,
                                 max_cpus: int = 16,
                                 max_mem_gb: int = 32) -> int:
        """
        Find held jobs for this user whose hold reason includes the memory-limit string,
        read their current RequestCpus and RequestMemory (from the job's schedd),
        increment them by (scale_cpu, scale_mem_gb) but not beyond (max_cpus, max_mem_gb),
        and release the job.
    
        Returns the number of jobs processed (edits or releases attempted).
        """
        key_phrase = "Docker job has gone over memory limit of "
    
        # Format: "<cluster>.<proc> <schedd> <hold reason...>\n"
        fmt_cmd = (
            r'condor_q $USER -held '
            r'-format "%d.%d " ClusterId ProcId '
            r'-format "%s " ScheddName '
            r'-format "%s\n" HoldReason'
        )
    
        held_out = self._run_cmd(fmt_cmd)
        if not held_out:
            return 0
    
        processed = 0
        for ln in held_out.splitlines():
            ln = ln.strip()
            if not ln:
                continue
    
            parts = ln.split(" ", 2)
            if len(parts) < 3:
                if self.verbose:
                    print(f"[CondorJobCountMonitor] Unexpected held line format (skipping): {ln}")
                continue
    
            jobid = parts[0].strip()        # e.g. "123456.0"
            schedd = parts[1].strip()       # e.g. "lpcschedd4.fnal.gov"
            hold_reason = parts[2].strip()
    
            if key_phrase not in hold_reason:
                continue
    
            if self.verbose:
                print(f"[CondorJobCountMonitor] Found held job {jobid} on schedd {schedd}: {hold_reason}")
    
            # Query full attributes from the correct schedd
            long_cmd = f"condor_q -long -name {shlex.quote(schedd)} {shlex.quote(jobid)}"
            long_out = self._run_cmd(long_cmd)
            if long_out is None:
                if self.verbose:
                    print(f"[CondorJobCountMonitor] Failed to fetch job attributes for {jobid} on {schedd}; skipping.")
                continue
    
            try:
                cur_cpus = self._parse_request_cpus(long_out)
                cur_mem_mb = self._parse_request_memory(long_out)
            except Exception as e:
                if self.verbose:
                    print(f"[CondorJobCountMonitor] Error parsing attributes for {jobid} on {schedd}: {e}")
                cur_cpus = 1
                cur_mem_mb = 2048
    
            # Compute new values, enforcing caps
            cap_cpus = int(max_cpus)
            cap_mem_mb = int(max_mem_gb * 1024)
    
            target_cpus = min(cur_cpus + int(scale_cpu), cap_cpus)
            target_mem_mb = min(cur_mem_mb + int(scale_mem_gb) * 1024, cap_mem_mb)
    
            if target_cpus == cur_cpus and target_mem_mb == cur_mem_mb:
                # Already at/above caps or no change; attempt a release
                if self.verbose:
                    print(f"[CondorJobCountMonitor] Job {jobid} already at caps or no increase needed "
                          f"(CPUs {cur_cpus}, Mem {cur_mem_mb}MB). Attempting release without edits.")
                release_cmd = f"condor_release -name {shlex.quote(schedd)} {shlex.quote(jobid)}"
                rel_out = self._run_cmd(release_cmd)
                if self.verbose:
                    print(f"[CondorJobCountMonitor] condor_release output: {rel_out or '<no output>'}")
                processed += 1
                continue
    
            if self.verbose:
                print(
                    f"[CondorJobCountMonitor] Updating {jobid} on {schedd}: "
                    f"RequestCpus {cur_cpus} -> {target_cpus}, "
                    f"RequestMemory {cur_mem_mb}MB -> {target_mem_mb}MB (caps: {cap_cpus} CPUS, {cap_mem_mb}MB)"
                )
    
            # Apply edits on the specific schedd
            edit_cpu_cmd = f"condor_qedit -name {shlex.quote(schedd)} {shlex.quote(jobid)} RequestCpus={target_cpus}"
            edit_mem_cmd = f"condor_qedit -name {shlex.quote(schedd)} {shlex.quote(jobid)} RequestMemory={target_mem_mb}"
            cpu_out = self._run_cmd(edit_cpu_cmd)
            mem_out = self._run_cmd(edit_mem_cmd)
    
            # Release the job on that schedd
            release_cmd = f"condor_release -name {shlex.quote(schedd)} {shlex.quote(jobid)}"
            rel_out = self._run_cmd(release_cmd)
    
            if self.verbose:
                print(f"[CondorJobCountMonitor] condor_qedit CPU output: {cpu_out or '<no output>'}")
                print(f"[CondorJobCountMonitor] condor_qedit MEM output: {mem_out or '<no output>'}")
                print(f"[CondorJobCountMonitor] condor_release output: {rel_out or '<no output>'}")
    
            processed += 1
        return processed

    def wait_until_jobs_below(self, clusters: Optional[Iterable[Tuple[str, Optional[str]]]] = None,
                              held_check_interval: int = 10,
                              scale_cpu: int = 1,
                              scale_mem_gb: int = 2):
        """
        Wait until total jobs are below the threshold.
        args:
          held_check_interval: run the held-job check every N loops (default 10).
          scale_cpu: how many CPUs to add when recovering a held job (default 1).
          scale_mem_gb: how many GB to add when recovering a held job (default 2).
        """
        check_count = 0
        active_clusters = list(clusters) if clusters is not None else None

        while True:
            try:
                # Run held-job check every held_check_interval loops
                if check_count % held_check_interval == 0:
                    try:
                        self._check_and_fix_held_jobs(scale_cpu=scale_cpu, scale_mem_gb=scale_mem_gb)
                    except Exception as e:
                        if self.verbose:
                            print(f"[CondorJobCountMonitor] Error during held-job handling: {e}")

                if active_clusters is None:
                    # check all schedds; proceed if any one of them drops below threshold
                    cmd = self._condor_q_user_cmd(total=True)
                    output = subprocess.check_output(cmd, shell=True, text=True)
                    total_jobs_per_schedd = []

                    for line in output.splitlines():
                        line = line.strip()
                        if line.startswith("Total for") and not line.startswith("Total for all users"):
                            parts = line.split()
                            for p in parts:
                                if p.isdigit():
                                    total_jobs_per_schedd.append(int(p))
                                    break

                    if not total_jobs_per_schedd:
                        print("[CondorJobCountMonitor] Could not parse any schedd totals, retrying...", flush=True)
                        total_jobs = -1
                    else:
                        total_jobs = min(total_jobs_per_schedd)  # proceed if *any* schedd drops below threshold

                else:
                    total_jobs = 0
                    new_active = []
                    for cluster_id, schedd in active_clusters:
                        cmd = self._condor_q_cmd(cluster_id, schedd, total=False)
                        try:
                            output = subprocess.check_output(cmd, shell=True, text=True)
                        except subprocess.CalledProcessError:
                            if self.verbose:
                                print(f"[CondorJobCountMonitor] condor_q failed for {cluster_id} on {schedd}; assuming finished.")
                            continue

                        job_count = self._count_jobs_from_output(output)
                        if job_count == 0:
                            if self.verbose:
                                print(f"[CondorJobCountMonitor] cluster {cluster_id} on {schedd} has no jobs; assuming finished.")
                            continue

                        new_active.append((cluster_id, schedd))
                        total_jobs += job_count

                    active_clusters = new_active
                    if not active_clusters:
                        if self.verbose:
                            print("[CondorJobCountMonitor] No active clusters remaining; exiting job count wait.")
                        break

                if total_jobs == -1:
                    print("[CondorJobCountMonitor] Error retrieving job count, retrying...", flush=True)
                elif total_jobs < self.threshold:
                    if self.verbose:
                        print(f"[CondorJobCountMonitor] Job count ({total_jobs}) is below threshold ({self.threshold}). Proceeding...")
                    break
                else:
                    if check_count % 10 == 0 and self.verbose:
                        print(f"[CondorJobCountMonitor] Current jobs: {total_jobs}. Waiting for jobs to drop below {self.threshold}...")

            except Exception as e:
                print(f"[CondorJobCountMonitor] Error while waiting for jobs: {e}", flush=True)
            check_count += 1
            time.sleep(15)

    def get_auto_THRESHOLD(self):
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

if __name__ == "__main__":
        condor_monitor = CondorJobCountMonitor(threshold=1,verbose=False)
        print("Waiting for jobs to finish...")
        condor_monitor.wait_until_jobs_below()
