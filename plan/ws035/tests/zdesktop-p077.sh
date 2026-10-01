#!/bin/sh
# ws035-p077: wl_subcompositor and wl_subsurface on the Venus guest, with /bin/subsurface-probe
# (userland/tests/subsurface-probe).  zdesktop --glass runs at 1280x800; the probe's window (400x300, dark 2b3444)
# is centred, with a (red e04040, 100x80) above it at (20,20), c (yellow e0e040, 40x40) a child of a at (70,50), and
# b (green 40c060, 100x80) below it at (-40,200).
#  1. shown.png: a and c over the window, b only left of the window (the window covers the rest).
#  2. Key s: a is moved and given a magenta image, but a is synchronized: nothing changes (sync.png).
#  3. Key c: the window commits: a is at (200,20), magenta, and c went with it (applied.png).
#  4. Key d: b is desynchronized and commits blue: it shows at once (desync.png).
#  5. Key o: b is placed above the window: all of it shows (above.png).
#  6. The pointer over a, over c, over the window: each hears enter with its own surface-local position.
#  7. Key x: a's wl_subsurface is destroyed: a and c are gone (destroyed.png).
#
#   plan/tools/titlebar/menu-guest.sh start   (the lean image with /bin/subsurface-probe)
#   plan/ws035/tests/zdesktop-p077.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws035-sq-run}"
export GUEST_RUNTIME
out=${1:-build/ws035-p077}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[s]ubsurface-probe" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[s]ubsurface-probe" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
status=0

# Fails the run unless a log has (within a few seconds) as many lines matching a pattern as asked (default 1).
expect_log() {
	tries=0
	found=0
	while [ $tries -lt 5 ]; do
		found=$(guest "grep -cE '$2' $1" | tail -1)
		[ "${found:-0}" -ge "${3:-1}" ] 2>/dev/null && break
		tries=$((tries + 1))
		sleep 1
	done
	if [ "${found:-0}" -ge "${3:-1}" ] 2>/dev/null; then
		echo "log: $2 ok"
	else
		echo "log: $2 MISSING (found ${found:-0})"
		status=1
	fi
}

guest "$stop_all" >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0; picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=300 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & for w in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30; do grep -q ZWL.READY /tmp/zdesktop.log 2>/dev/null && break; sleep 0.5; done; sleep 1
/bin/subsurface-probe --timeout-s=240 --token=s > /tmp/s.log 2>&1 </dev/null & sleep 4; echo started' >/dev/null
expect_log /tmp/s.log 'SUBPROBE ready run=s'
expect_log /tmp/zdesktop.log 'ZWL SUBSURFACE create client=1 ' 3
set -- $(guest "grep 'ZWL MAP client=1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2/p')
wx=${1:-0}; wy=${2:-0}
echo "window at $wx,$wy"
sleep 1

# 1. The first picture: a and c over the window, b left of it (and under it).
check "$out/shown.png" --expect $((wx + 30)),$((wy + 30)),e04040 --expect $((wx + 100)),$((wy + 85)),e0e040 \
    --expect $((wx - 20)),$((wy + 240)),40c060 --expect $((wx + 30)),$((wy + 240)),2b3444 \
    --expect $((wx + 250)),$((wy + 150)),2b3444 || status=1

# 2. s: a's move and new image wait for the window's commit.
keys 's'
expect_log /tmp/s.log 'SUBPROBE draw a color=c040c0'
sleep 1
check "$out/sync.png" --expect $((wx + 30)),$((wy + 30)),e04040 --expect $((wx + 230)),$((wy + 30)),2b3444 || status=1

# 3. c: the window commits, and a (with c) moves and turns magenta.
keys 'c'
expect_log /tmp/s.log 'SUBPROBE commit window'
sleep 1
check "$out/applied.png" --expect $((wx + 30)),$((wy + 30)),2b3444 --expect $((wx + 230)),$((wy + 30)),c040c0 \
    --expect $((wx + 280)),$((wy + 85)),e0e040 --expect $((wx + 100)),$((wy + 85)),2b3444 || status=1

# 4. d: b, desynchronized, shows its blue image at once.
keys 'd'
expect_log /tmp/s.log 'SUBPROBE draw b color=4060e0'
sleep 1
check "$out/desync.png" --expect $((wx - 20)),$((wy + 240)),4060e0 --expect $((wx + 30)),$((wy + 240)),2b3444 || status=1

# 5. o: b above the window.
keys 'o'
expect_log /tmp/s.log 'SUBPROBE place b above window'
sleep 1
check "$out/above.png" --expect $((wx + 30)),$((wy + 240)),4060e0 || status=1

# 6. The pointer: over a, over c, back over the window.
pointer move $((wx + 250)) $((wy + 40)) sleep 400 move $((wx + 290)) $((wy + 90)) sleep 400 move $((wx + 300)) $((wy + 200)) sleep 600
expect_log /tmp/s.log 'SUBPROBE enter a x=50 y=20'
expect_log /tmp/s.log 'SUBPROBE enter c x=20 y=20'
expect_log /tmp/s.log 'SUBPROBE enter window x=300 y=200'
expect_log /tmp/s.log 'SUBPROBE leave a'

# 7. x: a's role goes; a and c are not shown.
keys 'x'
expect_log /tmp/s.log 'SUBPROBE destroy a'
sleep 1
check "$out/destroyed.png" --expect $((wx + 230)),$((wy + 30)),2b3444 --expect $((wx + 280)),$((wy + 85)),2b3444 \
    --expect $((wx + 30)),$((wy + 240)),4060e0 || status=1

# Nothing failed.
guest 'grep -E "ERROR|FAILED|protocol error" /tmp/zdesktop.log /tmp/s.log' | tee "$out/errors.txt"
[ -s "$out/errors.txt" ] && status=1
guest 'cat /tmp/s.log' > "$out/probe.log"
guest 'grep -E "SUBSURFACE|MAP" /tmp/zdesktop.log' > "$out/zdesktop.log"
guest "$stop_all" >/dev/null
[ $status -eq 0 ] && echo "p077: PASS" || echo "p077: FAIL"
exit $status
