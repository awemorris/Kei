#!/bin/sh
# ws081-p016: builds touchlog (touchlog.c) for Kei with the build's clang against a build's sysroot (the way
# plan/ws100/tests/build-audiod-image.sh builds its client), into build/ws081-tests/touchlog.  With "image", then
# builds the test image (config-amd64-touchlog.mk) with it in /usr/bin.  Any Kei image can take the binary too
# (ZEDBSD_EXTRA_FILES += --file /usr/bin/touchlog=build/ws081-tests/touchlog).
#   plan/ws081/tests/build-touchlog.sh [BUILD] [image]     (default build/amd64; its sysroot must be built)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../.."
. plan/tools/guest/jobs.sh  # ZEDBSD_JOBS, the parallel jobs (default 16)
build=${1:-build/amd64}
sysroot=$(pwd)/$build/sysroot
[ -f "$sysroot/usr/lib/crt1.o" ] || make -j"$ZEDBSD_JOBS" ZEDBSD_CONFIG=plan/ws081/tests/config-amd64-touchlog.mk BUILD="$build" disk-image
mkdir -p build/ws081-tests
build/llvm/bin/clang --target=x86_64-unknown-zedbsd --sysroot="$sysroot" -nostdinc -I. -Iinclude -isystem "$sysroot/usr/include" \
    -DHAL_ARCH_AMD64 -DKERN_USER_ABI_LP64 -DKERN_DYNAMIC_LIBC -m64 -march=x86-64 -mno-red-zone -O2 -ffreestanding -fPIC \
    -fno-builtin -fno-stack-protector -Wall -Wextra -Werror -c plan/ws081/tests/touchlog.c -o build/ws081-tests/touchlog.o
build/llvm/bin/clang --target=x86_64-unknown-zedbsd --sysroot="$sysroot" -m64 -nostdlib -pie -Wl,--no-relax \
    -Wl,--hash-style=sysv,-z,now,-z,relro -Wl,--allow-shlib-undefined -Wl,--dynamic-linker=/lib/ld.so \
    "$sysroot/usr/lib/crt1.o" build/ws081-tests/touchlog.o -L"$build/dynamic" -Wl,-rpath-link,"$build/dynamic" \
    -l:libc.so -o build/ws081-tests/touchlog
echo "built build/ws081-tests/touchlog"
[ "${2:-}" = image ] || exit 0
extra=$(python3 plan/tools/guest/guest.py extra-files | sed -n "s/^ZEDBSD_TEST_EXTRA_FILES='\(.*\)'$/\1/p")
exec make -j"$ZEDBSD_JOBS" ZEDBSD_CONFIG=plan/ws081/tests/config-amd64-touchlog.mk BUILD="$build" "ZEDBSD_TEST_EXTRA_FILES=$extra" disk-image
