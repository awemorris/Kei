#!/usr/bin/env python3.11
# zedBSD; Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
"""Run only inside an owned FreeBSD fixture; restore native mixer permissions."""
import argparse
from pathlib import Path
import select
import stat
import subprocess
import sys

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('probe', type=Path)
args = parser.parse_args()
if not sys.platform.startswith('freebsd'):
    parser.error('this fixture requires native FreeBSD')

mixers = [p for p in Path('/dev').glob('mixer*') if p.is_char_device()]
if not mixers:
    parser.error('the owned fixture must expose a real mixer')
original = {p: stat.S_IMODE(p.stat().st_mode) for p in mixers}
child = None
try:
    for path in mixers:
        path.chmod(0)
    child = subprocess.Popen([str(args.probe.resolve())], stdin=subprocess.PIPE,
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    readable, _, _ = select.select([child.stdout], [], [], 10)
    if not readable:
        raise RuntimeError('probe did not reach native denial within 10 seconds')
    denied = child.stdout.readline()
    if denied != b'audio-retry: DENIED\n':
        raise RuntimeError(f'probe did not verify denial: {denied!r}')
    print(denied.decode(), end='')
    for path, mode in original.items():
        path.chmod(mode)
    output, errors = child.communicate(b'R', timeout=10)
    sys.stdout.write(output.decode())
    sys.stderr.write(errors.decode())
    if child.returncode != 0:
        raise RuntimeError(f'native retry probe exited {child.returncode}')
finally:
    for path, mode in original.items():
        path.chmod(mode)
    if child is not None and child.poll() is None:
        child.kill()
        child.wait(timeout=5)
for path, mode in original.items():
    if stat.S_IMODE(path.stat().st_mode) != mode:
        raise RuntimeError(f'permission restoration failed: {path}')
print('audio-retry fixture: PASS original native device permissions restored')
