#!/bin/sh
# ws131-p005: builds libkeiland-backend's power on the host twice -- with zedBSD's power-zedbsd.c (a socket pair for
# sessiond) and with the unsupported implementation of Linux and FreeBSD -- and runs host-power.c on each.
#   sh plan/ws131/tests/host-power.sh
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../.."
out=build/ws131-host
mkdir -p "$out"
flags="-std=gnu89 -Wall -Wextra -Werror -D_GNU_SOURCE -I. -fsanitize=address,undefined -g"
${CC:-cc} $flags -DHOST_POWER_ZEDBSD userland/desktop/libkeiland-backend/backend.c \
    userland/desktop/libkeiland-backend-zedbsd/power-zedbsd.c plan/ws131/tests/host-power.c -o "$out/host-power-zedbsd"
${CC:-cc} $flags userland/desktop/libkeiland-backend/backend.c \
    userland/desktop/libkeiland-backend/unsupported/power-unsupported.c plan/ws131/tests/host-power.c -o "$out/host-power-unsupported"
# A write to a closed sessiond raises SIGPIPE; the test ignores it to see the write's error (the compositor's
# behaviour there is the one greeter.c had before ws131-p005).
status=0
(trap '' PIPE; "$out/host-power-zedbsd") || status=1
"$out/host-power-unsupported" || status=1
exit $status
