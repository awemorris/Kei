# zedBSD; Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
"""Exercise the portable launcher with real shell processes and owned test paths."""
from pathlib import Path
import json
import os
import stat
import subprocess
import sys
import tempfile

source = Path(sys.argv[1]).read_text()
with tempfile.TemporaryDirectory(prefix='keiland-launcher-') as directory:
    base = Path(directory)
    prefix = base / 'prefix with spaces'
    (prefix / 'bin').mkdir(parents=True)
    fake = prefix / 'bin/wayland'
    fake.write_text('''#!/usr/bin/env python3
import json, os, signal, sys, time
waiting = '--wait-for-signal' in sys.argv
if waiting:
    signal.signal(signal.SIGTERM, lambda signum, frame: sys.exit(0))
print(json.dumps({'args': sys.argv[1:], 'runtime': os.environ.get('XDG_RUNTIME_DIR'), 'display': os.environ.get('WAYLAND_DISPLAY'), 'seat': os.environ.get('KEILAND_SEAT'), 'session': os.environ.get('XDG_SESSION_ID'), 'type': os.environ.get('XDG_SESSION_TYPE'), 'pid': os.getpid()}), flush=True)
if waiting:
    while True:
        signal.pause()
sys.exit(37)
''')
    fake.chmod(0o755)
    launcher = prefix / 'bin/keiland-desktop'
    launcher.write_text(source.replace('@PREFIX@', str(prefix)))
    launcher.chmod(0o755)
    home = base / 'home'
    home.mkdir()
    env = dict(os.environ, HOME=str(home))
    for name in ['XDG_RUNTIME_DIR', 'WAYLAND_DISPLAY', 'KEILAND_SEAT', 'XDG_SESSION_ID', 'XDG_SESSION_TYPE']:
        env.pop(name, None)
    result = subprocess.run([str(launcher), '--wallpaper=a picture.ppm', 'literal $value'], env=env, capture_output=True, text=True)
    assert result.returncode == 37, result
    got = json.loads(result.stdout)
    runtime = home / '.cache/keiland-runtime'
    assert got['runtime'] == str(runtime)
    assert stat.S_IMODE(runtime.stat().st_mode) == 0o700
    assert got['display'] == 'wayland-keiland'
    assert got['args'] == ['--session', '--glass', '--socket='+str(runtime/'wayland-keiland'), '--wallpaper='+str(prefix/'share/keiland/wallpaper.ppm'), '--wallpaper=a picture.ppm', 'literal $value']
    print('PASS console runtime, native defaults, prefix/argv spaces and exact exit status')

    managed = base / 'managed-runtime'
    managed.mkdir(mode=0o750)
    before = stat.S_IMODE(managed.stat().st_mode)
    session_env = dict(env, XDG_RUNTIME_DIR=str(managed), WAYLAND_DISPLAY='custom-0', KEILAND_SEAT='logind', XDG_SESSION_ID='42', XDG_SESSION_TYPE='wayland')
    result = subprocess.run([str(launcher)], env=session_env, capture_output=True, text=True)
    assert result.returncode == 37
    got = json.loads(result.stdout)
    assert got['runtime'] == str(managed) and got['display'] == 'custom-0'
    assert got['seat'] == 'logind' and got['session'] == '42' and got['type'] == 'wayland'
    assert '--socket='+str(managed/'custom-0') in got['args']
    assert stat.S_IMODE(managed.stat().st_mode) == before
    print('PASS existing native session environment and runtime permissions preserved')

    absolute = str(base / 'absolute-socket')
    result = subprocess.run([str(launcher)], env=dict(session_env, WAYLAND_DISPLAY=absolute), capture_output=True, text=True)
    assert result.returncode == 37 and '--socket='+absolute in json.loads(result.stdout)['args']
    for bad in ['relative-path', str(base/'missing')]:
        result = subprocess.run([str(launcher)], env=dict(env, XDG_RUNTIME_DIR=bad), capture_output=True, text=True)
        assert result.returncode != 0 and not result.stdout
    linked = base/'linked-home'
    (linked/'.cache').mkdir(parents=True)
    (linked/'.cache/keiland-runtime').symlink_to(managed, target_is_directory=True)
    result = subprocess.run([str(launcher)], env=dict(env, HOME=str(linked)), capture_output=True, text=True)
    assert result.returncode != 0 and not result.stdout
    assert stat.S_IMODE(managed.stat().st_mode) == before
    print('PASS absolute socket and rejection before launch for unusable runtime paths')

    process = subprocess.Popen([str(launcher), '--wait-for-signal'], env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    try:
        got = json.loads(process.stdout.readline())
        assert got['pid'] == process.pid
        process.terminate()
        assert process.wait(timeout=5) == 0
    finally:
        if process.poll() is None:
            process.kill()
            process.wait()
    print('PASS exec PID identity and normal SIGTERM delivery')
print('ALL PASS on '+sys.platform)
