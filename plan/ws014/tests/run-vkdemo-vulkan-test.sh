#!/bin/sh
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
# Build the ordinary app against the installed host Vulkan implementation.
set -eu

repo=$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)
work=${1:?usage: run-vkdemo-vulkan-test.sh OUTPUT_DIRECTORY}
mkdir -p "$work"
work=$(CDPATH= cd -- "$work" && pwd)

cc -std=c89 -pedantic -Wall -Wextra -Werror \
    -D_POSIX_C_SOURCE=200809L \
    "$repo/userland/tests/vkdemo/main.c" \
    "$repo/userland/tests/vkdemo/renderer.c" \
    "$repo/userland/tests/vkdemo/display.c" \
    "$repo/userland/base/common/sha256.c" \
    -lvulkan -o "$work/vkdemo"

python3 "$repo/plan/ws014/tests/vkdemo-vulkan-test.py" \
    "$work/vkdemo" "$work"
