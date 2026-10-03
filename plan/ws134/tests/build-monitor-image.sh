#!/bin/sh
# ws134-p002: builds the System Monitor's test image: the lean Files image (fonts, wallpaper, the guest harness) with
# /bin/monitor and the replays under /usr/share/monitor-tests (plan/ws134/tests/config-amd64-monitor.mk).
#
#   plan/ws134/tests/build-monitor-image.sh BUILD
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../.."
build=${1:?usage: build-monitor-image.sh BUILD}
extra=""
for replay in plan/ws134/tests/replay/*.txt; do
	extra="$extra --file /usr/share/monitor-tests/$(basename "$replay")=$replay"
done
FILES_CONFIG=plan/ws134/tests/config-amd64-monitor.mk FILES_EXTRA="$extra" exec sh plan/tools/files/build-files-image.sh "$build"
