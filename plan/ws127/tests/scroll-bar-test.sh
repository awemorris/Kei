#!/bin/sh
# ws127-p002: builds and runs the host check of libkeiui's overlay scroll bar (scroll-bar-test.c).
#   sh plan/ws127/tests/scroll-bar-test.sh
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../.."
out=build/ws127-host
mkdir -p "$out"
${CC:-cc} -O2 -g -Wall -Wextra -Werror -Iuserland/desktop/keiland -o "$out/scroll-bar-test" \
	plan/ws127/tests/scroll-bar-test.c userland/desktop/libkeiui/scroll-bar.c -lm
timeout 60 "$out/scroll-bar-test"
