#!/bin/sh
# ws115-p004: build the Meson cross probe with the generated zedBSD entry points
# and check what it produced.  Run from the repository root after
# `make packages-meson-cross zlib` with the same configuration.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
#
# usage: plan/ws115/tests/meson-probe.sh <cross-dir> <package-workroot> <work-dir> [machine]
set -eu

test $# -ge 3 || { echo "usage: ${0##*/} <cross-dir> <package-workroot> <work-dir> [machine]" >&2; exit 1; }
cross=$1
workroot=$(CDPATH= cd -- "$2" && pwd)
mkdir -p "$3"
work=$(CDPATH= cd -- "$3" && pwd)
machine=${4:-amd64}
source=$(CDPATH= cd -- "$(dirname -- "$0")/meson-probe" && pwd)
build="$work/build"
stage="$work/stage"

rm -rf "$build" "$stage"
mkdir -p "$work"

# The view external.mk would make for a package that declares zlib: symbolic
# links to zlib's staged files (the same cp external.mk runs).
view="$work/view"
rm -rf "$view"
mkdir -p "$view/usr"
cp -as "$workroot/zlib/stage/usr/." "$view/usr/"

# Without a view the wrapper sees neither zlib nor the build machine's own
# packages; with the view it sees zlib, at paths inside the view.
if "$cross/bin/zedbsd-pkg-config" --exists zlib; then
	echo "meson-probe: FAIL zlib visible without a view" >&2; exit 1
fi
if ZEDBSD_PKG_CONFIG_VIEW=$view "$cross/bin/zedbsd-pkg-config" --exists glib-2.0; then
	echo "meson-probe: FAIL the build machine's glib-2.0 is visible" >&2; exit 1
fi
flags=$(ZEDBSD_PKG_CONFIG_VIEW=$view "$cross/bin/zedbsd-pkg-config" --cflags --libs zlib | sed "s/ *$//")
echo "zlib flags: $flags"
test "$flags" = "-I$view/usr/include -L$view/usr/lib -lz" || {
	echo "meson-probe: FAIL zlib flags do not point into the view" >&2; exit 1; }
echo "meson-probe: pkg-config isolation PASS"

# The same command line external.mk uses for a Meson package.
ZEDBSD_PKG_CONFIG_VIEW=$view meson setup "$build" "$source" \
	--cross-file "$cross/meson-cross.ini" \
	--native-file "$cross/meson-native.ini" \
	--prefix=/usr --libdir=lib --buildtype=release \
	--wrap-mode=nodownload -Ddefault_library=shared \
	"-Dc_link_args=-Wl,-rpath-link,$view/usr/lib"
ZEDBSD_PKG_CONFIG_VIEW=$view ninja -C "$build"
ZEDBSD_PKG_CONFIG_VIEW=$view meson install -C "$build" --no-rebuild --quiet \
	--destdir "$stage"

# What was installed: a library with its SONAME that needs zlib and libc, a
# program that needs the library and libc, neither with an rpath, and a .pc
# that names the library.
python3 tools/build/check-dynamic-elf.py --machine "$machine" --role shared-library \
	--soname libprobe.so.1 --needed libz.so.1 --needed libc.so \
	"$stage/usr/lib/libprobe.so.1.0.0"
python3 tools/build/check-dynamic-elf.py --machine "$machine" --role application \
	--needed libprobe.so.1 --needed libc.so "$stage/usr/bin/probe-check"
grep -q '^Libs: .*-lprobe' "$stage/usr/lib/pkgconfig/probe.pc"
"$build/probe-generate" "$work/native-check.h"
grep -q PROBE_SEED "$work/native-check.h"
echo "meson-probe: PASS"
