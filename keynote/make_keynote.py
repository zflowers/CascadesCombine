#!/usr/bin/env python3
import os, argparse, subprocess, tempfile, sys, json
from pathlib import Path
from collections import defaultdict

# -------------------------
# CLI args
# -------------------------
parser = argparse.ArgumentParser(description="Create Keynote slides from plot PDFs (with optional rsync).")
parser.add_argument("--rsync-source", default=None, help="Optional rsync source (e.g. user@host:/path/to/runs/*/plots/*). If not provided, env KEYNOTE_RSYNC is checked.")
parser.add_argument("--no-rsync", action="store_true", help="Skip rsync even if rsync source is provided.")
parser.add_argument("--dry-run", action="store_true", help="Don't run AppleScript; print previews instead.")
args = parser.parse_args()

DRY_RUN = args.dry_run
SKIP_CUTFLOW = False
SKIP_SUMMARY = False

# -------------------------
# Hardcoded run-ids to loop over
# -------------------------
# Edit this list to the run ids (directory names under runs/) to process
# If this list is empty, the script will auto-discover all run directories and process them
RUN_IDS = [
]

# ordering for summary slides by token presence in filename (not run-order)
# any summary file/group whose name contains "Gold" will be placed before "Silver", etc.
SUMMARY_ORDER = ["Gold", "Silver", "Bronze"]  # customize sequence here (case-insensitive)

def token_rank(name: str):
    """Return the index of the first SUMMARY_ORDER token found in name (case-insensitive).
       If none are found, return len(SUMMARY_ORDER) to place after ordered tokens.
    """
    low = name.lower()
    for idx, tok in enumerate(SUMMARY_ORDER):
        if tok.lower() in low:
            return idx
    return len(SUMMARY_ORDER)

# -------------------------
# Config
# -------------------------
base_dir = Path(os.environ.get("KEYNOTE_BASE_DIR", "/Users/$USER/Desktop/Work/Cascades/")).expanduser()
top_level = "lpc_plots"
runs_root = base_dir / top_level / "runs"
bin_names = []

prefix_order = [
    "Cascades_220_220_209_200_190_180",
    "SMS_TChiWZ_SMS_300_290",
    "SMS_TChiWZ_SMS_300_270",
    "top", "boson", "Vfakeleps",
    "impacts",
]

ignore_bins = [
]

# For plotting yields — keep insertion order deterministic
METRIC_LABELS = {
    "yield": "Yield",
    #"SoB": "S / B",
    #"SoverSqrtB": "S / √B",
    "Zbi": "Zbi",
}
METRIC_ORDER = list(METRIC_LABELS.keys())

# -------------------------
# Auto-discover RUN_IDS if list is empty
# -------------------------
if not RUN_IDS:
    discovered = []
    if runs_root.exists():
        discovered = [p for p in runs_root.iterdir() if p.is_dir()]
    old_root = base_dir / "runs"
    if not discovered and old_root.exists():
        discovered = [p for p in old_root.iterdir() if p.is_dir()]

    if discovered:
        discovered.sort(key=lambda p: p.stat().st_mtime, reverse=True)
        RUN_IDS = [p.name for p in discovered]
        print(f"AUTO: discovered {len(RUN_IDS)} run-ids (newest first):")
        for r in RUN_IDS:
            print("  ", r)
    else:
        print("WARNING: RUN_IDS is empty and no run directories were found under "
              f"{runs_root} or {old_root}. The script will not process any runs.")

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
    dest = base_dir / top_level
    dest.mkdir(parents=True, exist_ok=True)
    print(f"Running rsync from '{rsync_src}' -> '{dest}' ...")
    try:
        subprocess.run(["rsync", "-aR", "--prune-empty-dirs", rsync_src, str(dest) + "/"], check=True)
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
    open {template_path_str}
end tell

-- Wait until Keynote has at least one document and that document has at least one slide.
tell application "System Events"
    set appIsRunning to (exists process "Keynote")
end tell

-- Poll until front document exists and has slides. Use a reasonable timeout to avoid infinite loops.
set timeout_seconds to 30
set start_time to (do shell script "date +%s")
repeat
    try
        tell application "Keynote"
            if (count of documents) > 0 then
                set thisDoc to front document
                try
                    if (count of slides of thisDoc) > 0 then
                        exit repeat
                    end if
                end try
            end if
        end tell
    end try
    set now_time to (do shell script "date +%s")
    if ((now_time as integer) - (start_time as integer)) > timeout_seconds then
        do shell script "echo 'Timeout waiting for Keynote document to become ready' >&2"
        exit repeat
    end if
    delay 0.5
end repeat
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

def format_var_title(u_var: str):
    var = u_var
    if var is None:
        return ""
    if "_vs_" in var:
        var = var.replace("_vs_", " vs ")
    if "_" in var:
        var = var.replace("_", " ")
    return var

def parse_pdf_stem(stem: str):
    parts = stem.split("__")
    is_stack = "stack" in stem.lower()
    is_overlay = "overlay" in stem.lower()
    is_cutflow = "cutflow" in stem.lower()
    parsed = {"bin": None, "proc": None, "var": None, "is_stack": is_stack, "is_overlay": is_overlay, "is_cutflow": is_cutflow, "is_2d": False}
    if len(parts) >= 3:
        parsed["bin"] = parts[0]
        parsed["proc"] = parts[1]
        parsed["var"] = "__".join(parts[2:])
    elif len(parts) == 2:
        left, right = parts
        if "stack" in left.lower() or "overlay" in left.lower():
            tokens = left.split("_")
            parsed["bin"] = tokens[-1] if tokens else left
            if "stack" in left.lower(): parsed["proc"] = "stack"
            if "overlay" in left.lower(): parsed["proc"] = "overlay"
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

tell application "Keynote"
    if (count of documents) = 0 then
        set thisDoc to make new document
    else
        set thisDoc to front document
    end if
    tell thisDoc
        set thisSlide to make new slide with properties {{base slide:master slide "Plots"}}
        delay 0.2
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
                delay 0.2
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
        delay 0.2
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
                delay 0.2
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
        delay 0.2
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
            delay 0.2
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
        delay 0.2
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
            delay 0.2
        end tell
    end tell
end tell
'''
    run_applescript(script)

def make_applescript_call_add_significance(pdf_path, bin_names_text):
    applescript_list = "{" + escape_for_applescript(pdf_path) + "}"
    script = f'''
set pdfPaths to {applescript_list}
set binNamesText to "{bin_names_text}"

tell application "Keynote"
    if (count of documents) = 0 then
        set thisDoc to make new document
    else
        set thisDoc to front document
    end if
    tell thisDoc
        set thisSlide to make new slide with properties {{base slide:master slide "Plots"}}
        delay 0.2
        -- Add the PDF
        set thisPDF to item 1 of pdfPaths
        set pdfAlias to POSIX file thisPDF as alias
        tell thisSlide
            set newImg to make new image with properties {{file:pdfAlias}}
            set position of newImg to {{100, 267}}
            set width of newImg to 825
            set height of newImg to 464
            delay 0.2
        end tell
        -- Add a text box with bin names
        repeat with ti in text items of thisSlide
            try
                set tiPosition to position of ti
                if (item 1 of tiPosition) is 107 then
                    set object text of ti to "Significances"
                end if
                if (item 1 of tiPosition) is 79 then
                    set object text of ti to binNamesText
                end if
            end try
        end repeat
    end tell
end tell
'''
    run_applescript(script)

def make_applescript_call_add_run_title(run_id: str):
    title_text = f"{run_id}".replace('"', '\\"')
    if DRY_RUN:
        print(f"[DRY RUN] Would add run title slide with title: {title_text}")
        return
    script = f'''
tell application "Keynote"
    repeat until (count of documents) > 0
        delay 0.2
    end repeat
    set thisDoc to front document
    tell thisDoc
        set newSlide to make new slide with properties {{base slide:master slide "Plots"}}
        delay 0.2
        repeat with ti in text items of newSlide
            try
                set tiPosition to position of ti
                if (item 1 of tiPosition) is 107 then
                    set object text of ti to "{title_text}"
                    exit repeat
                end if
            end try
        end repeat
        try
            set the slide index of newSlide to 1
        end try
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

    token_prefit = "can_stack_shapes_prefit_"
    token_fitb = "can_stack_shapes_fit_b_"

    stems_map = {ent["path"].stem: ent for ent in parsed_list}

    pairs = []
    used_stems = set()
    for stem, ent in list(stems_map.items()):
        if stem in used_stems:
            continue
        if token_prefit in stem:
            counterpart = stem.replace(token_prefit, token_fitb, 1)
            if counterpart in stems_map:
                pairs.append((stems_map[stem], stems_map[counterpart]))
                used_stems.add(stem); used_stems.add(counterpart)
        elif token_fitb in stem:
            counterpart = stem.replace(token_fitb, token_prefit, 1)
            if counterpart in stems_map:
                pairs.append((stems_map[counterpart], stems_map[stem]))
                used_stems.add(stem); used_stems.add(counterpart)

    special_pairs_found = len(pairs) > 0

    if used_stems:
        parsed_list = [p for p in parsed_list if p["path"].stem not in used_stems]

    stack_pdfs = [d for d in parsed_list if d["is_stack"]]
    overlay_pdfs = [d for d in parsed_list if d["is_overlay"]]
    two_d = [d for d in parsed_list if d["is_2d"] and not d["is_stack"] and not d["is_overlay"] and not d["is_cutflow"] and (d.get("proc") in prefix_order)]

    var_to_entries = defaultdict(list)
    for ent in two_d:
        var_to_entries[ent["var"]].append(ent)
    for var in var_to_entries:
        var_to_entries[var].sort(key=lambda e: prefix_order.index(e["proc"]) if e["proc"] in prefix_order else 999)

    if not SKIP_CUTFLOW and not special_pairs_found:
        make_applescript_call_add_folder_title(bin_name, str(cutflow_pdf) if cutflow_pdf else None)

    if special_pairs_found:
        bin_title_base = bin_name.replace("_", " ")
        for pre_ent, fitb_ent in pairs:
            pre_title = f"{bin_title_base} Pre-Fit"
            make_applescript_call_add_single_large(str(pre_ent["path"]), pre_title)
            post_title = f"{bin_title_base} Post-Fit"
            make_applescript_call_add_single_large(str(fitb_ent["path"]), post_title)

    for sp in stack_pdfs:
        var_title = sp.get("var") or "stack"
        make_applescript_call_add_single_large(str(sp["path"]), format_var_title(var_title))

    for op in overlay_pdfs:
        var_title = op.get("var") or "overlay"
        make_applescript_call_add_single_large(str(op["path"]), format_var_title(var_title))

    for var in sorted(var_to_entries.keys()):
        entries = var_to_entries[var]
        paths = [str(e["path"]) for e in entries]
        for chunk in chunk_list(paths, 6):
            make_applescript_call_add_plots(chunk, format_var_title(var))

    if not SKIP_SUMMARY:
    	# Per-bin summary slides inside a real bin directory — keep loose matching to allow variants
    	summary_candidates = [p for p in bin_dir.glob("*_Cutflow2D_*.pdf")]
    	summary_candidates = [p for p in summary_candidates if p.stem.split("_Cutflow2D")[0].startswith(bin_name)]

    	for metric in METRIC_ORDER:
    	    label = METRIC_LABELS[metric]
    	    matched = [p for p in summary_candidates if p.stem.endswith(f"_Cutflow2D_{metric}") or f"_Cutflow2D_{metric}" in p.stem]
    	    # prioritize files containing tokens in SUMMARY_ORDER
    	    matched.sort(key=lambda p: (token_rank(p.stem), p.stem.lower()))
    	    for p in matched:
    	        group_name = p.stem.split("_Cutflow2D")[0].replace("_", " ")
    	        slide_title = f"{group_name} {label}"
    	        make_applescript_call_add_single_Summary(str(p), slide_title)

# -------------------------
# New helper to handle flat layout cutflow grouping
# -------------------------
def process_flat_plots_dir(plots_dir: Path):
    """Handle flat layout: group *_Cutflow2D_*.pdf by the prefix before _Cutflow2D
       and create per-group summary slides for each metric"""
    print(f"Processing flat plots dir (grouping by prefix): {plots_dir}")
    cutflow_files = sorted(plots_dir.glob("*_Cutflow2D_*.pdf"))
    groups = defaultdict(list)
    for p in cutflow_files:
        key = p.stem.split("_Cutflow2D")[0]
        groups[key].append(p)

    # stable but token-prioritized iteration order:
    sorted_groups = sorted(groups.keys(), key=lambda k: (token_rank(k), k.lower()))
    for group in sorted_groups:
        files = groups[group]
        group_title = group.replace("_", " ")
        # For each metric, find matching file(s) for this group and create slides
        for metric in METRIC_ORDER:
            label = METRIC_LABELS[metric]
            matched = [f for f in files if f.stem.endswith(f"_Cutflow2D_{metric}") or f"_Cutflow2D_{metric}" in f.stem]
            # sort matched by token_rank first, then stable name order
            matched.sort(key=lambda p: (token_rank(p.stem), p.name.lower()))
            for p in matched:
                slide_title = f"{group_title} {label}"
                make_applescript_call_add_single_Summary(str(p), slide_title)

# -------------------------
# Layout helpers
# -------------------------
def choose_run_dir_by_id(run_id: str) -> Path:
    candidate = runs_root / run_id
    if candidate.exists():
        return candidate
    candidate_old = base_dir / "runs" / run_id
    if candidate_old.exists():
        return candidate_old
    raise FileNotFoundError(f"Requested run-id not found in either expected locations: {candidate} or {candidate_old}")

def get_plots_dir_for_run(run_dir: Path):
    plots_dir_candidate = run_dir / "plots" / "pdfs"
    if plots_dir_candidate.exists():
        return plots_dir_candidate
    alt_candidate = run_dir / "pdfs"
    if alt_candidate.exists():
        return alt_candidate
    raise FileNotFoundError(f"Plots directory not found for run '{run_dir.name}': tried {plots_dir_candidate} and {alt_candidate}")

def get_target_bin_dirs_for_run(run_dir: Path):
    plots_dir = get_plots_dir_for_run(run_dir)
    subdirs = [d for d in sorted(plots_dir.iterdir()) if d.is_dir()]
    if subdirs:
        return [d for d in subdirs if d.name not in ignore_bins]
    if plots_dir.name in ignore_bins:
        return []
    return [plots_dir]

def add_summary_slides(summary_pdfs):
    for pdf_path, title in summary_pdfs:
        if pdf_path.exists():
            make_applescript_call_add_single_Summary(str(pdf_path), title)
        else:
            print(f"INFO: summary PDF not found (skipping): {pdf_path}")

def get_latest_significance_pdf(plots_root):
    all_sig_pdfs = sorted(plots_root.glob("**/Significance.pdf"), key=lambda p: p.stat().st_mtime, reverse=True)
    if all_sig_pdfs:
        latest = all_sig_pdfs[0]
        return latest
    return None

# -------------------------
# Main
# -------------------------
def main():
    template_file = base_dir / "PlotsTemplate.key"
    if not template_file.exists():
        raise FileNotFoundError(f"Keynote template not found: {template_file}")
    open_keynote_template(str(template_file))
    print("Hiding keynote document until after slides are updated")
    make_applescript_call_show('false')

    any_run_processed = False

    for run_id in RUN_IDS:
        print(f"\n=== Processing run-id: {run_id} ===")
        try:
            run_dir = choose_run_dir_by_id(run_id)
        except FileNotFoundError as e:
            print(f"WARNING: {e} — skipping run-id {run_id}")
            continue

        try:
            plots_dir = get_plots_dir_for_run(run_dir)
        except FileNotFoundError as e:
            print(f"WARNING: {e} — skipping run-id {run_id}")
            continue

        bin_dirs = get_target_bin_dirs_for_run(run_dir)
        if not bin_dirs:
            print(f"WARNING: No bins found for run {run_id} (maybe all bins are ignored). Skipping.")
            continue

        any_run_processed = True

        # find which procs are present
        found_procs = set()
        for bin_dir in bin_dirs:
            for p in bin_dir.glob("*.pdf"):
                proc = parse_pdf_stem(p.stem).get("proc")
                if proc:
                    found_procs.add(proc)

        global prefix_order
        missing_procs = [proc for proc in prefix_order if proc not in found_procs]

        print(f"Adding run title slide for run: {run_dir.name}")
        make_applescript_call_add_run_title(run_dir.name)

        sig_pdf = get_latest_significance_pdf(plots_dir)
        bin_names = [d.name for d in bin_dirs]

        bin_text = "Included bins: " + ", ".join(bin_names) if bin_names and [d.name for d in bin_dirs] != ['pdfs'] else ""
        if bin_text == "":
            rsync_src = args.rsync_source or os.environ.get("KEYNOTE_RSYNC")
            if rsync_src:
                if ":" not in rsync_src:
                    raise ValueError("rsync source must include user@host:/path")
                host, remote_path = rsync_src.split(":", 1)
                parts = Path(remote_path).parts
                for i in range(len(parts) - 1):
                    if parts[i] == "CascadesCombine" and parts[i + 1] == "runs":
                        idx = i
                        break
                else:
                    raise ValueError("Could not find 'CascadesCombine/runs' in rsync_src")
                base_remote_path = Path(*parts[:idx + 2])
                json_path = f"{host}:{base_remote_path}/{Path(run_dir).name}/flattened.json"
                dest = base_dir / top_level / run_dir
                subprocess.run(["rsync", "-a", str(json_path), str(dest) + "/"], check=True)
                json_path = dest / "flattened.json"
                if json_path.exists():
                    with json_path.open("r") as f:
                        data = json.load(f)
                    bin_text = "Included bins: " + ", ".join([k for k in data.keys() if k.startswith("Bin")] if data else "")

        if sig_pdf:
            print(f"Adding latest significance plot: {sig_pdf}")
            make_applescript_call_add_significance(str(sig_pdf), bin_text)
        else:
            print("No Significance PDF found for this run.")

        if "LepEta" in run_id:
            prefix_order = [
                "Cascades_220_220_209_200_190_180",
                "SMS_TChiWZ_SMS_300_290",
                "SMS_TChiWZ_SMS_300_270",
                "Wjets_2018",  "top_2018", "boson_2018",
            ]

        if "Gluinos" in run_id:
            prefix_order = [
                "SMS_Gluinos_SMS_1200_1100", "SMS_Gluinos_SMS_1200_1176",
                "Wjets", "ZInv", "ttbar", "QCD",
            ]

        if "NSFProposal" in run_id:
            if "bkg" in run_id:
                prefix_order = [
                    "Wjets", "ttbar", "DBTB",
                    "ZInv", "DY", "QCD",
                ]
            elif "sig" in run_id:
                prefix_order = [
                    "SMS_TChiWZ_SMS_300_297",
                    "SMS_TChiWZ_SMS_300_290",
                    "SMS_TChiWZ_SMS_300_270",
                    "SMS_TChiWZ_SMS_300_250",
                    "Cascades_220_220_209_200_190_180",
                    "Cascades_300_300_289_280_275_270",
                ]
            else:
                prefix_order = [
                    "Wjets", "ttbar", "DBTB",
                    "SMS_TChiWZ_SMS_300_290",
                    "SMS_TChiWZ_SMS_300_270",
                    "Cascades_220_220_209_200_190_180",
                ]

        if "ANPlot" in run_id:
            prefix_order = [
                "SMS_TChiWZ_SMS_300_295",
                "SMS_TChiWZ_SMS_300_290",
                "SMS_TChiWZ_SMS_300_270",
                "Wjets",  "top", "DBTB",
            ]

        # If there are real subdirs (bins), process them using the existing logic
        if [d.name for d in bin_dirs] != ['pdfs']:
            print("Will make slides for bins:", [d.name for d in bin_dirs])
            for bin_dir in bin_dirs:
                if bin_dir.name == "impacts": continue
                print("  Making slides for bin:", bin_dir.name)
                process_bin_dir(bin_dir)
        elif not SKIP_SUMMARY:
            # Flat layout detected: handle Cutflow2D files by grouping their prefix before _Cutflow2D
            plots_dir = get_plots_dir_for_run(run_dir)
            process_flat_plots_dir(plots_dir)

    if not any_run_processed:
        print("No runs were processed. Check RUN_IDS and the filesystem layout (runs_root/base_dir).")

    make_applescript_call_show('true')

if __name__ == "__main__":
    main()
