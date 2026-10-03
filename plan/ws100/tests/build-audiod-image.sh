#!/bin/sh
# ws100-p002: builds the audiod test image (config-amd64-audiod.mk) with the test client in /usr/bin (the client by
# build-audiod-feedback.sh, against the amd64 sysroot and BUILD's libc.so).
#   plan/ws100/tests/build-audiod-image.sh [BUILD]     (default build/amd64)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../.."
. plan/tools/guest/jobs.sh  # ZEDBSD_JOBS, the parallel jobs (default 16)
build=${1:-build/amd64}
AUDIOD_CONFIG=plan/ws100/tests/config-amd64-audiod.mk sh plan/ws100/tests/build-audiod-feedback.sh "$build"
exec make -j"$ZEDBSD_JOBS" ZEDBSD_CONFIG=plan/ws100/tests/config-amd64-audiod.mk BUILD="$build" disk-image
