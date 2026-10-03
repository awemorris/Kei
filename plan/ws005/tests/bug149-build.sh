#!/bin/sh
# ws005-p025 (BUG-149): builds the guest probes bug149-poll and bug149-keiland, static, for amd64.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
#
# Uses the shared clang (read only) and this tree's amd64 sysroot (build/amd64/sysroot).
#   sh plan/ws005/tests/bug149-build.sh [OUTPUT-DIRECTORY]      (default build/p1-bug149)
set -eu
cd "$(dirname -- "$0")/../../.."
out=${1:-build/p1-bug149}
clang=${CLANG:-build/llvm/bin/clang}
sysroot=${SYSROOT:-build/amd64/sysroot}
mkdir -p "$out"
cflags="--target=x86_64-unknown-zedbsd --sysroot=$sysroot -m64 -march=x86-64 -mno-red-zone
	-DHAL_ARCH_AMD64 -DKERN_USER_ABI_LP64 -nostdinc -isystem $sysroot/usr/include
	-ffreestanding -fno-pic -fno-pie -fno-stack-protector -O1 -Wall -Wextra -Werror
	-I. -Iuserland/desktop/keiland"
link() {
	"$clang" --target=x86_64-unknown-zedbsd --sysroot="$sysroot" -m64 -nostdlib -static \
		-Wl,--build-id=none -Wl,-T,"$sysroot/usr/lib/zedbsd/amd64/user.ld" \
		"$sysroot/usr/lib/crt0.o" "$@" \
		-Wl,--start-group "$sysroot/usr/lib/libc.a" "$sysroot/usr/lib/libzedbsd-compiler-rt.a" \
		"$sysroot/usr/lib/libclang_rt.builtins.a" -Wl,--end-group
}
# shellcheck disable=SC2086
"$clang" $cflags -c plan/ws005/tests/bug149-poll.c -o "$out/bug149-poll.o"
link "$out/bug149-poll.o" -o "$out/bug149-poll"
# shellcheck disable=SC2086
"$clang" $cflags -c plan/ws005/tests/bug149-keiland.c -o "$out/bug149-keiland.o"
# libkeiland's network calls (system-compat.c) over the zedBSD backend (ws131-p003 moved network-zedbsd.c
# from libkeiland/zedbsd/ to libkeiland-backend-zedbsd/; its sources.mk lists the backend's files).  Since
# ws131-p003 the backend also checks readability with MSG_PEEK after poll, so this probe now checks the join
# end to end rather than the kernel fix alone.
objects="$out/bug149-keiland.o"
for source in userland/desktop/libkeiland/system-compat.c userland/desktop/libkeiland-backend/backend.c \
	userland/desktop/libkeiland-backend-zedbsd/network-zedbsd.c \
	userland/desktop/libkeiland-backend-zedbsd/network-link-zedbsd.c \
	userland/base/net/protocol.c userland/base/net/wifi-conf.c userland/base/net/wifi-store.c; do
	object=$out/$(basename "$source" .c).o
	# shellcheck disable=SC2086
	"$clang" $cflags -c "$source" -o "$object"
	objects="$objects $object"
done
# shellcheck disable=SC2086
link $objects -o "$out/bug149-keiland"
echo "bug149 probes: $out/bug149-poll $out/bug149-keiland"
