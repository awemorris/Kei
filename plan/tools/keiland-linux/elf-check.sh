#!/bin/sh
# zedBSD; Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
# Audits the staged ELF files without loading their code.
set -eu
exec python3 - "$@" <<'PY'
from pathlib import Path
import os
import re
import subprocess
import sys

stage = Path(sys.argv[1])
prefix = os.environ.get('KEILAND_PREFIX', '/opt/keiland')
library_dir = stage / prefix.lstrip('/') / 'lib'
failed = False
count = 0
ours = {'libwayland-client.so', 'libkeiland.so', 'libkeiui.so', 'libtruetype.so', 'libpdf.so',
	'libz-compat.so', 'libpng-compat.so', 'libjpeg-compat.so', 'libgif-compat.so', 'libvulkan.so.1'}
for path in sorted(stage.rglob('*')):
	if not path.is_file():
		continue
	with path.open('rb') as stream:
		if stream.read(4) != b'\x7fELF':
			continue
	count += 1
	output = subprocess.check_output(['readelf', '-d', str(path)], text=True, timeout=10)
	runpaths = re.findall(r'\(RUNPATH\).*\[(.*?)\]', output)
	if runpaths != [prefix + '/lib']:
		print(f'elf-check: FAIL {path} RUNPATH {runpaths}')
		failed = True
	if path.parent == library_dir:
		sonames = re.findall(r'\(SONAME\).*\[(.*?)\]', output)
		if path.name not in ours or sonames != [path.name]:
			print(f'elf-check: FAIL {path} SONAME {sonames}')
			failed = True
	for needed in re.findall(r'\(NEEDED\).*\[(.*?)\]', output):
		if needed in ours and not (library_dir / needed).is_file():
			print(f'elf-check: FAIL {path} missing NEEDED {needed}')
			failed = True
if count == 0:
	print('elf-check: FAIL no ELF files')
	failed = True
print(f'elf-check: {"FAIL" if failed else "PASS"} ({count} ELF)')
sys.exit(1 if failed else 0)
PY
