#!/bin/sh
# ws115-p008: build the image and keymap probe for the target against the
# staged packages, gather its test images and the compositor's keymap text,
# and print the extra files that put them into a test image.
# Run from the repository root after the p008 packages are built.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
#
# usage: plan/ws115/tests/image-probe.sh <cross-dir> <package-workroot> <work-dir> [machine]
set -eu

test $# -ge 3 || { echo "usage: ${0##*/} <cross-dir> <package-workroot> <work-dir> [machine]" >&2; exit 1; }
cross=$1
workroot=$(CDPATH= cd -- "$2" && pwd)
mkdir -p "$3"
work=$(CDPATH= cd -- "$3" && pwd)
machine=${4:-amd64}
source=$(CDPATH= cd -- "$(dirname -- "$0")/image-probe" && pwd)

# The view external.mk would give a package that builds against the three.
view="$work/view"
rm -rf "$view"
mkdir -p "$view/usr"
for package in zlib libpng libjpeg-turbo libtiff libffi pcre2 glib gdk-pixbuf \
	xkeyboard-config libxkbcommon libepoxy; do
	cp -as "$workroot/$package/stage/usr/." "$view/usr/"
done

# The probe, compiled and linked through the cross wrapper with the flags
# pkg-config gives for the three libraries it calls.
flags=$(ZEDBSD_PKG_CONFIG_VIEW=$view "$cross/bin/zedbsd-pkg-config" --cflags --libs \
	gdk-pixbuf-2.0 xkbcommon epoxy)
"$cross/bin/zedbsd-clang" -std=gnu11 -Wall -Wextra -Werror -O2 \
	-o "$work/image-probe" "$source/image-probe.c" $flags \
	"-Wl,-rpath-link,$view/usr/lib"
python3 tools/build/check-dynamic-elf.py --machine "$machine" --role application \
	--needed libgdk_pixbuf-2.0.so.0 --needed libgobject-2.0.so.0 \
	--needed libglib-2.0.so.0 --needed libxkbcommon.so.0 --needed libepoxy.so.0 \
	--needed libc.so "$work/image-probe"

# The test pictures come from the packages' own sources.
mkdir -p "$work/images"
cp "$workroot/libpng/src/pngtest.png" "$workroot/libjpeg-turbo/src/testimages/testorig.jpg" \
	"$workroot/libtiff/src/test/images/rgb-3c-8b.tiff" "$work/images/"

# The keymap text the compositor sends, taken from its source: the string
# literal keymap_text in userland/desktop/wayland/keymap.c.
python3 - userland/desktop/wayland/keymap.c "$work/compositor.xkb" <<'PY'
import re, sys
source = open(sys.argv[1]).read()
body = source[source.index('keymap_text[] ='):]
body = body[:body.index(';\n')]
pieces = re.findall(r'"((?:[^"\\]|\\.)*)"', body)
text = ''.join(pieces).encode('latin-1').decode('unicode_escape')
open(sys.argv[2], 'w').write(text)
PY

echo "ZEDBSD_TEST_EXTRA_FILES=--file /usr/bin/image-probe=$work/image-probe --mode /usr/bin/image-probe=0755 --file /usr/share/image-probe/pngtest.png=$work/images/pngtest.png --file /usr/share/image-probe/testorig.jpg=$work/images/testorig.jpg --file /usr/share/image-probe/rgb-3c-8b.tiff=$work/images/rgb-3c-8b.tiff --file /usr/share/image-probe/compositor.xkb=$work/compositor.xkb"
