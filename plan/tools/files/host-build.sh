#!/bin/sh
# ws071: builds files' host tests with the host's C compiler into build/ws071-host/.
#
# The drawing (canvas, text, icons), the interface and the model of files are built
# without Wayland and Vulkan (window.c, present.c, menu.c and titlebar.c stay out); libtruetype is built
# from its sources.  The test programs:
#   files-render   draws scenes of the interface into PPM pictures (host-render.c)
#   files-model    checks the model in temporary directories (host-model.c)
#
#   plan/tools/files/host-build.sh
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../.."
out=build/ws071-host
src=userland/desktop/files
mkdir -p "$out/include" "$out/obj"
ln -sf "$(pwd)/userland/desktop/keiland/truetype.h" "$out/include/truetype.h"
ln -sf "$(pwd)/userland/desktop/keiland/keiland.h" "$out/include/keiland.h"
ln -sf "$(pwd)/userland/desktop/keiland/keiui.h" "$out/include/keiui.h"
ln -sf "$(pwd)/include/libc/sha2.h" "$out/include/sha2.h"
ln -sf "$(pwd)/include/libc/pdf.h" "$out/include/pdf.h"
ln -sfn "$(pwd)/include/libc/compat" "$out/include/compat"
cc=${CC:-cc}
flags="-O2 -g -Wall -Wextra -Werror -Wno-unused-parameter -D_GNU_SOURCE -I$out/include -I$src -I."

# The libraries the program uses, from their sources.
objects=""
for file in userland/desktop/libtruetype/face.c userland/desktop/libtruetype/cmap.c \
    userland/desktop/libtruetype/outline.c userland/desktop/libtruetype/render.c \
    userland/desktop/libtruetype/glyph.c; do
	object="$out/obj/truetype-$(basename "$file" .c).o"
	"$cc" $flags -Wno-error -Iuserland/desktop/libtruetype -c "$file" -o "$object"
	objects="$objects $object"
done
# The C library's SHA-2 (the information's checksum), which the host's C library does not have.
"$cc" $flags -c src/libc/openbsd-sha2.c -o "$out/obj/libc-sha2.o"
objects="$objects $out/obj/libc-sha2.o"
# libz-compat and libpng-compat (the PNG thumbnails); libjpeg-compat, libgif-compat and the decoding shared with Image
# Viewer (the JPEG and GIF thumbnails, ws094-p013).
for file in userland/base/libz-compat/inflate.c userland/base/libz-compat/checksum.c userland/base/libpng-compat/read.c \
    userland/base/libjpeg-compat/decompress.c userland/base/libjpeg-compat/error.c userland/base/libjpeg-compat/huffman.c \
    userland/base/libjpeg-compat/idct.c userland/base/libjpeg-compat/marker.c userland/base/libjpeg-compat/memory.c \
    userland/base/libjpeg-compat/source.c userland/base/libgif-compat/decode.c userland/base/libgif-compat/lzw.c \
    userland/desktop/picture/picture.c; do
	object="$out/obj/compat-$(basename "$file" .c).o"
	"$cc" $flags -c "$file" -o "$object"
	objects="$objects $object"
done
if [ -f userland/desktop/libkeiland/recent.c ]; then
	"$cc" $flags -c userland/desktop/libkeiland/recent.c -o "$out/obj/zdesktop-recent.o"
	objects="$objects $out/obj/zdesktop-recent.o"
fi

# libkeiui's overlay scroll bar (files/ui-scrollbar.c uses it since ws127-p002), which needs no canvas.
"$cc" $flags -c userland/desktop/libkeiui/scroll-bar.c -o "$out/obj/keiui-scroll-bar.o"
objects="$objects $out/obj/keiui-scroll-bar.o"

# libkeiland's gesture, scroller and motion (files/touch.c uses them since ws081-p010; ws093-p003).
for file in userland/desktop/libkeiland/gesture.c userland/desktop/libkeiland/scroll.c userland/desktop/libkeiland/motion.c; do
	object="$out/obj/keiland-$(basename "$file" .c).o"
	"$cc" $flags -c "$file" -o "$object"
	objects="$objects $object"
done

# files without the window, the presenter, the menus, the titlebar and the glass.
# The shared mount-table adapter supplies the same real table on the host and zedBSD.
for file in $src/*.c $src/mntent/*.c; do
	case $(basename "$file") in
	main.c|window.c|present.c|menu.c|titlebar.c|glass.c|dnd.c) continue ;;
	esac
	object="$out/obj/files-$(basename "$file" .c).o"
	"$cc" $flags -c "$file" -o "$object"
	objects="$objects $object"
done

# The Kei mark (ws035-p108), shared with the compositor.
"$cc" $flags -c userland/desktop/artwork/mark.c -o "$out/obj/artwork-mark.o"
objects="$objects $out/obj/artwork-mark.o"

# The test programs.
for test in render model; do
	if [ -f "plan/tools/files/host-$test.c" ]; then
		"$cc" $flags -c "plan/tools/files/host-$test.c" -o "$out/obj/host-$test.o"
		extra=
		if [ "$test" = render ]; then
			"$cc" $flags -c plan/tools/files/host-glass.c -o "$out/obj/host-glass.o"
			extra="$out/obj/host-glass.o"
		fi
		"$cc" -o "$out/files-$test" "$out/obj/host-$test.o" $extra $objects -lm -ldl
		echo "built $out/files-$test"
	fi
done
