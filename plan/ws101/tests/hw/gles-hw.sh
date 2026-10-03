#!/bin/sh
# ws101-p010: OpenGL ES 3.1's compute (and egltest's OpenGL ES 3 transform feedback and query scenes) on the i915
# passthrough of the 5330.  Builds the image (plan/ws101/tests/hw/gles/config.mk and its services: the compositor,
# then run-gles.sh, then a power-off) into BUILD outside the machine's lock, then under flock /tmp/i915-hw.lock
# copies it to the 5330, runs it with ~/bigbang/run-parity-vk.sh (the reference passthrough; the run ends at the
# guest's power-off) and reads the programs' logs from the guest's disk with plan/ws031/tests/ufs-cat.py into
# OUTDIR/guest-logs.txt.  The lock is held only for the copy, the run and the read.
# Prints each program's verdict lines (GLESCOMPUTE ... PASS/FAIL/SKIP, EGLTEST CHECK/DONE) and a summary.
#
#   plan/ws101/tests/hw/gles-hw.sh [OUTDIR]     BUILD (default build/ws101-p010-hw), I915_HOST (default solaris10-man)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../../.."
. plan/tools/guest/jobs.sh  # ZEDBSD_JOBS, the parallel jobs (default 16)
out=${1:-build/ws101-p010-gles-hw}
BUILD=${BUILD:-build/ws101-p010-hw}
host=${I915_HOST:-solaris10-man}
D=plan/ws101/tests/hw/gles
Z=plan/ws031/tests/zdesktop
fonts=/home/awe/zedBSD-rpi4/build/ws035-fonts
mkdir -p "$out" "$BUILD"

# The image: the services of plan/ws031/tests/vkloop-hw.sh zdesktop's first steps, then this run's.
FILES="--file /etc/service.d/vkwait1=plan/ws031/tests/vkwait1 --file /etc/service.d/zdesktop=$Z/zdesktop"
FILES="$FILES --file /etc/service.d/wlwait=$Z/wlwait --file /etc/service.d/gles1=$D/gles1 --file /etc/service.d/poweroff=$D/poweroff"
FILES="$FILES --file /etc/keiland/run-zdesktop.sh=$D/run-zdesktop.sh --file /etc/keiland/run-gles.sh=$D/run-gles.sh"
FILES="$FILES --file /etc/keiland/run-poweroff.sh=$Z/run-poweroff.sh"
[ -f "$fonts/Inter.ttf" ] && FILES="$FILES --file /usr/share/fonts/keiland.ttf=$fonts/Inter.ttf"
timeout 3600 make -j"$ZEDBSD_JOBS" BUILD="$BUILD" ZEDBSD_CONFIG=$D/config.mk I915_TESTS=n I915_TEST_ORACLE=n I915_TEST_VBT=y \
	I915_TEST_CAPTURE=n ZEDBSD_TEST_RC_CONF=$D/rc.conf "ZEDBSD_TEST_EXTRA_FILES=$FILES" ZEDBSD_TEST_IMAGE_TAG=ws101gles \
	disk-image > "$out/build.log" 2>&1 || { echo "gles-hw: BUILD FAILED ($out/build.log)"; exit 1; }
echo "gles-hw: built $BUILD/hdd-image.img"
[ "${GLES_HW_BUILD_ONLY:-0}" = 1 ] && exit 0

# The machine, for the copy, the run and the read only.
exec 9>/tmp/i915-hw.lock
flock 9
echo "gles-hw: machine taken at $(date '+%H:%M:%S')"
status=0
ssh "$host" bigbang/igpu-mode.sh vfio > /dev/null || { echo "gles-hw: iGPU is not on vfio-pci"; exit 1; }
scp -q "$BUILD/hdd-image.img" "$host:bigbang/guest-parity.img" || exit 1
timeout 600 ssh "$host" 'cd ~/bigbang && rm -f run-parity-serial.log && ./run-parity-vk.sh > run-parity-out.log 2>&1; cp run-parity-serial.log vkloop-last.log'
scp -q plan/ws031/tests/ufs-cat.py tools/build/check-ufs-image.py "$host:bigbang/" || exit 1
timeout 120 ssh "$host" 'python3 bigbang/ufs-cat.py bigbang/guest-parity.img /var/log/gles-auto.log /var/log/gles-default.log /var/log/egl-feedback.log /var/log/egl-queries.log /var/log/gles-ps.log /var/log/zdesktop.log /var/log/dmesg.log' \
	> "$out/guest-logs.txt" 2>&1
flock -u 9
echo "gles-hw: machine given back at $(date '+%H:%M:%S')"

# The verdicts.
grep -E '^GLESCOMPUTE (CONTEXT|VERSION|LIMITS|[a-z]+ (PASS|FAIL|SKIP)|DONE)|^EGLTEST (CHECK|DONE|FAILED|FEEDBACK|QUERIES)|^RUN exit|^== |/bin/wayland' "$out/guest-logs.txt"
[ "$(grep -c 'GLESCOMPUTE DONE failures=0' "$out/guest-logs.txt")" -eq 2 ] || status=1
grep -q 'EGLTEST CHECK run=feedback failures=0 glerror=0x0' "$out/guest-logs.txt" || status=1
grep -q 'EGLTEST CHECK run=queries failures=0 glerror=0x0' "$out/guest-logs.txt" || status=1
grep -q 'GLES HW DONE' "$out/guest-logs.txt" || status=1
[ $status -eq 0 ] && echo "gles-hw: PASS" || echo "gles-hw: FAIL"
exit $status
