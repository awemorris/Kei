#!/usr/bin/env python3
"""Observe original popup stages with shell clocks and no pre-capture SSH snapshots.

Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib

This q589 diagnostic preserves the shared p076 pointer and pixel checker calls.
Shell builtins read /proc/uptime around input and capture, at about 10-ms
resolution. Guest logs are collected only after first verdicts and at completion.
Later captures never replace an original failure. One owned guest must be ready.
"""
import argparse
import os
from pathlib import Path
import shlex
import signal
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[3]
SOURCE = ROOT / "plan/ws035/tests/zdesktop-p076.sh"

# Timestamping uses shell builtins without adding a Python or SSH process.
STAMP = r'''stamp() {
    read p017_uptime p017_idle < /proc/uptime
    printf '%s %s\n' "$p017_uptime" "$*" >> "$out/timeline.txt"
}'''

# Post-verdict logs can affect following steps, but cannot settle the first capture.
CHECK = r'''check() {
    stamp capture_begin "$@"
    python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"
    first_status=$?
    stamp capture_end "$first_status" "$1"
    picture_stem=${1%.png}
    stamp first_log_begin "$1"
    timeout 10 python3 plan/tools/guest/guest.py run 'cat /tmp/zdesktop.log; echo P017_PROBE_BOUNDARY; cat /tmp/p.log' > "$picture_stem-first.log" 2>&1
    log_status=$?
    stamp first_log_end "$log_status" "$1"
    if [ "$first_status" -eq 0 ]; then
        return 0
    fi
    observed_png=$1
    shift
    observation=1
    while [ "$observation" -le 6 ]; do
        sleep 0.5
        stamp later_capture_begin "$observation" "$observed_png"
        python3 plan/ws035/tests/zdesktop-check.py "${observed_png%.png}-later-$observation.png" "$@" --runtime "$GUEST_RUNTIME"
        later_status=$?
        stamp later_capture_end "$later_status" "$observation" "$observed_png"
        printf 'observation: original=%s later=%s poll=%s\n' "$first_status" "$later_status" "$observation"
        timeout 10 python3 plan/tools/guest/guest.py run 'cat /tmp/zdesktop.log; echo P017_PROBE_BOUNDARY; cat /tmp/p.log' > "${observed_png%.png}-later-$observation.log" 2>&1
        log_status=$?
        stamp later_log_end "$log_status" "$observation" "$observed_png"
        observation=$((observation + 1))
    done
    return "$first_status"
}'''

POINTER = r'''pointer() {
    stamp pointer_begin "$@"
    python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"
    pointer_status=$?
    stamp pointer_end "$pointer_status"
    return "$pointer_status"
}'''


def replace_once(source, before, after):
    """Reject changed shared boundaries rather than guessing at another procedure."""
    if source.count(before) != 1:
        raise RuntimeError("Missing or ambiguous shared boundary: " + before)
    return source.replace(before, after, 1)


def prepare(output):
    """Copy original stages and checker arguments without any readiness variant."""
    source = SOURCE.read_text()
    boundary = "# 5. A press on the desktop closes the popups"
    if source.count(boundary) != 1:
        raise RuntimeError("Missing or ambiguous p076 stage boundary")
    source = source.split(boundary, 1)[0]
    source = replace_once(source, 'cd "$(dirname -- "$0")/../../.."',
                          "cd " + shlex.quote(str(ROOT)))
    source = replace_once(source,
                          'check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }',
                          STAMP + "\n\n" + CHECK)
    source = replace_once(source,
                          'pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }',
                          POINTER)
    source = replace_once(source, '/bin/wayland --timeout=400',
                          '/bin/wayland --log-frames --timeout=400')
    source = replace_once(source, "finish() {",
                          'finish() {\n\tguest \'cat /tmp/zdesktop.log\' > "$out/frames.log"')
    source += "# Retains the original first-capture verdict and complete final logs.\nfinish\n"
    runner = output / "observe.sh"
    runner.write_text(source)
    subprocess.run(["sh", "-n", str(runner)], check=True)
    return runner


def run(output):
    """Terminate only this run's host shell children when its 240-second limit ends."""
    runner = prepare(output)
    started = time.monotonic()
    with (output / "verdict.txt").open("w") as log:
        process = subprocess.Popen(["sh", str(runner), str(output)], cwd=ROOT,
                                   stdout=log, stderr=subprocess.STDOUT,
                                   start_new_session=True)
        try:
            status = process.wait(timeout=240)
        except subprocess.TimeoutExpired:
            os.killpg(process.pid, signal.SIGTERM)
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                process.wait()
            status = 124
            log.write("\np017 low-overhead: whole-run deadline reached; no PASS\n")
    print("popup low-overhead: status=%d seconds=%.3f output=%s" %
          (status, time.monotonic() - started, output), flush=True)
    return status


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output")
    parser.add_argument("--prepare-only", action="store_true")
    arguments = parser.parse_args()
    output = Path(arguments.output).resolve()
    output.mkdir(parents=True, exist_ok=False)
    if arguments.prepare_only:
        print(prepare(output))
        return 0
    return run(output)


if __name__ == "__main__":
    sys.exit(main())
