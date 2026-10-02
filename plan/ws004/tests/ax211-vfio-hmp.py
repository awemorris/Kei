#!/usr/bin/env python3
# Sends human-monitor commands to the AX211 VFIO runner's QEMU monitor socket.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
#
#   ax211-vfio-hmp.py SOCKET COMMAND [COMMAND...]
#
# Each COMMAND is one monitor line (for example "info registers",
# "screendump /path/shot.ppm" or "gdbserver tcp:127.0.0.1:1234").  The reply
# of each command is printed after a "### COMMAND" header.  The monitor is the
# one run-intel-ax211-vfio-qemu.sh creates as WORK-DIR/monitor.sock.

import socket
import sys
import time

PROMPT = b"(qemu) "


def read_until_prompt(sock, deadline):
    data = b""
    while not data.endswith(PROMPT):
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            raise TimeoutError("monitor did not answer")
        sock.settimeout(remaining)
        chunk = sock.recv(65536)
        if not chunk:
            raise ConnectionError("monitor closed the connection")
        data += chunk
    return data[:-len(PROMPT)]


def main():
    if len(sys.argv) < 3:
        print("usage: ax211-vfio-hmp.py SOCKET COMMAND [COMMAND...]",
              file=sys.stderr)
        return 2
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    sock.connect(sys.argv[1])
    read_until_prompt(sock, time.monotonic() + 10)
    for command in sys.argv[2:]:
        sock.sendall(command.encode() + b"\n")
        try:
            reply = read_until_prompt(sock, time.monotonic() + 60)
        except ConnectionError:
            # "quit" ends QEMU, which closes the monitor without a prompt.
            if command.strip() == "quit":
                print("### quit")
                break
            raise
        text = reply.decode(errors="replace").replace("\r", "")
        # The monitor echoes the command line first; drop that echo.
        lines = text.split("\n")
        if lines and command in lines[0]:
            lines = lines[1:]
        print("### " + command)
        print("\n".join(lines).rstrip())
    sock.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
