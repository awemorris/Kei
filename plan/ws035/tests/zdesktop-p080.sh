#!/bin/sh
# ws035-p080: xdg-decoration, cursor-shape and viewporter on the Venus guest, with /bin/extras-probe
# (userland/tests/extras-probe: the protocols' code of its own over libwayland's generic marshalling).
#  1. The window asks for client-side decorations and is told server-side (twice: at creation and after set_mode);
#     zdesktop still draws its title bar (decoration.png).
#  2. The sub-surface's viewport shows only the red quarter of its buffer, stretched to 200x100 at (20,20).
#  3. The pointer enters the window: the cursor is the text I-beam; keys h and e: the pointing hand and the left-right
#     arrow.  Each is told from zdesktop's arrow by pixels near the pointer (white where the arrow is black or clear).
#
#   plan/tools/titlebar/menu-guest.sh start   (the lean image with /bin/extras-probe)
#   plan/ws035/tests/zdesktop-p080.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws035-sq-run}"
export GUEST_RUNTIME
out=${1:-build/ws035-p080}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
. plan/tools/guest/zwl-clients.sh
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[e]xtras-probe" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[e]xtras-probe" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
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
/bin/wayland --timeout=200 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & for w in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30; do grep -q ZWL.READY /tmp/zdesktop.log 2>/dev/null && break; sleep 0.5; done; sleep 1
/bin/extras-probe --timeout-s=150 --token=x > /tmp/x.log 2>&1 </dev/null & sleep 4; echo started' >/dev/null
expect_log /tmp/x.log 'EXTRAS ready run=x'
zwl_app_clients
set -- $(guest "grep 'ZWL MAP client=$zc1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2/p')
wx=${1:-0}; wy=${2:-0}
echo "window at $wx,$wy"

# 1. Server-side decorations, whatever the client asks.
expect_log /tmp/zdesktop.log "ZWL DECORATION asked client=$zc1 mode=1"
expect_log /tmp/zdesktop.log "ZWL DECORATION configure client=$zc1 mode=2" 2
expect_log /tmp/x.log 'EXTRAS decoration mode=2' 2

# 2. The viewport: the red quarter at 200x100 (inside it red; right of it the window).
expect_log /tmp/zdesktop.log 'ZWL VIEWPORT surface=[0-9]+ source=0,0,12800,12800 destination=200,100'
pointer move 40 700 sleep 600
check "$out/decoration.png" --expect $((wx + 40)),$((wy + 40)),e04040 --expect $((wx + 200)),$((wy + 100)),e04040 \
    --expect $((wx + 120)),$((wy + 70)),e04040 --expect $((wx + 240)),$((wy + 70)),2b3444 --expect $((wx + 120)),$((wy + 140)),2b3444 || status=1

# 3. The cursor shapes, at a point of the window away from the sub-surface.
px=$((wx + 300)); py=$((wy + 200))
pointer move $((px - 3)) "$py" sleep 200 move "$px" "$py" sleep 800
expect_log /tmp/zdesktop.log "ZWL CURSOR shape client=$zc1 shape=9 image=0"
check "$out/text.png" --expect "$px","$py",ffffff --expect "$px",$((py + 6)),ffffff || status=1
keys 'h'
expect_log /tmp/zdesktop.log "ZWL CURSOR shape client=$zc1 shape=4 image=1"
pointer move $((px + 1)) "$py" sleep 150 move "$px" "$py" sleep 600
check "$out/hand.png" --expect "$px","$py",ffffff --expect "$px",$((py + 6)),ffffff || status=1
keys 'e'
expect_log /tmp/zdesktop.log "ZWL CURSOR shape client=$zc1 shape=26 image=3"
pointer move $((px + 1)) "$py" sleep 150 move "$px" "$py" sleep 600
check "$out/resize.png" --expect "$px","$py",ffffff --expect $((px + 5)),"$py",ffffff || status=1

# Nothing failed.
guest 'grep -E "ERROR|FAILED|protocol error" /tmp/zdesktop.log /tmp/x.log' | tee "$out/errors.txt"
[ -s "$out/errors.txt" ] && status=1
guest 'cat /tmp/x.log' > "$out/probe.log"
guest 'grep -E "DECORATION|CURSOR|VIEWPORT|MAP" /tmp/zdesktop.log' > "$out/zdesktop.log"
guest "$stop_all" >/dev/null
[ $status -eq 0 ] && echo "p080: PASS" || echo "p080: FAIL"
exit $status
