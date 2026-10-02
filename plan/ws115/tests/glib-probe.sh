#!/bin/sh
# ws115-p005: build the GLib probe for the target against the staged packages
# and print the extra files that put it, and its schema, into a test image.
# Run from the repository root after `make libffi pcre2 zlib glib`.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
#
# usage: plan/ws115/tests/glib-probe.sh <cross-dir> <package-workroot> <work-dir> [machine]
set -eu

test $# -ge 3 || { echo "usage: ${0##*/} <cross-dir> <package-workroot> <work-dir> [machine]" >&2; exit 1; }
cross=$1
workroot=$(CDPATH= cd -- "$2" && pwd)
mkdir -p "$3"
work=$(CDPATH= cd -- "$3" && pwd)
machine=${4:-amd64}
source=$(CDPATH= cd -- "$(dirname -- "$0")/glib-probe" && pwd)

# The view external.mk would give a package that builds against GLib.
view="$work/view"
rm -rf "$view"
mkdir -p "$view/usr"
for package in libffi pcre2 zlib glib; do
	cp -as "$workroot/$package/stage/usr/." "$view/usr/"
done

# The probe, compiled and linked through the cross wrapper with the flags
# pkg-config gives for GIO.
flags=$(ZEDBSD_PKG_CONFIG_VIEW=$view "$cross/bin/zedbsd-pkg-config" --cflags --libs gio-2.0)
"$cross/bin/zedbsd-clang" -std=gnu11 -Wall -Wextra -Werror -O2 \
	-o "$work/glib-probe" "$source/glib-probe.c" $flags \
	"-Wl,-rpath-link,$view/usr/lib"
python3 tools/build/check-dynamic-elf.py --machine "$machine" --role application \
	--needed libgio-2.0.so.0 --needed libgobject-2.0.so.0 \
	--needed libglib-2.0.so.0 --needed libc.so "$work/glib-probe"

# The schema, compiled by the build machine's glib-compile-schemas, which is
# the same 2.84 release as the target's GLib.
mkdir -p "$work/schemas"
cp "$source/org.zedbsd.GlibProbe.gschema.xml" "$work/schemas/"
glib-compile-schemas --strict "$work/schemas"

echo "ZEDBSD_TEST_EXTRA_FILES=--file /usr/bin/glib-probe=$work/glib-probe --mode /usr/bin/glib-probe=0755 --file /usr/share/glib-probe/schemas/gschemas.compiled=$work/schemas/gschemas.compiled"
