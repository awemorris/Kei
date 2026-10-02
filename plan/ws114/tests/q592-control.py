#!/usr/bin/env python3
# zedBSD; Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
"""Recorded q592 control operations; use only in the P3-owned q592 runtime (SSH 10322)."""
import os, json, subprocess, time, importlib.util
from pathlib import Path
root = Path(__file__).resolve().parents[3]
os.environ.setdefault('GUEST_DIR', '/home/awe/zedBSD-worktrees/p9/build/p9-q580/guest')
os.environ.setdefault('GUEST_RUN', str(root / 'build/p3-q592/guest'))
os.environ.setdefault('SSH_PORT', '10322')
spec = importlib.util.spec_from_file_location('guest', root / 'plan/tools/keiland-linux/guest.py')
g = importlib.util.module_from_spec(spec)
spec.loader.exec_module(g)
evidence = root / 'plan/ws114/evidence/q592'
evidence.mkdir(parents=True, exist_ok=True)


def record(action, **args):
    with (evidence / 'actions.jsonl').open('a') as f:
        f.write(json.dumps({'utc': time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()), 'action': action, **args}) + '\n')


def ssh(cmd, name=None, timeout=120):
    record('ssh', command=cmd, evidence=name)
    p = subprocess.run(['ssh', *g.SSH_OPTIONS, '-p', g.PORT, g.SSH_USER + '@127.0.0.1', cmd],
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=timeout)
    if name:
        (evidence / name).write_text(p.stdout)
    return p.stdout


def shot(name):
    record('screenshot', name=name)
    g.screenshot(evidence / name)


def point(x, y):
    record('point', x=x, y=y)
    g.input_events([{'type': 'abs', 'data': {'axis': 'x', 'value': x * 32767 // 1279}},
                    {'type': 'abs', 'data': {'axis': 'y', 'value': y * 32767 // 799}}])


def button(down, kind='left'):
    record('button', down=down, kind=kind)
    g.input_events([{'type': 'btn', 'data': {'down': down, 'button': kind}}])


def click(x, y, kind='left'):
    point(x, y)
    time.sleep(.1)
    button(True, kind)
    button(False, kind)


def key(*keys):
    record('key', keys=keys)
    g.key(keys)


def drag(x, y, tx, ty):
    point(x, y)
    button(True)
    time.sleep(.2)
    for i in range(1, 11):
        point(x + (tx - x) * i // 10, y + (ty - y) * i // 10)
        time.sleep(.05)
    button(False)
    time.sleep(.5)


def wait_ready(limit=180):
    deadline = time.monotonic() + limit
    while not g.ready():
        if time.monotonic() > deadline:
            raise RuntimeError('SSH timeout')
        time.sleep(2)


def type_text(text):
    names = {' ': 'spc', '-': 'minus', '.': 'dot', '/': 'slash'}
    for ch in text:
        key(names.get(ch, ch))
        time.sleep(.08)
