import time
import subprocess
from pathlib import Path
from typing import Iterable, Tuple, Optional, List

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

    def set_threshold(self, threshold: int):
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
                    cmd = self._condor_q_cmd(cluster_id, schedd, total=False)
                    try:
                        output = subprocess.check_output(cmd, shell=True, text=True)
                        total += self._count_jobs_from_output(output)
                    except subprocess.CalledProcessError as e:
                        if self.verbose:
                            print(f"[CondorJobCountMonitor] Warning: condor_q failed for {cluster_id} on {schedd}: {e}")
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
                        cmd = self._condor_q_cmd(cluster_id, schedd, total=False)
                        try:
                            output = subprocess.check_output(cmd, shell=True, text=True)
                        except subprocess.CalledProcessError:
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
            time.sleep(5)
    
    
    def wait_until_jobs_below(self, clusters: Optional[Iterable[Tuple[str, Optional[str]]]] = None):
        check_count = 0
        active_clusters = list(clusters) if clusters is not None else None
    
        while True:
            try:
                if active_clusters is None:
                    total_jobs = self.get_total_jobs(clusters=None)
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
            time.sleep(5)

# Example usage snippets:
# 1) Read the global clusters file and wait only on those:
# clusters = CondorJobCountMonitor.load_submitted_clusters("condor")
# monitor = CondorJobCountMonitor(threshold=1)
# monitor.wait_until_jobs_below(clusters=clusters)

# 2) After resubmitting only some work_dirs:
# clusters = CondorJobCountMonitor.load_clusters_for_dirs(resubmitted_dirs, condor_root="condor")
# monitor.wait_until_jobs_below(clusters=clusters)

if __name__ == "__main__":
        condor_monitor = CondorJobCountMonitor(threshold=1,verbose=False)
        print("Waiting for jobs to finish...")
        condor_monitor.wait_until_jobs_below()
