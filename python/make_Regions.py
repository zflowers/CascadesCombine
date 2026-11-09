#!/usr/bin/env python3
"""Generate combined and control-region YAMLs from Derek's master regions file."""

import argparse
import re
from collections.abc import Mapping
from pathlib import Path

from ruamel.yaml import YAML
from ruamel.yaml.comments import CommentedMap
from ruamel.yaml.scalarstring import LiteralScalarString


DEFAULT_MASTER = Path("config/bin_cfgs/derek_master_Regions.yaml")
DEFAULT_OUTDIR = Path("config/bin_cfgs")
BIN_NAME_RE = re.compile(r"^Bin_(Run[23])_([234])L_(Gold|Silver|Bronze)_(.+)$")
REQUIRED_FIELDS = {"cuts", "lep-cuts", "predefined-cuts", "user-cuts"}


def _values(text, expression):
    return [float(value) for value in re.findall(expression, text)]


def validate_bin(name, entry):
    match = BIN_NAME_RE.fullmatch(name)
    if not match:
        raise ValueError(f"Unexpected bin name: {name}")

    _, multiplicity, _, axes = match.groups()
    multiplicity = int(multiplicity)
    cuts = entry["cuts"]
    nlep = {
        (operator, float(value))
        for operator, value in re.findall(
            r"(?:^|;)Nlep\s*(==|>=|<)\s*(\d+(?:\.\d+)?)", cuts
        )
    }

    if multiplicity in (2, 4):
        valid_nlep = ("==", float(multiplicity)) in nlep
    else:
        valid_nlep = ("==", 3.0) in nlep or (
            (">=", 3.0) in nlep and ("<", 5.0) in nlep
        )
    if not valid_nlep:
        raise ValueError(f"{name}: lepton multiplicity disagrees with cuts")

    pt_match = re.search(r"(?:^|_)P(\d+)(?:_|$)", axes)
    if pt_match and float(pt_match.group(1)) not in _values(
        cuts, r"(?:^|;)PTISR(?:_LEP)?\s*>=\s*(\d+(?:\.\d+)?)"
    ):
        raise ValueError(f"{name}: PTISR name token disagrees with cuts")

    jet_match = re.search(r"(?:^|_)(0J|1J)(?:_|$)", axes)
    if jet_match:
        if jet_match.group(1) == "0J":
            valid_jet = bool(
                re.search(r"(?:^|;)Njet_S\s*==\s*0(?:\.0+)?(?:;|$)", cuts)
            )
        else:
            valid_jet = bool(
                re.search(r"(?:^|;)Njet_S\s*>=\s*1(?:\.0+)?(?:;|$)", cuts)
            )
        if not valid_jet:
            raise ValueError(f"{name}: jet name token disagrees with cuts")

    r_match = re.search(r"(?:^|_)R(\d+)(?:_|$)", axes)
    if r_match:
        lower_edge = float(f"0.{r_match.group(1)}")
        r_edges = _values(
            cuts, r"(?:^|;)RISR(?:_LEP)?\s*>=\s*(\d+(?:\.\d+)?)"
        )
        if lower_edge not in r_edges:
            raise ValueError(f"{name}: RISR name token disagrees with cuts")

    mass_match = re.search(r"(?:^|_)(M(?:lt\d+|\d+(?:_\d+)?))(?:_|$)", axes)
    if mass_match:
        mass = mass_match.group(1)
        if mass.startswith("Mlt"):
            valid_mass = bool(
                re.search(rf"(?:^|;)Mperp\s*<\s*{mass[3:]}(?:\.0+)?;", cuts)
            )
        elif "_" in mass:
            low, high = mass[1:].split("_")
            valid_mass = bool(
                re.search(rf"(?:^|;)Mperp\s*>=\s*{low}(?:\.0+)?;", cuts)
                and re.search(rf"(?:^|;)Mperp\s*<\s*{high}(?:\.0+)?;", cuts)
            )
        else:
            valid_mass = bool(
                re.search(rf"(?:^|;)Mperp\s*>=\s*{mass[1:]}(?:\.0+)?;", cuts)
            )
        if not valid_mass:
            raise ValueError(f"{name}: Mperp name token disagrees with cuts")

    energy_match = re.search(
        r"(?:^|_)(E(?:lt)?\d+p\d+(?:_\d+p\d+)?)(?:_|$)", axes
    )
    if energy_match:
        energy = energy_match.group(1)
        if energy.startswith("Elt"):
            threshold = energy[3:].replace("p", ".")
            valid_energy = bool(
                re.search(
                    rf"(?:^|;)lephad_E_ratio\s*<\s*{re.escape(threshold)};", cuts
                )
            )
        elif "_" in energy:
            low, high = energy[1:].split("_")
            low = low.replace("p", ".")
            high = high.replace("p", ".")
            valid_energy = bool(
                re.search(
                    rf"(?:^|;)lephad_E_ratio\s*>=\s*{re.escape(low)};", cuts
                )
                and re.search(
                    rf"(?:^|;)lephad_E_ratio\s*<\s*{re.escape(high)};", cuts
                )
            )
        else:
            threshold = energy[1:].replace("p", ".")
            valid_energy = bool(
                re.search(
                    rf"(?:^|;)lephad_E_ratio\s*>=\s*{re.escape(threshold)};",
                    cuts,
                )
            )
        if not valid_energy:
            raise ValueError(f"{name}: energy-ratio name token disagrees with cuts")

    if "_4L_" in name and name.endswith(("_22", "_31")):
        expected = 2 if name.endswith("_22") else 1
        if not re.search(
            rf"(?:^|;)Nlep_b(?:_LEP)?\s*==\s*{expected}(?:\.0+)?(?:;|$)",
            cuts,
        ):
            raise ValueError(f"{name}: 4L suffix disagrees with Nlep_b cut")

    if "Btag" in axes and not re.search(
        r"(?:^|;)Nbjet\s*(?:>=\s*1(?:\.0+)?|>\s*0(?:\.0+)?)(?:;|$)",
        cuts,
    ):
        raise ValueError(f"{name}: Btag name token disagrees with cuts")


def load_master(path):
    yaml = YAML(typ="rt")
    yaml.allow_duplicate_keys = False
    yaml.preserve_quotes = True
    with path.open(encoding="utf-8") as source:
        master = yaml.load(source)

    if not isinstance(master, Mapping) or not master:
        raise ValueError(f"{path}: expected a non-empty mapping of bins")

    combinations = set()
    for name, entry in master.items():
        if not isinstance(name, str):
            raise ValueError(f"{path}: bin names must be strings")
        if not isinstance(entry, Mapping):
            raise ValueError(f"{name}: expected a mapping of bin fields")
        missing = REQUIRED_FIELDS - set(entry)
        extra = set(entry) - REQUIRED_FIELDS
        if missing or extra:
            raise ValueError(
                f"{name}: field mismatch; missing={sorted(missing)}, "
                f"unexpected={sorted(extra)}"
            )
        if any(not isinstance(entry[field], str) for field in REQUIRED_FIELDS):
            raise ValueError(f"{name}: all standard bin fields must be strings")

        match = BIN_NAME_RE.fullmatch(name)
        if not match:
            raise ValueError(f"Unexpected bin name: {name}")
        era, multiplicity, tier, _ = match.groups()
        combinations.add((era, multiplicity, tier))
        validate_bin(name, entry)

    expected = {
        (era, multiplicity, tier)
        for era in ("Run2", "Run3")
        for multiplicity in ("2", "3", "4")
        for tier in ("Gold", "Silver", "Bronze")
    }
    if combinations != expected:
        raise ValueError(
            f"{path}: incomplete bin families; "
            f"missing={sorted(expected - combinations)}, "
            f"unexpected={sorted(combinations - expected)}"
        )
    return yaml, master


def select_bins(master, predicate):
    return {name: entry for name, entry in master.items() if predicate(name)}


def write_regions(yaml, path, bins):
    formatted_bins = CommentedMap()
    formatted_bins.fa.set_block_style()
    for name, entry in bins.items():
        formatted_entry = CommentedMap()
        formatted_entry.fa.set_block_style()
        for field, value in entry.items():
            if field == "lep-cuts":
                cuts = [cut.strip() for cut in value.split(";") if cut.strip()]
                formatted_entry[str(field)] = LiteralScalarString(
                    "\n".join(f"{cut};" for cut in cuts) + "\n"
                )
            else:
                formatted_entry[str(field)] = value
        formatted_bins[str(name)] = formatted_entry

    yaml.indent(mapping=2, sequence=4, offset=2)
    yaml.default_flow_style = False
    with path.open("w", encoding="utf-8") as output:
        yaml.dump(formatted_bins, output)


def main():
    parser = argparse.ArgumentParser(
        description="Build combined and CR Regions YAMLs from a canonical master."
    )
    parser.add_argument("--master", type=Path, default=DEFAULT_MASTER)
    parser.add_argument("--outdir", type=Path, default=DEFAULT_OUTDIR)
    parser.add_argument(
        "--dry-run", action="store_true", help="validate and report without writing"
    )
    args = parser.parse_args()

    yaml, master = load_master(args.master)
    outputs = {
        "Regions.yaml": master,
        "Regions_Run2.yaml": select_bins(
            master, lambda key: key.startswith("Bin_Run2_")
        ),
        "Regions_Run3.yaml": select_bins(
            master, lambda key: key.startswith("Bin_Run3_")
        ),
        "Regions_CR_Run2.yaml": select_bins(
            master,
            lambda key: key.startswith("Bin_Run2_") and "_Gold_" not in key,
        ),
        "Regions_CR_Run3.yaml": select_bins(
            master,
            lambda key: key.startswith("Bin_Run3_") and "_Gold_" not in key,
        ),
        "Regions_CR.yaml": select_bins(master, lambda key: "_Gold_" not in key),
    }

    if not args.dry_run:
        args.outdir.mkdir(parents=True, exist_ok=True)
    for filename, bins in outputs.items():
        target = args.outdir / filename
        if args.dry_run:
            print(f"[DRY-RUN] {target}: {len(bins)} bins")
        else:
            write_regions(yaml, target, bins)
            print(f"WROTE: {target} ({len(bins)} bins)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
