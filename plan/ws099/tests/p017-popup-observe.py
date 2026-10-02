#!/usr/bin/env python3
"""Observe the original p076 popup verdict and bounded subsequent frames.

Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib

This diagnosis runs original stages 1–4 only. It retains any original failure,
even if a later capture matches, and changes no shared regression expectation.
The existing P8 single guest must already be running. Each diagnosis gets a
separate output directory. It does not count toward the 20 whole-p076 criteria.
"""
import argparse
import os
import shlex
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
SOURCE = ROOT / "plan/ws035/tests/zdesktop-p076.sh"

OBSERVER = r'''# Saves the first verdict, then observes six subsequent pictures without clearing it.
check() {
	python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"
	first_status=$?
	if [ "$first_status" -eq 0 ]; then
		return 0
	fi
	observed_png=$1
	shift
	observation=1
	while [ "$observation" -le 6 ]; do
		sleep 0.5
		python3 plan/ws035/tests/zdesktop-check.py "${observed_png%.png}-later-$observation.png" "$@" --runtime "$GUEST_RUNTIME"
		later_status=$?
		echo "observation: original=$first_status later=$later_status poll=$observation"
		guest 'cat /tmp/zdesktop.log' > "${observed_png%.png}-later-$observation.log"
		observation=$((observation + 1))
	done
	return "$first_status"
}'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output")
    arguments = parser.parse_args()
    output = Path(arguments.output).resolve()
    output.mkdir(parents=True, exist_ok=True)
    source = SOURCE.read_text()
    marker = "# 5. A press on the desktop closes the popups"
    if source.count(marker) != 1:
        raise RuntimeError("Expected p076 stage boundary is missing or ambiguous")
    source = source.split(marker, 1)[0]
    replacements = {
        'cd "$(dirname -- "$0")/../../.."': 'cd ' + shlex.quote(str(ROOT)),
        'check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }': OBSERVER,
        '/bin/wayland --timeout=400': '/bin/wayland --log-frames --timeout=400',
        "finish() {": "finish() {\n\tguest 'cat /tmp/zdesktop.log' > \"$out/frames.log\"",
    }
    for before, after in replacements.items():
        if source.count(before) != 1:
            raise RuntimeError("Expected p076 helper is missing or ambiguous: " + before)
        source = source.replace(before, after, 1)
    source += "# Keeps the original popup verdict and full logs.\nfinish\n"
    runner = output / "observe.sh"
    runner.write_text(source)
    subprocess.run(["sh", "-n", str(runner)], check=True)
    environment = os.environ.copy()
    with (output / "verdict.txt").open("w") as log:
        status = subprocess.run(["sh", str(runner), str(output)], cwd=ROOT,
                                env=environment, stdout=log, stderr=subprocess.STDOUT).returncode
    print("popup observation: " + str(output) + " exit=" + str(status))
    return status


if __name__ == "__main__":
    sys.exit(main())
