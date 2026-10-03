#!/bin/sh
# ws035-p052: a fullscreen window and back, checked on the Venus guest (the two modes until ws099-p015, which
# removed the direct scanout: the fullscreen window is composed).
#
# zdesktop runs at 1280x800.  Client a is a red 400x300 window, client b a green
# 300x200 window that asks for fullscreen before its frame FULL and leaves
# it before frame BACK.  The screen is read three times (zdesktop-check.py):
# two windows on the background (a centred at 440,250; b cascaded one step
# at 522,332), then all green (fullscreen mode), then the two windows again.
# The compositor's log gives the direct presents, the Vulkan imports, the
# frames, the mode switches and the frame times.
#
#   plan/ws035/tests/zdesktop-guest.sh start     (the guest must be up)
#   plan/ws035/tests/zdesktop-p052.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
export GUEST_RUNTIME="${GUEST_RUNTIME:-$PWD/build/ws035-sq-run}"
out=${1:-build/ws035-p052}
mkdir -p "$out"
full=${FULL:-60}
back=${BACK:-160}
# The SSH to the guest, tried again when ssh itself fails (plan/ws099/tests/guest-retry.sh, ws099-p023).
. plan/ws099/tests/guest-retry.sh
guest() { guest_retry 60 "$1"; }
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }

# Waits until a guest command prints something, up to about two minutes.
wait_for() {
	i=0
	while [ $i -lt 40 ]; do
		result=$(guest "$1")
		[ -n "$result" ] && return 0
		sleep 3
		i=$((i + 1))
	done
	echo "p052: timed out waiting for: $1"
	return 1
}

# A clean start: no compositor or client left from before.
guest 'service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[w]ltest" | awk "{print \$1}"); do kill $p; done; sleep 1; rm -f /tmp/wayland-0' >/dev/null
guest '/bin/wayland --timeout=400 --width=1280 --height=800 --log-frames > /tmp/zdesktop.log 2>&1 </dev/null & for w in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30; do grep -q ZWL.READY /tmp/zdesktop.log 2>/dev/null && break; sleep 0.5; done; sleep 1
XDG_RUNTIME_DIR=/tmp /bin/wltest --windowed --size=400x300 --color=ff0000 --frames=3000 --token=a > /tmp/a.log 2>&1 </dev/null & sleep 3
XDG_RUNTIME_DIR=/tmp /bin/wltest --windowed --size=300x200 --color=00ff00 --frames=3000 --fullscreen-at='"$full"' --unfullscreen-at='"$back"' --token=b > /tmp/b.log 2>&1 </dev/null & sleep 1; echo started' >/dev/null

# Two windows on the background.
status=0
wait_for 'grep "frame=20 " /tmp/b.log' || status=1
check "$out/windows.png" --expect 10,10,203040 --expect 1270,790,203040 \
    --expect 450,260,ff0000 --expect 460,540,ff0000 --expect 830,260,ff0000 \
    --expect 530,340,00ff00 --expect 810,520,00ff00 --expect 830,545,ff0000 \
    --expect 850,540,203040 || status=1

# Fullscreen: client b's image is the whole output, composed (the direct scanout went in ws099-p015).
wait_for 'grep -E "CONFIGURE .*fullscreen=1" /tmp/zdesktop.log' || status=1
sleep 2
check "$out/fullscreen.png" --expect 10,10,00ff00 --expect 640,400,00ff00 \
    --expect 1270,790,00ff00 --expect 450,260,00ff00 || status=1

# Back to a window, at the same places.
wait_for 'sed -n "/CONFIGURE .*fullscreen=1/,\$p" /tmp/zdesktop.log | grep -E "CONFIGURE .*fullscreen=0"' || status=1
sleep 2
check "$out/back.png" --expect 10,10,203040 --expect 450,260,ff0000 \
    --expect 530,340,00ff00 --expect 810,520,00ff00 --expect 850,540,203040 || status=1

# What the compositor did.
guest 'echo "direct presents: $(grep -c "direct=1" /tmp/zdesktop.log)"
echo "vulkan imports: $(grep -c VULKAN_IMPORT /tmp/zdesktop.log)"
echo "compose frames: $(grep -c "ZWL COMPOSE" /tmp/zdesktop.log)"
grep -E "MODE|OUTPUT|ERROR" /tmp/zdesktop.log
grep "PERF compose" /tmp/zdesktop.log | tail -5' | tee "$out/log-summary.txt"
guest 'for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[w]ltest" | awk "{print \$1}"); do kill $p; done' >/dev/null
[ $status -eq 0 ] && echo "p052: PASS" || echo "p052: FAIL"
exit $status
