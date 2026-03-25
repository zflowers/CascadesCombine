#!/usr/bin/env python3
import sys
import subprocess

# ======== CONFIG ========
BASE = "root://cmseos.fnal.gov//store/user/lpcsusylep/NTUPLES_Cascades_v8"
#SOURCE_DIR = "Summer22_130X_SMS"
#DEST_DIR   = "Summer20UL16APV_106X_SMS"

OLD_SUFFIX = "Summer22_130X"
NEW_SUFFIX = "Summer26_130X"
SOURCE_DIR = OLD_SUFFIX + "_SMS"
DEST_DIR = NEW_SUFFIX + "_SMS"
# ========================

if len(sys.argv) != 2:
    print("Usage: python3 scripts/duplicate_eos_dir.py filelist.txt")
    sys.exit(1)

filelist = sys.argv[1]

with open(filelist) as f:
    for line in f:
        filename = line.strip()
        if not filename:
            continue

        # Replace only the suffix portion
        dest_filename = filename.replace(OLD_SUFFIX, NEW_SUFFIX)

        source = f"{BASE}/{SOURCE_DIR}/{filename}"
        dest   = f"{BASE}/{DEST_DIR}/{dest_filename}"

        cmd = ["xrdcp", "-f", source, dest]
        print("Running:", " ".join(cmd))

        subprocess.run(cmd, check=True)

