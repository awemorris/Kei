#!/bin/sh
# ws115-p009: builds plan/ws115/tests/wl-log/wl-log.c against libwayland-client's sources on the host (as
# plan/ws073/tests/wayland-dispatch-once.sh does), plainly and under ASan/UBSan, and runs it.
#   sh plan/ws115/tests/wl-log.sh [OUTDIR]     (default build/ws115-wl-log)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../.."
out=${1:-build/ws115-wl-log}
mkdir -p "$out/include/wayland"
cp userland/desktop/libwayland/zed-*-client-protocol.h "$out/include/wayland/"
sources=$(ls userland/desktop/libwayland/*.c)
flags="-std=c99 -D_GNU_SOURCE -Wall -Wextra -Werror -Wno-cast-function-type -Iuserland/desktop/keiland/wayland -I$out/include -I. -idirafter userland/desktop/keiland -idirafter include/libc -pthread"
cc $flags $sources plan/ws115/tests/wl-log/wl-log.c -o "$out/wl-log"
timeout 30 "$out/wl-log"
cc $flags -fsanitize=address,undefined -fno-omit-frame-pointer -g $sources plan/ws115/tests/wl-log/wl-log.c -o "$out/wl-log-asan"
ASAN_OPTIONS=detect_leaks=1 timeout 30 "$out/wl-log-asan"
