#!/bin/sh
# ws035-p081: a window's own wp_viewport on the Venus guest, with /bin/extras-probe --body-viewport: the
# window's 400x300 buffer is four quarters (red, green / blue, white) and its viewport shows the right
# half (green above white) at 600x300; its sub-surface's viewport shows the red quarter at 200x100 at (20,20).
#  1. glass.png (zdesktop --glass): the body is 600 wide (green at x=590, white below), the sub-surface
#     red, the left half of the buffer nowhere (no red or blue outside the sub-surface); the floating
#     title bar's close button, placed from the 600-pixel width, closes the window (EXTRAS DONE).
#  2. plain.png (zdesktop without --glass): the same pixels in the plain look (compose.c's quad with
#     the source's uv), the sub-surface at 200x100 there too.
#
#   plan/tools/files/files-guest.sh start     (the lean image with /bin/extras-probe; GUEST_RUNTIME as for it)
#   plan/ws035/tests/zdesktop-p081.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws035-p081}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
. plan/tools/guest/zwl-clients.sh
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[e]xtras-probe" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[e]xtras-probe" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
status=0

# Fails the run unless a log has a line matching a pattern (within a few seconds).
expect_log() {
	tries=0
	found=0
	while [ $tries -lt 5 ]; do
		found=$(guest "grep -cE '$2' $1" | tail -1)
		[ "${found:-0}" -gt 0 ] 2>/dev/null && break
		tries=$((tries + 1))
		sleep 1
	done
	if [ "${found:-0}" -gt 0 ] 2>/dev/null; then
		echo "log: $2 ok"
	else
		echo "log: $2 MISSING"
		status=1
	fi
}

# Starts zdesktop (its options) and the probe; sets wx, wy to the window's place.
start() {
	guest "$stop_all" >/dev/null
	guest "export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0; picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=200 --width=1280 --height=800 $1 > /tmp/zdesktop.log 2>&1 </dev/null & for w in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30; do grep -q ZWL.READY /tmp/zdesktop.log 2>/dev/null && break; sleep 0.5; done; sleep 1
/bin/extras-probe --timeout-s=150 --token=v --body-viewport > /tmp/v.log 2>&1 </dev/null & sleep 4; echo started" >/dev/null
	expect_log /tmp/v.log 'EXTRAS ready run=v'
	expect_log /tmp/v.log 'EXTRAS body-viewport source=200,0,200,300 destination=600,300'
	expect_log /tmp/zdesktop.log 'ZWL VIEWPORT surface=[0-9]+ source=51200,0,51200,76800 destination=600,300'
	zwl_app_clients
	set -- $(guest "grep 'ZWL MAP client=$zc1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2/p')
	wx=${1:-0}; wy=${2:-0}
	echo "window at $wx,$wy"
}

# The pixels of the window: the right half at 600x300, the sub-surface's red quarter at 200x100.
pixels() {
	pointer move 20 790 sleep 600
	check "$out/$1" \
	    --expect $((wx + 300)),$((wy + 60)),40c060 --expect $((wx + 590)),$((wy + 100)),40c060 \
	    --expect $((wx + 500)),$((wy + 250)),ffffff --expect $((wx + 30)),$((wy + 250)),ffffff \
	    --expect $((wx + 120)),$((wy + 70)),e04040 --expect $((wx + 210)),$((wy + 110)),e04040 \
	    --expect $((wx + 250)),$((wy + 70)),40c060 || status=1
}

# 1. The glass look, and the close button from the viewport's width.
start --glass
pixels glass.png
cx=$((wx + 600 - 26)); cy=$((wy - 8 - 22))
pointer move $((cx - 2)) "$cy" sleep 200 move "$cx" "$cy" sleep 400 down sleep 60 up sleep 1500
expect_log /tmp/v.log 'EXTRAS DONE run=v'
guest 'cat /tmp/v.log' > "$out/glass-probe.log"

# 2. The plain look.
start ''
pixels plain.png
guest 'cat /tmp/v.log' > "$out/plain-probe.log"

# Nothing failed.
guest 'grep -E "ERROR|FAILED|protocol error" /tmp/zdesktop.log /tmp/v.log' | tee "$out/errors.txt"
[ -s "$out/errors.txt" ] && status=1
guest "$stop_all" >/dev/null
[ $status = 0 ] && echo "zdesktop-p081: PASS" || echo "zdesktop-p081: FAIL"
exit $status
