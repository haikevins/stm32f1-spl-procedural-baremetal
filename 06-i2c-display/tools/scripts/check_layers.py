#!/usr/bin/env python3
"""Reject source-level dependencies that violate the project layer rules."""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

LAYER_DIRS = {
    "app": ROOT / "app",
    "services": ROOT / "services",
    "ecual": ROOT / "ecual",
    "bsp": ROOT / "bsp",
    "common": ROOT / "common",
    "system": ROOT / "system",
    "platform": ROOT / "platform",
    "runtime": ROOT / "runtime",
}

ALLOWED = {
    "app": {"app", "services", "common", "config"},
    "services": {"services", "bsp", "ecual", "common", "config"},
    "ecual": {"ecual", "bsp", "common", "config"},
    "bsp": {"bsp", "common", "vendor", "config"},
    "common": {"common", "config"},
    "platform": {"platform", "common", "vendor", "config"},
    "runtime": {"runtime", "common", "vendor", "config"},
    # Composition root: may connect all runtime layers.
    "system": {
        "app", "services", "ecual", "bsp", "common",
        "system", "platform", "runtime", "vendor", "config",
    },
}

INCLUDE_RE = re.compile(r'^\s*#\s*include\s*[<"]([^">]+)[">]', re.MULTILINE)

VENDOR_HEADERS = {
    "core_cm3.h",
    "misc.h",
    "stm32f10x.h",
    "system_stm32f10x.h",
}

def collect_header_owners() -> dict[str, set[str]]:
    owners: dict[str, set[str]] = {}

    for layer, directory in LAYER_DIRS.items():
        if not directory.exists():
            continue

        for header in directory.rglob("*.h"):
            owners.setdefault(header.name, set()).add(layer)

    config_dir = ROOT / "config"
    if config_dir.exists():
        for header in config_dir.rglob("*.h"):
            owners.setdefault(header.name, set()).add("config")

    third_party = ROOT / "third_party"
    if third_party.exists():
        for header in third_party.rglob("*.h"):
            owners.setdefault(header.name, set()).add("vendor")

    return owners

def classify_include(include: str, owners: dict[str, set[str]]) -> set[str]:
    basename = Path(include).name

    if basename in owners:
        return owners[basename]

    if (
        basename in VENDOR_HEADERS
        or basename.startswith("stm32f10x_")
        or basename.startswith("core_cm")
    ):
        return {"vendor"}

    # Standard-library or toolchain header.
    return set()

def main() -> int:
    owners = collect_header_owners()
    errors: list[str] = []

    for source_layer, directory in LAYER_DIRS.items():
        if not directory.exists():
            continue

        for source in sorted(directory.rglob("*")):
            if source.suffix not in {".c", ".h"}:
                continue

            text = source.read_text(encoding="utf-8", errors="replace")

            for include in INCLUDE_RE.findall(text):
                targets = classify_include(include, owners)

                for target in targets:
                    if target not in ALLOWED[source_layer]:
                        relative = source.relative_to(ROOT)
                        errors.append(
                            f"{relative}: layer '{source_layer}' may not include "
                            f"'{include}' from layer '{target}'"
                        )

    if errors:
        print("Layer dependency check failed:", file=sys.stderr)
        for error in errors:
            print(f"  - {error}", file=sys.stderr)
        return 1

    print("Layer dependency check passed.")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
