#!/usr/bin/env python3
# zedBSD; Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
"""Exercise real xdg-decoration and KDE server-decoration requests, configure ordering and commit boundaries.

ws114-p008 (2026-10-03 user): a toplevel without a declaration is decorated by the compositor (SSD); xdg-decoration's
client_side, KDE's request_mode CLIENT/NONE, or a destroyed xdg-decoration object leave the client's decoration (CSD).
With WS114_COMPOSITOR_LOG naming the compositor's log, each checkpoint also checks the mode the compositor applied
(its last "ZWL DECORATION applied" line since the connection was made); without it the checkpoints are only printed.
"""
import json
import os
import re
import socket
import struct

LOG = os.environ.get('WS114_COMPOSITOR_LOG')
APPLIED = re.compile(r'^ZWL DECORATION applied client=\d+ surface=\d+ mode=(\d+)$', re.M)


def word(*values):
    return struct.pack('=' + 'I' * len(values), *values)


def string(value):
    data = value.encode() + b'\0'
    return word(len(data)) + data + bytes((-len(data)) % 4)


class Wire:
    def __init__(self, kde=False):
        self.socket = socket.socket(socket.AF_UNIX)
        self.socket.settimeout(3)
        self.socket.connect(os.path.join(os.environ['XDG_RUNTIME_DIR'], os.environ['WAYLAND_DISPLAY']))
        self.next_id = 2
        self.events = []
        self.log_start = os.path.getsize(LOG) if LOG else 0
        self.globals = {}
        self.registry = self.new()
        self.send(1, 1, word(self.registry))
        self.roundtrip()
        self.compositor = self.bind('wl_compositor', 4)
        self.shell = self.bind('xdg_wm_base', 1)
        self.manager = self.bind('zxdg_decoration_manager_v1', 1)
        self.native = self.bind('keiland_titlebar_manager_v1', 1)
        self.kde_manager = self.bind('org_kde_kwin_server_decoration_manager', 1) if kde else None
        self.surface = self.new()
        self.send(self.compositor, 0, word(self.surface))
        self.xdg = self.new()
        self.send(self.shell, 2, word(self.xdg, self.surface))
        self.top = self.new()
        self.send(self.xdg, 1, word(self.top))
        self.decoration = None
        self.kde = None
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
        self.assert_mode(2)

    def kde_create(self):
        self.kde = self.new()
        self.send(self.kde_manager, 0, word(self.kde, self.surface))
        self.roundtrip()
        self.assert_kde(2)

    def kde_request(self, mode):
        self.send(self.kde, 1, word(mode))
        self.roundtrip()
        self.assert_kde(mode)

    def assert_kde(self, mode):
        events = [payload for target, opcode, payload in self.events if target == self.kde and opcode == 0]
        assert events and events[-1] == word(mode), (mode, events)

    def last_configure(self):
        return [struct.unpack('=I', payload)[0] for target, opcode, payload in self.events if target == self.xdg and opcode == 0][-1]

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
        self.assert_mode(mode or 2)
        new = self.events[start:]
        offered = next(i for i, event in enumerate(new) if event[0] == self.decoration and event[1] == 0)
        configured = next(i for i, event in enumerate(new) if event[0] == self.xdg and event[1] == 0)
        assert offered < configured, new
        return struct.unpack('=I', new[configured][2])[0]

    def initial(self, expected=None):
        self.send(self.surface, 6)
        self.roundtrip()
        serial = [struct.unpack('=I', payload)[0] for target, opcode, payload in self.events if target == self.xdg and opcode == 0][-1]
        self.ack(serial)
        self.commit('initial', expected)

    def ack(self, serial):
        self.send(self.xdg, 4, word(serial))
        self.roundtrip()

    def commit(self, label, expected=None):
        self.send(self.surface, 6)
        self.roundtrip()
        print(json.dumps({'checkpoint': label, 'expected_mode': expected}), flush=True)
        if LOG and expected is not None:
            with open(LOG, encoding='utf-8', errors='replace') as log:
                log.seek(self.log_start)
                modes = APPLIED.findall(log.read())
            applied = int(modes[-1]) if modes else None
            assert applied == expected, (label, expected, applied)
            print(json.dumps({'applied_mode': applied, 'checkpoint': label}), flush=True)

    def close(self):
        self.socket.close()


def run():
    client = Wire()
    client.initial(2)
    client.close()
    print('PASS no declaration: SSD by default')

    client = Wire(kde=True)
    assert any(target == client.kde_manager and opcode == 0 and payload == word(2) for target, opcode, payload in client.events), 'KDE default_mode is not SSD'
    client.initial(1)
    client.close()
    print('PASS KDE manager bound, no decoration object (GTK4): CSD; default_mode SSD')

    client = Wire()
    client.decorate()
    client.initial(2)
    b = client.configure(1)
    client.commit('CSD requested, not acknowledged: SSD remains', 2)
    client.ack(b)
    client.commit('CSD acknowledged: CSD applies', 1)
    a = client.configure(2)
    client.ack(a)
    b = client.configure(1)
    client.commit('ackSSD-requestCSD-commit: SSD applies', 2)
    client.ack(b)
    client.commit('next CSD acknowledgment applies', 1)
    a = client.configure(1)
    client.ack(a)
    b = client.configure(2)
    client.commit('ackCSD-requestSSD-commit: CSD remains', 1)
    client.ack(b)
    client.commit('SSD acknowledgment applies', 2)
    a = client.configure(1)
    b = client.configure(2)
    client.ack(a)
    client.commit('older outstanding CSD serial applies', 1)
    client.ack(b)
    client.commit('newer SSD serial applies', 2)
    a = client.configure(1)
    client.ack(a)
    client.commit('CSD before unset', 1)
    a = client.configure(0)
    client.ack(a)
    client.commit('unset defaults to SSD', 2)
    client.send(client.decoration, 0)
    client.roundtrip()
    client.commit('destroy withdraws SSD at the next commit', 1)
    client.send(client.surface, 6)
    client.roundtrip()
    client.commit('the withdrawn window keeps CSD', 1)
    client.close()
    print('PASS mode events/configure serials/commit boundaries')

    client = Wire()
    titlebar = client.new()
    client.send(client.native, 1, word(titlebar, client.top))
    client.initial(2)
    client.send(titlebar, 0)
    client.roundtrip()
    client.commit('native titlebar destroyed: the default SSD remains', 2)
    client.close()
    print('PASS native opt-in and withdrawal')

    client = Wire()
    titlebar = client.new()
    client.send(client.native, 1, word(titlebar, client.top))
    client.decorate()
    client.send(client.decoration, 1, word(1))
    client.roundtrip()
    client.assert_mode(1)
    client.initial(1)
    client.close()
    print('PASS explicit xdg CSD takes precedence over native opt-in')

    client = Wire(kde=True)
    client.kde_create()
    client.kde_request(1)
    client.initial(1)
    client.kde_request(2)
    serial = client.last_configure()
    client.commit('KDE SERVER requested, not acknowledged: CSD remains', 1)
    client.ack(serial)
    client.commit('KDE SERVER acknowledged: SSD applies', 2)
    client.kde_request(0)
    serial = client.last_configure()
    client.ack(serial)
    client.commit('KDE NONE: the client decorates (CSD)', 1)
    client.kde_request(2)
    serial = client.last_configure()
    client.ack(serial)
    client.commit('KDE SERVER again: SSD', 2)
    client.send(client.kde, 0)
    client.roundtrip()
    serial = client.last_configure()
    client.ack(serial)
    client.commit('KDE decoration released: the window has no object again (CSD for a KDE client)', 1)
    client.close()
    print('PASS KDE server decoration: CLIENT before the first commit, SERVER, NONE, release')

    client = Wire(kde=True)
    client.kde_create()
    client.kde_request(1)
    client.decorate()
    client.initial(2)
    client.close()
    print('PASS xdg-decoration takes precedence over KDE')

    for case in ('duplicate', 'invalid', 'kde-invalid'):
        client = Wire(kde=(case == 'kde-invalid'))
        if case == 'kde-invalid':
            client.kde_create()
            client.send(client.kde, 1, word(9))
        else:
            client.decorate()
        if case == 'duplicate':
            client.send(client.manager, 1, word(client.new(), client.top))
        elif case == 'invalid':
            client.send(client.decoration, 1, word(9))
        try:
            client.roundtrip()
        except RuntimeError as error:
            assert 'display protocol error' in str(error) or 'connection close' in str(error), error
            print('PASS expected ' + case + ' protocol error')
        else:
            raise AssertionError('missing ' + case + ' error')
        finally:
            client.close()


if __name__ == '__main__':
    run()
