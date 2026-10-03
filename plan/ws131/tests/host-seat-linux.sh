#!/bin/sh
# ws131-p006: builds libkeiland-backend's Linux seat (the choice, the direct seat, logind's seat, the D-Bus reader) and
# power on the host and runs host-seat-linux.c (standard input from /dev/null, so no virtual terminal is touched).
#   sh plan/ws131/tests/host-seat-linux.sh
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../.."
out=build/ws131-host
mkdir -p "$out"
b=userland/desktop/libkeiland-backend-linux
${CC:-cc} -std=gnu17 -Wall -Wextra -Werror -D_GNU_SOURCE -I. -fsanitize=address,undefined -g \
    userland/desktop/libkeiland-backend/backend.c userland/desktop/libkeiland-backend/session/session-none.c \
    $b/seat-linux.c $b/seat-direct-linux.c $b/seat-logind-linux.c $b/dbus-linux.c $b/power-linux.c \
    plan/ws131/tests/host-seat-linux.c -o "$out/host-seat-linux"
exec "$out/host-seat-linux" </dev/null
