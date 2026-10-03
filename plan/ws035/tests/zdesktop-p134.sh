#!/bin/sh
# ws035-p134, p136: the terminal's window body keeps square corners (its text runs into them) while its floating
# title bar stays rounded (p136, the user's request); other windows keep both rounded,
# on the Venus guest.  zdesktop --glass at 1280x800 with the wallpaper; /bin/terminal (app_id "terminal") filled
# with text to its edges, then /bin/popup-probe (no app_id, 400x300) beside it.
#  1. terminal.png: the terminal's four body corners are square, its title bar's four rounded (p134-corners.py:
#     each corner pixel is like the rectangle's inside, or like what is outside it).
#  2. both.png: the probe opens over the terminal (seen in the picture: square and rounded side by side).
#  3. wiseview.png: Super+Tab opens Wiseview; the terminal's tile is square, the probe's rounded (seen in the picture).
#  4. maximized.png: the probe closed, a double click on the terminal's title bar docks it; the docked body's
#     corners are square.  restored.png: a double click on the title in the bar undocks it: body square, title bar
#     rounded again.
#  5. probe.png: the terminal closed, the probe opened alone: its body's and its title bar's corners are rounded.
#
#   plan/ws035/tests/zdesktop-guest.sh start IMAGE     (the guest must be up)
#   plan/ws035/tests/zdesktop-p134.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws035-sq-run}"
export GUEST_RUNTIME
out=${1:-build/ws035-shots/p134}
mkdir -p "$out"
# The SSH to the guest, tried again when ssh itself fails (plan/ws099/tests/guest-retry.sh, ws099-p023).
. plan/ws099/tests/guest-retry.sh
guest() { guest_retry 90 "$1" </dev/null; }
. plan/tools/guest/zwl-clients.sh
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; sleep 0.8; }
corners() { python3 plan/ws035/tests/p134-corners.py "$@" | tail -1; python3 plan/ws035/tests/p134-corners.py "$@" >/dev/null || status=1; }
# A floating title bar is ZWL_GLASS_TITLE (44) high, ZWL_GLASS_GAP (8) above the body.
title_y() { echo $(($1 - 8 - 44)); }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[t]erminal|[p]opup-probe" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[t]erminal|[p]opup-probe" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
status=0

# Fails the run unless a log has a line matching a pattern (within a few seconds).
expect_log() {
	tries=0
	found=0
	while [ $tries -lt 6 ]; do
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

# Double clicks a point.
double_click() {
	pointer move $(($1 - 2)) "$2" sleep 150 move "$1" "$2" sleep 300 down sleep 50 up sleep 120 down sleep 50 up sleep 1500
}

guest "$stop_all" >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0; picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=600 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & for w in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30; do grep -q ZWL.READY /tmp/zdesktop.log 2>/dev/null && break; sleep 0.5; done; sleep 1
/bin/terminal --token=t1 --timeout-s=500 > /tmp/t.log 2>&1 </dev/null & sleep 6; echo started' >/dev/null
expect_log /tmp/t.log 'ZTERM START'
zwl_app_clients
set -- $(guest "grep 'ZWL MAP client=$zc1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2/p')
tx=${1:-0}; ty=${2:-0}
set -- $(guest "grep 'ZTERM START' /tmp/t.log | tail -1" | sed -n 's/.* columns=\([0-9]*\) .* window=\([0-9]*\)x\([0-9]*\).*/\1 \2 \3/p')
columns=${1:-80}; tw=${2:-0}; th=${3:-0}
echo "terminal at $tx,$ty size ${tw}x$th columns $columns"

# 1. The terminal filled with text to its edges (a full row of # on every line), the pointer away from it.
pointer move $((tx + tw / 2)) $((ty + th / 2)) sleep 200 down sleep 60 up sleep 300
keys "clear; i=0; while [ \$i -lt 60 ]; do printf '%${columns}s' '' | tr ' ' '#'; i=\$((i+1)); done" '\n'
sleep 2
pointer move 5 790 sleep 600
check "$out/terminal.png" >/dev/null
corners "$out/terminal.png" "$tx" "$ty" "$tw" "$th" square terminal-body
corners "$out/terminal.png" "$tx" "$(title_y "$ty")" "$tw" 44 round terminal-title

# 2. The probe over it (its corners are checked alone, in 5).
guest 'export XDG_RUNTIME_DIR=/tmp; /bin/popup-probe --timeout-s=300 --token=p > /tmp/p.log 2>&1 </dev/null & sleep 4; echo started' >/dev/null
expect_log /tmp/p.log 'POPUPPROBE ready run=p'
set -- $(guest "grep 'ZWL MAP client=$zc2 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2/p')
px=${1:-0}; py=${2:-0}
echo "probe at $px,$py size 400x300"
pointer move 5 790 sleep 600
check "$out/both.png" >/dev/null

# 3. Wiseview.
keys '<super-tab>'
sleep 1.5
expect_log /tmp/zdesktop.log 'ZWL WISEVIEW opening key'
check "$out/wiseview.png" >/dev/null
keys '<esc>'
sleep 1

# 4. Docked (maximized) and back: the probe goes first, then a double click on the terminal's title bar.
guest 'for p in $(ps -A -o pid,args | grep "[p]opup-probe" | awk "{print \$1}"); do kill $p; done; sleep 1; echo ok' >/dev/null
double_click $((tx + tw / 2)) $((ty - 8 - 22))
expect_log /tmp/zdesktop.log 'ZWL GLASS dock surface=[0-9]+ via='
set -- $(guest "grep 'ZWL GLASS dock surface=' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\) w=\([0-9]*\) h=\([0-9]*\).*/\1 \2 \3 \4/p')
dx=${1:-0}; dy=${2:-38}; dw=${3:-1280}; dh=${4:-762}
echo "docked at $dx,$dy size ${dw}x$dh"
sleep 1.5
pointer move 1200 400 sleep 600
check "$out/maximized.png" >/dev/null
corners "$out/maximized.png" "$dx" "$dy" "$dw" $((800 - dy)) square docked-body
set -- $(guest "grep 'ZWL GLASS dock surface=' /tmp/zdesktop.log | tail -1" | sed -n 's/.* title=\([0-9]*\) .*/\1/p')
double_click $((${1:-400} + 20)) 17
expect_log /tmp/zdesktop.log 'ZWL GLASS undock surface=[0-9]+ via='
sleep 1.5
pointer move 5 790 sleep 600
check "$out/restored.png" >/dev/null
corners "$out/restored.png" "$tx" "$ty" "$tw" "$th" square restored-body
corners "$out/restored.png" "$tx" "$(title_y "$ty")" "$tw" 44 round restored-title

# 5. The probe alone: rounded.
guest 'for p in $(ps -A -o pid,args | grep "[t]erminal" | awk "{print \$1}"); do kill $p; done; sleep 1
export XDG_RUNTIME_DIR=/tmp; /bin/popup-probe --timeout-s=300 --token=q > /tmp/q.log 2>&1 </dev/null & sleep 4; echo started' >/dev/null
expect_log /tmp/q.log 'POPUPPROBE ready run=q'
set -- $(guest "grep 'ZWL MAP client=' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2/p')
qx=${1:-0}; qy=${2:-0}
echo "probe alone at $qx,$qy size 400x300"
pointer move 5 790 sleep 600
check "$out/probe.png" >/dev/null
corners "$out/probe.png" "$qx" "$qy" 400 300 round probe-body
corners "$out/probe.png" "$qx" "$(title_y "$qy")" 400 44 round probe-title

# Nothing failed.
guest 'grep -E "ERROR|FAILED|protocol error" /tmp/zdesktop.log /tmp/t.log /tmp/p.log /tmp/q.log' | tee "$out/errors.txt"
[ -s "$out/errors.txt" ] && status=1
guest 'grep -E "MAP|GLASS (dock|undock)|WISEVIEW (opening|close)" /tmp/zdesktop.log' > "$out/zdesktop.log"
guest "$stop_all" >/dev/null
[ $status -eq 0 ] && echo "p134/p136: PASS" || echo "p134/p136: FAIL"
exit $status
