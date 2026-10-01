#!/bin/sh
# zedBSD; Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
# Compares maintained package source lists without running the target build.
set -eu
exec python3 - <<'PY'
from pathlib import Path
import fnmatch
import re
import sys

failed = False
for path in sorted(Path('userland').glob('*/*/Makefile.linux')):
	linux = path.read_text()
	target_path = path.with_name('Makefile')
	target = target_path.read_text() if target_path.exists() else ''
	# Tokens are literal source paths; variable references do not supply source membership.
	target_sources = set(re.findall(r'userland/[^\s,()]+\.c', target))
	linux_sources = set(re.findall(r'userland/[^\s,()]+\.c', linux))
	rules = {'skip': [], 'only': []}
	for kind, pattern in re.findall(r'^# keiland-linux-sync: (skip|only) (\S+)', linux, re.M):
		rules[kind].append(pattern)
	for source in sorted(target_sources - linux_sources):
		if '/zedbsd/' in source or any(fnmatch.fnmatchcase(source, glob) for glob in rules['skip']):
			continue
		print(f'makefile-sync: FAIL {path.parent} missing {source}')
		failed = True
	for source in sorted(linux_sources - target_sources):
		if '/linux/' in source or '/wpa/' in source or any(fnmatch.fnmatchcase(source, glob) for glob in rules['only']):
			continue
		print(f'makefile-sync: FAIL {path.parent} extra {source}')
		failed = True
print('makefile-sync: FAIL' if failed else 'makefile-sync: PASS')
sys.exit(1 if failed else 0)
PY
