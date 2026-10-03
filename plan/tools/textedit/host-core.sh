#!/bin/sh
# Text Editor (ws092): builds and runs the host tests of Text Editor's core (host-core.c) with the
# editor's sources, libtruetype, and libkeiui's scroll and text view touch (WS090) with libkeiland's
# scroller, on Linux; the fonts are the tree's.
#   sh plan/tools/textedit/host-core.sh [OUTPUT]   (default build/textedit/host-core)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -e
cd "$(dirname "$0")/../../.."
out=${1:-build/textedit/host-core}
mkdir -p "$(dirname "$out")/inc"
cp userland/desktop/keiland/truetype.h userland/desktop/keiland/keiland.h userland/desktop/keiland/keiui.h "$(dirname "$out")/inc/"
ln -sfn "$(pwd)/include/libc/compat" "$(dirname "$out")/inc/compat"
D=userland/desktop/textedit
cc -std=c11 -D_GNU_SOURCE -O1 -g -Wall -Wextra -I$D -I. -I"$(dirname "$out")/inc" -Iuserland/desktop/libtruetype \
	plan/tools/textedit/host-core.c $D/buffer.c $D/undo.c $D/file.c $D/layout.c $D/find.c $D/edit.c \
	$D/app.c $D/draw.c $D/canvas.c $D/text.c userland/desktop/libtruetype/*.c \
	userland/desktop/libkeiui/input.c userland/desktop/libkeiui/scroll.c userland/desktop/libkeiui/scroll-bar.c userland/desktop/libkeiui/text-touch.c \
	userland/desktop/libkeiui/canvas.c userland/desktop/libkeiland/scroll.c userland/desktop/picture/color-glyph.c \
	userland/base/libz-compat/inflate.c userland/base/libz-compat/checksum.c userland/base/libpng-compat/read.c -lm -o "$out"
"$out"
