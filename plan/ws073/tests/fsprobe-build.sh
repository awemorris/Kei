#!/bin/sh
# ws073-p045: builds fsprobe.c for the amd64 guest with the build's clang against the sysroot (as the guest's
# programs are linked), into OUT (default build/ws073-p045/fsprobe).  BUILD names the build whose dynamic libc it links.
#   BUILD=build/amd64 sh plan/ws073/tests/fsprobe-build.sh [OUT]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../.."
build=${BUILD:-build/amd64}
out=${1:-build/ws073-p045/fsprobe}
mkdir -p "$(dirname "$out")"
sysroot=$(pwd)/build/amd64/sysroot
build/llvm/bin/clang --target=x86_64-unknown-zedbsd --sysroot="$sysroot" -nostdinc -I. -Iinclude -isystem "$sysroot/usr/include" \
    -DHAL_ARCH_AMD64 -DKERN_USER_ABI_LP64 -DKERN_DYNAMIC_LIBC -m64 -march=x86-64 -mno-red-zone -O2 -ffreestanding -fPIC \
    -fno-builtin -fno-stack-protector -Wall -Wextra -Werror -c plan/ws073/tests/fsprobe.c -o "$out.o"
build/llvm/bin/clang --target=x86_64-unknown-zedbsd --sysroot="$sysroot" -m64 -nostdlib -pie -Wl,--no-relax \
    -Wl,--hash-style=sysv,-z,now,-z,relro -Wl,--allow-shlib-undefined -Wl,--dynamic-linker=/lib/ld.so \
    "$sysroot/usr/lib/crt1.o" "$out.o" -L"$build/dynamic" -Wl,-rpath-link,"$build/dynamic" -l:libc.so -o "$out"
echo "built $out"
