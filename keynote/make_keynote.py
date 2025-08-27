#!/usr/bin/env python3
import os
import argparse
from pathlib import Path
from collections import defaultdict
import subprocess
import tempfile
import sys

# -------------------------
# CLI args
# -------------------------
parser = argparse.ArgumentParser(description="Create Keynote slides from plot PDFs (with optional rsync).")
parser.add_argument("--rsync-source", default=None,
                    help="Optional rsync source (e.g. user@host:/path/to/plots/*). If not provided, env KEYNOTE_RSYNC is checked.")
parser.add_argument("--no-rsync", action="store_true", help="Skip rsync even if rsync source is provided.")
parser.add_argument("--dry-run", action="store_true", help="Don't run AppleScript; print previews instead.")
args = parser.parse_args()

DRY_RUN = args.dry_run

# -------------------------
# Config
# -------------------------
base_dir = Path(os.environ.get("KEYNOTE_BASE_DIR", "/Users/$USER/Desktop/Work/Cascades/"))
top_level = "lpc_plots"
bin_names = []

prefix_order = [
    "Cascades_300_300_289_280_275_270", "Cascades_209_220_209_200_190_180",
    "Wjets", "DY", "ttbar", "DBTB",
]

ignore_bins = [
    # signals:
    "Cascades_209_220_209_200_190_180",
    "Cascades_289_300_289_280_270_260",
    "Cascades_300_300_289_260_240_220",
    "Cascades_300_300_289_280_275_270",
    "SMS_TChiWZ_SMS_300_290",
    "SMS_TChiWZ_Sandwich_SMS_300_290",
    # backgrounds:
    "DBTB", "DY", "QCD",
    "ST", "ZInv", "ttbar", "Wjets",
]

# -------------------------
# Rsync
# -------------------------
def run_initial_rsync_if_requested():
    if args.no_rsync:
        print("Skipping rsync due to --no-rsync.")
        return
    rsync_src = args.rsync_source or os.environ.get("KEYNOTE_RSYNC")
    if not rsync_src:
        return
    base_dir.mkdir(parents=True, exist_ok=True)
    print(f"Running rsync from '{rsync_src}' -> '{base_dir}' ...")
    try:
        subprocess.run(["rsync", "-ar", rsync_src, str(base_dir)+"/lpc_plots/"], check=True)
        print("rsync finished.")
    except subprocess.CalledProcessError as e:
        print(f"rsync failed (code {e.returncode}); continuing without rsync.", file=sys.stderr)

run_initial_rsync_if_requested()

# -------------------------
# Helpers
# -------------------------
def open_keynote_template(template_path):
    template_path_str = escape_for_applescript(template_path)
    script = f'''
tell application "Keynote"
    activate
    delay 2.5
    open {template_path_str}
    delay 2.5
end tell
'''
    run_applescript(script)

def escape_for_applescript(path):
    return f'"{str(path)}"'

def chunk_list(lst, n):
    for i in range(0, len(lst), n):
        yield lst[i:i+n]

def run_applescript(script_text):
    if DRY_RUN:
        preview = script_text if len(script_text) < 2000 else script_text[:2000] + "\n... (truncated)\n"
        print("----- DRY RUN AppleScript START -----")
        print(preview)
        print("----- DRY RUN AppleScript END -----")
        return
    with tempfile.NamedTemporaryFile("w", suffix=".applescript", delete=False) as f:
        f.write(script_text)
        script_path = f.name
    subprocess.run(["osascript", script_path])

def format_var_title(var: str):
    if var is None:
        return ""
    if "_vs_" in var:
        return var.replace("_vs_", " vs ")
    return var

def parse_pdf_stem(stem: str):
    parts = stem.split("__")
    is_stack = "stack" in stem.lower()
    is_cutflow = "cutflow" in stem.lower()
    parsed = {"bin": None, "proc": None, "var": None, "is_stack": is_stack, "is_cutflow": is_cutflow, "is_2d": False}
    if len(parts) >= 3:
        parsed["bin"] = parts[0]
        parsed["proc"] = parts[1]
        parsed["var"] = "__".join(parts[2:])
    elif len(parts) == 2:
        left, right = parts
        if "stack" in left.lower():
            tokens = left.split("_")
            parsed["bin"] = tokens[-1] if tokens else left
            parsed["proc"] = "stack"
            parsed["var"] = right
        else:
            parsed["bin"] = left
            parsed["proc"] = None
            parsed["var"] = right
    else:
        parsed["var"] = stem
    if parsed["var"]:
        parsed["is_2d"] = "_vs_" in parsed["var"]
    return parsed

# -------------------------
# AppleScript helpers
# -------------------------
def make_applescript_call_show(show):
    script = f'''
tell application "System Events"
    set visible of application process "Keynote" to {show}
end tell
'''
    run_applescript(script)

def make_applescript_call_add_folder_title(bin_name, cutflow_pdf_path=None):
    title = bin_name.replace("_", " ")
    cutflow_str = escape_for_applescript(cutflow_pdf_path) if cutflow_pdf_path else '""'
    script = f'''
set slideTitle to "{title}"
set cutflowPathRaw to {cutflow_str}

tell application "System Events"
    set visible of application process "Keynote" to false
end tell

tell application "Keynote"
    if (count of documents) = 0 then
        set thisDoc to make new document
    else
        set thisDoc to front document
    end if
    tell thisDoc
        set thisSlide to make new slide with properties {{base slide:master slide "Plots"}}
        delay 0.1
        repeat with ti in text items of thisSlide
            try
                set tiPosition to position of ti
                if (item 1 of tiPosition) is 107 then
                    set object text of ti to slideTitle
                    exit repeat
                end if
            end try
        end repeat
        if cutflowPathRaw is not "" then
            set pdfAlias to POSIX file cutflowPathRaw as alias
            tell thisSlide
                set newImg to make new image with properties {{file:pdfAlias}}
                set position of newImg to {{29, 143}}
                set width of newImg to 840
                set height of newImg to 543
                delay 0.1
            end tell
        end if
    end tell
end tell
'''
    run_applescript(script)

def make_applescript_call_add_plots(pdf_paths, slide_title):
    if not pdf_paths:
        return
    applescript_list = "{" + ", ".join([escape_for_applescript(p) for p in pdf_paths]) + "}"
    title_text = slide_title
    script = f'''
set pdfPaths to {applescript_list}
set slideTitle to "{title_text}"
set imageFrames_6 to {{ ¬
    {{{{80, 493}}, {{289, 237}}}}, ¬
    {{{{366, 493}}, {{289, 237}}}}, ¬
    {{{{655, 493}}, {{289, 237}}}}, ¬
    {{{{366, 257}}, {{289, 237}}}}, ¬
    {{{{80, 257}}, {{289, 237}}}}, ¬
    {{{{655, 257}}, {{289, 237}}}} ¬
}}

tell application "Keynote"
    if (count of documents) = 0 then
        set thisDoc to make new document
    else
        set thisDoc to front document
    end if
    tell thisDoc
        set thisSlide to make new slide with properties {{base slide:master slide "Plots"}}
        delay 0.1
        repeat with ti in text items of thisSlide
            try
                set tiPosition to position of ti
                if (item 1 of tiPosition) is 107 then
                    set object text of ti to slideTitle
                    exit repeat
                end if
            end try
        end repeat
        set imageFrames to imageFrames_6
        set pdfCount to count of pdfPaths
        repeat with i from 1 to pdfCount
            set thisPDF to item i of pdfPaths
            set pdfAlias to POSIX file thisPDF as alias
            set frame to item i of imageFrames
            set xpos to item 1 of item 1 of frame
            set ypos to item 2 of item 1 of frame
            set imageWidth to item 1 of item 2 of frame
            set imageHeight to item 2 of item 2 of frame
            tell thisSlide
                set newImg to make new image with properties {{file:pdfAlias}}
                set position of newImg to {{xpos, ypos}}
                set width of newImg to imageWidth
                set height of newImg to imageHeight
                delay 0.1
            end tell
        end repeat
    end tell
end tell
'''
    run_applescript(script)

def make_applescript_call_add_single_large(pdf_path, slide_title):
    applescript_list = "{" + escape_for_applescript(pdf_path) + "}"
    title_text = slide_title
    script = f'''
set pdfPaths to {applescript_list}
set slideTitle to "{title_text}"

tell application "Keynote"
    if (count of documents) = 0 then
        set thisDoc to make new document
    else
        set thisDoc to front document
    end if
    tell thisDoc
        set thisSlide to make new slide with properties {{base slide:master slide "Plots"}}
        delay 0.1
        repeat with ti in text items of thisSlide
            try
                set tiPosition to position of ti
                if (item 1 of tiPosition) is 107 then
                    set object text of ti to slideTitle
                    exit repeat
                end if
            end try
        end repeat
        set thisPDF to item 1 of pdfPaths
        set pdfAlias to POSIX file thisPDF as alias
        tell thisSlide
            set newImg to make new image with properties {{file:pdfAlias}}
            set position of newImg to {{29, 143}}
            set width of newImg to 840
            set height of newImg to 543
            delay 0.1
        end tell
    end tell
end tell
'''
    run_applescript(script)

def make_applescript_call_add_single_Summary(pdf_path, slide_title):
    applescript_list = "{" + escape_for_applescript(pdf_path) + "}"
    title_text = slide_title
    script = f'''
set pdfPaths to {applescript_list}
set slideTitle to "{title_text}"

tell application "Keynote"
    if (count of documents) = 0 then
        set thisDoc to make new document
    else
        set thisDoc to front document
    end if
    tell thisDoc
        set thisSlide to make new slide with properties {{base slide:master slide "Plots"}}
        delay 0.1
        repeat with ti in text items of thisSlide
            try
                set tiPosition to position of ti
                if (item 1 of tiPosition) is 107 then
                    set object text of ti to slideTitle
                    exit repeat
                end if
            end try
        end repeat
        set thisPDF to item 1 of pdfPaths
        set pdfAlias to POSIX file thisPDF as alias
        tell thisSlide
            set newImg to make new image with properties {{file:pdfAlias}}
            set position of newImg to {{16, 135}}
            set width of newImg to 862
            set height of newImg to 558
            delay 0.1
        end tell
    end tell
end tell
'''
    run_applescript(script)

# -------------------------
# Bin processing
# -------------------------
def process_bin_dir(bin_dir: Path):
    bin_name = bin_dir.name
    pdf_files = sorted(bin_dir.glob("*.pdf"))
    cutflow_candidates = [p for p in pdf_files if p.stem.startswith(f"{bin_name}__") and "cutflow" in p.stem.lower()]
    cutflow_pdf = cutflow_candidates[0] if cutflow_candidates else None

    parsed_list = []
    for p in pdf_files:
        parsed = parse_pdf_stem(p.stem)
        parsed["path"] = p
        if not parsed["bin"]:
            parsed["bin"] = bin_name
        parsed_list.append(parsed)

    stack_pdfs = [d for d in parsed_list if d["is_stack"]]
    two_d = [d for d in parsed_list if d["is_2d"] and not d["is_stack"] and not d["is_cutflow"] and (d.get("proc") in prefix_order)]

    var_to_entries = defaultdict(list)
    for ent in two_d:
        var_to_entries[ent["var"]].append(ent)
    for var in var_to_entries:
        var_to_entries[var].sort(key=lambda e: prefix_order.index(e["proc"]) if e["proc"] in prefix_order else 999)

    make_applescript_call_add_folder_title(bin_name, str(cutflow_pdf) if cutflow_pdf else None)

    for sp in stack_pdfs:
        var_title = sp.get("var") or "stack"
        make_applescript_call_add_single_large(str(sp["path"]), format_var_title(var_title))

    for var in sorted(var_to_entries.keys()):
        entries = var_to_entries[var]
        paths = [str(e["path"]) for e in entries]
        for chunk in chunk_list(paths, 6):
            make_applescript_call_add_plots(chunk, format_var_title(var))

# -------------------------
# Discovery & main
# -------------------------
def get_target_bin_dirs():
    pdfs_root = base_dir / top_level / "pdfs"
    if not pdfs_root.exists():
        raise FileNotFoundError(f"Top-level pdfs dir not found: {pdfs_root}")
    if bin_names:
        found = []
        for b in bin_names:
            if b in ignore_bins:
                print(f"INFO: requested bin '{b}' is in ignore_bins and will be skipped.")
                continue
            candidate = pdfs_root / b
            if candidate.exists() and candidate.is_dir():
                found.append(candidate)
            else:
                print(f"WARNING: requested bin '{b}' not found under {pdfs_root}")
        return found
    else:
        return [d for d in sorted(pdfs_root.iterdir()) if d.is_dir() and d.name not in ignore_bins]

def add_summary_slides(summary_pdfs):
    for pdf_path, title in summary_pdfs:
        make_applescript_call_add_single_Summary(pdf_path, title)

def main():
    try:
        bin_dirs = get_target_bin_dirs()
    except FileNotFoundError as e:
        print("ERROR:", e)
        return

    template_file = base_dir / "PlotsTemplate.key"
    if not template_file.exists():
        raise FileNotFoundError(f"Keynote template not found: {template_file}")
    open_keynote_template(str(template_file))

    found_procs = set()
    for bin_dir in bin_dirs:
        for p in bin_dir.glob("*.pdf"):
            proc = parse_pdf_stem(p.stem).get("proc")
            if proc:
                found_procs.add(proc)

    missing_procs = [proc for proc in prefix_order if proc not in found_procs]
    if missing_procs:
        print("WARNING: The following requested procs were not found in any bins:")
        for proc in missing_procs:
            print(f"  {proc}")

    make_applescript_call_show('false')
    print("Making summary yield slides")
    summary_pdfs = [
        (base_dir / top_level / "pdfs/CutFlow2D_yield.pdf", "Yield"),
        (base_dir / top_level / "pdfs/CutFlow2D_SoB.pdf", "S / B"),
        (base_dir / top_level / "pdfs/CutFlow2D_SoverSqrtB.pdf", "S / √B"),
        (base_dir / top_level / "pdfs/CutFlow2D_Zbi.pdf", "Zbi"),
    ]
    add_summary_slides(summary_pdfs)

    print("Will make slides for bins:", [d.name for d in bin_dirs])
    for bin_dir in bin_dirs:
        print("  Making slides for bin:", bin_dir.name)
        process_bin_dir(bin_dir)

    make_applescript_call_show('true')

if __name__ == "__main__":
    main()
