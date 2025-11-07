#!/usr/bin/env python3
import json, sys, os
from pathlib import Path
from collections import defaultdict
import math

def load_all_jsons(root):
    """Load all JSON files under root into a dict: bin -> sample -> file -> array of 6 floats."""
    out = defaultdict(lambda: defaultdict(dict))  # bin -> sample -> filepath -> array
    for p in sorted(Path(root).rglob("*.json")):
        try:
            j = json.loads(p.read_text())
        except Exception as e:
            print(f"# ERROR parsing {p}: {e}", file=sys.stderr)
            continue
        for binname, binobj in j.items():
            for sample, sval in binobj.items():
                if isinstance(sval, list):
                    key = f"__file_totals__:{p.name}"
                    out[binname][sample][key] = tuple(float(x) for x in (sval + [0]*(6-len(sval))))
                elif isinstance(sval, dict):
                    if "files" in sval and isinstance(sval["files"], dict):
                        for fpath, farr in sval["files"].items():
                            out[binname][sample][f"{p.name}::{fpath}"] = tuple(float(x) for x in (farr + [0]*(6-len(farr))))
                    else:
                        arr = sval.get("totals", [])
                        key = f"__totals_from__:{p.name}"
                        out[binname][sample][key] = tuple(float(x) for x in (arr + [0]*(6-len(arr))))
    return out

def compare_dirs_multi_bin(dir1, dir2):
    """Compare two directories with JSONs possibly containing multiple bins, only showing differences."""
    data1 = load_all_jsons(dir1)
    data2 = load_all_jsons(dir2)

    all_bins = sorted(set(data1.keys()) | set(data2.keys()))

    for binname in all_bins:
        samples1 = data1.get(binname, {})
        samples2 = data2.get(binname, {})
        all_samples = sorted(set(samples1.keys()) | set(samples2.keys()))

        bin_has_diff = False
        sample_outputs = []

        for sample in all_samples:
            files1_dict = samples1.get(sample, {})
            files2_dict = samples2.get(sample, {})

        # compute totals
        arr1 = [0.0]*6
        for farr in files1_dict.values():
            for i in range(6):
                arr1[i] += farr[i]
        arr1[2] = math.sqrt(arr1[5]) if arr1[5] > 0 else 0.0
        
        arr2 = [0.0]*6
        for farr in files2_dict.values():
            for i in range(6):
                arr2[i] += farr[i]
        arr2[2] = math.sqrt(arr2[5]) if arr2[5] > 0 else 0.0
        
        if arr1 != arr2:
            bin_has_diff = True
            output_lines = [f"  SAMPLE: {sample}"]
            output_lines.append(f"    dir1 total: raw={arr1[0]}  weight={arr1[1]}  (files={len(files1_dict)})")
            output_lines.append(f"    dir2 total: raw={arr2[0]}  weight={arr2[1]}  (files={len(files2_dict)})")
            sample_outputs.extend(output_lines)

        if bin_has_diff:
            print(f"BIN: {binname}")
            for line in sample_outputs:
                print(line)
            print("")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: compare_json_dirs.py dir1 dir2", file=sys.stderr)
        sys.exit(1)

    dir1, dir2 = sys.argv[1], sys.argv[2]
    compare_dirs_multi_bin(dir1, dir2)

