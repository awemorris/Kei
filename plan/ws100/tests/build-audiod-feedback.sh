#!/bin/sh
# ws100: builds the audiod test client audiod-feedback (audiod-feedback.c) into build/ws100-tests/, the way the build
# makes a dynamic program, for the images that carry it (config-amd64-audiod.mk, config-amd64-volume.mk).
# The headers come from the amd64 sysroot the build uses for every BUILD (the Makefile's ZEDBSD_TARGET_SYSROOT,
# build/amd64/sysroot), and libc.so from BUILD's dynamic directory, made first when it is not there yet.
# (T1-014: it used BUILD/sysroot, which only the default BUILD has, and stopped at stdint.h.)
#   plan/ws100/tests/build-audiod-feedback.sh [BUILD]     (default build/amd64)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../.."
build=${1:-build/amd64}
config=${AUDIOD_CONFIG:-plan/ws100/tests/config-amd64-volume.mk}
sysroot=$(pwd)/build/amd64/sysroot
[ -f "$build/dynamic/libc.so" ] || make -j"$(nproc)" ZEDBSD_CONFIG="$config" BUILD="$build" "$build/dynamic/libc.so"
[ -f "$sysroot/usr/include/stdint.h" ] || { echo "build-audiod-feedback: no sysroot headers in $sysroot"; exit 1; }
mkdir -p build/ws100-tests
build/llvm/bin/clang --target=x86_64-unknown-zedbsd --sysroot="$sysroot" -nostdinc -I. -Iinclude -isystem "$sysroot/usr/include" \
    -DHAL_ARCH_AMD64 -DKERN_USER_ABI_LP64 -DKERN_DYNAMIC_LIBC -m64 -march=x86-64 -mno-red-zone -O2 -ffreestanding -fPIC \
    -fno-builtin -fno-stack-protector -Wall -Wextra -Werror -c plan/ws100/tests/audiod-feedback.c -o build/ws100-tests/audiod-feedback.o
build/llvm/bin/clang --target=x86_64-unknown-zedbsd --sysroot="$sysroot" -m64 -nostdlib -pie -Wl,--no-relax \
    -Wl,--hash-style=sysv,-z,now,-z,relro -Wl,--allow-shlib-undefined -Wl,--dynamic-linker=/lib/ld.so \
    "$sysroot/usr/lib/crt1.o" build/ws100-tests/audiod-feedback.o -L"$build/dynamic" -Wl,-rpath-link,"$build/dynamic" \
    -l:libc.so -o build/ws100-tests/audiod-feedback
echo "built build/ws100-tests/audiod-feedback"
