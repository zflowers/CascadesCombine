#!/usr/bin/env python3
"""
Create baseline Regions_2L_0J_hPTISR_Gold.yaml and Regions_2L_1J_hPTISR_Gold.yaml
from scratch using tunable RISR and Mperp bin definitions.
Run inside CMSSW (after cmsenv) where ruamel.yaml is available.
"""
from pathlib import Path
from ruamel.yaml import YAML
from ruamel.yaml.scalarstring import PreservedScalarString
from ruamel.yaml.scalarstring import PlainScalarString
from ruamel.yaml.scalarstring import DoubleQuotedScalarString
import argparse, os, subprocess

# ------------------ CONFIGURATION ------------------
OUTDIR = Path("config/bin_cfgs")

# PTISR tag and cut used for high-PTISR baseline files
PTISR_TAG = 350
PTISR_CUT_STR = f"PTISR_LEP>={PTISR_TAG};"
PTISR_PNAME = f"P{PTISR_TAG}"

# baseline parts of the cuts (keeps consistent with examples)
BASE_CUTS_TOKENS = [
    "Nlep==2;",
    "MET>=150;",
    "METORtrigger==1;",
    "EventFilter==1;",
    "PassesJVM==1;",
    "MIN(abs(Eta_lep))<1.4442;",
    "Nbjet==0;",
    "Njet>0;",
    # S jet-dependent tokens added later
]

# baseline cuts
PREDEFINED_CUTS = "Cleaning_LEP;dphiMETV_LEP;"
USER_CUTS = "minMll_minDR_2D_low;HEM_Veto;leadSjet_pt;"
COMMON_LEP = ["=0OSSF|mass>=3|mass<=3.2;", "=2Gold;"]

# S jet categories to build baseline gold files for (0J and 1J)
JET_CONFIGS = {
    "0J": {
        "njet_cond": "Njet_S==0;",
        "njet_note": "Njet_S==0;",
    },
    "1J": {
        "njet_cond": "Njet_S>=1;",
        "njet_note": "Njet_S>=1;",
    },
}

# RISR binning and Mperp thresholds
# Each dict: name, min, max, Mh threshold, Mm range low/high, Ml upper bound
RISR_BINS = [
    #{"name": "R95", "min": 0.95, "max": 1.0,  "Mh": 20, "Mm_lo": 10, "Mm_hi": 20, "Ml_hi": 10},
    #{"name": "R9",  "min": 0.9,  "max": 0.95, "Mh": 30, "Mm_lo": 15, "Mm_hi": 30, "Ml_hi": 15},
    #{"name": "R8",  "min": 0.8,  "max": 0.9,  "Mh": 40, "Mm_lo": 20, "Mm_hi": 40, "Ml_hi": 20},
    #{"name": "R7",  "min": 0.7,  "max": 0.8,  "Mh": 50, "Mm_lo": 30, "Mm_hi": 50, "Ml_hi": 30},

    {"name": "R95", "min": 0.95, "max": 1.0,  "Mh": 20, "Mm_lo": 10, "Mm_hi": 20, "Ml_hi": 10},
    {"name": "R9",  "min": 0.9,  "max": 0.95, "Mh": 25, "Mm_lo": 15, "Mm_hi": 25, "Ml_hi": 15},
    {"name": "R85", "min": 0.85, "max": 0.9,  "Mh": 30, "Mm_lo": 20, "Mm_hi": 30, "Ml_hi": 20},
    {"name": "R8",  "min": 0.8,  "max": 0.85, "Mh": 35, "Mm_lo": 25, "Mm_hi": 35, "Ml_hi": 25},
    {"name": "R75", "min": 0.75, "max": 0.8,  "Mh": 40, "Mm_lo": 30, "Mm_hi": 40, "Ml_hi": 30},
    {"name": "R7",  "min": 0.7,  "max": 0.75, "Mh": 45, "Mm_lo": 35, "Mm_hi": 45, "Ml_hi": 35},
]

# Flavor splits to produce per RISR Mperp
# Each flavor entry: suffix_key (used in bin name), lep_cuts_extra_lines (list)
FLAVOR_SPLITS = [
    ("OS_ee", ["=1OSSF;", "=2Elec;"]),
    ("OS_emu", ["=1OSOF;"]),          # mixed flavors
    ("OS_mumu", ["=1OSSF;", "=2Muon;"]),
    ("SS", ["AllSS;"]),               # inclusive SS
]

# Additional constant tokens shared across generated bins
COMMON_ADDITIONAL = "Njet_S"  # not used directly; kept for reference
# ---------------------------------------------------

yaml = YAML()
yaml.default_flow_style = False
yaml.indent(mapping=2, sequence=4, offset=2)
yaml.width = 4096
yaml.preserve_quotes = True

def format_rISR_condition(r):
    """Return RISR_LEP condition string and a human readable label part"""
    if r["max"] >= 1.0:
        # R9 special case with <=1.
        cond = f"RISR_LEP>={r['min']};RISR_LEP<=1.;"
    else:
        cond = f"RISR_LEP>={r['min']};RISR_LEP<{r['max']};"
    return cond

def make_mperp_cut_tokens(r, category):
    """
    category: "Mh" (high), "Mm" (mid), "Ml" (low)
    returns (cut_string, name_fragment) where cut_string is appended to cuts and
    name_fragment is used for the bin name (like M35 or M20_40 or Mlt20)
    """
    if category == "Mh":
        cut = f"Mperp_LEP>={r['Mh']}"
        name_frag = f"M{r['Mh']}"
    elif category == "Mm":
        cut = f"Mperp_LEP>={r['Mm_lo']};Mperp_LEP<{r['Mm_hi']}"
        name_frag = f"M{r['Mm_lo']}_{r['Mm_hi']}"
    elif category == "Ml":
        # Ml is <= Ml_hi - use '<' style
        cut = f"Mperp_LEP<{r['Ml_hi']}"
        name_frag = f"Mlt{r['Ml_hi']}"
    else:
        raise ValueError("unknown category")
    # ensure trailing semicolons in the returned string pieces
    # if cut contains ';' already, preserve; otherwise add semicolon
    if not cut.endswith(";"):
        cut = cut + ";"
    return cut, name_frag

def assemble_cuts_string(base_tokens, ptisr_token, risr_token, jet_token, mperp_token):
    """assemble and return a quoted cuts string."""
    parts = []
    parts.extend(base_tokens)
    parts.append(ptisr_token)
    parts.append(risr_token)
    parts.append("METORtrigger==1;")
    parts.append("EventFilter==1;")
    parts.append("PassesJVM==1;")
    # jet-specific token(s)
    if jet_token:
        parts.append(jet_token)
    parts.append("MIN(abs(Eta_lep))<1.4442;")
    parts.append("Nbjet==0;")
    parts.append(mperp_token)
    # remove duplicates while preserving order
    seen = set()
    out = []
    for p in parts:
        p = p.strip()
        if p == "":
            continue
        # ensure it ends with semicolon
        if not p.endswith(";"):
            p = p + ";"
        if p not in seen:
            seen.add(p)
            out.append(p)
    return "".join(out)

def make_lep_cuts_block(common_lep_lines, flavor_extra):
    """
    common_lep_lines: list (e.g. ['=0OSSF|mass>=3|mass<=3.2;', '=2Gold;'])
    flavor_extra: list, e.g. ['=1OSSF;', '=2Elec;'] or ['AllSS;']
    returns a PreservedScalarString with proper newline endings
    """
    lines = []
    for l in common_lep_lines:
        if not l.endswith(";"):
            l = l + ";"
        lines.append(l)
    for fe in flavor_extra:
        if not fe.endswith(";"):
            fe = fe + ";"
        lines.append(fe)
    # ensure newline-terminated block
    return PreservedScalarString("\n".join(lines) + "\n")

def build_bins_for_jet(jtag, jet_conf):
    """
    Build dictionary of bins for a single jet tag (0J or 1J) for the hPTISR PTISR_TAG
    """
    out = {}
    for r in RISR_BINS:
        risr_cond = format_rISR_condition(r)
        # generate three M categories: Mh, Mm, Ml
        for mcat in ("Mh", "Mm", "Ml"):
            mcut_token, mnamefrag = make_mperp_cut_tokens(r, mcat)
            # for each flavor split produce bin entry
            for (flav_key, flavor_lep_lines) in FLAVOR_SPLITS:
                # name: Bin2L_Gold_{JTAG}_P{PTISR}_R{RNAME}_M{...}_{flav}
                bin_key = f"Bin2L_Gold_{jtag}_{PTISR_PNAME}_{r['name']}_{mnamefrag}_{flav_key}"
                # assemble cuts string
                # jet_token differs between 0J and 1J
                jet_token = jet_conf["njet_cond"]
                cuts_txt = DoubleQuotedScalarString(
                    assemble_cuts_string(
                        BASE_CUTS_TOKENS,
                        PTISR_CUT_STR,
                        risr_cond,
                        jet_token,
                        mcut_token,
                    )
                )
                # flavor-specific additions
                lep_block = make_lep_cuts_block(COMMON_LEP, flavor_lep_lines)
                # add predefined and user cuts
                predef = DoubleQuotedScalarString(PREDEFINED_CUTS)
                user = DoubleQuotedScalarString(USER_CUTS)
                out[bin_key] = {
                    "cuts": cuts_txt,
                    "lep-cuts": lep_block,
                    "predefined-cuts": predef,
                    "user-cuts": user,
                }
    return out

def write_yaml_map(filename: Path, mapping):
    filename.parent.mkdir(parents=True, exist_ok=True)
    with filename.open("w", encoding="utf-8") as f:
        yaml.dump(mapping, f)
    print("WROTE:", filename)

def main(outdir: Path, dry_run=False):
    # Build both 0J and 1J baseline Gold files
    files_to_write = {}
    for jtag, jconf in JET_CONFIGS.items():
        doc_map = build_bins_for_jet(jtag, jconf)
        fname = outdir / f"Regions_2L_{jtag}_{'hPTISR'}_Gold.yaml"
        files_to_write[fname] = doc_map

    # Write files
    for path, doc in files_to_write.items():
        if dry_run:
            print("[DRY-RUN] would write", path, "with", len(doc), "bins")
        else:
            write_yaml_map(path, doc)

if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--outdir", "-o", default=str(OUTDIR))
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()
    main(Path(args.outdir), dry_run=args.dry_run)
    make_GSB_cmd = ["python3", "python/make_GSB_Regions.py"]
    print("making other regions: python3 python/make_GSB_Regions.py")
    subprocess.run(make_GSB_cmd, check=True, text=True)
    print("DONE!")

