#!/usr/bin/env python3
# zedBSD
# Copyright (C) 2026 Awe Morris
# SPDX-License-Identifier: Zlib
"""Runs browser's own DOM and page-script tests (plan/ws074/tests/dom/*.html) and compares their consoles.

  run-dom-tests.py [--program PATH]            run each page with --run and compare with NAME.expected
  run-dom-tests.py --reference                 make NAME.expected with the host's headless Chromium
  run-dom-tests.py --outputs DIR               compare outputs made elsewhere (DIR/NAME.out, e.g. the guest's)

A test page writes lines with console.log (and its uncaught errors become "Uncaught ..." lines).  The expected
output is the console of Chromium for the same page, with timers run on a virtual clock of 5000 ms as
browser's headless modes run them, so the reference is another engine, not this one.  A page uses only
what browser has so far (ES5 and the DOM of ws074-p030) and messages whose text every engine writes
alike (no engine-specific error messages).
"""

import argparse
import glob
import os
import re
import subprocess
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "../../.."))
TESTS = os.path.join(ROOT, "plan/ws074/tests/dom")
CONSOLE = re.compile(r':CONSOLE(?:\(\d+\)|:\d+)\] "(.*)", source: ')


def fonts():
    # Geometry needs the same comparison fonts on hosts without the guest font paths.
    # Use staged artifacts or their checked-in sources; missing fonts must not silently
    # turn every geometry answer into zero.
    names = ("Inter.ttf", "JetBrainsMono-Regular.ttf", "DroidSansFallbackFull.ttf")
    for directory in ("build/ws035-fonts", "userland/desktop/fonts"):
        paths = [os.path.join(ROOT, directory, name) for name in names]
        if all(os.path.isfile(path) for path in paths):
            return ["--font=" + paths[0], "--mono-font=" + paths[1],
                    "--fallback-font=" + paths[2]]
    raise RuntimeError("the DOM comparison fonts were not found")


def tests():
    return sorted(glob.glob(os.path.join(TESTS, "*.html")))


def reference():
    profile = os.path.join(ROOT, "build/ws074-chrome/profile")
    os.makedirs(profile, exist_ok=True)
    for path in tests():
        name = os.path.splitext(os.path.basename(path))[0]
        run = subprocess.run(["chromium", "--headless", "--no-sandbox", "--disable-gpu", "--allow-file-access-from-files",
                              "--user-data-dir=" + profile, "--enable-logging=stderr", "--v=0",
                              "--virtual-time-budget=5000", "--dump-dom", "file://" + path],
                             stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True, errors="replace", timeout=120)
        lines = []
        for line in run.stderr.splitlines():
            match = CONSOLE.search(line)
            if match:
                lines.append(match.group(1))
        if not lines:
            print("%s: no console from Chromium" % name)
            return 1
        with open(os.path.join(TESTS, name + ".expected"), "w") as stream:
            stream.write("\n".join(lines) + "\n")
        print("%s: %d lines" % (name, len(lines)))
    return 0


def compare(name, output):
    with open(os.path.join(TESTS, name + ".expected")) as stream:
        expected = stream.read()
    if output == expected:
        print("pass %s" % name)
        return True
    expected_lines = expected.splitlines()
    output_lines = output.splitlines()
    print("FAIL %s" % name)
    for index in range(max(len(expected_lines), len(output_lines))):
        want = expected_lines[index] if index < len(expected_lines) else "(nothing)"
        got = output_lines[index] if index < len(output_lines) else "(nothing)"
        if want != got:
            print("  line %d: expected %s\n  line %d: got      %s" % (index + 1, want, index + 1, got))
    return False


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--program", default=os.path.join(ROOT, "build/ws074-host/plain/browser"))
    parser.add_argument("--reference", action="store_true")
    parser.add_argument("--outputs")
    args = parser.parse_args()
    if args.reference:
        return reference()
    passed = 0
    for path in tests():
        name = os.path.splitext(os.path.basename(path))[0]
        if args.outputs:
            with open(os.path.join(args.outputs, name + ".out"), errors="replace") as stream:
                output = stream.read()
        else:
            # ws074-p080: localStorage is kept under XDG_DATA_HOME; the tests keep theirs apart from the user's.
            environment = dict(os.environ, XDG_DATA_HOME=os.path.join(ROOT, "build/ws074-dom-tests/data"))
            run = subprocess.run([args.program, "--run"] + fonts() + [path], stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE, text=True, errors="replace", timeout=120, env=environment)
            output = run.stdout
            if run.returncode != 0:
                output += "(exit status %d: %s)\n" % (run.returncode, run.stderr.strip())
        # The place of an uncaught exception ("(at LINE:COLUMN)") is ours; Chromium's console writes it apart.
        output = re.sub(r" \(at (?:.*:)?\d+:\d+\)$", "", output, flags=re.M)
        if compare(name, output):
            passed += 1
    print("dom-tests %d/%d" % (passed, len(tests())))
    return 0 if passed == len(tests()) else 1


if __name__ == "__main__":
    sys.exit(main())
