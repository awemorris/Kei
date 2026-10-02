#!/usr/bin/env python3
"""Observe p076 popup input, mapping and capture order without changing its verdict.

Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib

The original mode preserves stages 1–4. The handshake mode waits for a fresh
far-menu map/configure/focus before the second click. Both retain the first
pixel failure even when later pictures match. Each run needs an already running
owned guest and ends within 240 seconds. These are partial diagnostics only.
"""
import argparse
import json
import os
from pathlib import Path
import shlex
import signal
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[3]
SELF = Path(__file__).resolve()
SOURCE = ROOT / "plan/ws035/tests/zdesktop-p076.sh"

# All three observations must advance beyond the first menu's existing entries.
HANDSHAKE = r'''
far_maps=$(count_log /tmp/zdesktop.log '^ZWL POPUP map surface=')
far_configures=$(count_log /tmp/p.log 'POPUPPROBE configure menu x=360 y=60 width=360 height=180')
far_focuses=$(count_log /tmp/p.log 'POPUPPROBE focus menu')
case "$far_maps:$far_configures:$far_focuses" in
*[!0-9:]*|:*|*::|*:) echo 'far-menu: invalid initial count'; status=1; finish ;;
esac
click $((wx + 360)) $((wy + 60)) 1200
far_reply=$(guest "i=0; while [ \$i -lt 6 ]; do m=\$(grep -cE '^ZWL POPUP map surface=' /tmp/zdesktop.log); c=\$(grep -cE 'POPUPPROBE configure menu x=360 y=60 width=360 height=180' /tmp/p.log); f=\$(grep -cE 'POPUPPROBE focus menu' /tmp/p.log); if [ \$m -gt $far_maps ] && [ \$c -gt $far_configures ] && [ \$f -gt $far_focuses ]; then grep '^ZWL POPUP map surface=' /tmp/zdesktop.log | tail -1; echo P017_FAR_READY=1; exit 0; fi; sleep 0.5; i=\$((i + 1)); done; echo P017_FAR_READY=0; exit 1")
far_status=$?
printf '%s\n' "$far_reply"
if [ "$far_status" -ne 0 ] || ! printf '%s\n' "$far_reply" | grep -q '^P017_FAR_READY=1$'; then
    echo 'far-menu: fresh map/configure/focus missing; no submenu click sent'
    status=1
    finish
fi
click $((wx + 360 + 100)) $((wy + 60 + 130)) 1200
'''


def record(output, event, **fields):
    """Append host times without claiming that the guest clock shares their origin."""
    entry = {"event": event, "host_monotonic_ns": time.monotonic_ns(),
             "host_utc_ns": time.time_ns(), **fields}
    with (output / "timeline.jsonl").open("a") as log:
        log.write(json.dumps(entry, sort_keys=True) + "\n")


def command(output, kind, arguments, deadline=10):
    """Bracket a real input or SSH operation while preserving its output and status."""
    record(output, kind + "-begin", arguments=arguments)
    try:
        status = subprocess.run(arguments, cwd=ROOT, timeout=deadline).returncode
    except subprocess.TimeoutExpired:
        status = 124
    record(output, kind + "-end", status=status)
    return status


def snapshot(output, label):
    """Keep application logs around capture as ordinal guest-frame evidence."""
    arguments = [sys.executable, str(ROOT / "plan/tools/guest/guest.py"), "run",
                 "cat /tmp/zdesktop.log; echo P017_PROBE_BOUNDARY; cat /tmp/p.log"]
    record(output, "snapshot-begin", label=label)
    with (output / (label + ".log")).open("w") as log:
        try:
            status = subprocess.run(arguments, cwd=ROOT, stdout=log,
                                    stderr=subprocess.STDOUT, timeout=10).returncode
        except subprocess.TimeoutExpired:
            status = 124
    record(output, "snapshot-end", label=label, status=status)
    return status


def capture(output, arguments):
    """Save original pixels and at most six later captures without clearing failure."""
    picture = Path(arguments[0])
    stem = picture.stem
    snapshot(output, stem + "-before")
    checker = [sys.executable, str(ROOT / "plan/ws035/tests/zdesktop-check.py")]
    first_status = command(output, "capture", checker + arguments, deadline=15)
    snapshot(output, stem + "-after")

    # Later captures explain the first failure; they never replace its verdict.
    if first_status != 0:
        for observation in range(1, 7):
            time.sleep(0.5)
            later = picture.with_name(stem + "-later-" + str(observation) + ".png")
            later_status = command(output, "capture-later",
                                   checker + [str(later)] + arguments[1:], deadline=15)
            snapshot(output, stem + "-later-" + str(observation))
            print("observation: original=%d later=%d poll=%d" %
                  (first_status, later_status, observation), flush=True)
    return first_status


def replace_once(source, before, after):
    """Refuse a changed shared harness instead of silently generating another test."""
    if source.count(before) != 1:
        raise RuntimeError("Missing or ambiguous p076 boundary: " + before)
    return source.replace(before, after, 1)


def prepare(output, mode):
    """Copy only the authorized stages and retain original geometry and pixel checks."""
    source = SOURCE.read_text()
    boundary = "# 5. A press on the desktop closes the popups"
    if source.count(boundary) != 1:
        raise RuntimeError("Missing or ambiguous p076 stage boundary")
    source = source.split(boundary, 1)[0]
    helper = shlex.quote(sys.executable) + " " + shlex.quote(str(SELF))
    source = replace_once(source, 'cd "$(dirname -- "$0")/../../.."',
                          "cd " + shlex.quote(str(ROOT)))
    source = replace_once(source,
                          'guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }',
                          'guest() { ' + helper + ' --event "$out" guest -- python3 plan/tools/guest/guest.py run "$1" 2>&1; }')
    source = replace_once(source,
                          'pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }',
                          'pointer() { ' + helper + ' --event "$out" pointer -- python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }')
    source = replace_once(source,
                          'check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }',
                          'check() { ' + helper + ' --capture "$out" -- "$@" --runtime "$GUEST_RUNTIME"; }')
    source = replace_once(source, '/bin/wayland --timeout=400',
                          '/bin/wayland --log-frames --timeout=400')
    source = replace_once(source, "finish() {",
                          'finish() {\n\tguest \'cat /tmp/zdesktop.log\' > "$out/frames.log"')

    # The second mode changes just the readiness boundary under investigation.
    if mode == "handshake":
        original_clicks = ('click $((wx + 360)) $((wy + 60)) 1200\n'
                           'click $((wx + 360 + 100)) $((wy + 60 + 130)) 1200\n')
        source = replace_once(source, original_clicks, HANDSHAKE)
    source += "# Saves the first partial-test verdict and complete application logs.\nfinish\n"
    runner = output / "observe.sh"
    runner.write_text(source)
    subprocess.run(["sh", "-n", str(runner)], check=True)
    return runner


def run(output, mode):
    """Bound the entire diagnostic and retire only its own shell children on timeout."""
    runner = prepare(output, mode)
    record(output, "run-begin", mode=mode, deadline_seconds=240)
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
            log.write("\np017 diagnostic: whole-run deadline reached; no PASS\n")
    record(output, "run-end", mode=mode, status=status)
    print("popup timeline: mode=%s status=%d output=%s" % (mode, status, output))
    return status


def main():
    # Internal wrappers keep binary input/capture tools as the shared authority.
    if len(sys.argv) > 1 and sys.argv[1] == "--event":
        output = Path(sys.argv[2])
        return command(output, sys.argv[3], sys.argv[5:])
    if len(sys.argv) > 1 and sys.argv[1] == "--capture":
        return capture(Path(sys.argv[2]), sys.argv[4:])

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output")
    parser.add_argument("--mode", choices=["original", "handshake"], default="original")
    parser.add_argument("--prepare-only", action="store_true")
    arguments = parser.parse_args()
    output = Path(arguments.output).resolve()
    output.mkdir(parents=True, exist_ok=False)
    if arguments.prepare_only:
        runner = prepare(output, arguments.mode)
        print(runner)
        return 0
    return run(output, arguments.mode)


if __name__ == "__main__":
    sys.exit(main())
