#!/bin/sh
# Checks Keiland's common-source OS boundary and installation paths.
# Usage: sh plan/tools/keiland-os-boundary/check.sh (from any directory).
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../.."
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT HUP INT TERM
status=0

# Collect common compositor and system-library sources, excluding OS modules.
find userland/desktop/libkeiland userland/desktop/wayland \
    \( -path '*/zedbsd' -o -path '*/linux' -o -path '*/freebsd' -o -path '*/wpa' \) -prune \
    -o -name '*.[ch]' -print | LC_ALL=C sort > "$work/common"

# The compositor and libkeiland include no OS header (the evdev header choice is
# libkeiland-backend's keiland-backend-evdev.h since ws131-p007).
while IFS= read -r file; do
    awk '/^[[:space:]]*#[[:space:]]*include[[:space:]]*[<"](uapi\/|userland\/base\/)/ {print FILENAME ":" FNR ": " $0}' "$file"
done < "$work/common" > "$work/C1"

# No device ioctl in the compositor or libkeiland: the input devices are libkeiland-backend's (ws131-p007).
while IFS= read -r file; do
    awk '/ioctl[[:space:]]*\(/ {print FILENAME ":" FNR ": " $0}' "$file"
done < "$work/common" > "$work/C2"

# A zedBSD wire layout must stay inside the zedBSD GPU backend.
find userland/desktop/wayland -path '*/zedbsd' -prune -o -name '*.[ch]' -print |
while IFS= read -r file; do
    awk '/zwl_buffer_layout/ {print FILENAME ":" FNR ": " $0}' "$file"
done > "$work/C3"

# Inspect each literal so a system-shell exception cannot hide another path.
find userland/desktop -path '*/sessiond' -prune -o -path '*/keiland' -prune \
    -o -name '*.[ch]' -print |
while IFS= read -r file; do
    [ "$file" != userland/desktop/paths.h ] || continue
    awk '{
        remaining = $0
        while (match(remaining, /"([^"\\]|\\.)*"/)) {
            literal = substr(remaining, RSTART + 1, RLENGTH - 2)
            if (literal ~ /^\/(usr\/share|usr\/libexec|etc\/keiland|bin\/|usr\/bin\/)/ &&
                literal != "/bin/sh" && literal !~ /^\/bin\/sh /) {
                print FILENAME ":" FNR ": " $0
                break
            }
            remaining = substr(remaining, RSTART + RLENGTH)
        }
    }' "$file"
done > "$work/C4"

# Desktop headers are owned by desktop rather than libc.
find include/libc \( -name 'keiland.h' -o -name 'keiui.h' -o -name 'truetype.h' \
    -o -name 'browser.h' -o -name 'wayland*' -o -name 'xdg-shell*' \
    -o -name 'primary-selection*' -o -name 'tablet-unstable*' \) -print > "$work/C5"

# Linux selection belongs to the OS modules; the evdev header bridges constants.
find userland/desktop \
    \( -path '*/zedbsd' -o -path '*/linux' -o -path '*/freebsd' -o -path '*/wpa' \
    -o -path 'userland/desktop/libkeiland-backend-*' \) -prune \
    -o -name '*.[ch]' -print |
while IFS= read -r file; do
    [ "$file" != userland/desktop/libkeiland-backend/keiland-backend-evdev.h ] || continue
    awk '/^[[:space:]]*#[[:space:]]*(if|ifdef|elif).*(__linux__|__FreeBSD__)/ {print FILENAME ":" FNR ": " $0}' "$file"
done > "$work/L1"

# Each OS module consumes only its own kernel and service interfaces (libkeiland-backend's trees, WS131).
find userland/desktop/wayland/linux \
    userland/desktop/libkeiland-backend-linux userland/desktop/libkeiland-backend-freebsd \
    userland/desktop/libkeiland-backend/wpa -name '*.[ch]' -print 2>/dev/null |
while IFS= read -r file; do
    awk '/^[[:space:]]*#[[:space:]]*include[[:space:]]*[<"](uapi\/|userland\/base\/(net|audiod)\/)/ {print FILENAME ":" FNR ": " $0}' "$file"
done > "$work/L2"
find userland/desktop/libkeiland/zedbsd userland/desktop/wayland/zedbsd \
    userland/desktop/libkeiland-backend-zedbsd -name '*.[ch]' -print 2>/dev/null |
while IFS= read -r file; do
    awk '/^[[:space:]]*#[[:space:]]*include[[:space:]]*[<"](linux\/|drm\/|sound\/)/ {print FILENAME ":" FNR ": " $0}' "$file"
done > "$work/L3"

# The Linux Vulkan frontend never includes or compiles zedBSD Vulkan sources.
find userland/desktop/libvulkan-compat -type f \
    \( -name '*.[ch]' -o -name 'Makefile.linux' \) -print |
while IFS= read -r file; do
    awk '/^[[:space:]]*#[[:space:]]*include.*(userland\/desktop\/libvulkan\/|\.\.\/libvulkan\/)/ ||
         /^[^#]*userland\/desktop\/libvulkan\/.*\.c/ {print FILENAME ":" FNR ": " $0}' "$file"
done > "$work/L4"
make -s -f userland/desktop/keiland-linux.mk print-sources > "$work/linux-sources"
awk '/^userland\/desktop\/libvulkan\// {print "compiled Linux source: " $0}' "$work/linux-sources" >> "$work/L4"

# libkeiland-backend never reaches into the compositor or the applications' library (WS131 B1).
find userland/desktop/libkeiland-backend userland/desktop/libkeiland-backend-zedbsd \
    userland/desktop/libkeiland-backend-linux userland/desktop/libkeiland-backend-freebsd \
    -name '*.[ch]' -print 2>/dev/null |
while IFS= read -r file; do
    awk '/^[[:space:]]*#[[:space:]]*include[[:space:]]*([<"]userland\/desktop\/wayland\/|"[^"]*zwl[^"]*\.h"|<keiland\.h>|<keiui\.h>)/ {print FILENAME ":" FNR ": " $0}' "$file"
done > "$work/B1"

# Only the compositor uses libkeiland-backend; libkeiland forwards through it only in
# system-compat.c and audio-compat.c until Settings reaches the network and the sound through the compositor (WS131 B3,
# ws131-p011).
find userland/desktop -path 'userland/desktop/wayland' -prune \
    -o -path 'userland/desktop/libkeiland-backend*' -prune \
    -o -name '*.[ch]' -print |
while IFS= read -r file; do
    case $file in
    userland/desktop/libkeiland/system-compat.c|userland/desktop/libkeiland/audio-compat.c) continue ;;
    esac
    awk '/^[[:space:]]*#[[:space:]]*include[[:space:]]*[<"].*keiland-backend\.h[>"]/ {print FILENAME ":" FNR ": " $0}' "$file"
done > "$work/B3"

# libkeiland keeps no operating-system directory: its OS code is libkeiland-backend's (WS131 L6, ws131-p004).
for os_dir in zedbsd linux freebsd; do
    if [ -e "userland/desktop/libkeiland/$os_dir" ]; then
        echo "userland/desktop/libkeiland/$os_dir: an OS directory in libkeiland"
    fi
done > "$work/L6"

# Inspect the actual target package membership and wildcard filename boundary.
make -pn disk-image > "$work/make-database"
python3 - "$work/make-database" > "$work/L5" <<'PY'
from pathlib import Path
import fnmatch
import re
import sys
text = Path('Makefile').read_text()
patterns = re.findall(r'\$\(wildcard ([^)]+)\)', text)
for path in Path('userland').rglob('Makefile.linux'):
    for pattern in patterns:
        if fnmatch.fnmatchcase(str(path), pattern):
            print(f'{path}: matches top-level wildcard {pattern}')
for line in Path(sys.argv[1]).read_text().splitlines():
    if line.startswith('USERLAND_PACKAGE_MAKEFILES :=') and 'Makefile.linux' in line:
        print(line)
    if line.startswith('MAKEFILE_LIST :='):
        for name in line.split()[2:]:
            if name.endswith('Makefile.linux'):
                print(f'target includes Linux rules: {name}')
PY

# Report every violated condition before returning the aggregate outcome.
for check in C1 C2 C3 C4 C5 L1 L2 L3 L4 L5 L6 B1 B3; do
    if [ -s "$work/$check" ]; then
        while IFS= read -r detail; do
            printf 'check: %s FAIL %s\n' "$check" "$detail"
        done < "$work/$check"
        status=1
    else
        printf 'check: %s PASS\n' "$check"
    fi
done
if [ "$status" -ne 0 ]; then
    echo 'keiland-os-boundary: FAIL'
    exit 1
fi
echo 'keiland-os-boundary: PASS'
