#!/bin/sh
# ws074: builds browser's host tests for zedBSD (amd64), to run in the guest.
#
#   sh plan/ws074/tests/guest-build.sh NAME...     (e.g. host-heap)
#
# Each plan/ws074/tests/NAME.c is compiled with the flags the base programs use and linked
# with the engine's objects from the last build of the browser (build/amd64/dynamic/obj,
# made by `make ZEDBSD_CONFIG=plan/ws074/tests/config-amd64-browser.mk build/amd64/bin/browser`),
# into build/ws074-guest/NAME.  browser-guest.sh put copies them into a running guest.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../.."
root=$(pwd)
src=userland/desktop/libbrowser
sysroot=$root/build/amd64/sysroot
objdir=build/amd64/dynamic/obj/$src
out=build/ws074-guest
cc="$root/build/llvm/bin/clang --target=x86_64-unknown-zedbsd --sysroot=$sysroot"
cflags="-nostdinc -I. -Iinclude -isystem $sysroot/usr/include -DHAL_ARCH_AMD64 -DKERN_USER_ABI_LP64 -DKERN_DYNAMIC_LIBC
	-I$src -Iuserland/desktop/browser -Iplan/ws074/tests -m64 -march=x86-64 -mno-red-zone -Os -ffreestanding -fPIC -fno-builtin
	-fno-stack-protector -Wall -Wextra -Werror"
mkdir -p "$out"

# The engine's objects, from the package's list (without main.c and the window).
engine=""
for file in $(sh plan/ws074/tests/list-sources.sh); do
	case $file in
	*/main.c|*/shell/*) continue ;;
	esac
	object=build/amd64/dynamic/obj/${file%.c}.o
	[ -f "$object" ] || { echo "guest-build: $object is missing; build the browser first" >&2; exit 1; }
	engine="$engine $object"
done

for name in "$@"; do
	$cc $cflags -c "plan/ws074/tests/$name.c" -o "$out/$name.o"
	$cc $cflags -c plan/ws074/tests/host-shell.c -o "$out/host-shell.o"
	$cc -m64 -nostdlib -pie -Wl,--no-relax -Wl,--hash-style=sysv,-z,now,-z,relro,-z,separate-code \
		-Wl,-z,stack-size=0x100000,--allow-shlib-undefined -Wl,--dynamic-linker=/lib/ld.so \
		"$sysroot/usr/lib/crt1.o" "$out/$name.o" $engine "$out/host-shell.o" \
		-Lbuild/amd64/dynamic -Wl,-rpath-link,build/amd64/dynamic -l:libvulkan.so -l:libtruetype.so -l:libc.so -o "$out/$name"
	echo "built $out/$name"
done
