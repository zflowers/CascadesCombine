#!/usr/bin/env python3
"""
Scan a (possibly huge) yields JSON for bins/processes with dangerously low
raw MC statistics -- the usual culprit behind a pathologically large
uncertainty band in a stacked prefit plot.

Usage:
    python3 find_lowstat_bins.py yields.json
    python3 find_lowstat_bins.py yields.json --raw-threshold 3
    python3 find_lowstat_bins.py yields.json --match 3L 200to300 0p9to1
    python3 find_lowstat_bins.py yields.json --top 30

If the file is too large to comfortably json.load() into memory, install
ijson (`pip install ijson --break-system-packages`) and this script will
automatically fall back to streaming parsing.
"""
import argparse
import json
import math
import sys


def load_json_streaming_or_full(path):
    try:
        with open(path, "r") as f:
            return json.load(f)
    except MemoryError:
        pass
    try:
        import ijson
    except ImportError:
        print("File too large for json.load and ijson is not installed.\n"
              "Run: pip install ijson --break-system-packages", file=sys.stderr)
        sys.exit(1)
    with open(path, "rb") as f:
        return next(ijson.items(f, ""))


def walk(node, path, out_leaves):
    """Recursively find every dict containing a 'nominal' 3-element
    [raw, yield, stat_err] list, and record it with its full path."""
    if isinstance(node, dict):
        if "nominal" in node and isinstance(node["nominal"], list) and len(node["nominal"]) == 3:
            out_leaves.append((path, node))
        for k, v in node.items():
            if k == "nominal":
                continue
            walk(v, path + (k,), out_leaves)
    elif isinstance(node, list):
        for i, v in enumerate(node):
            walk(v, path + (str(i),), out_leaves)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("json_file")
    ap.add_argument("--raw-threshold", type=float, default=5,
                     help="Flag entries with raw_events <= this (default 5)")
    ap.add_argument("--sys-rel-threshold", type=float, default=0.5,
                     help="Flag systematic Up/Down variations that differ "
                          "from nominal yield by more than this relative "
                          "fraction (default 0.5 = 50%%)")
    ap.add_argument("--match", nargs="*", default=None,
                     help="Only consider paths containing ALL of these "
                          "substrings (e.g. --match 3L 200to300 0p9to1). "
                          "Use this to zero in on the one pathological bin "
                          "once you know its region name.")
    ap.add_argument("--top", type=int, default=25,
                     help="How many worst offenders to print (default 25)")
    ap.add_argument("--check-consistency", action="store_true",
                     help="Flag (bin, sample, systematic) entries where the "
                          "raw event count (nominal[0]) differs from the "
                          "systematic's own raw count. For weight-only "
                          "systematics (SF-type: btag, lepton, pileup, ISR "
                          "weight, etc. -- anything that reweights events "
                          "rather than reselecting them) these should be "
                          "IDENTICAL. A mismatch means the merge dropped or "
                          "double-counted a per-file JSON for that "
                          "systematic (silent parse-error skip, missed "
                          "subdirectory, etc.) -- a pipeline bug, not real "
                          "low MC statistics. NOTE: selection-altering "
                          "systematics (JES/JER and anything that can move "
                          "events between bins) will legitimately show "
                          "differing cnt -- exclude those by name with "
                          "--exclude-syst.")
    ap.add_argument("--exclude-syst", nargs="*", default=[],
                     help="Systematic name substrings to exclude from the "
                          "consistency check (e.g. --exclude-syst JES JER)")
    ap.add_argument("--impact-summary", default=None,
                     help="Systematic name (exact match, e.g. MuR). Sums "
                          "nominal yield and Up/Down deltas PER SAMPLE "
                          "across every matching bin (no low-stat filter -- "
                          "this is about aggregate pull, not sparsity). "
                          "Ranks samples by total |delta| and, for each, "
                          "reports what fraction of that delta comes from "
                          "its single worst bin and the raw MC count there "
                          "-- a dominant-single-bin fraction near 1.0 with "
                          "few raw events means a handful of MC events with "
                          "pathological weights are driving the pull; a low "
                          "fraction spread over many well-populated bins "
                          "means it's a genuine shape/normalization tension.")
    args = ap.parse_args()

    print(f"Loading {args.json_file} ...", file=sys.stderr)
    data = load_json_streaming_or_full(args.json_file)

    leaves = []
    walk(data, (), leaves)
    print(f"Found {len(leaves)} (process, bin) entries with a nominal triple.", file=sys.stderr)

    if args.match:
        leaves = [(p, n) for p, n in leaves if all(m in "/".join(p) for m in args.match)]
        print(f"{len(leaves)} entries remain after --match filter.", file=sys.stderr)

    lowstat = []
    sys_outliers = []

    for path, node in leaves:
        raw, yld, err = node["nominal"]
        path_str = "/".join(path)

        if raw <= args.raw_threshold:
            rel = (err / yld) if yld else float("nan")
            expected_rel = (1 / math.sqrt(raw)) if raw > 0 else float("inf")
            lowstat.append((rel, path_str, raw, yld, err, expected_rel))

        systs = node.get("systematics", {})
        for sys_name, variations in systs.items():
            for direction, arr in variations.items():
                if not isinstance(arr, list) or len(arr) != 3:
                    continue
                _, syld, _ = arr
                if yld:
                    dev = abs(syld - yld) / yld
                    if dev >= args.sys_rel_threshold:
                        syst_cnt = arr[0]
                        avg_w_nom = (yld / raw) if raw else float("nan")
                        avg_w_syst = (syld / syst_cnt) if syst_cnt else float("nan")
                        sys_outliers.append((dev, path_str, sys_name, direction, yld,
                                              syld, raw, syst_cnt, avg_w_nom, avg_w_syst))

    lowstat.sort(reverse=True, key=lambda x: x[0] if x[0] == x[0] else -1)  # NaN-safe sort
    sys_outliers.sort(reverse=True, key=lambda x: x[0])

    print("\n=== Worst low-raw-stat entries (raw_events <= "
          f"{args.raw_threshold}) ===")
    print(f"{'rel_err':>8} {'raw':>5} {'yield':>10} {'stat_err':>10}  path")
    for rel, path_str, raw, yld, err, exp in lowstat[:args.top]:
        print(f"{rel:8.3f} {raw:5.0f} {yld:10.5f} {err:10.5f}  {path_str}")

    print(f"\n=== Worst systematic swings (>= {args.sys_rel_threshold*100:.0f}% "
          "relative to nominal) ===")
    print("cnt_ratio = syst_raw/nominal_raw (>>1 or <<1 => real bin migration).")
    print("wgt_ratio = avg-per-event-weight(syst)/avg-per-event-weight(nominal) "
          "(>>1 with cnt_ratio~1 => a single event's weight exploded -- check "
          "SF/correction maps at extrapolated kinematics, not a stats problem).")
    print(f"{'rel_dev':>8} {'nom_raw':>7} {'sys_raw':>7} {'cnt_rat':>7} {'wgt_rat':>9}  syst  dir  path")
    for dev, path_str, sys_name, direction, yld, syld, raw, syst_cnt, avg_w_nom, avg_w_syst in sys_outliers[:args.top]:
        cnt_rat = (syst_cnt / raw) if raw else float("nan")
        wgt_rat = (avg_w_syst / avg_w_nom) if avg_w_nom not in (0, float("nan")) else float("nan")
        print(f"{dev:8.3f} {raw:7.0f} {syst_cnt:7.0f} {cnt_rat:7.2f} {wgt_rat:9.2f}  {sys_name:<12} {direction:<5} {path_str}")

    if args.check_consistency:
        mismatches = []
        for path, node in leaves:
            nominal = node.get("nominal")
            if not (isinstance(nominal, list) and len(nominal) == 3):
                continue
            nom_cnt = nominal[0]
            path_str = "/".join(path)
            for sys_name, variations in node.get("systematics", {}).items():
                if any(ex in sys_name for ex in args.exclude_syst):
                    continue
                for direction, arr in variations.items():
                    if not (isinstance(arr, list) and len(arr) == 3):
                        continue
                    cnt = arr[0]
                    if cnt != nom_cnt:
                        mismatches.append((abs(cnt - nom_cnt), path_str, sys_name,
                                            direction, nom_cnt, cnt))

        mismatches.sort(reverse=True, key=lambda x: x[0])
        print(f"\n=== cnt(nominal) != cnt(systematic) -- likely merge/parse "
              f"artifact, not real low stats ({len(mismatches)} found) ===")
        print(f"{'|diff|':>8} {'nom_cnt':>8} {'syst_cnt':>9}  syst  dir  path")
        for diff, path_str, sys_name, direction, nom_cnt, cnt in mismatches[:args.top]:
            print(f"{diff:8.1f} {nom_cnt:8.1f} {cnt:9.1f}  {sys_name:<12} {direction:<5} {path_str}")
        if not mismatches:
            print("None found -- merge looks internally consistent for the "
                  "checked systematics. The low-stat bins above are likely "
                  "genuine sparse-MC corners, not a pipeline artifact.")

    if args.impact_summary:
        from collections import defaultdict
        agg = defaultdict(lambda: {"nom": 0.0, "dup": 0.0, "ddown": 0.0, "bins": []})

        for path, node in leaves:
            systs = node.get("systematics", {})
            if args.impact_summary not in systs:
                continue
            nominal = node.get("nominal")
            if not (isinstance(nominal, list) and len(nominal) == 3):
                continue
            nom_cnt, nom_yld, _ = nominal
            variations = systs[args.impact_summary]
            up = variations.get("Up")
            down = variations.get("Down")
            sample = path[-1]
            bin_path = "/".join(path[:-1])
            rec = agg[sample]
            rec["nom"] += nom_yld
            d_up = (up[1] - nom_yld) if isinstance(up, list) and len(up) == 3 else 0.0
            d_down = (down[1] - nom_yld) if isinstance(down, list) and len(down) == 3 else 0.0
            rec["dup"] += d_up
            rec["ddown"] += d_down
            rec["bins"].append((bin_path, nom_cnt, nom_yld, d_up, d_down))

        ranked = sorted(agg.items(),
                         key=lambda kv: max(abs(kv[1]["dup"]), abs(kv[1]["ddown"])),
                         reverse=True)

        print(f"\n=== Aggregate impact of systematic '{args.impact_summary}' "
              "per sample, summed over all matching bins ===")
        print(f"{'sample':<40} {'tot_nom':>10} {'tot_dUp':>10} {'tot_dDn':>10} "
              f"{'rel_Up':>8} {'worst_bin_frac':>14} {'worst_bin_raw':>13}  worst_bin")
        for sample, rec in ranked[:args.top]:
            tot_nom = rec["nom"]
            rel_up = (rec["dup"] / tot_nom) if tot_nom else float("nan")
            # find the single bin contributing most to the Up delta (by magnitude)
            worst = max(rec["bins"], key=lambda b: abs(b[3])) if rec["bins"] else None
            if worst:
                bin_path, nom_cnt, nom_yld, d_up, d_down = worst
                frac = (d_up / rec["dup"]) if rec["dup"] else float("nan")
                print(f"{sample:<40} {tot_nom:10.4f} {rec['dup']:10.4f} {rec['ddown']:10.4f} "
                      f"{rel_up:8.2f} {frac:14.2f} {nom_cnt:13.0f}  {bin_path}")
            else:
                print(f"{sample:<40} {tot_nom:10.4f} {rec['dup']:10.4f} {rec['ddown']:10.4f} "
                      f"{rel_up:8.2f} {'--':>14} {'--':>13}  --")
        print("\nworst_bin_frac close to 1.0 with a low worst_bin_raw => a "
              "handful of low-stat MC events dominate this sample's entire "
              "pull. Spread closer to 0 (delta shared across many bins with "
              "decent raw counts) => genuine shape/normalization tension, "
              "not a low-stat artifact.")


if __name__ == "__main__":
    main()
