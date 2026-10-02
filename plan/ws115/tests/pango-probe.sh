#!/bin/sh
# ws115-p007: build the Pango probe for the target against the staged packages
# and print the extra files that put it, and the text fonts, into a test image.
# Run from the repository root after `make pixman fribidi cairo pango`.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
#
# usage: plan/ws115/tests/pango-probe.sh <cross-dir> <package-workroot> <work-dir> [machine]
set -eu

test $# -ge 3 || { echo "usage: ${0##*/} <cross-dir> <package-workroot> <work-dir> [machine]" >&2; exit 1; }
cross=$1
workroot=$(CDPATH= cd -- "$2" && pwd)
mkdir -p "$3"
work=$(CDPATH= cd -- "$3" && pwd)
machine=${4:-amd64}
source=$(CDPATH= cd -- "$(dirname -- "$0")/pango-probe" && pwd)

# The view external.mk would give a package that builds against pangocairo.
view="$work/view"
rm -rf "$view"
mkdir -p "$view/usr"
for package in zlib libpng freetype expat libffi pcre2 glib harfbuzz fontconfig \
	pixman fribidi cairo pango; do
	cp -as "$workroot/$package/stage/usr/." "$view/usr/"
done

# The probe, compiled and linked through the cross wrapper with the flags
# pkg-config gives for pangocairo.
flags=$(ZEDBSD_PKG_CONFIG_VIEW=$view "$cross/bin/zedbsd-pkg-config" --cflags --libs pangocairo)
"$cross/bin/zedbsd-clang" -std=gnu11 -Wall -Wextra -Werror -O2 \
	-o "$work/pango-probe" "$source/pango-probe.c" $flags \
	"-Wl,-rpath-link,$view/usr/lib"
python3 tools/build/check-dynamic-elf.py --machine "$machine" --role application \
	--needed libpangocairo-1.0.so.0 --needed libpango-1.0.so.0 \
	--needed libgobject-2.0.so.0 --needed libglib-2.0.so.0 --needed libharfbuzz.so.0 \
	--needed libcairo.so.2 --needed libc.so \
	"$work/pango-probe"

# The text fonts the desktop installs (userland/desktop/wayland), as for the
# text probe; the colour emoji font comes with the noto-color-emoji package.
fonts=userland/desktop/fonts
echo "ZEDBSD_TEST_EXTRA_FILES=--file /usr/bin/pango-probe=$work/pango-probe --mode /usr/bin/pango-probe=0755 --file /usr/share/fonts/keiland.ttf=$PWD/$fonts/Inter.ttf --file /usr/share/fonts/keiland-mono.ttf=$PWD/$fonts/JetBrainsMono-Regular.ttf --file /usr/share/fonts/keiland-fallback.ttf=$PWD/$fonts/DroidSansFallbackFull.ttf"
