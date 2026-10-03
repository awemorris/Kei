#!/bin/sh
# ws131-p006: builds libkeiland-backend's zedBSD session (session-zedbsd.c, with the backend object and the power) on
# the host and runs host-session.c against a socket pair standing for sessiond.
#   sh plan/ws131/tests/host-session.sh
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../.."
out=build/ws131-host
mkdir -p "$out"
${CC:-cc} -std=gnu89 -Wall -Wextra -Werror -D_GNU_SOURCE -I. -fsanitize=address,undefined -g \
    userland/desktop/libkeiland-backend/backend.c userland/desktop/libkeiland-backend-zedbsd/session-zedbsd.c \
    userland/desktop/libkeiland-backend-zedbsd/power-zedbsd.c plan/ws131/tests/host-session.c -o "$out/host-session"
exec "$out/host-session"
