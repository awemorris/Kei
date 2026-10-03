#!/bin/sh
# ws063: builds the SSH guest image (plan/tools/guest/config-amd64-ssh.mk) into BUILD.
#   sh plan/tools/guest/build-ssh-image.sh [BUILD]    (default build/amd64, which the packages link against)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
root=$(cd "$(dirname "$0")/../../.." && pwd)
cd "$root" || exit 1
. plan/tools/guest/jobs.sh  # ZEDBSD_JOBS, the parallel jobs (default 16)
build=${1:-build/amd64}
extra=$(python3 plan/tools/guest/guest.py extra-files)
eval "make -j$ZEDBSD_JOBS ZEDBSD_CONFIG=plan/tools/guest/config-amd64-ssh.mk BUILD=$build $extra disk-image"
