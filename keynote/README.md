# Cascades Keynote Automation

This repository provides tools to automatically generate Keynote slides from physics plots.  
It is designed so that once you have the setup done, you only need to run a single command to update slides with new plots.
Note this was developed using Keynote version 12.1 and OS X 12.7.6. An earlier version of this has been used on a newer machine.
For non Apple users, visit [this link](https://www.apple.com)

---

## Quick Start

0. If for some reason you have not already, do the kinit command as if you were normally going to login to the LPC

1. **Download the keynote directory**  
   Copy the `keynote/` folder from this repository (contains `PlotTemplate.key`, `setup.sh`, and `make_keynote.py`) to your working machine.  
   Example using `rsync`:
   ```bash
   rsync -avz $USER@cmslpc-el9.fnal.gov:/uscms/home/$USER/nobackup/CascadesCombine/keynote/* .
   ```

2. **Run setup**  
   ```bash
   bash setup.sh
   ```

   This creates a local working directory at `~/Cascades_keynote/` (and `cd`s you there).

3. **Generate slides**  
   After plots are produced on LPC and synced locally:
   ```bash
   make_keynote
   ```

4. **Repeat**  
   Anytime you update plots, just re-run:
   ```bash
   make_keynote
   ```

---

## Expected Directory Structure

Plots should be organized on LPC under:

```
plots/
  ├── [bin]/
  │     ├── [process]/
  │     │      ├── can_[process]_[var].pdf
  │     │      └── ...
  │     └── ...
  └── ...
```

- `[bin]`: Physics bin name (e.g., `SuperBin2L`, `SuperBin3L`).
- `[process]`: Process name (e.g., `ttbar`, `Wjets`, `Cascades_220`).
- `[var]`: Variable name used in plots.

The script automatically groups plots by variable across processes.

---

## Plot Template

- `PlotTemplate.key` is required and included in `keynote/`.
- All generated slides will be based on this template.

---

## Configuration

Two lists are hardcoded in `make_keynote.py`:

- `prefix_order`: Controls ordering of process plots.
- `ignore_bins`: Any bin in this list will be skipped. Useful because some "bins" are actually processes (grouping multiple bins)

---

## Example Workflow

```bash
# Step 1: Copy keynote directory
rsync -avz $USER@cmslpc-el9.fnal.gov:/uscms/home/$USER/nobackup/CMSSW_14_1_0_pre4/src/CascadesCombine/keynote/* .

# Step 2: Setup working directory
bash setup.sh

# Step 3: Sync plots from LPC
rsync -avz $USER@cmslpc-el9.fnal.gov:/uscms/home/$USER/nobackup/CMSSW_14_1_0_pre4/src/CascadesCombine/plots/* lpc_plots/

# Step 4: Generate slides
make_keynote
```

This will produce updated slides in `~/Cascades_keynote/`.

---

## Notes

- `make_keynote` is safe to run multiple times — it will add on slides to existing keynote.
- You can copy the keynote file and rename to help keep things organized, or remove existing added slides to update.

