#!/usr/bin/env python3
"""
Make Silver/Bronze YAMLs and optional lPTISR Golds from Gold YAMLs; enforce Regions predef/user

- For *_Gold.yaml files (in config/bin_cfgs by default):
  1) If filename contains _hPTISR and is 2L or 3L, create a corresponding lPTISR Gold:
     - 2L: _P350 -> _P250 and PTISR_LEP>=350 -> PTISR_LEP>=250;PTISR_LEP<350
     - 3L: _P300 -> _P200 and PTISR_LEP>=300 -> PTISR_LEP>=200;PTISR_LEP<300
  2) For any file whose filename starts with "Regions" (case-insensitive),
     set every bin's `predefined-cuts` and `user-cuts` to the hard-coded strings below.
  3) Produce Silver and Bronze YAMLs (Gold -> Silver / Bronze) with lep-cuts modified
     according to NEW_LEP_CUTS mapping.

Run this inside a CMSSW environment (after cmsenv) with ruamel.yaml installed.
"""
from pathlib import Path
import sys
from ruamel.yaml import YAML
from ruamel.yaml.scalarstring import PreservedScalarString

NEW_LEP_CUTS = {
    "Silver": {
        "2L": ["<=1Gold;", "=0Bronze;"],
        "3L": [">=1Silver;", "=0Bronze;"],
        "4L": [">=1Silver;", "=1Bronze;"],
    },
    "Bronze": {
        "2L": ["=1Bronze;"],
        "3L": [">=1Gold;", "=1Bronze;"],
        "4L": [">=1Gold;", "=2Bronze;"],
    },
}

# Hard-coded strings for Regions* files
REGIONS_PREDEFINED_CUTS = "Cleaning_LEP;dphiMETV_LEP;"
REGIONS_USER_CUTS = "minMll_minDR_2D_low;HEM_Veto;leadSjet_pt;"

def detect_mult_from_key(key: str):
    """Expect keys like Bin2L_, Bin3L_, Bin4L_ at start."""
    for m in ("2L", "3L", "4L"):
        if key.startswith(f"Bin{m}_"):
            return m
    # fallback: search anywhere
    for m in ("2L", "3L", "4L"):
        if f"Bin{m}_" in key:
            return m
    return None

def enforce_common_cuts(doc):
    """
    Overwrite predefined-cuts and user-cuts for every bin in the document.
    Operates in-place on the ruamel mapping.
    """
    for key, entry in doc.items():
        if not isinstance(entry, dict):
            continue
        entry["predefined-cuts"] = REGIONS_PREDEFINED_CUTS
        entry["user-cuts"] = REGIONS_USER_CUTS

def transform_lep_cuts_ruamel(old_scalar, multiplicity, tier):
    """
    Replace any Gold marker lines in lep-cuts and prepend the new tier lines.
    Returns a PreservedScalarString to keep block style.
    """
    old_text = "" if old_scalar is None else str(old_scalar)
    # split into logical lines, ignore empty lines
    old_lines = [ln.rstrip() for ln in old_text.splitlines() if ln.strip() != ""]
    # remove any lines that contain 'Gold'
    kept = [ln for ln in old_lines if "Gold" not in ln]
    new_lines = NEW_LEP_CUTS[tier].get(multiplicity, [])
    final = new_lines + kept
    # return as block scalar with trailing newline
    if not final:
        return PreservedScalarString("")
    return PreservedScalarString("\n".join(final) + "\n")

def make_lptisr_doc_ruamel(doc):
    """
    Produce a new ruamel CommentedMap for the lPTISR Gold version.

    2L: _P350 -> _P250 and PTISR_LEP>=350 -> PTISR_LEP>=250;PTISR_LEP<350
    3L: _P300 -> _P200 and PTISR_LEP>=300 -> PTISR_LEP>=200;PTISR_LEP<300
    """
    from ruamel.yaml.comments import CommentedMap
    out = CommentedMap()

    for key in list(doc.keys()):
        value = doc[key]
        # copy entry preserving ruamel types
        entry = value.copy() if hasattr(value, "copy") else value

        # detect multiplicity from key
        mult = None
        if key.startswith("Bin2L_"):
            mult = "2L"
        elif key.startswith("Bin3L_"):
            mult = "3L"

        # default: keep same key
        new_key = key

        # 2L mapping
        if mult == "2L" and "_P350" in key:
            new_key = key.replace("_P350", "_P250")

        # 3L mapping
        if mult == "3L" and "_P300" in key:
            new_key = key.replace("_P300", "_P200")

        # modify the cuts (string replacement)
        if isinstance(entry, dict) and "cuts" in entry:
            old_text = str(entry.get("cuts", ""))
            new_text = old_text

            if mult == "2L":
                # 350 -> 250–350
                new_text = new_text.replace(
                    "PTISR_LEP>=350;", "PTISR_LEP>=250;PTISR_LEP<350;"
                ).replace(
                    "PTISR_LEP>=350", "PTISR_LEP>=250;PTISR_LEP<350"
                )

            if mult == "3L":
                # 300 -> 200–300
                new_text = new_text.replace(
                    "PTISR_LEP>=300;", "PTISR_LEP>=200;PTISR_LEP<300;"
                ).replace(
                    "PTISR_LEP>=300", "PTISR_LEP>=200;PTISR_LEP<300"
                )

            if new_text != old_text:
                entry["cuts"] = new_text

        out[new_key] = entry
        # preserve comments if they exist
        try:
            out.ca.items[new_key] = doc.ca.items[key]
        except Exception:
            pass

    return out

def process_ruamel_doc_for_tier(doc, tier):
    """
    Return a new ruamel mapping with Gold->tier in keys and lep-cuts transformed.
    Does NOT touch predefined-cuts/user-cuts — those are applied separately via enforce_common_cuts.
    """
    from ruamel.yaml.comments import CommentedMap
    out = CommentedMap()
    for key in list(doc.keys()):
        value = doc[key]
        # new key: replace 'Gold' with tier
        new_key = key.replace("Gold", tier)
        # shallow copy entry when possible
        if hasattr(value, "copy"):
            entry = value.copy()
        else:
            entry = value
        multiplicity = detect_mult_from_key(key)
        if isinstance(entry, dict) and multiplicity is not None and "lep-cuts" in entry:
            old_lep = entry.get("lep-cuts")
            entry["lep-cuts"] = transform_lep_cuts_ruamel(old_lep, multiplicity, tier)
        out[new_key] = entry
        # preserve comments for the key if present
        try:
            out.ca.items[new_key] = doc.ca.items[key]
        except Exception:
            pass
    return out

def main(indir="config/bin_cfgs", outdir=None, dry_run=False):
    indir = Path(indir)
    outdir = Path(outdir) if outdir else indir
    outdir.mkdir(parents=True, exist_ok=True)

    yaml = YAML(typ="rt")     # round-trip mode -- crucial
    yaml.preserve_quotes = True
    yaml.indent(mapping=2, sequence=4, offset=2)
    yaml.width = 4096

    gold_files = sorted(indir.glob("*_Gold.yaml"))
    if not gold_files:
        print("No *_Gold.yaml files found in", indir)
        return 0

    for gf in gold_files:
        print("Processing Gold file:", gf.name)
        with gf.open("r", encoding="utf-8") as f:
            doc = yaml.load(f)

        # If this is a Regions* file, enforce the predefined/user cuts on the Gold doc immediately
        if gf.name.lower().startswith("regions"):
            print("  -> enforcing predefined-cuts and user-cuts for Regions file (Gold) and writing updated Gold")
            enforce_common_cuts(doc)
            # overwrite the Gold file with enforced values so on-disk Gold is updated
            if not dry_run:
                with gf.open("w", encoding="utf-8") as f:
                    yaml.dump(doc, f)
                print(f"  wrote updated Gold: {gf}")

        # We'll build a list of (doc_obj, base_name) to create Silver/Bronze from.
        process_list = [(doc, gf.name)]

        # If the filename indicates high-PTISR 2L or 3L file, create an lPTISR Gold first
        fname_lower = gf.name.lower()
        is_hpt = ("_hptisr" in fname_lower)
        is_2l = ("2l" in fname_lower)
        is_3l = ("3l" in fname_lower)
        if is_hpt and (is_2l or is_3l):
            # form new lPTISR filename (preserve casing of 'hPTISR' if present)
            if "hPTISR" in gf.name:
                l_name = gf.name.replace("hPTISR", "lPTISR")
            elif "HPTISR" in gf.name:
                l_name = gf.name.replace("HPTISR", "lPTISR")
            else:
                l_name = gf.name.replace("_hptisr", "_lptisr")  # fallback lower-case

            # create l_doc from the already-enforced Gold doc (so it inherits enforced cuts)
            l_doc = make_lptisr_doc_ruamel(doc)

            # If Regions* file, also enforce predefined/user on the generated l_doc (already inherited, but do it to be explicit)
            if gf.name.lower().startswith("regions"):
                enforce_common_cuts(l_doc)

            l_path = outdir / l_name
            if dry_run:
                print(f"  [dry-run] would write generated lPTISR Gold: {l_path}")
            else:
                with l_path.open("w", encoding="utf-8") as f:
                    yaml.dump(l_doc, f)
                print("  wrote lPTISR Gold:", l_path)

            # also schedule it for Silver/Bronze generation
            process_list.append((l_doc, l_name))

        # Now produce Silver and Bronze for each doc in process_list
        for (dobj, base_name) in process_list:
            for tier in ("Silver", "Bronze"):
                newdoc = process_ruamel_doc_for_tier(dobj, tier)

                # If this is a Regions* base name, ensure overwrite predef/user on the tier docs too
                if base_name.lower().startswith("regions"):
                    enforce_common_cuts(newdoc)

                outname = base_name.replace("Gold", tier)
                outpath = outdir / outname
                if dry_run:
                    print(f"  [dry-run] would write {outpath}")
                else:
                    with outpath.open("w", encoding="utf-8") as f:
                        yaml.dump(newdoc, f)
                    print("  wrote", outpath)
    return 0

if __name__ == "__main__":
    import argparse
    ap = argparse.ArgumentParser(description="Make Silver/Bronze YAMLs and optional lPTISR Golds from Gold YAMLs; enforce Regions predef/user")
    ap.add_argument("-i", "--indir", default="config/bin_cfgs", help="input directory")
    ap.add_argument("-o", "--outdir", default=None, help="output directory (defaults to input)")
    ap.add_argument("--dry-run", action="store_true", help="don't write files, just show actions")
    args = ap.parse_args()
    sys.exit(main(args.indir, args.outdir, args.dry_run))

