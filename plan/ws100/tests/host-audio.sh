#!/bin/sh
# ws100-p003: builds libkeiland-backend's zedBSD audio-zedbsd.c (ws131-p004) on the host with its socket in build/ws100-host (AUDIO_SOCKET_PATH) and runs
# host-audio.c against a pretend audiod.
#   sh plan/ws100/tests/host-audio.sh
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../.."
out=$(pwd)/build/ws100-host
mkdir -p "$out/include"
ln -sf "$(pwd)/userland/desktop/keiland/keiland.h" "$out/include/keiland.h"
${CC:-cc} -std=gnu89 -Wall -Wextra -Werror -D_GNU_SOURCE -I"$out/include" -I. \
    "-DAUDIO_SOCKET_PATH=\"$out/audiod.sock\"" \
    userland/desktop/libkeiland-backend-zedbsd/audio-zedbsd.c plan/ws100/tests/host-audio.c -o "$out/host-audio"
exec "$out/host-audio"
