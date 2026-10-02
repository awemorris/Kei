#!/usr/bin/env python3
# zedBSD; Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
"""Map a bounded CSD wire client and record exact button completion outside its image."""
import array
import importlib.util
from pathlib import Path
import socket
import os
import struct
import sys
import time
spec = importlib.util.spec_from_file_location("decoration_wire", Path(__file__).with_name("decoration-wire.py"))
protocol = importlib.util.module_from_spec(spec)
spec.loader.exec_module(protocol)
Wire, word, string = protocol.Wire, protocol.word, protocol.string

client = Wire()
client.socket.settimeout(90)
shm = client.bind('wl_shm', 1)
seat = client.bind('wl_seat', 5)
pointer = client.new()
client.send(seat, 0, word(pointer))
client.send(client.top, 2, string('q587 bounded wire client'))
client.send(client.top, 7, word(360, 200))
client.send(client.top, 8, word(300, 180))
client.initial()
size = 300 * 180 * 4
descriptor = os.memfd_create('q587-wire-image')
os.write(descriptor, struct.pack('=I', 0xff2070a0) * (300 * 180))
pool = client.new()
payload = word(pool, size)
client.socket.sendmsg([word(shm, ((len(payload) + 8) << 16)) + payload], [(socket.SOL_SOCKET, socket.SCM_RIGHTS, array.array('i', [descriptor]))])
buffer = client.new()
client.send(pool, 0, word(buffer, 0, 300, 180, 1200, 0))
client.send(pool, 1)
os.close(descriptor)
client.send(client.xdg, 3, word(0, 0, 300, 180))
client.send(client.surface, 1, word(buffer, 0, 0))
client.send(client.surface, 2, word(0, 0, 300, 180))
client.commit('mapped bounded 300x180')
print('READY', flush=True)
mode = sys.argv[1] if len(sys.argv) > 1 else 'resize'
received = []
deadline = time.monotonic() + 90
while time.monotonic() < deadline:
    target, opcode, payload = client.receive()
    if target == pointer and opcode == 3:
        serial, event_time, button, state = struct.unpack('=IIII', payload)
        received.append((button, state))
        if state:
            if mode == 'move':
                client.send(client.top, 5, word(seat, serial))
            else:
                client.send(client.top, 6, word(seat, serial, 10))
        else:
            assert received == [(button, 1), (button, 0)], received
            print('PASS one matching release ' + repr(received), flush=True)
            # Keep processing until one frame confirms the origin's release group.
            while True:
                event = client.receive()
                if event[0] == pointer and event[1] == 5:
                    print('PASS release frame', flush=True)
                    break
            time.sleep(.3)
            break
    if target == client.xdg and opcode == 0:
        # Preserve the original image while acknowledging the bounded resize proposal.
        client.send(client.xdg, 4, payload)
        client.send(client.surface, 6)
else:
    raise RuntimeError('interactive input timeout')
client.close()
