#!/bin/sh
# ws089: builds Settings' host test with the host's C compiler into build/ws089-host/.
#
# The model and the drawing of Settings (settings.h: ui, widgets, glyphs, pages, about) and the
# file manager's canvas, text and icons it shares are built without Wayland and Vulkan (main.c,
# window.c, present.c, menu.c, titlebar.c and glass.c stay out); libtruetype from its sources.
#   settings-render   draws the interface's frames into PPM pictures (host-render.c), with a network of
#                     made-up states in place of the daemon (host-network.c)
#
#   plan/ws089/tests/host-build.sh
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../.."
out=build/ws089-host
src=userland/desktop/settings
mkdir -p "$out/include" "$out/obj"
ln -sf "$(pwd)/userland/desktop/keiland/truetype.h" "$out/include/truetype.h"
ln -sf "$(pwd)/userland/desktop/keiland/keiland.h" "$out/include/keiland.h"
cc=${CC:-cc}
flags="-O2 -g -std=gnu89 -Wall -Wextra -Werror -Wno-unused-parameter -D_GNU_SOURCE -I$out/include -I$src -I."

objects=""
for file in userland/desktop/libtruetype/face.c userland/desktop/libtruetype/cmap.c \
    userland/desktop/libtruetype/outline.c userland/desktop/libtruetype/render.c \
    userland/desktop/libtruetype/glyph.c; do
	object="$out/obj/truetype-$(basename "$file" .c).o"
	"$cc" -O2 -g -w -I$out/include -Iuserland/desktop/libtruetype -c "$file" -o "$object"
	objects="$objects $object"
done
for file in userland/desktop/files/canvas.c userland/desktop/files/text.c userland/desktop/files/icons.c userland/desktop/artwork/mark.c userland/desktop/libkeiland/preferences.c; do
	object="$out/obj/shared-$(basename "$file" .c).o"
	"$cc" -O2 -g -Wall -Werror -D_GNU_SOURCE -I$out/include -c "$file" -o "$object"
	objects="$objects $object"
done
# The sound page's link to audiod (ws100-p005): libkeiland's zedbsd/audio-zedbsd.c (no audiod on the host: the page shows no sound).
object="$out/obj/shared-audio.o"
"$cc" -O2 -g -Wall -Werror -D_GNU_SOURCE -I$out/include -I. -c userland/desktop/libkeiland/zedbsd/audio-zedbsd.c -o "$object"
objects="$objects $object"
for file in $src/*.c; do
	case $(basename "$file") in
	main.c|window.c|present.c|menu.c|titlebar.c|glass.c|network.c) continue ;;
	esac
	object="$out/obj/settings-$(basename "$file" .c).o"
	"$cc" $flags -c "$file" -o "$object"
	objects="$objects $object"
done
"$cc" $flags -c plan/ws089/tests/host-render.c -o "$out/obj/host-render.o"
"$cc" $flags -c plan/ws089/tests/host-network.c -o "$out/obj/host-network.o"
"$cc" -o "$out/settings-render" "$out/obj/host-render.o" "$out/obj/host-network.o" $objects -lm -pthread
echo "built $out/settings-render"
