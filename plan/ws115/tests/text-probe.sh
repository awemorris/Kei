#!/bin/sh
# ws115-p006: build the text probe for the target against the staged packages
# and print the extra files that put it, and the text fonts, into a test image.
# Run from the repository root after `make libpng freetype harfbuzz fontconfig`.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
#
# usage: plan/ws115/tests/text-probe.sh <cross-dir> <package-workroot> <work-dir> [machine]
set -eu

test $# -ge 3 || { echo "usage: ${0##*/} <cross-dir> <package-workroot> <work-dir> [machine]" >&2; exit 1; }
cross=$1
workroot=$(CDPATH= cd -- "$2" && pwd)
mkdir -p "$3"
work=$(CDPATH= cd -- "$3" && pwd)
machine=${4:-amd64}
source=$(CDPATH= cd -- "$(dirname -- "$0")/text-probe" && pwd)

# The view external.mk would give a package that builds against all four.
view="$work/view"
rm -rf "$view"
mkdir -p "$view/usr"
for package in zlib libpng freetype expat libffi pcre2 glib harfbuzz fontconfig; do
	cp -as "$workroot/$package/stage/usr/." "$view/usr/"
done

# The probe, compiled and linked through the cross wrapper with the flags
# pkg-config gives for the three libraries it calls.
flags=$(ZEDBSD_PKG_CONFIG_VIEW=$view "$cross/bin/zedbsd-pkg-config" --cflags --libs fontconfig freetype2 harfbuzz)
"$cross/bin/zedbsd-clang" -std=gnu11 -Wall -Wextra -Werror -O2 \
	-o "$work/text-probe" "$source/text-probe.c" $flags \
	"-Wl,-rpath-link,$view/usr/lib"
python3 tools/build/check-dynamic-elf.py --machine "$machine" --role application \
	--needed libfontconfig.so.1 --needed libfreetype.so.6 \
	--needed libharfbuzz.so.0 --needed libc.so "$work/text-probe"

# The text fonts the desktop installs (userland/desktop/wayland), so the
# image carries fonts without the compositor; the colour emoji font comes
# with the noto-color-emoji package.
fonts=userland/desktop/fonts
echo "ZEDBSD_TEST_EXTRA_FILES=--file /usr/bin/text-probe=$work/text-probe --mode /usr/bin/text-probe=0755 --file /usr/share/fonts/keiland.ttf=$PWD/$fonts/Inter.ttf --file /usr/share/fonts/keiland-mono.ttf=$PWD/$fonts/JetBrainsMono-Regular.ttf --file /usr/share/fonts/keiland-fallback.ttf=$PWD/$fonts/DroidSansFallbackFull.ttf"
