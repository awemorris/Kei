#!/bin/sh
# ws099-p020 (BUG-125): a client that answers its press late must still get the whole move or resize.
# /bin/popup-probe --request-delay-ms=DELAY sends xdg_toplevel.move or .resize DELAY ms after the press, while the
# button is still held; the pointer's drag starts at once, as a user's does.  zdesktop must start the move or resize
# from the press (where the client was pressed), not from where the pointer is when the request is read:
#   1. move from the top strip by +100,+100 -> ZWL GLASS moved x=wx+100 y=wy+100
#   2. the bottom-right corner by +150,+100 -> ZWL RESIZE end 550x400
#   3. the left edge by +400 (clamped to the minimum 200) -> ZWL RESIZE end 200x400, settled at the old right edge
# Before the fix (compositor anchored at the pointer when the request came) each step comes short.
#
#   plan/ws035/tests/zdesktop-guest.sh start IMAGE   (guest up)
#   [DELAY=400] [WAYLAND=BUILD/bin/wayland] [PROBE=BUILD/bin/popup-probe] plan/ws099/tests/p020-late-request.sh [OUTDIR]
# WAYLAND and PROBE, when given, are copied into the guest (/tmp) and used instead of the image's /bin ones.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws035-sq-run}"
export GUEST_RUNTIME
out=${1:-build/ws099-p020-late}
delay=${DELAY:-400}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
put() { timeout 90 python3 plan/tools/guest/guest.py put "$1" "$2" >/dev/null 2>&1; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[p]opup-probe" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[p]opup-probe" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
status=0

# Fails the run unless a log has (within a few seconds) a line matching a pattern.
expect_log() {
	tries=0
	found=0
	while [ $tries -lt 5 ]; do
		found=$(guest "grep -cE '$2' $1" | tail -1)
		[ "${found:-0}" -ge 1 ] 2>/dev/null && break
		tries=$((tries + 1))
		sleep 1
	done
	if [ "${found:-0}" -ge 1 ] 2>/dev/null; then
		echo "log: $2 ok"
	else
		echo "log: $2 MISSING"
		status=1
	fi
}

# The binaries under test: the image's, or the ones given (copied in).
wayland=/bin/wayland
probe=/bin/popup-probe
if [ -n "${WAYLAND:-}" ]; then
	put "$WAYLAND" /tmp/p020-wayland
	guest 'chmod 755 /tmp/p020-wayland' >/dev/null
	wayland=/tmp/p020-wayland
fi
if [ -n "${PROBE:-}" ]; then
	put "$PROBE" /tmp/p020-popup-probe
	guest 'chmod 755 /tmp/p020-popup-probe' >/dev/null
	probe=/tmp/p020-popup-probe
fi
echo "wayland $wayland probe $probe delay $delay"

guest "$stop_all" >/dev/null
guest "export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0
$wayland --timeout=400 --width=1280 --height=800 --glass > /tmp/zdesktop.log 2>&1 </dev/null & for w in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30; do grep -q ZWL.READY /tmp/zdesktop.log 2>/dev/null && break; sleep 0.5; done; sleep 1
$probe --wide --request-delay-ms=$delay --timeout-s=300 --token=p > /tmp/p.log 2>&1 </dev/null & sleep 4; echo started" >/dev/null
expect_log /tmp/p.log 'POPUPPROBE ready run=p'
set -- $(guest "grep 'ZWL MAP client=1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2/p')
wx=${1:-0}; wy=${2:-0}
echo "window at $wx,$wy"

# 1. A move from the top strip; the drag starts with the press and lasts past the late request.
pointer move $((wx + 198)) $((wy + 10)) sleep 150 move $((wx + 200)) $((wy + 10)) sleep 300 down sleep 60 \
    move $((wx + 240)) $((wy + 40)) sleep 100 move $((wx + 300)) $((wy + 110)) sleep $((delay + 600)) up sleep 800
expect_log /tmp/p.log 'POPUPPROBE delayed ms='
expect_log /tmp/zdesktop.log "ZWL GLASS moved surface=[0-9]+ x=$((wx + 100)) y=$((wy + 100))"
mx=$((wx + 100)); my=$((wy + 100))

# 2. The bottom-right corner +150,+100.
pointer move $((mx + 388)) $((my + 290)) sleep 150 move $((mx + 390)) $((my + 290)) sleep 300 down sleep 60 \
    move $((mx + 440)) $((my + 323)) sleep 100 move $((mx + 490)) $((my + 356)) sleep 100 \
    move $((mx + 540)) $((my + 390)) sleep $((delay + 600)) up sleep 1200
expect_log /tmp/zdesktop.log 'ZWL RESIZE end surface=[0-9]+ width=550 height=400'

# 3. The left edge +400, clamped to the minimum width 200; the right edge stays.
pointer move $((mx + 3)) $((my + 200)) sleep 150 move $((mx + 5)) $((my + 200)) sleep 300 down sleep 60 \
    move $((mx + 138)) $((my + 200)) sleep 100 move $((mx + 271)) $((my + 200)) sleep 100 \
    move $((mx + 405)) $((my + 200)) sleep $((delay + 600)) up sleep 1200
expect_log /tmp/zdesktop.log 'ZWL RESIZE end surface=[0-9]+ width=200 height=400'
expect_log /tmp/zdesktop.log "ZWL RESIZE settled surface=[0-9]+ x=$((mx + 350)) y=$my width=200 height=400"

# The evidence and the verdict.
guest 'cat /tmp/p.log' > "$out/probe.log"
guest 'grep -E "POPUP|RESIZE|GLASS (request|moved)|ERROR" /tmp/zdesktop.log' > "$out/zdesktop.log"
guest "$stop_all" >/dev/null
[ $status -eq 0 ] && echo "p020-late-request: PASS" || echo "p020-late-request: FAIL"
exit $status
