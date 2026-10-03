#!/bin/sh
# BUG-095 (ws099-p027): the SSH guest image (plan/tools/guest/config-amd64-ssh.mk) with a oneshot service,
# bug095_poweroff, that runs /sbin/poweroff 30 seconds after the boot (plan/ws099/tests/bug095/).
#
#   plan/ws099/tests/bug095/build-bug095-image.sh BUILD
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../../.."
. plan/tools/guest/jobs.sh  # ZEDBSD_JOBS, the parallel jobs (default 16)
build=${1:?usage: build-bug095-image.sh BUILD}
extra=$(python3 plan/tools/guest/guest.py extra-files | sed -n "s/^ZEDBSD_TEST_EXTRA_FILES='\(.*\)'$/\1/p")
[ -n "$extra" ] || { echo "build-bug095-image: no guest files (plan/tools/guest/guest.py keys?)"; exit 1; }
extra="$extra --file /etc/service.d/bug095_poweroff=plan/ws099/tests/bug095/bug095_poweroff"
extra="$extra --file /etc/bug095-poweroff.sh=plan/ws099/tests/bug095/bug095-poweroff.sh"
exec make -j"$ZEDBSD_JOBS" ZEDBSD_CONFIG=plan/tools/guest/config-amd64-ssh.mk BUILD="$build" \
    ZEDBSD_TEST_RC_CONF=plan/ws099/tests/bug095/rc.conf "ZEDBSD_TEST_EXTRA_FILES=$extra" disk-image
