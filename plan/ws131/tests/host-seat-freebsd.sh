#!/bin/sh
# ws131-p006: builds libkeiland-backend's FreeBSD seat (seat-freebsd.c) on the host with a pretend libseat
# (plan/ws131/tests/fake-libseat/libseat.h and the functions in host-seat-freebsd.c) and runs host-seat-freebsd.c.
# The FreeBSD build itself is deferred (2026-10-03 user); this checks the seat's logic and its callbacks' order.
#   sh plan/ws131/tests/host-seat-freebsd.sh
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../.."
out=build/ws131-host
mkdir -p "$out"
${CC:-cc} -std=gnu89 -Wall -Wextra -Werror -D_GNU_SOURCE -I. -Iplan/ws131/tests/fake-libseat -fsanitize=address,undefined -g \
    userland/desktop/libkeiland-backend/backend.c userland/desktop/libkeiland-backend/session/session-none.c \
    userland/desktop/libkeiland-backend-freebsd/seat-freebsd.c plan/ws131/tests/host-seat-freebsd.c -o "$out/host-seat-freebsd"
exec "$out/host-seat-freebsd"
