#!/bin/sh
# ws134-p002: builds and runs the host tests of the System Monitor (host-test.c) with the host's compiler.
#   plan/ws134/tests/host/run.sh [OUTDIR]     (prints "monitor-host: PASS" or FAIL)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../../.."
out=${1:-build/ws134-host}
mkdir -p "$out"
m=userland/desktop/monitor
cc -std=c11 -D_DEFAULT_SOURCE -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -I"$m" \
    plan/ws134/tests/host/host-test.c "$m/source.c" "$m/history.c" "$m/rules.c" "$m/format.c" -lm -o "$out/host-test"
"$out/host-test" plan/ws134/tests/replay/normal.txt
