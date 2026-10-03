#!/bin/sh
# ws101-p011: G3 (Noct's automatic parallelization on the GPU, the demonstration's scene S13) on the i915
# passthrough of the 5330.  Builds the image (plan/ws101/tests/hw/g3/config.mk and its services: the compositor,
# then run-g3.sh, then a power-off) into BUILD outside the machine's lock, with /bin/noct taken from a build made
# with ZEDBSD_NOCT_ACCEL := y (NOCT, by default the main checkout's demonstration build); then under
# flock /tmp/i915-hw.lock copies it to the 5330, runs it with ~/bigbang/run-parity-vk.sh (the run ends at the
# guest's power-off) and reads the logs from the guest's disk with plan/ws031/tests/ufs-cat.py into
# OUTDIR/guest-logs.txt.  The lock is held only for the copy, the run and the read.
# It passes when the demonstration script says the results are the same, both runs of mix.nct check with none
# wrong and have equal checksums, and the GPU run's dispatches (one a call) are in its log.
#
#   plan/ws101/tests/hw/g3-hw.sh [OUTDIR]     BUILD (default build/ws101-p011-hw), NOCT, I915_HOST (solaris10-man)
#   G3_HW_BUILD_ONLY=1 builds the image only.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../../.."
. plan/tools/guest/jobs.sh  # ZEDBSD_JOBS, the parallel jobs (default 16)
out=${1:-build/ws101-p011-g3-hw}
BUILD=${BUILD:-build/ws101-p011-hw}
host=${I915_HOST:-solaris10-man}
noct=${NOCT:-/home/awe/zedBSD-rpi4/build/demo-lcd9/bin/noct}
D=plan/ws101/tests/hw/g3
Z=plan/ws031/tests/zdesktop
G=plan/ws101/tests/hw/gles
fonts=/home/awe/zedBSD-rpi4/build/ws035-fonts
mkdir -p "$out" "$BUILD"
[ -x "$noct" ] || { echo "g3-hw: no accelerator-enabled noct at $noct"; exit 1; }

# The image: the compositor's services, this run's, and the accelerator-enabled noct.
FILES="--file /etc/service.d/vkwait1=plan/ws031/tests/vkwait1 --file /etc/service.d/zdesktop=$Z/zdesktop"
FILES="$FILES --file /etc/service.d/wlwait=$Z/wlwait --file /etc/service.d/g3run=$D/g3run --file /etc/service.d/poweroff=$D/poweroff"
FILES="$FILES --file /etc/keiland/run-zdesktop.sh=$G/run-zdesktop.sh --file /etc/keiland/run-g3.sh=$D/run-g3.sh"
FILES="$FILES --file /etc/keiland/run-poweroff.sh=$Z/run-poweroff.sh --file /bin/noct=$noct --mode /bin/noct=0755"
[ -f "$fonts/Inter.ttf" ] && FILES="$FILES --file /usr/share/fonts/keiland.ttf=$fonts/Inter.ttf"
timeout 3600 make -j"$ZEDBSD_JOBS" BUILD="$BUILD" ZEDBSD_CONFIG=$D/config.mk I915_TESTS=n I915_TEST_ORACLE=n I915_TEST_VBT=y \
	I915_TEST_CAPTURE=n ZEDBSD_TEST_RC_CONF=$D/rc.conf "ZEDBSD_TEST_EXTRA_FILES=$FILES" ZEDBSD_TEST_IMAGE_TAG=ws101g3 \
	disk-image > "$out/build.log" 2>&1 || { echo "g3-hw: BUILD FAILED ($out/build.log)"; exit 1; }
echo "g3-hw: built $BUILD/hdd-image.img"
[ "${G3_HW_BUILD_ONLY:-0}" = 1 ] && exit 0

# The machine, for the copy, the run and the read only.
exec 9>/tmp/i915-hw.lock
flock 9
echo "g3-hw: machine taken at $(date '+%H:%M:%S')"
status=0
ssh "$host" bigbang/igpu-mode.sh vfio > /dev/null || { echo "g3-hw: iGPU is not on vfio-pci"; exit 1; }
scp -q "$BUILD/hdd-image.img" "$host:bigbang/guest-parity.img" || exit 1
timeout 600 ssh "$host" 'cd ~/bigbang && rm -f run-parity-serial.log && ./run-parity-vk.sh > run-parity-out.log 2>&1; cp run-parity-serial.log vkloop-last.log'
scp -q plan/ws031/tests/ufs-cat.py tools/build/check-ufs-image.py "$host:bigbang/" || exit 1
timeout 120 ssh "$host" 'python3 bigbang/ufs-cat.py bigbang/guest-parity.img /var/log/g3-s13.log /var/log/g3-cpu.log /var/log/g3-gpu.log /var/log/g3-ps.log /var/log/zdesktop.log /var/log/dmesg.log' \
	> "$out/guest-logs.txt" 2>&1
flock -u 9
echo "g3-hw: machine given back at $(date '+%H:%M:%S')"

# The logs apart: the script's, the CPU run's and the GPU run's (ufs-cat.py prints each file after a line "===== PATH").
awk '/^===== \/var\/log\/g3-s13.log/ { f = 1; next } /^===== / { f = 0 } f' "$out/guest-logs.txt" > "$out/s13.txt"
awk '/^===== \/var\/log\/g3-cpu.log/ { f = 1; next } /^===== / { f = 0 } f' "$out/guest-logs.txt" > "$out/cpu.txt"
awk '/^===== \/var\/log\/g3-gpu.log/ { f = 1; next } /^===== / { f = 0 } f' "$out/guest-logs.txt" > "$out/gpu.txt"
for f in s13 cpu gpu; do echo "== $f"; grep -v '^gles: compute dispatch' "$out/$f.txt"; done
grep -E '/bin/wayland|^== |G3 HW DONE' "$out/guest-logs.txt" | tail -12

# The verdict.
cpu_sum=$(grep '^MIX checksum' "$out/cpu.txt")
gpu_sum=$(grep '^MIX checksum' "$out/gpu.txt")
dispatches=$(grep -c '^gles: compute dispatch' "$out/gpu.txt")
echo "gpu dispatches: $dispatches"
grep -q 'The results are the same on the CPU and the GPU.' "$out/s13.txt" || { echo "g3-hw: the script did not find the same results"; status=1; }
grep -q '^MIX check=1001 wrong=0' "$out/cpu.txt" || { echo "g3-hw: the CPU run's check failed"; status=1; }
grep -q '^MIX check=1001 wrong=0' "$out/gpu.txt" || { echo "g3-hw: the GPU run's check failed"; status=1; }
[ -n "$cpu_sum" ] && [ "$cpu_sum" = "$gpu_sum" ] || { echo "g3-hw: the checksums differ"; status=1; }
[ "$dispatches" -ge 8 ] || { echo "g3-hw: fewer dispatches than calls (the GPU was not used)"; status=1; }
grep -q 'G3 HW DONE' "$out/guest-logs.txt" || status=1
[ $status -eq 0 ] && echo "g3-hw: PASS" || echo "g3-hw: FAIL"
exit $status
