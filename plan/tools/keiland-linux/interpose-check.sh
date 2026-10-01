#!/bin/sh
# zedBSD; Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
# Checks both default binding isolation and the supported binding opt-out.
set -eu
unset WAYLAND_DISPLAY DISPLAY KEILAND_VULKAN_BACKEND KEILAND_VULKAN_NO_DEEPBIND
BUILD=${KEILAND_LINUX_BUILD:-build/keiland-linux}
mkdir -p "$BUILD/test/empty-xdg"
BUILD=$(CDPATH= cd -- "$BUILD" && pwd)
STAGE=${STAGE:-$BUILD/stage/opt/keiland/lib}
STAGE=$(CDPATH= cd -- "$STAGE" && pwd)
export KEILAND_DRM_DEVICE=none
export XDG_RUNTIME_DIR="$BUILD/test/empty-xdg"
export LD_LIBRARY_PATH="$STAGE"
mkdir -p "$XDG_RUNTIME_DIR"
chmod 700 "$XDG_RUNTIME_DIR"
timeout 60 env LD_DEBUG=bindings "$BUILD/test/vk-chain-test" > "$BUILD/test/interpose-default.out" 2> "$BUILD/test/interpose-bindings.log"
python3 - "$BUILD/test/interpose-bindings.log" "$STAGE/libvulkan.so.1" <<'PY'
from pathlib import Path
import re
import sys

ours = Path(sys.argv[2]).resolve()
bindings = []
for line in Path(sys.argv[1]).read_text().splitlines():
	match = re.search(r'binding file (.*?) \[\d+\] to (.*?) \[\d+\]:.*symbol [`\x27](vk\w+)', line)
	if not match:
		continue
	source, destination, name = match.groups()
	if Path(source).name.startswith('libvulkan.so') and Path(source).resolve() != ours and Path(destination).resolve() == ours:
		bindings.append(line)
for line in bindings:
	print(line)
print(f'interpose: backend-to-compat bindings={len(bindings)}')
if bindings:
	sys.exit(1)
PY
timeout 60 env KEILAND_VULKAN_NO_DEEPBIND=1 "$BUILD/test/vk-chain-test" > "$BUILD/test/interpose-optout.out" 2> "$BUILD/test/interpose-optout.err"
rg -q '^vk-chain-test: PASS$' "$BUILD/test/interpose-default.out"
rg -q '^vk-chain-test: PASS$' "$BUILD/test/interpose-optout.out"
printf 'interpose-check: PASS\n'
