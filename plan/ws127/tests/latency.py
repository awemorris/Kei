#!/usr/bin/env python3
"""ws127-p001: measures, from the host, how long a click on files takes to show on the guest's screen.

    latency.py RUNTIME --at X Y [--watch X Y] [--away X Y] [--trials 3]

The pointer goes to (X, Y) of the output and clicks (press, 50 ms, release), and the time is taken from the button press sent
through QMP to the first moment a small square at (X, Y) (or at --watch, for a click whose
effect shows elsewhere, such as a sidebar row that changes the folder shown), read again and again through
QEMU's VNC socket (RFB, RAW, one 2x2 rectangle per request), differs from its colour before
the press.  Between trials a click at --away (an empty place) clears the selection.  The
value includes QEMU's display refresh (egl-headless reads the scanout back on its own
timer) and the RFB round trip, so it is an upper bound of the guest's own latency, and a
QEMU value, not a physical machine's.  Prints each trial and the median in milliseconds.
Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
"""
import argparse
import json
import os
import socket
import statistics
import struct
import time


def rfb_connect(path):
    """Opens the RFB socket and returns it with the framebuffer's size (pixel format BGRX)."""
    stream = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    stream.settimeout(10)
    stream.connect(path)
    version = receive(stream, 12)
    stream.sendall(version)
    count = receive(stream, 1)[0]
    types = receive(stream, count)
    if 1 not in types:
        raise RuntimeError("no None security")
    stream.sendall(b"\x01")
    if struct.unpack("!I", receive(stream, 4))[0] != 0:
        raise RuntimeError("security refused")
    stream.sendall(b"\x01")
    header = receive(stream, 20)
    width, height = struct.unpack_from("!HH", header)
    name_length = struct.unpack("!I", receive(stream, 4))[0]
    receive(stream, name_length)
    pixel_format = struct.pack("!BBBBHHHBBB3x", 32, 24, 0, 1, 255, 255, 255, 16, 8, 0)
    stream.sendall(b"\x00\x00\x00\x00" + pixel_format)
    stream.sendall(struct.pack("!BBHi", 2, 0, 1, 0))
    return stream, width, height


def receive(stream, count):
    """Reads exactly count bytes."""
    data = bytearray()
    while len(data) < count:
        part = stream.recv(count - len(data))
        if not part:
            raise EOFError("RFB closed")
        data.extend(part)
    return bytes(data)


def square(stream, x, y, size=2):
    """Reads the pixels of a small square (non-incremental request) as BGRX bytes, row by row.

    QEMU answers with rectangles aligned to its own tiles (the first answer is the whole
    screen), so the square is cut out of whichever rectangles cover it.
    """
    stream.sendall(struct.pack("!BBHHHH", 3, 0, x, y, size, size))
    while True:
        kind = receive(stream, 1)[0]
        if kind == 0:
            break
        if kind == 2:
            continue
        if kind == 3:
            receive(stream, 3)
            receive(stream, struct.unpack("!I", receive(stream, 4))[0])
            continue
        raise RuntimeError(f"RFB message {kind}")
    count = struct.unpack_from("!H", receive(stream, 3), 1)[0]
    found = {}
    for _ in range(count):
        rx, ry, rw, rh, encoding = struct.unpack("!HHHHi", receive(stream, 12))
        pixels = receive(stream, rw * rh * 4)
        for py in range(y, y + size):
            for px in range(x, x + size):
                if rx <= px < rx + rw and ry <= py < ry + rh:
                    offset = ((py - ry) * rw + (px - rx)) * 4
                    found[(px, py)] = pixels[offset:offset + 4]
    result = b""
    for py in range(y, y + size):
        for px in range(x, x + size):
            result += found.get((px, py), b"\0\0\0\0")
    return result


def differs(before, after, threshold=24):
    """Tells whether two squares differ by more than the threshold in any channel."""
    for index, (left, right) in enumerate(zip(before, after)):
        if index % 4 == 3:
            continue
        if abs(left - right) > threshold:
            return True
    return False


class Qmp:
    """One QMP connection for the pointer (usb-tablet absolute coordinates)."""

    def __init__(self, path, width, height):
        self.stream = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.stream.connect(path)
        self.file = self.stream.makefile("rw")
        self.file.readline()
        self.send("qmp_capabilities", {})
        self.width = width
        self.height = height

    def send(self, command, arguments):
        self.file.write(json.dumps({"execute": command, "arguments": arguments}) + "\n")
        self.file.flush()
        while True:
            reply = json.loads(self.file.readline())
            if "return" in reply or "error" in reply:
                return reply

    def move(self, x, y):
        events = [{"type": "abs", "data": {"axis": "x", "value": int(x * 32767 / (self.width - 1))}},
                  {"type": "abs", "data": {"axis": "y", "value": int(y * 32767 / (self.height - 1))}}]
        self.send("input-send-event", {"events": events})

    def button(self, down):
        self.send("input-send-event", {"events": [{"type": "btn", "data": {"down": down, "button": "left"}}]})


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("runtime")
    parser.add_argument("--at", nargs=2, type=int, required=True)
    parser.add_argument("--away", nargs=2, type=int)
    parser.add_argument("--watch", nargs=2, type=int)
    parser.add_argument("--expect", help="R,G,B the watched square must reach (instead of any change)")
    parser.add_argument("--trials", type=int, default=3)
    parser.add_argument("--timeout", type=float, default=3.0)
    arguments = parser.parse_args()
    runtime = os.path.abspath(arguments.runtime)
    stream, width, height = rfb_connect(os.path.join(runtime, "vnc.sock"))
    qmp = Qmp(os.path.join(runtime, "qmp.sock"), 1280, 800)
    x, y = arguments.at
    wx, wy = arguments.watch or arguments.at
    expected = None
    if arguments.expect:
        red, green, blue = (int(part) for part in arguments.expect.split(","))
        expected = bytes((blue, green, red, 0))
    results = []
    for trial in range(arguments.trials):
        if arguments.away:
            qmp.move(*arguments.away)
            time.sleep(0.2)
            qmp.button(True)
            qmp.button(False)
            time.sleep(3.0)
        qmp.move(x - 3, y)
        time.sleep(0.2)
        qmp.move(x, y)
        time.sleep(0.5)
        before = square(stream, wx, wy)
        print(f"trial {trial + 1}: before RGB {before[2]},{before[1]},{before[0]}")
        started = time.monotonic()
        qmp.button(True)
        time.sleep(0.05)
        qmp.button(False)
        changed = None
        while time.monotonic() - started < arguments.timeout:
            after = square(stream, wx, wy)
            if expected is not None:
                if not differs(expected * (len(after) // 4), after, 20):
                    changed = time.monotonic()
                    break
            elif differs(before, after):
                changed = time.monotonic()
                break
        if changed is None:
            print(f"trial {trial + 1}: no change within {arguments.timeout:.1f} s (last {after[2]},{after[1]},{after[0]})")
        else:
            milliseconds = (changed - started) * 1000.0
            results.append(milliseconds)
            print(f"trial {trial + 1}: {milliseconds:.0f} ms")
        time.sleep(1.0)
    if results:
        print(f"median: {statistics.median(results):.0f} ms over {len(results)} trials")


if __name__ == "__main__":
    main()
