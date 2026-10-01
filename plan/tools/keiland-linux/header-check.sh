#!/bin/sh
# zedBSD; Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
# Audits system-inclusive dependency output; ordinary .d files omit system headers.
set -eu
BUILD=${KEILAND_LINUX_BUILD:-build/keiland-linux}
make -s -B -f userland/desktop/keiland-linux.mk header-dependencies CC="${CC:-cc}" KEILAND_LINUX_BUILD="$BUILD"
make -s -f userland/desktop/keiland-linux.mk print-sources KEILAND_LINUX_BUILD="$BUILD" > "$BUILD/header-check/sources.txt"
exec python3 - "$BUILD" <<'PY'
from pathlib import Path
import re
import sys

build = Path(sys.argv[1])
sources = (build / 'header-check/sources.txt').read_text().splitlines()
failed = False
for source in sources:
	dependency = build / 'header-check' / Path(source).with_suffix('.d')
	content = dependency.read_text().replace('\\\n', ' ')
	for header in content.split():
		if re.search(r'/usr/include/(?:[^/]+/)?(?:wayland-|EGL/|GLES(?:2|3)?/)', header):
			print(f'header-check: FAIL {source} {header}')
			failed = True
print(f'header-check: {"FAIL" if failed else "PASS"} ({len(sources)} sources)')
sys.exit(1 if failed else 0)
PY
