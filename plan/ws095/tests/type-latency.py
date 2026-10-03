#!/usr/bin/env python3
"""Measures how long a typed key takes to show on the Venus guest's screen (BUG-143).

Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib

For each trial: a picture of the screen (QEMU's VNC on the Venus console, as
zdesktop-check.py takes it), then one key pressed and let go through QMP, then
pictures as fast as they can be taken until more than --threshold pixels of
the region differ from the picture before the key (or --limit seconds).  The
time is from the key to the end of the first picture that shows the change: an
upper bound, a picture's time (tens of milliseconds) over the real latency.
Prints one line a trial and a summary:

    LATENCY name=NAME trials=N median_ms=M max_ms=X missed=K

    type-latency.py --runtime DIR --name NAME --key QCODE --region X,Y,W,H
                    [--threshold PIXELS] [--trials N] [--pause MS] [--limit S]
"""
import argparse
import json
import socket
import statistics
import sys
import tempfile
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(ROOT / "plan/ws014/tests"))
sys.path.insert(0, str(ROOT / "plan/ws035/tests"))

from venus_rfb import capture  # noqa: E402
from ppm2png import read_ppm  # noqa: E402


def region_pixels(path, region):
    """Returns the region's pixels of a PPM picture as a list of rows."""
    width, height, pixels = read_ppm(path)
    x, y, w, h = region
    rows = []
    for row in range(y, min(y + h, height)):
        start = (row * width + x) * 3
        rows.append(pixels[start:start + min(w, width - x) * 3])
    return rows


def changed(before, after):
    """Counts the region's pixels that differ between two pictures."""
    count = 0
    for old, new in zip(before, after):
        if old == new:
            continue
        for index in range(0, len(old), 3):
            if old[index:index + 3] != new[index:index + 3]:
                count += 1
    return count


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--runtime", required=True)
    parser.add_argument("--name", required=True)
    parser.add_argument("--key", required=True)
    parser.add_argument("--region", required=True)
    parser.add_argument("--threshold", type=int, default=0)
    parser.add_argument("--trials", type=int, default=8)
    parser.add_argument("--pause", type=int, default=1200)
    parser.add_argument("--limit", type=float, default=3.0)
    arguments = parser.parse_args()
    region = tuple(int(value) for value in arguments.region.split(","))
    runtime = Path(arguments.runtime).resolve()
    vnc = str(runtime / "vnc.sock")
    connection = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    connection.connect(str(runtime / "qmp.sock"))
    stream = connection.makefile("rw")
    stream.readline()

    def call(command, data=None):
        stream.write(json.dumps({"execute": command, "arguments": data or {}}) + "\n")
        stream.flush()
        while True:
            reply = json.loads(stream.readline())
            if "return" in reply or "error" in reply:
                return reply

    def key(down):
        call("input-send-event", {"events": [{"type": "key", "data": {"down": down, "key": {"type": "qcode", "data": arguments.key}}}]})

    call("qmp_capabilities")
    times = []
    missed = 0
    with tempfile.TemporaryDirectory() as scratch:
        picture = str(Path(scratch) / "frame.ppm")
        for trial in range(arguments.trials):
            capture(vnc, picture, 10)
            before = region_pixels(picture, region)
            started = time.monotonic()
            key(True)
            key(False)
            shown = None
            while time.monotonic() - started < arguments.limit:
                capture(vnc, picture, 10)
                count = changed(before, region_pixels(picture, region))
                if count > arguments.threshold:
                    shown = (time.monotonic() - started) * 1000.0
                    break
            if shown is None:
                missed += 1
                print("TRIAL name=%s index=%d shown=none" % (arguments.name, trial))
            else:
                times.append(shown)
                print("TRIAL name=%s index=%d ms=%.0f pixels=%d" % (arguments.name, trial, shown, count))
            sys.stdout.flush()
            time.sleep(arguments.pause / 1000.0)
    median = statistics.median(times) if times else -1
    largest = max(times) if times else -1
    print("LATENCY name=%s trials=%d median_ms=%.0f max_ms=%.0f missed=%d" % (arguments.name, arguments.trials, median, largest, missed))
    return 0


if __name__ == "__main__":
    sys.exit(main())
