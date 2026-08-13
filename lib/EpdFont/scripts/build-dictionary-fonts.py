#!/usr/bin/env python3
"""Build dictionary SD-card fonts with broad IPA coverage.

This is a dictionary-specific wrapper around ``build-sd-fonts.py``. It keeps
the family/style/size catalog in ``sd-fonts.yaml`` while replacing each
family's interval list with the reading ranges plus IPA and combining-mark
ranges needed by dictionary definitions. The shared builder supplies
CrossSmudge's built-in intervals and fallback stack, with bundled Noto Sans
remaining the final fallback for every style.

The default output is the sibling crosssmudge-fonts repository:
``../crosssmudge-fonts/dictionary-fonts``.

Usage:
    python3 build-dictionary-fonts.py
    python3 build-dictionary-fonts.py --clean --jobs 2
    python3 build-dictionary-fonts.py --only GentiumBookPlus,ChareInk
    python3 build-dictionary-fonts.py --output-dir /tmp/dictionary-fonts
"""

from __future__ import annotations

import argparse
import copy
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

import yaml


SCRIPT_DIR = Path(__file__).resolve().parent
SHARED_BUILDER = SCRIPT_DIR / "build-sd-fonts.py"
DEFAULT_CONFIG = SCRIPT_DIR / "sd-fonts.yaml"
PROJECT_ROOT = SCRIPT_DIR.parents[2]
DEFAULT_OUTPUT = PROJECT_ROOT.parent / "crosssmudge-fonts" / "dictionary-fonts"
FALLBACK_FONT = PROJECT_ROOT / "lib/EpdFont/builtinFonts/source/NotoSans/NotoSans-Regular.ttf"

# ``build-sd-fonts.py`` appends its ``builtin`` preset to this list. The
# reading preset supplies the same broad reader ranges used by the built-in
# fonts, while these explicit ranges add dictionary IPA and mark coverage.
DICTIONARY_INTERVALS = (
    "reading,(0x0250-0x02FF),(0x1D00-0x1DBF),"
    "(0x1DC0-0x1DFF),(0x20D0-0x20FF),(0xFE20-0xFE2F)"
)


def load_dictionary_config(config_path: Path) -> tuple[dict, list[dict]]:
    """Load the catalog and replace family intervals for dictionary output."""
    with config_path.open() as config_file:
        config = yaml.safe_load(config_file)

    if not isinstance(config, dict) or not config.get("families"):
        raise ValueError(f"No families defined in {config_path}")

    dictionary_config = copy.deepcopy(config)
    for family in dictionary_config["families"]:
        family["intervals"] = DICTIONARY_INTERVALS
    return dictionary_config, dictionary_config["families"]


def write_temporary_config(config: dict) -> Path:
    """Write a temporary transformed catalog for the shared builder."""
    temp_file = tempfile.NamedTemporaryFile(
        mode="w", prefix="crosssmudge-dictionary-fonts-", suffix=".yaml", delete=False
    )
    try:
        yaml.safe_dump(config, temp_file, sort_keys=False)
    finally:
        temp_file.close()
    return Path(temp_file.name)


def package_family(output_dir: Path, family: dict) -> Path:
    """Create ``<Family>.zip`` containing the matching ``<Family>/`` folder."""
    family_name = family["name"]
    family_dir = output_dir / family_name
    if not family_dir.is_dir():
        raise FileNotFoundError(f"Expected generated family directory: {family_dir}")

    expected_files = len(family.get("sizes", []))
    generated_files = list(family_dir.glob("*.cpfont"))
    if len(generated_files) != expected_files:
        raise RuntimeError(
            f"{family_name}: expected {expected_files} .cpfont files, "
            f"found {len(generated_files)}; rerun with --clean to remove stale output"
        )

    archive_base = output_dir / family_name
    archive_path = Path(
        shutil.make_archive(
            str(archive_base), "zip", root_dir=str(output_dir), base_dir=family_name
        )
    )
    return archive_path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Build dictionary SD-card fonts and package each family as a ZIP"
    )
    parser.add_argument(
        "--config", default=str(DEFAULT_CONFIG), help="Font catalog YAML (default: sd-fonts.yaml)"
    )
    parser.add_argument(
        "--output-dir",
        default=str(DEFAULT_OUTPUT),
        help="Output directory (default: sibling crosssmudge-fonts/dictionary-fonts)",
    )
    parser.add_argument("--only", help="Comma-separated family names to build")
    parser.add_argument("--jobs", "-j", type=int, default=None, help="Maximum parallel families")
    parser.add_argument("--timeout", type=int, default=600, help="Per-family timeout in seconds")
    parser.add_argument("--clean", action="store_true", help="Clean the output directory before building")
    parser.add_argument(
        "--verbose", "-v", action="store_true", help="Stream child font-converter output"
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    config_path = Path(args.config).expanduser().resolve()
    output_dir = Path(args.output_dir).expanduser().resolve()

    if not SHARED_BUILDER.is_file():
        print(f"ERROR: shared builder not found: {SHARED_BUILDER}", file=sys.stderr)
        return 1
    if not FALLBACK_FONT.is_file():
        print(f"ERROR: Noto Sans fallback not found: {FALLBACK_FONT}", file=sys.stderr)
        return 1
    if not config_path.is_file():
        print(f"ERROR: config not found: {config_path}", file=sys.stderr)
        return 1

    try:
        transformed_config, families = load_dictionary_config(config_path)
    except (OSError, ValueError, yaml.YAMLError) as error:
        print(f"ERROR: unable to load dictionary config: {error}", file=sys.stderr)
        return 1

    if args.only:
        requested = {name.strip() for name in args.only.split(",") if name.strip()}
        families = [family for family in families if family.get("name") in requested]
        if not families:
            print(f"ERROR: no matching families for --only {args.only}", file=sys.stderr)
            return 1
        transformed_config["families"] = families

    temp_config = write_temporary_config(transformed_config)
    command = [
        sys.executable,
        str(SHARED_BUILDER),
        "--config",
        str(temp_config),
        "--output-dir",
        str(output_dir),
        "--timeout",
        str(args.timeout),
        "--no-package",
    ]
    if args.only:
        command.extend(["--only", args.only])
    if args.jobs is not None:
        command.extend(["--jobs", str(args.jobs)])
    if args.clean:
        command.append("--clean")
    if args.verbose:
        command.append("--verbose")

    print(f"Dictionary intervals: {DICTIONARY_INTERVALS},builtin")
    print(f"Noto Sans fallback: {FALLBACK_FONT}")
    try:
        result = subprocess.run(command, check=False)
    finally:
        temp_config.unlink(missing_ok=True)

    if result.returncode != 0:
        return result.returncode

    print("\n=== Packaging dictionary font ZIPs ===\n")
    packaged_archives = []
    try:
        for family in families:
            archive = package_family(output_dir, family)
            packaged_archives.append(archive)
            print(f"  ZIP: {archive}")
    except (FileNotFoundError, OSError, RuntimeError) as error:
        print(f"ERROR: packaging failed: {error}", file=sys.stderr)
        return 1

    total_files = sum(
        len(list((output_dir / family["name"]).glob("*.cpfont"))) for family in families
    )
    print(
        f"\nPackaged {total_files} .cpfont files and {len(packaged_archives)} ZIPs "
        f"in {output_dir}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
