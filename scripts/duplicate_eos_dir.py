#!/usr/bin/env python3
import subprocess
import time

# ======== CONFIG ========
BASE = "root://cmseos.fnal.gov//store/user/lpcsusylep/NTUPLES_Cascades_v9"
DRY_RUN = False
MAX_RETRIES = 3
RETRY_DELAY = 5  # seconds

# Each job:
#   source_dir:      subdirectory under BASE to copy from
#   dest_dirs:       list of subdirectories under BASE to copy to
#   files:           list of source filenames to copy
#   compound_naming: if True, dest filename embeds both the original era token
#                    and the target era token (e.g. _106X -> _106X_Summer22EE_130X)
#                    if False, dest filename just swaps the source dir suffix for the target
#                    (e.g. _Summer22_130X -> _Summer25_130X)

JOBS = [

    # ----------------------------------------------------------------
    # Step 1: Summer22_130X_SMS -> all 106X and 130X dirs
    # Simple swap naming (Summer22_130X -> target)
    # Just for TSlepSlep_TEST and TChiWZTEST single-mass files
    # ----------------------------------------------------------------
    {
        "source_dir": "Summer22_130X_SMS",
        "source_suffix": "Summer22_130X",
        "dest_dirs": [
            "Summer20UL16APV_106X_SMS",
            "Summer20UL16_106X_SMS",
            "Summer20UL17_106X_SMS",
            "Summer20UL18_106X_SMS",
            "Summer22EE_130X_SMS",
            "Summer23_130X_SMS",
            "Summer23BPix_130X_SMS",
            "Summer24_130X_SMS",
            "Summer25_130X_SMS",
            "Summer26_130X_SMS",
        ],
        "compound_naming": False,
        "files": [
            "TChiWZ_MNLSP300_MLSP290_EDFilterOR_TuneCP5_13p6TeV-madgraph-pythia8_Summer22_130X.root",
            "TSlepSlep_MSlep250_MLSP245_TuneCP5_13p6TeV-madgraph-pythia8_Summer22_130X.root",
        ],
    },

    # ----------------------------------------------------------------
    # Step 2: Fan-out with compound naming
    # ----------------------------------------------------------------
    {
        "source_dir": "Summer20UL16APV_106X_SMS",
        "source_suffix": "Summer20UL16APV_106X",
        "dest_dirs": [
            "Summer22EE_130X_SMS",
        ],
        "compound_naming": True,
        "files": [
            "SMS-TChiWZ_ZToLL_mZMin-0p1_TuneCP5_13TeV-madgraphMLM-pythia8_Summer20UL16APV_106X.root",
        ],
    },
    {
        "source_dir": "Summer20UL16_106X_SMS",
        "source_suffix": "Summer20UL16_106X",
        "dest_dirs": [
            "Summer22_130X_SMS",
        ],
        "compound_naming": True,
        "files": [
            "SMS-TChiWZ_ZToLL_mZMin-0p1_TuneCP5_13TeV-madgraphMLM-pythia8_Summer20UL16_106X.root",
        ],
    },
    {
        "source_dir": "Summer20UL17_106X_SMS",
        "source_suffix": "Summer20UL17_106X",
        "dest_dirs": [
            "Summer23_130X_SMS",
            "Summer24_130X_SMS",
            "Summer26_130X_SMS",
        ],
        "compound_naming": True,
        "files": [
            "SMS-TChiWZ_ZToLL_mZMin-0p1_TuneCP5_13TeV-madgraphMLM-pythia8_Summer20UL17_106X.root",
        ],
    },
    {
        "source_dir": "Summer20UL18_106X_SMS",
        "source_suffix": "Summer20UL18_106X",
        "dest_dirs": [
            "Summer23BPix_130X_SMS",
            "Summer25_130X_SMS",
        ],
        "compound_naming": True,
        "files": [
            "SMS-TChiWZ_ZToLL_mZMin-0p1_TuneCP5_13TeV-madgraphMLM-pythia8_Summer20UL18_106X.root",
        ],
    },
    {
        "source_dir": "Summer16_102X_SMS",
        "source_suffix": "Summer16_102X",
        "dest_dirs": [
            "Summer22_130X_SMS",
            "Summer22EE_130X_SMS",
        ],
        "compound_naming": True,
        "files": [
            "SMS-TChiWZ_TuneCUETP8M1_13TeV-madgraphMLM-pythia8_Summer16_102X.root",
            "SMS-TChiWZ_dM-60to90_genHT-160_genMET-80_TuneCUETP8M1_13TeV-madgraphMLM-pythia8_Summer16_102X.root",
            "SMS-TChipmWW_TuneCUETP8M1_13TeV-madgraphMLM-pythia8_Summer16_102X.root",
            "SMS-TChipmWW_dM-3to50_genHT-160_genMET-80_TuneCUETP8M1_13TeV-madgraphMLM-pythia8_Summer16_102X.root",
            "SMS-TChipmWW_dM-60to90_genHT-160_genMET-80_TuneCUETP8M1_13TeV-madgraphMLM-pythia8_Summer16_102X.root",
            "SMS-TSlepSlep_TuneCUETP8M1_13TeV-madgraphMLM-pythia8_Summer16_102X.root",
            "SMS-TSlepSlep_genHT-160_genMET-80_TuneCUETP8M1_13TeV-madgraphMLM-pythia8_Summer16_102X.root",
            "TChiWZ_genHT-160_genMET-80_TuneCUETP8M1_13TeV-madgraphMLM-pythia8_Summer16_102X.root",
        ],
    },
    {
        "source_dir": "Fall17_102X_SMS",
        "source_suffix": "Fall17_102X",
        "dest_dirs": [
            "Summer23_130X_SMS",
            "Summer23BPix_130X_SMS",
        ],
        "compound_naming": True,
        "files": [
            "SMS-TChiWZ_TuneCP2_13TeV-madgraphMLM-pythia8_Fall17_102X.root",
            "SMS-TChiWZ_dM-60to90_genHT-160_genMET-80_TuneCP2_13TeV-madgraphMLM-pythia8_Fall17_102X.root",
            "SMS-TChipmWW_TuneCP2_13TeV-madgraphMLM-pythia8_Fall17_102X.root",
            "SMS-TChipmWW_dM-3to50_genHT-160_genMET-80_TuneCP2_13TeV-madgraphMLM-pythia8_Fall17_102X.root",
            "SMS-TChipmWW_dM-60to90_genHT-160_genMET-80_TuneCP2_13TeV-madgraphMLM-pythia8_Fall17_102X.root",
            "SMS-TSlepSlep_TuneCP2_13TeV-madgraphMLM-pythia8_Fall17_102X.root",
            "SMS-TSlepSlep_genHT-160_genMET-80_TuneCP2_13TeV-madgraphMLM-pythia8_Fall17_102X.root",
            "TChiWZ_genHT-160_genMET-80_TuneCP2_13TeV-madgraphMLM-pythia8_Fall17_102X.root",
        ],
    },
    {
        "source_dir": "Autumn18_102X_SMS",
        "source_suffix": "Autumn18_102X",
        "dest_dirs": [
            "Summer24_130X_SMS",
            "Summer25_130X_SMS",
            "Summer26_130X_SMS",
        ],
        "compound_naming": True,
        "files": [
            "SMS-TChiWZ_TuneCP2_13TeV-madgraphMLM-pythia8_Autumn18_102X.root",
            "SMS-TChiWZ_dM-60to90_genHT-160_genMET-80_TuneCP2_13TeV-madgraphMLM-pythia8_Autumn18_102X.root",
            "SMS-TChipmWW_TuneCP2_13TeV-madgraphMLM-pythia8_Autumn18_102X.root",
            "SMS-TChipmWW_dM-3to50_genHT-160_genMET-80_TuneCP2_13TeV-madgraphMLM-pythia8_Autumn18_102X.root",
            "SMS-TChipmWW_dM-60to90_genHT-160_genMET-80_TuneCP2_13TeV-madgraphMLM-pythia8_Autumn18_102X.root",
            "SMS-TSlepSlep_TuneCP2_13TeV-madgraphMLM-pythia8_Autumn18_102X.root",
            "SMS-TSlepSlep_genHT-160_genMET-80_TuneCP2_13TeV-madgraphMLM-pythia8_Autumn18_102X.root",
            "TChiWZ_genHT-160_genMET-80_TuneCP2_13TeV-madgraphMLM-pythia8_Autumn18_102X.root",
        ],
    },

]
# ========================


def run_cmd(cmd):
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


n_total = 0
n_failed = 0

for job in JOBS:
    source_dir    = job["source_dir"]
    source_suffix = job["source_suffix"]
    compound      = job["compound_naming"]

    for dest_dir in job["dest_dirs"]:
        # Derive the target era suffix from the dest dir name (strip trailing _SMS)
        target_suffix = dest_dir.replace("_SMS", "")

        for src_filename in job["files"]:
            if compound:
                # Replace just the X-version token (_106X or _102X) with _<target_suffix>
                # e.g. _Summer20UL17_106X -> _Summer20UL17_Summer23_130X
                dest_filename = src_filename.replace(
                    source_suffix,
                    source_suffix.rsplit("_", 2)[0] + "_" + target_suffix,
                    1
                )
            else:
                # simple swap: _Summer22_130X -> _Summer23_130X
                dest_filename = src_filename.replace(source_suffix, target_suffix, 1)

            source = f"{BASE}/{source_dir}/{src_filename}"
            dest   = f"{BASE}/{dest_dir}/{dest_filename}"
            cmd    = ["xrdcp", "-f", source, dest]
            n_total += 1

            if DRY_RUN:
                print("[DRY-RUN]", " ".join(cmd))
            else:
                print("[COPY]", " ".join(cmd))
                if not run_cmd(cmd):
                    n_failed += 1

print("\n===== SUMMARY =====")
print(f"Total operations: {n_total}")
print(f"Failed:           {n_failed}")
