#!/usr/bin/env python3
# zedBSD
# Copyright (C) 2026 Awe Morris
# SPDX-License-Identifier: Zlib
"""Builds and runs the explicit cross-realm investigation probe.

Run host-build.sh for the chosen variant first. Exit 1 preserves assertion
failures; the q510 baseline is 3/8 in both variants, rather than a passing test.
"""

import argparse
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]


def run(variant, out):
    base = Path(os.environ.get("BROWSER_HOST_BUILD", "build/ws074-host"))
    folder = ROOT / base / variant
    sources = subprocess.check_output(
        ["sh", "plan/ws074/tests/list-sources.sh"], cwd=ROOT, text=True
    ).splitlines()
    header_time = max(p.stat().st_mtime for p in (ROOT / "userland/desktop/libbrowser").rglob("*.h"))
    objects = []
    for source in sources:
        if "/shell/" in source or source.endswith("/main.c"):
            continue
        name = source.removeprefix("userland/desktop/").removesuffix(".c").replace("/", "_")
        obj = folder / "obj" / (name + ".o")
        if not obj.is_file() or obj.stat().st_mtime < max((ROOT / source).stat().st_mtime, header_time):
            raise RuntimeError("run host-build.sh " + variant + " first (missing/stale " + str(obj) + ")")
        objects.append(str(obj))
    objects.extend(str(p) for p in sorted((folder / "obj").glob("truetype-*.o")))
    objects.extend(str(p) for p in sorted((folder / "obj").glob("lib*-compat-*.o")))
    objects.append(str(folder / "obj/host-shell.o"))
    program = out / ("realm-probe-" + variant)
    command = [os.environ.get("CC", "cc"), "-std=gnu11", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
               "-D_GNU_SOURCE", "-Iuserland/desktop/libbrowser", "-Iplan/ws074/tests", "-I" + str(ROOT / base / "include")]
    if variant == "asan":
        command.extend(["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-sanitize-recover=undefined"])
    command.extend(["plan/ws074/tests/realm-context-probe.c", "-o", str(program), *objects, "-lvulkan", "-lm"])
    subprocess.run(command, cwd=ROOT, check=True)
    environment = dict(os.environ)
    if variant == "asan":
        environment["ASAN_OPTIONS"] = "detect_stack_use_after_return=0"
    result = subprocess.run([str(program)], cwd=ROOT, env=environment, text=True, capture_output=True, timeout=30)
    (out / ("realm-" + variant + ".txt")).write_text(result.stdout)
    (out / ("realm-" + variant + ".err")).write_text(result.stderr)
    print(variant + ": exit " + str(result.returncode))
    print(result.stdout, end="")
    print(result.stderr, end="", file=sys.stderr)
    return result.returncode


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--variant", choices=("plain", "asan", "both"), default="both")
    parser.add_argument("--out", type=Path, default=ROOT / "build/ws074-p104")
    args = parser.parse_args()
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    variants = ("plain", "asan") if args.variant == "both" else (args.variant,)
    results = [run(variant, out) for variant in variants]
    return 1 if any(results) else 0


if __name__ == "__main__":
    sys.exit(main())
