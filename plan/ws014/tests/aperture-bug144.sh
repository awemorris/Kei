#!/bin/sh
# ws014-p011 (BUG-144): measures what the Model viewer and the other applications hold of Venus's host-visible aperture
# (QEMU's virtio-gpu hostmem, plan/tools/guest/venus-hostmem.sh) and of the CPU's memory, on the Venus guest of an
# image that has mview: plan/ws035/tests/build-zdesktop-image.sh (config-amd64-zdesktop.mk), started with
# plan/tools/files/files-guest.sh start IMAGE.  (The CI image has no mview since 2026-10-02.)  Nothing is judged:
# the numbers are the evidence for the user's advice.  The kernel logs (venus.c):
#   venus: context=N opened pid=P                     each Vulkan connection, with its process
#   venus: aperture full context=N request=B aperture=A used=U blobs=K largest_hole=H
#   venus: aperture context=N bytes=B blobs=K         each context's share, when an allocation found no room
# The CPU's memory: zedBSD's ps has no RSS, so each step records the process's VSZ (ps -o pid,vsz,args, KiB) and the
# system's allocated physical memory (sysctl hw.memory.stats, allocated=BYTES); the step's cost is the difference.
# The run:
#  1. Only with APPHOME=1 (an image with App Home's applications, e.g. the CI one): App Home's every application in
#     order (plan/ws129/phase010/apphome.sh), then the venus lines, ps and the memory into apphome-*.txt.
#  2. Everything stopped; zdesktop alone, then Model viewers started one at a time (mview --windowed --size=960x640,
#     as App Home starts it) up to 14 or until one fails (MVIEW FAILED in its log); after each, its VSZ, the
#     system's allocated bytes and the venus lines.  mview-*.txt; mview-steps.txt has one line per viewer.
#   plan/tools/files/files-guest.sh start IMAGE
#   [APPHOME=1] plan/ws014/tests/aperture-bug144.sh [OUTDIR]     (default build/ws014-shots/bug144)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws014-shots/bug144}
mkdir -p "$out"
guest() { timeout 120 python3 plan/tools/guest/guest.py run "$1" 2>&1 </dev/null; }
stop_all='service stop greeter >/dev/null 2>&1; ps -A -o pid,comm | awk "{ n = \$2; sub(\".*/\", \"\", n) } n ~ /^(wayland|xserver|files|terminal|notes|pdfviewer|imageview|textedit|settings|browser|zgears|zterm|mview)$/ {print \$1}" | while read p; do kill $p; done; sleep 2'

allocated='/sbin/sysctl hw.memory.stats | sed -n "s/.* allocated=\([0-9]*\).*/\1/p"'

# The image must have the Model viewer (the CI image has not since 2026-10-02).
if [ "$(guest 'test -x /bin/mview && echo yes' | tail -1)" != yes ]; then
	echo "aperture-bug144: MISSING /bin/mview in the guest's image (use plan/ws035/tests/build-zdesktop-image.sh)"
	exit 1
fi

# 1. Every application of App Home, when asked for.
if [ "${APPHOME:-0}" = 1 ]; then
	GUEST_RUNTIME=$GUEST_RUNTIME timeout 1500 sh plan/ws129/phase010/apphome.sh "$out/apphome" > "$out/apphome.out" 2>&1
	tail -3 "$out/apphome.out"
	guest 'dmesg | grep "venus: \(context\|aperture\)"' > "$out/apphome-venus.txt"
	guest 'ps -A -o pid,vsz,args' > "$out/apphome-ps.txt"
	guest "$allocated" > "$out/apphome-allocated.txt"
	guest 'grep -h "MVIEW" /tmp/*.log 2>/dev/null | tail -20' > "$out/apphome-mview.txt"
	echo "apphome: $(grep -c 'aperture full' "$out/apphome-venus.txt") aperture-full lines"
fi

# 2. Model viewers alone, one at a time.
guest "$stop_all" >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0; /bin/wayland --timeout=1200 --width=1280 --height=800 --glass > /tmp/zdesktop.log 2>&1 </dev/null & sleep 5; echo started' >/dev/null
guest 'dmesg | grep -c "venus: aperture full"' > "$out/mview-before.txt"
before=$(guest "$allocated" | tail -1)
echo "zdesktop alone: allocated=${before:-?}" | tee "$out/mview-steps.txt"
n=1
while [ $n -le 14 ]; do
	guest "export XDG_RUNTIME_DIR=/tmp; /bin/mview --windowed --size=960x640 --token=m$n > /tmp/mview-$n.log 2>&1 </dev/null & sleep 8; echo started" >/dev/null
	failed=$(guest "grep -c 'MVIEW FAILED' /tmp/mview-$n.log" | tail -1)
	vsz=$(guest "ps -A -o vsz,args | grep '[m]view.*--token=m$n\$' | awk '{print \$1}'" | tail -1)
	now=$(guest "$allocated" | tail -1)
	echo "mview $n: vsz_kib=${vsz:-gone} allocated=${now:-?} failed=${failed:-?}" | tee -a "$out/mview-steps.txt"
	if [ "${failed:-0}" != 0 ]; then
		guest "cat /tmp/mview-$n.log" > "$out/mview-$n-failed.log"
		break
	fi
	n=$((n + 1))
done
guest 'dmesg | grep "venus: \(context\|aperture\)"' > "$out/mview-venus.txt"
guest 'ps -A -o pid,vsz,args' > "$out/mview-ps.txt"
guest "grep -h 'MVIEW' /tmp/mview-1.log" > "$out/mview-1.log"
guest "$stop_all" >/dev/null
grep 'aperture full' "$out/mview-venus.txt" | tail -1
echo "aperture-bug144: DONE"
