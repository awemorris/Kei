#!/usr/bin/env python3
# zedBSD; Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
"""Exercise the native production WPA path in an owned FreeBSD guest only.

The independent Unix datagram peer verifies command syntax and ordering.
It is a wire fixture, not a supplicant or evidence of actual radio operation.
"""
import errno
import json
import os
from pathlib import Path
import socket
import stat
import subprocess
import sys
import tempfile
import threading


def run(probe):
    if not sys.platform.startswith('freebsd') or os.geteuid() != 0:
        raise RuntimeError('requires root in the owned FreeBSD fixture')
    directory = Path('/var/run/wpa_supplicant')
    if not directory.is_dir() or list(directory.iterdir()):
        raise RuntimeError('requires an existing empty native control directory')
    endpoint = directory / 'ws109wire'
    ssid = 'WS109"\\wire'
    key = 'test"\\passphrase'
    encoded = 'WS109\\x22\\x5cwire'
    commands = []
    monitors = set()
    errors = []
    stopped = threading.Event()
    peer = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
    peer.bind(str(endpoint))
    identity = endpoint.stat().st_ino
    peer.settimeout(0.1)
    state = {'wpa_state': 'DISCONNECTED', 'scan': 0, 'ssid': '', 'key': ''}

    with tempfile.TemporaryDirectory(prefix='ws109-wpa-proof-') as temporary:
        persisted = Path(temporary) / 'mock-config.json'

        def send(data, destination):
            try:
                peer.sendto(data.encode(), destination)
            except OSError as error:
                # A closed watch removes its own socket before late mock replies.
                if error.errno not in (errno.ENOENT, errno.ECONNREFUSED):
                    raise

        def event(text):
            for destination in tuple(monitors):
                send('<3>' + text + '\n', destination)

        def serve():
            try:
                while not stopped.is_set():
                    try:
                        data, source = peer.recvfrom(4096)
                    except socket.timeout:
                        continue
                    command = data.decode('ascii')
                    commands.append(command)
                    if command == 'ATTACH':
                        monitors.add(source)
                        send('OK\n', source)
                    elif command == 'DETACH':
                        monitors.discard(source)
                        send('OK\n', source)
                    elif command == 'STATUS':
                        reply = 'wpa_state=' + state['wpa_state'] + '\n'
                        if state['wpa_state'] == 'COMPLETED':
                            reply += 'ssid=' + encoded + '\n'
                        send(reply, source)
                    elif command == 'LIST_NETWORKS':
                        reply = 'network id / ssid / bssid / flags\n'
                        if state['ssid']:
                            reply += '7\t' + encoded + '\tany\t[DISABLED]\n'
                        send(reply, source)
                    elif command == 'ADD_NETWORK':
                        assert not state['ssid'], 'unexpected duplicate profile'
                        send('7\n', source)
                    elif command.startswith('SET_NETWORK 7 ssid '):
                        assert command == 'SET_NETWORK 7 ssid ' + ssid.encode().hex()
                        state['ssid'] = ssid
                        send('OK\n', source)
                    elif command.startswith('SET_NETWORK 7 psk '):
                        quoted = key.replace('\\', '\\\\').replace('"', '\\"')
                        assert command == 'SET_NETWORK 7 psk "' + quoted + '"'
                        state['key'] = key
                        send('OK\n', source)
                    elif command == 'SAVE_CONFIG':
                        assert state['ssid'] == ssid and state['key'] == key
                        assert 'SELECT_NETWORK 7' not in commands
                        assert 'ENABLE_NETWORK 7' not in commands
                        persisted.write_text(json.dumps({'ssid': ssid, 'key': key}))
                        send('OK\n', source)
                    elif command == 'SCAN':
                        state['scan'] += 1
                        if state['scan'] == 1:
                            send('OK\n', source)
                            event('CTRL-EVENT-SCAN-RESULTS')
                        else:
                            # Deliberately withhold the second reply to prove the real deadline.
                            assert state['scan'] == 2
                    elif command == 'SCAN_RESULTS':
                        rows = 'bssid / frequency / signal level / flags / ssid\n'
                        rows += '00:00:00:00:00:01\t2412\t-70\t[WPA2-PSK-CCMP][ESS]\t' + encoded + '\n'
                        rows += '00:00:00:00:00:02\t2437\t-50\t[ESS]\topen\n'
                        rows += '00:00:00:00:00:03\t2462\t-35\t[WPA2-PSK-CCMP][ESS]\t' + encoded + '\n'
                        send(rows, source)
                    elif command == 'ENABLE_NETWORK 7':
                        assert persisted.is_file()
                        send('OK\n', source)
                    elif command == 'SELECT_NETWORK 7':
                        assert 'ENABLE_NETWORK 7' in commands
                        state['wpa_state'] = 'COMPLETED'
                        send('OK\n', source)
                        event('CTRL-EVENT-CONNECTED')
                    elif command == 'DISCONNECT':
                        assert 'SELECT_NETWORK 7' in commands
                        state['wpa_state'] = 'DISCONNECTED'
                        send('OK\n', source)
                        event('CTRL-EVENT-DISCONNECTED')
                    else:
                        raise AssertionError('unexpected client command: ' + command)
            except BaseException as error:
                errors.append(error)

        worker = threading.Thread(target=serve)
        worker.start()
        try:
            completed = subprocess.run([probe], text=True, capture_output=True, timeout=20)
            print(completed.stdout, end='')
            print(completed.stderr, end='', file=sys.stderr)
            if errors:
                raise errors[0]
            completed.check_returncode()
            assert json.loads(persisted.read_text()) == {'ssid': ssid, 'key': key}
            assert commands.count('ADD_NETWORK') == 1
            assert commands.count('SAVE_CONFIG') == 1
            assert commands.count('ENABLE_NETWORK 7') == 1
            assert commands.count('SELECT_NETWORK 7') == 1
            assert commands.count('DISCONNECT') == 1
            assert state['scan'] == 2
            print('PASS independent peer exact escaping, mock-file persistence, explicit join ordering')
            print(json.dumps(commands))
        finally:
            stopped.set()
            worker.join(timeout=2)
            peer.close()
            if worker.is_alive():
                raise RuntimeError('fixture server did not stop')
            if endpoint.exists():
                current = endpoint.stat()
                if current.st_ino != identity or not stat.S_ISSOCK(current.st_mode):
                    raise RuntimeError('fixture endpoint ownership changed')
                endpoint.unlink()


if __name__ == '__main__':
    run(sys.argv[1])
