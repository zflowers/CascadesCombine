#!/usr/bin/env python3
import sys
import subprocess
import time

# ======== CONFIG ========
BASE = "root://cmseos.fnal.gov//store/user/lpcsusylep/NTUPLES_Cascades_v9"

SOURCE_SUFFIX = "Summer23BPix_130X"
#SOURCE_DIR = SOURCE_SUFFIX + "_SMS"
SOURCE_DIR = SOURCE_SUFFIX

TARGET_SUFFIXES = [
    #"Summer20UL16APV_106X",
    #"Summer20UL16_106X",
    #"Summer20UL17_106X",
    #"Summer20UL18_106X",
    #"Summer22EE_130X",
    #"Summer23BPix_130X",
    #"Summer23_130X",
    "Summer24_130X",
    "Summer25_130X",
    #"Summer26_130X",
]

#DIR_SUFFIX = "_SMS"
DIR_SUFFIX = ""

DRY_RUN = False
MAX_RETRIES = 3
RETRY_DELAY = 5  # seconds
# ========================

def run_cmd(cmd):
    """Run a command with retries."""
    for attempt in range(1, MAX_RETRIES + 1):
        try:
            subprocess.run(cmd, check=True)
            return True
        except subprocess.CalledProcessError:
            print(f"[WARN] Attempt {attempt} failed: {' '.join(cmd)}")
            if attempt < MAX_RETRIES:
                time.sleep(RETRY_DELAY)
            else:
                print(f"[ERROR] Failed after {MAX_RETRIES} attempts.")
                return False


if len(sys.argv) != 2:
    print("Usage: python3 scripts/duplicate_eos_dir.py filelist.txt")
    sys.exit(1)

filelist = sys.argv[1]

n_total = 0
n_skipped = 0
n_failed = 0

with open(filelist) as f:
    for line in f:
        filename = line.strip()
        if not filename:
            continue

        source = f"{BASE}/{SOURCE_DIR}/{filename}"

        for target_suffix in TARGET_SUFFIXES:
            DEST_DIR = target_suffix + DIR_SUFFIX
            src_parts = SOURCE_SUFFIX.split("_")   # ["Summer23BPix", "130X"]
            tgt_parts = target_suffix.split("_")   # ["Summer24", "130X"]

            shared_tokens = []
            for s, t in zip(reversed(src_parts), reversed(tgt_parts)):
                if s == t:
                    shared_tokens.insert(0, s)
                else:
                    break

            if shared_tokens:
                shared = "_" + "_".join(shared_tokens)          # "_130X"
                replacement = "_" + target_suffix               # "_Summer24_130X"
                dest_filename = filename.replace(shared, replacement, 1) if shared in filename else filename
            elif SOURCE_SUFFIX in filename:
                dest_filename = filename.replace(SOURCE_SUFFIX, target_suffix, 1)
            else:
                dest_filename = filename

            dest = f"{BASE}/{DEST_DIR}/{dest_filename}"

            n_total += 1

            cmd = ["xrdcp", "-f", source, dest]

            if DRY_RUN:
                print("[DRY-RUN]", " ".join(cmd))
                continue

            print("[COPY]", " ".join(cmd))
            success = run_cmd(cmd)

            if not success:
                n_failed += 1

print("\n===== SUMMARY =====")
print(f"Total operations: {n_total}")
print(f"Skipped (exists): {n_skipped}")
print(f"Failed:           {n_failed}")
