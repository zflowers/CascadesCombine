#!/usr/bin/env bash
set -e

# One-time setup for the keynote helper scripts.
# - Creates a working directory (DEFAULT: $(pwd)/Cascades_keynote)
# - Copies the entire keynote/ directory contents into that working dir
# - Optionally runs rsync to fetch plots into the working dir (to lpc_plots/)
# - Adds aliases to the user's shell rc

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEFAULT_WORKDIR="$(pwd)/Cascades_keynote"

echo "Keynote setup script"
echo "script dir: $SCRIPT_DIR"
echo

# Ask for working dir (default is ./Cascades_keynote)
read -p "Enter working directory (default: $DEFAULT_WORKDIR): " WORKDIR
WORKDIR="${WORKDIR:-$DEFAULT_WORKDIR}"
mkdir -p "$WORKDIR"
echo "Using working directory: $WORKDIR"
echo

# Copy the files into the WORKDIR
echo "Copying files into working dir..."
ESSENTIAL_FILES=("make_keynote.py" "PlotsTemplate.key" "README.md" "setup.sh")
for f in "${ESSENTIAL_FILES[@]}"; do
    src="$SCRIPT_DIR/$f"
    if [ -e "$src" ]; then
        mv "$src" "$WORKDIR"/
        echo "  Copied $f"
    else
        echo "  WARNING: $f not found in $SCRIPT_DIR -> skipping."
    fi
done
echo "Copy complete."

# Remote rsync example (prefilled); user can paste their own or leave empty to skip.
DEFAULT_REMOTE="$USER@cmslpc-el9.fnal.gov:/uscms/home/$USER/nobackup/CMSSW_14_1_0_pre4/src/CascadesCombine/./runs/*/plots/pdfs/"
read -p "Remote rsync source to fetch plots into the working dir (leave blank to skip) [example: $DEFAULT_REMOTE]: " REMOTE_SRC
REMOTE_SRC="${REMOTE_SRC:-$DEFAULT_REMOTE}"

if [ -n "$REMOTE_SRC" ]; then
  # Ensure lpc_plots/ exists
  mkdir -p "$WORKDIR/lpc_plots/"
  echo "Running rsync from '$REMOTE_SRC' -> '$WORKDIR/lpc_plots/' ..."
  rsync -aR --prune-empty-dirs "$REMOTE_SRC" "$WORKDIR/lpc_plots/"
  echo "rsync finished."
else
  echo "Skipping rsync (you can run rsync later manually)."
fi
echo

# Verify PlotsTemplate.key is present in the workdir (required)
if [ -f "$WORKDIR/PlotsTemplate.key" ]; then
  echo "Found PlotsTemplate.key in $WORKDIR"
else
  echo "ERROR: PlotsTemplate.key not found in $WORKDIR."
  echo "Please ensure you downloaded the entire keynote/ directory and that PlotsTemplate.key is present."
  echo "You can copy it into $WORKDIR now and re-run setup.sh if needed."
fi
echo

# Detect shell rc file
SHELL_RC="$HOME/.bash_profile"
if [ -n "$ZSH_VERSION" ] || [ -n "$ZSH_NAME" ]; then
    SHELL_RC="$HOME/.zshrc"
fi

# If the user provided a remote source, store it in env for Python
if [ -n "$REMOTE_SRC" ]; then
    echo "export KEYNOTE_RSYNC=\"$REMOTE_SRC\"" >> "$SHELL_RC"
    echo "Added KEYNOTE_RSYNC env variable to $SHELL_RC"
fi

ALIAS_CMD="alias make_keynote='cd \"$WORKDIR\" && export KEYNOTE_RSYNC=\"\$KEYNOTE_RSYNC\" && export KEYNOTE_BASE_DIR=\"\$PWD\" && python3 make_keynote.py'"
RSYNC_ALIAS_CMD="alias lpc_download_plots='rsync -aR --prune-empty-dirs \"\$KEYNOTE_RSYNC\" \"$WORKDIR\"/lpc_plots/'"

# Only append alias if not already present
if ! grep -Fqs "alias make_keynote=" "$SHELL_RC"; then
    echo "$ALIAS_CMD" >> "$SHELL_RC"
    echo "Added alias 'make_keynote' to $SHELL_RC"
    echo "You may need to run: source $SHELL_RC"
else
    echo "Alias 'make_keynote' already found in $SHELL_RC — not modifying it."
fi

if ! grep -Fqs "alias lpc_download_plots=" "$SHELL_RC"; then
    echo "$RSYNC_ALIAS_CMD" >> "$SHELL_RC"
    echo "Added alias 'lpc_download_plots' to $SHELL_RC"
    echo "You may need to run: source $SHELL_RC"
else
    echo "Alias 'lpc_download_plots' already found in $SHELL_RC — not modifying it."
fi

echo
echo "Setup complete."
echo "Next steps:"
echo "  1) Open a new terminal or run: source $SHELL_RC"
echo "  2) Run: make_keynote"
echo "     (This will cd into $WORKDIR, set KEYNOTE_BASE_DIR, and run make_keynote.py)"
