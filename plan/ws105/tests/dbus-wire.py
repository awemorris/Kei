#!/usr/bin/env python3
# zedBSD; Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
"""Marshals peer frames independently and checks production public receive paths."""
import array
import errno
import os
import socket
import struct
import subprocess
import sys
import time


def align(data, boundary):
    return data + bytes((-len(data)) % boundary)


def string(data, value, signature=False):
    encoded = value.encode()
    if signature:
        return data + bytes([len(encoded)]) + encoded + b'\0'
    return align(data, 4) + struct.pack('<I', len(encoded)) + encoded + b'\0'


def frame(kind, serial, fields, body=b''):
    encoded = bytes(16)
    for code, typ, value in fields:
        encoded = align(encoded, 8) + bytes([code])
        encoded = string(encoded, typ, True)
        if typ == 'u':
            encoded = align(encoded, 4) + struct.pack('<I', value)
        else:
            encoded = string(encoded, value, typ == 'g')
    header = b'l' + bytes([kind, 0, 1]) + struct.pack('<III', len(body), serial, len(encoded) - 16)
    return align(header + encoded[16:], 8) + body


def signal(number, declared=1):
    return frame(4, number + 2, [(1, 'o', '/peer'), (2, 's', 'org.example.Peer'),
        (3, 's', 'ResumeDevice'), (8, 'g', 'uuh'), (9, 'u', declared)],
        struct.pack('<III', 13, number + 64, 0))


def read_request(peer):
    data = b''
    while len(data) < 16:
        data += peer.recv(16 - len(data))
    body, _, headers = struct.unpack_from('<III', data, 4)
    total = ((16 + headers + 7) & ~7) + body
    while len(data) < total:
        data += peer.recv(total - len(data))


def check(name, expected, sender):
    parent, child = socket.socketpair()
    process = subprocess.Popen([sys.argv[1], str(child.fileno()), str(expected)], pass_fds=[child.fileno()],
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    child.close()
    try:
        sender(parent)
        output, errors = process.communicate(timeout=8)
        print(name, output.strip(), errors.strip())
        assert process.returncode == 0, (name, process.returncode)
    finally:
        parent.close()
        if process.poll() is None:
            process.kill()
            process.wait()


def interleave(peer):
    read_request(peer)
    for number in range(2):
        fd = os.memfd_create('dbus-wire', os.MFD_CLOEXEC)
        try:
            os.write(fd, bytes([ord('a') + number]))
            os.lseek(fd, 0, os.SEEK_SET)
            packet = signal(number)
            peer.sendmsg([packet[:5]], [(socket.SOL_SOCKET, socket.SCM_RIGHTS, array.array('i', [fd]))])
            for byte in packet[5:]:
                peer.sendall(bytes([byte]))
                time.sleep(.0001)
        finally:
            os.close(fd)
    peer.sendall(frame(2, 9, [(5, 'u', 1), (8, 'g', 'u')], struct.pack('<I', 1337)))


def partial_right(peer):
    fd = os.memfd_create('partial', os.MFD_CLOEXEC)
    try:
        peer.sendmsg([signal(0)[:3]], [(socket.SOL_SOCKET, socket.SCM_RIGHTS, array.array('i', [fd]))])
        peer.shutdown(socket.SHUT_WR)
    finally:
        os.close(fd)


def overflow_rights(peer):
    fds = [os.memfd_create('overflow', os.MFD_CLOEXEC) for _ in range(17)]
    try:
        peer.sendmsg([signal(0, 17)], [(socket.SOL_SOCKET, socket.SCM_RIGHTS, array.array('i', fds))])
    finally:
        for fd in fds:
            os.close(fd)


check('fragmented/interleaved/fd-boundaries', 0, interleave)
check('undeclared-missing-right', errno.EPROTO, lambda p: p.sendall(signal(0)))
check('oversized-body', errno.EOVERFLOW, lambda p: p.sendall(b'l\x04\x00\x01' + struct.pack('<III', 0xffffffff, 1, 0)))
check('partial-frame-right/EOF', errno.EPIPE, partial_right)
check('ancillary-overflow', errno.EPROTO, overflow_rights)
print('dbus-wire: ALL PASS')
