#!/usr/bin/env python3
# zedBSD; Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
"""Exercise real xdg-decoration requests, configure ordering and commit boundaries."""
import json
import os
import socket
import struct


def word(*values):
    return struct.pack('=' + 'I' * len(values), *values)


def string(value):
    data = value.encode() + b'\0'
    return word(len(data)) + data + bytes((-len(data)) % 4)


class Wire:
    def __init__(self):
        self.socket = socket.socket(socket.AF_UNIX)
        self.socket.settimeout(3)
        self.socket.connect(os.path.join(os.environ['XDG_RUNTIME_DIR'], os.environ['WAYLAND_DISPLAY']))
        self.next_id = 2
        self.events = []
        self.globals = {}
        self.registry = self.new()
        self.send(1, 1, word(self.registry))
        self.roundtrip()
        self.compositor = self.bind('wl_compositor', 4)
        self.shell = self.bind('xdg_wm_base', 1)
        self.manager = self.bind('zxdg_decoration_manager_v1', 1)
        self.native = self.bind('keiland_titlebar_manager_v1', 1)
        self.surface = self.new()
        self.send(self.compositor, 0, word(self.surface))
        self.xdg = self.new()
        self.send(self.shell, 2, word(self.xdg, self.surface))
        self.top = self.new()
        self.send(self.xdg, 1, word(self.top))
        self.decoration = None
        self.roundtrip()

    def new(self):
        value = self.next_id
        self.next_id += 1
        return value

    def send(self, target, opcode, payload=b''):
        self.socket.sendall(word(target, ((len(payload) + 8) << 16) | opcode) + payload)
        print(json.dumps({'request': [target, opcode], 'payload': list(struct.unpack('=' + 'I' * (len(payload) // 4), payload))}), flush=True)

    def receive(self):
        header = self.read(8)
        target, combined = struct.unpack('=II', header)
        payload = self.read((combined >> 16) - 8)
        opcode = combined & 65535
        self.events.append((target, opcode, payload))
        print(json.dumps({'event': [target, opcode], 'payload_hex': payload.hex()}), flush=True)
        if target == 1 and opcode == 0:
            raise RuntimeError('display protocol error: ' + payload.hex())
        if target == self.registry and opcode == 0:
            name, length = struct.unpack('=II', payload[:8])
            interface = payload[8:8 + length - 1].decode()
            version = struct.unpack('=I', payload[8 + ((length + 3) & ~3):])[0]
            self.globals[interface] = (name, version)
        if target == getattr(self, 'shell', None) and opcode == 0:
            self.send(self.shell, 3, payload)
        return target, opcode, payload

    def read(self, length):
        data = b''
        while len(data) < length:
            part = self.socket.recv(length - len(data))
            if not part:
                raise RuntimeError('unexpected connection close')
            data += part
        return data

    def roundtrip(self):
        callback = self.new()
        self.send(1, 0, word(callback))
        while True:
            target, opcode, payload = self.receive()
            if target == callback and opcode == 0:
                return

    def bind(self, interface, requested):
        name, version = self.globals[interface]
        value = self.new()
        self.send(self.registry, 0, word(name) + string(interface) + word(min(version, requested), value))
        return value

    def decorate(self):
        self.decoration = self.new()
        self.send(self.manager, 1, word(self.decoration, self.top))
        self.roundtrip()
        self.assert_mode(1)

    def assert_mode(self, mode):
        events = [payload for target, opcode, payload in self.events if target == self.decoration and opcode == 0]
        assert events and events[-1] == word(mode), (mode, events)

    def configure(self, mode):
        start = len(self.events)
        if mode == 0:
            self.send(self.decoration, 2)
        else:
            self.send(self.decoration, 1, word(mode))
        self.roundtrip()
        self.assert_mode(mode or 1)
        new = self.events[start:]
        offered = next(i for i, event in enumerate(new) if event[0] == self.decoration and event[1] == 0)
        configured = next(i for i, event in enumerate(new) if event[0] == self.xdg and event[1] == 0)
        assert offered < configured, new
        return struct.unpack('=I', new[configured][2])[0]

    def initial(self):
        self.send(self.surface, 6)
        self.roundtrip()
        serial = [struct.unpack('=I', payload)[0] for target, opcode, payload in self.events if target == self.xdg and opcode == 0][-1]
        self.ack(serial)
        self.commit('initial')

    def ack(self, serial):
        self.send(self.xdg, 4, word(serial))
        self.roundtrip()

    def commit(self, label):
        self.send(self.surface, 6)
        self.roundtrip()
        print(json.dumps({'checkpoint': label}), flush=True)

    def close(self):
        self.socket.close()


def run():
    client = Wire()
    client.initial()
    client.close()
    print('PASS no decoration object')

    client = Wire()
    client.decorate()
    client.initial()
    b = client.configure(2)
    client.commit('SSD requested, not acknowledged: CSD remains')
    client.ack(b)
    client.commit('SSD acknowledged: SSD applies')
    a = client.configure(1)
    client.ack(a)
    b = client.configure(2)
    client.commit('ackCSD-requestSSD-commit: CSD applies')
    client.ack(b)
    client.commit('next SSD acknowledgment applies')
    a = client.configure(2)
    client.ack(a)
    b = client.configure(1)
    client.commit('ackSSD-requestCSD-commit: SSD remains')
    client.ack(b)
    client.commit('CSD acknowledgment applies')
    a = client.configure(2)
    b = client.configure(1)
    client.ack(a)
    client.commit('older outstanding SSD serial applies')
    client.ack(b)
    client.commit('newer CSD serial applies')
    a = client.configure(2)
    client.ack(a)
    client.commit('SSD before unset')
    a = client.configure(0)
    client.ack(a)
    client.commit('unset defaults to CSD')
    a = client.configure(2)
    client.ack(a)
    client.commit('SSD before destroy')
    a = client.configure(2)
    client.send(client.decoration, 0)
    client.ack(a)
    client.commit('destroy prevents stale SSD resurrection')
    client.close()
    print('PASS mode events/configure serials/commit boundaries')

    client = Wire()
    titlebar = client.new()
    client.send(client.native, 1, word(titlebar, client.top))
    client.initial()
    client.send(titlebar, 0)
    client.commit('native titlebar destroyed: CSD')
    client.close()
    print('PASS native opt-in and withdrawal')

    client = Wire()
    titlebar = client.new()
    client.send(client.native, 1, word(titlebar, client.top))
    client.decorate()
    client.initial()
    client.close()
    print('PASS explicit CSD takes precedence over native opt-in')

    for case in ('duplicate', 'invalid'):
        client = Wire()
        client.decorate()
        if case == 'duplicate':
            client.send(client.manager, 1, word(client.new(), client.top))
        else:
            client.send(client.decoration, 1, word(9))
        try:
            client.roundtrip()
        except RuntimeError as error:
            assert 'display protocol error' in str(error), error
            print('PASS expected ' + case + ' protocol error')
        else:
            raise AssertionError('missing ' + case + ' error')
        finally:
            client.close()


if __name__ == '__main__':
    run()
