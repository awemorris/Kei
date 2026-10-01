# zedBSD; Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
"""Audit the private native install and exercise real application connection refusal."""
from pathlib import Path
import os
import shutil
import subprocess
import sys
import tempfile

stage = Path(sys.argv[1]).resolve()
prefix = Path('/opt/keiland')
assert not prefix.exists(), 'The fixture must not replace an existing installation'
assert (stage / 'bin/wayland').is_file()
environment = dict(os.environ)
environment.pop('WAYLAND_SOCKET', None)
environment.pop('LD_LIBRARY_PATH', None)
environment['VK_DRIVER_FILES'] = '/usr/local/share/vulkan/icd.d/lvp_icd.x86_64.json'
try:
    prefix.parent.mkdir(exist_ok=True)
    shutil.copytree(stage, prefix)
    for binary in sorted([*prefix.glob('bin/*'), *prefix.glob('libexec/*'), *prefix.glob('lib/*.so*')]):
        linked = subprocess.run(['ldd', str(binary)], env=environment, capture_output=True, text=True, check=True, timeout=10)
        assert 'not found' not in linked.stdout, linked.stdout
        assert 'libbasu' not in linked.stdout, linked.stdout
        for line in linked.stdout.splitlines():
            if 'libseat.so.1 =>' in line:
                assert '=> /opt/keiland/lib/libseat.so.1 ' in line, line
        header = subprocess.run(['readelf', '-h', '-d', str(binary)], capture_output=True, text=True, check=True, timeout=10)
        assert 'FreeBSD' in header.stdout, header.stdout
        if binary.name == 'libseat.so.1':
            dependencies = [line for line in header.stdout.splitlines() if '(NEEDED)' in line]
            assert len(dependencies) == 1 and '[libc.so.7]' in dependencies[0]
        else:
            assert '/opt/keiland/lib' in header.stdout, header.stdout
        print('ELF', binary.name, flush=True)
        print(linked.stdout, end='', flush=True)
    for line in (prefix / 'etc/keiland/apps.conf').read_text().splitlines():
        if line and not line.startswith('#'):
            command = line.split('|')[1].split()[0]
            assert Path(command).is_file(), command
    dictionaries = prefix / 'share/kei/ime/ja'
    assert (dictionaries / 'SKK-JISYO.kei').stat().st_size > 0
    assert (dictionaries / 'SKK-JISYO.X').stat().st_size > 0
    assert (prefix / 'share/mview/qs40/model.txt').stat().st_size > 0
    assert len(list((prefix / 'share/mview/qs40/tex').glob('*.pam'))) > 0
    assert len(list((prefix / 'share/keiland/wallpapers').glob('*.ppm'))) == 5
    assert (prefix / 'share/keiland/wallpaper.ppm').read_bytes() == (prefix / 'share/keiland/wallpapers/Aurora.ppm').read_bytes()
    assert (prefix / 'share/fonts/keiland.ttf').is_file()
    assert (prefix / 'share/fonts/keiland-mono.ttf').is_file()
    assert (prefix / 'share/fonts/keiland-fallback.ttf').is_file()
    assert (prefix / 'share/fonts/keiland-emoji.ttf').is_file()
    assert (prefix / 'share/licenses/libseat/LICENSE').is_file()
    assert not (prefix / 'include/pty.h').exists()
    assert not (prefix / 'include/sys/xattr.h').exists()
    print('PASS App Home/data/model/textures/font/private-header boundaries', flush=True)
    with tempfile.TemporaryDirectory(prefix='ws109-app-runtime-', dir='/tmp') as runtime:
        environment['HOME'] = runtime
        environment['XDG_CONFIG_HOME'] = runtime
        environment['XDG_DATA_HOME'] = runtime
        environment['XDG_RUNTIME_DIR'] = runtime
        environment['WAYLAND_DISPLAY'] = 'ws109-absent-display'
        for name in ['terminal', 'files', 'settings', 'notes', 'textedit', 'imageview', 'pdfviewer', 'wlshm', 'wltest', 'mview', 'kuidemo', 'keiland-ime']:
            directory = 'libexec' if name == 'keiland-ime' else 'bin'
            result = subprocess.run([str(prefix / directory / name)], cwd=runtime, env=environment, capture_output=True, text=True, timeout=10)
            print('REFUSAL', name, 'exit', result.returncode, result.stdout, result.stderr, flush=True)
            assert result.returncode == 1, name
        result = subprocess.run([str(prefix / 'bin/vkdemo'), '--ws109-invalid-option'], cwd=runtime, env=environment, capture_output=True, text=True, timeout=10)
        assert result.returncode == 2 and 'usage: vkdemo' in result.stderr
        print('PASS actual application refusal/CLI entry (GUI operation not exercised)', flush=True)
finally:
    if prefix.exists():
        shutil.rmtree(prefix)
print('PASS owned installation/runtime fixtures removed', flush=True)
