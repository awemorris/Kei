#!/bin/sh
# Host test of the mview model reader, camera and input translation.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
#
# Builds the viewer's portable sources with the host compiler (warnings are
# errors, AddressSanitizer and UBSan enabled) and runs them against the test
# fixture, malformed models, and the converted qs40 model when present.
set -eu
root=$(cd "$(dirname "$0")/../../.." && pwd)
work=$(mktemp -d "${TMPDIR:-/tmp}/mview-host.XXXXXX")
trap 'rm -rf "$work"; exit 1' INT TERM
mview="$root/userland/tests/mview"
${CC:-cc} -std=c99 -D_DEFAULT_SOURCE -O1 -g -Wall -Wextra -Werror \
	-fsanitize=address,undefined -fno-sanitize-recover=all \
	"$root/plan/ws031/tests/mview-host-test.c" \
	"$mview/model.c" "$mview/camera.c" "$mview/input.c" \
	-lm -o "$work/mview-host-test"
status=0
"$work/mview-host-test" "$mview/models/test" "$work/scratch" "$mview/models/qs40" \
	> "$work/log" 2>&1 || status=$?
# The viewer's own MVIEW INPUT lines are omitted; checks and the verdict remain.
grep -v '^MVIEW INPUT ' "$work/log" || true
rm -rf "$work"
exit "$status"
