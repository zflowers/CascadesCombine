#!/usr/bin/env python3
"""
make_silver_bronze_ruamel.py
Round-trip edit of *_Gold.yaml -> produce *_Silver.yaml and *_Bronze.yaml
Preserves formatting/comments using ruamel.yaml in round-trip mode.
Run after cmsenv and ensure ruamel.yaml is installed in the active Python.
"""
from pathlib import Path
import sys
from ruamel.yaml import YAML
from ruamel.yaml.scalarstring import PreservedScalarString

# Map new lep-cuts lines for each tier and multiplicity
NEW_LEP_CUTS = {
    "Silver": {
        "2L": ["<=1Gold;", "=0Bronze;"],
        "3L": [">=1Silver;", "=0Bronze;"],
        "4L": [">=1Silver;", "<=1Bronze;"],
    },
    "Bronze": {
        "2L": ["=1Bronze;"],
        "3L": [">=1Gold;", "=1Bronze;"],
        "4L": [">=1Gold;", "=2Bronze;"],
    },
}

def detect_mult_from_key(key: str):
    # Expect keys like Bin2L_, Bin3L_, Bin4L_
    for m in ("2L", "3L", "4L"):
        if key.startswith(f"Bin{m}_"):
            return m
    # fallback search
    for m in ("2L", "3L", "4L"):
        if f"Bin{m}_" in key:
            return m
    return None

def transform_lep_cuts_ruamel(old_scalar, multiplicity, tier):
    """
    old_scalar is a ruamel scalar or string. Return a PreservedScalarString for block style.
    Keep any non-Gold lines; remove lines containing 'Gold' and prepend new tier lines.
    """
    old_text = "" if old_scalar is None else str(old_scalar)
    # split lines, ignore fully-empty lines
    old_lines = [ln.rstrip() for ln in old_text.splitlines() if ln.strip() != ""]
    # remove any lines that contain 'Gold' (we want to replace Gold markers)
    kept = [ln for ln in old_lines if "Gold" not in ln]
    new_lines = NEW_LEP_CUTS[tier].get(multiplicity, [])
    final = new_lines + kept
    if not final:
        return PreservedScalarString("")  # empty block
    # ensure newline-terminated block scalar
    return PreservedScalarString("\n".join(final) + "\n")

def process_ruamel_doc(doc, tier):
    """
    doc is ruamel CommentedMap-like mapping.
    Return a new ruamel mapping for the new document (we will mutate a copy).
    """
    # We will create a new CommentedMap by copying top-level keys but not converting to plain dicts.
    from ruamel.yaml.comments import CommentedMap
    out = CommentedMap()
    for key in list(doc.keys()):
        value = doc[key]
        # new key name: replace 'Gold' with tier in the key string
        new_key = key.replace("Gold", tier)
        # keep original value object but shallow-copy mappings to avoid aliasing
        if hasattr(value, 'copy'):
            entry = value.copy()
        else:
            # fallback primitive
            entry = value
        multiplicity = detect_mult_from_key(key)
        if isinstance(entry, dict) and multiplicity is not None and "lep-cuts" in entry:
            entry_lep = entry.get("lep-cuts")
            entry["lep-cuts"] = transform_lep_cuts_ruamel(entry_lep, multiplicity, tier)
        out[new_key] = entry
        # preserve comments associated with original key (if any)
        try:
            out.ca.items[new_key] = doc.ca.items[key]
        except Exception:
            pass
    return out

def main(indir="config/bin_cfgs", outdir=None, dry_run=False):
    indir = Path(indir)
    outdir = Path(outdir) if outdir else indir
    outdir.mkdir(parents=True, exist_ok=True)

    yaml = YAML(typ='rt')          # crucial: round-trip mode
    yaml.preserve_quotes = True
    yaml.indent(mapping=2, sequence=4, offset=2)
    yaml.width = 4096              # avoid automatic line wrapping

    gold_files = sorted(indir.glob("*_Gold.yaml"))
    if not gold_files:
        print("No *_Gold.yaml files found.")
        return 0

    for gf in gold_files:
        print("Processing:", gf)
        with gf.open("r", encoding="utf-8") as f:
            data = yaml.load(f)

        for tier in ("Silver", "Bronze"):
            newdoc = process_ruamel_doc(data, tier)
            outname = gf.name.replace("Gold", tier)
            outpath = outdir / outname
            if dry_run:
                print(f"  [dry-run] would write {outpath}")
                continue
            # Write preserving round-trip metadata
            with outpath.open("w", encoding="utf-8") as f:
                yaml.dump(newdoc, f)
            print("  wrote", outpath)
    return 0

if __name__ == "__main__":
    import argparse
    ap = argparse.ArgumentParser(description="Create Silver and Bronze YAMLs from Gold using ruamel round-trip")
    ap.add_argument("-i", "--indir", default="config/bin_cfgs")
    ap.add_argument("-o", "--outdir", default=None, help="defaults to input dir")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()
    sys.exit(main(args.indir, args.outdir, args.dry_run))

