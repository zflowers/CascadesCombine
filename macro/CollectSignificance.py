#!/usr/bin/env python3
import os
import sys
import ROOT as rt

def get_directories(path):
    """Returns a list of directory names within the specified path."""
    directories = []
    try:
        for entry in os.listdir(path):
            full_path = os.path.join(path, entry)
            if os.path.isdir(full_path):
                directories.append(entry)
    except FileNotFoundError:
        print(f"Error: The path '{path}' was not found.")
    except Exception as e:
        print(f"An error occurred: {e}")
    return directories

# Use first argument as datacard directory, default to datacards_cascades
datacard_dir = sys.argv[1] if len(sys.argv) > 1 else "datacards_cascades"
print(f"[CollectSignificance] Using datacard directory: {datacard_dir}")

datacard_subdir_list = get_directories(datacard_dir)

# Ensure output directory exists
os.makedirs("output", exist_ok=True)
output_file = os.path.join("output", f"Significance_{os.path.basename(datacard_dir)}.txt")

with open(output_file, "w") as file:
    sig = -1
    for subdir in datacard_subdir_list:
        fpath = os.path.join(datacard_dir, subdir, "higgsCombine.Test.Significance.mH120.root")
        f = rt.TFile.Open(fpath)
        tree = f.Get("limit")
        for entry in tree:
            sig = entry.limit
        line = f"{subdir} {sig}\n"
        print(line, end='')
        file.write(line)

