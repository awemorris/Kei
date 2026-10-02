# zedBSD; Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
"""Check real native selected source objects, system headers and installed shared ABI."""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys

build = Path(sys.argv[1])
stage = Path(sys.argv[2])
command = ['gmake', '-s', '-f', 'userland/desktop/keiland-freebsd.mk', 'print-sources', 'KEILAND_FREEBSD_BUILD=' + str(build)]
sources = subprocess.check_output(command, text=True).splitlines()
headers = set()
for source in sources:
    assert (build / 'obj' / Path(source).with_suffix('.o')).is_file(), source
    dependency = build / 'header-check' / Path(source).with_suffix('.d')
    words = dependency.read_text().replace('\\\n', ' ').split()
    headers.update(word for word in words if not word.endswith(':'))
for header in headers:
    assert not re.search(r'/usr/local/include/(wayland(?:/|-)|EGL/|GLES[23]?/|basu/)', header), header
    assert not re.search(r'(^|/)include/(uapi/|libc/(stdio|stdlib|unistd|sys/socket)\.h)', header), header
    assert '/usr/include/linux/' not in header, header
assert '/usr/include/sys/socket.h' in headers
assert '/usr/include/sys/ioccom.h' in headers
assert any('/usr/local/include/vulkan/' in header for header in headers)
libraries = []
for library in sorted((stage / 'lib').glob('*.so*')):
    dynamic = subprocess.check_output(['readelf', '-d', str(library)], text=True)
    soname = re.findall(r'\(SONAME\).*\[(.*?)\]', dynamic)
    needed = re.findall(r'\(NEEDED\).*\[(.*?)\]', dynamic)
    assert soname == [library.name], (library, soname)
    assert 'libc.so.7' in needed, (library, needed)
    assert 'libwayland-client.so.0' not in needed
    assert not any('basu' in dependency for dependency in needed)
    libraries.append({'file': library.name, 'soname': soname, 'needed': needed, 'sha256': hashlib.sha256(library.read_bytes()).hexdigest()})
assert len(libraries) == 11
assert not (stage / 'lib/libvulkan.so').exists()
assert (stage / 'include/libseat.h').is_file()
assert (stage / 'include/wayland-client.h').is_file()
assert (stage / 'include/keiui.h').is_file()
assert (stage / 'include/pdf.h').is_file()
assert not (stage / 'include/drm').exists()
print(json.dumps({'status': 'PASS', 'source_memberships': len(sources), 'unique_sources': len(set(sources)), 'header_count': len(headers), 'libraries': libraries}, indent=2))
