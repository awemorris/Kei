#!/bin/sh
# ws118-p005: builds and runs the host test of the boot parameters i915.start= and i915.debug=
# (host-boot-i915-test.c): src/kern/boot.c with the host compiler, ASan and UBSan, linked with
# --gc-sections so only the parser and what it reaches stay (no kernel service is reached: no stubs).
#
#   plan/ws118/tests/host-boot-i915-test.sh [OUTDIR]      (default build/ws118-boot-host)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../.."
out=${1:-build/ws118-boot-host}
mkdir -p "$out"
CC=${CC:-clang}
FLAGS="-std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-sanitize-recover=all -ffunction-sections -fdata-sections -Iinclude -Isrc -I. -DHAL_ARCH_AMD64 -DHAL_BOARD_PCAT -DKERN_USER_ABI_LP64 -D_POSIX_C_SOURCE=200809L -include time.h"
for f in src/kern/boot.c plan/ws118/tests/host-boot-i915-test.c ; do
	$CC $FLAGS -c "$f" -o "$out/$(basename "$f" .c).o"
done
$CC -fsanitize=address,undefined -Wl,--gc-sections "$out/boot.o" "$out/host-boot-i915-test.o" -o "$out/host-boot-i915-test"
"$out/host-boot-i915-test"
