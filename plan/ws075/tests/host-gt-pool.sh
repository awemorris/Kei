#!/bin/sh
# BUG-120: builds and runs the host test of the i915 GT object pool's growth (host-gt-pool.c).
# Last line: host-gt-pool: PASS.
#   sh plan/ws075/tests/host-gt-pool.sh
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../.."
out=build/ws075-host
mkdir -p "$out"
${CC:-cc} -std=gnu11 -O1 -g -w -fsanitize=address,undefined -DHAL_ARCH_AMD64 -DKERN_USER_ABI_LP64 \
    -Iinclude -Isrc -I. -Isrc/drivers/gpu/i915 plan/ws075/tests/host-gt-pool.c -o "$out/host-gt-pool"
exec "$out/host-gt-pool"
