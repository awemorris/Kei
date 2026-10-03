#!/bin/sh
# ws035-p138 (BUG-114): a window leaving fullscreen keeps its title bar under the system bar, on the Venus guest.
# zdesktop --glass at 1280x800, the pointer driven through QMP.
#  1. Notes started fullscreen by the top-right swipe (corner.c, /bin/notes --fullscreen), then Esc (Notes' own
#     xdg_toplevel.unset_fullscreen): it had no place as a window, so zdesktop centres it in the space
#     (ZWL WINDOW unfullscreen ... placed=0, then ZWL WINDOW centred at its window size), its title bar below the
#     system bar (y >= ZWL_GLASS_TOP, 98).  unfullscreen.png.
#  2. A drag on its title bar moves it (ZWL GLASS moved).  moved.png.
#  3. The swipe again: the compositor makes the Notes window fullscreen (the compositor's path); Esc brings it back
#     to where it was moved (placed=1), kept inside the space (moved up if its body would overhang the bottom).  back.png.
#
#   plan/ws035/tests/zdesktop-guest.sh start IMAGE     (the guest must be up)
#   plan/ws035/tests/zdesktop-p138.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws035-sq-run}"
export GUEST_RUNTIME
out=${1:-build/ws035-shots/p138}
mkdir -p "$out"
# The SSH to the guest, tried again when ssh itself fails (plan/ws099/tests/guest-retry.sh, ws099-p023).
. plan/ws099/tests/guest-retry.sh
guest() { guest_retry 90 "$1" </dev/null; }
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; sleep 0.8; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|/bin/[n]otes" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|/bin/[n]otes" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
status=0
top=98

# Fails the run unless zdesktop's log has (within a few seconds) as many lines matching a pattern as asked (default 1).
expect_log() {
	tries=0
	found=0
	while [ $tries -lt 8 ]; do
		found=$(guest "grep -cE '$1' /tmp/zdesktop.log" | tail -1)
		[ "${found:-0}" -ge "${2:-1}" ] 2>/dev/null && break
		tries=$((tries + 1))
		sleep 1
	done
	if [ "${found:-0}" -ge "${2:-1}" ] 2>/dev/null; then
		echo "log: $1 (${2:-1}) ok"
	else
		echo "log: $1 (${2:-1}) MISSING (found ${found:-0})"
		status=1
	fi
}

# A drag from one point to another in n steps of ms milliseconds (the swipe).
stroke() {
	x0=$1; y0=$2; x1=$3; y1=$4; n=$5; ms=$6
	steps="move $x0 $y0 sleep 200 down sleep 30"
	i=1
	while [ $i -le $n ]; do
		steps="$steps move $((x0 + (x1 - x0) * i / n)) $((y0 + (y1 - y0) * i / n)) sleep $ms"
		i=$((i + 1))
	done
	echo "$steps"
}

guest "$stop_all" >/dev/null
guest 'rm -rf /root/Documents/Notes /root/.local/share/keiland/notes; export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0; picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=600 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & for w in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30; do grep -q ZWL.READY /tmp/zdesktop.log 2>/dev/null && break; sleep 0.5; done; sleep 1; echo started' >/dev/null

# 1. The swipe starts Notes fullscreen; Esc leaves it: centred, its title bar under the system bar.
pointer $(stroke 1272 6 1072 206 10 30) up sleep 5000
expect_log 'ZWL CORNER commit'
pointer move 640 400 sleep 300
check "$out/fullscreen.png" >/dev/null
keys '<esc>'
sleep 2
expect_log 'ZWL WINDOW unfullscreen surface=[0-9]+ x=-?[0-9]+ y=-?[0-9]+ placed=0'
expect_log 'ZWL WINDOW centred surface=[0-9]+ x=[0-9]+ y=[0-9]+ width=[0-9]+ height=[0-9]+'
set -- $(guest "grep 'ZWL WINDOW centred' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\) width=\([0-9]*\) height=\([0-9]*\).*/\1 \2 \3 \4/p')
nx=${1:-0}; ny=${2:-0}; nw=${3:-0}; nh=${4:-0}
echo "notes centred at $nx,$ny size ${nw}x$nh"
if [ "$ny" -ge "$top" ] 2>/dev/null; then
	echo "title bar under the system bar (y=$ny >= $top) ok"
else
	echo "title bar under the system bar (y=$ny >= $top) FAIL"
	status=1
fi
centre=$((nx + nw / 2))
if [ $((centre - 640)) -le 2 ] && [ $((640 - centre)) -le 2 ]; then
	echo "centred across (middle $centre) ok"
else
	echo "centred across (middle $centre) FAIL"
	status=1
fi
pointer move 1270 790 sleep 600
check "$out/unfullscreen.png" >/dev/null

# 2. Its title bar drags it: an empty part of the title bar (left of the buttons, right of Notes' menus), 30 pixels above the body.
tx=$((nx + nw - 250)); ty=$((ny - 30))
pointer move $((tx - 2)) $ty sleep 200 move $tx $ty sleep 300 down sleep 200 move $((tx + 40)) $((ty + 20)) sleep 150 move $((tx + 100)) $((ty + 60)) sleep 300 up sleep 800
expect_log "ZWL GLASS moved surface=[0-9]+ x=$((nx + 100)) y=$((ny + 60))"
pointer move 1270 790 sleep 600
check "$out/moved.png" >/dev/null

# 3. The swipe makes the window fullscreen (the compositor's path); Esc brings it back where it was moved.
pointer $(stroke 1272 6 1072 206 10 30) up sleep 3000
expect_log 'ZWL CORNER commit' 2
keys '<esc>'
sleep 2
# Its place, kept inside the space: a body that would overhang the space's bottom is moved up (zwl_glass_fit).
space_bottom=$((800 - 12))
want_y=$((ny + 60))
[ $((want_y + nh)) -gt $space_bottom ] && want_y=$((space_bottom - nh))
[ $want_y -lt $top ] && want_y=$top
expect_log "ZWL WINDOW unfullscreen surface=[0-9]+ x=$((nx + 100)) y=$want_y placed=1"
pointer move 1270 790 sleep 600
check "$out/back.png" >/dev/null

# Nothing failed.
guest 'grep -E "ERROR|FAILED|protocol error" /tmp/zdesktop.log' | tee "$out/errors.txt"
[ -s "$out/errors.txt" ] && status=1
guest 'grep -E "ZWL (CORNER|WINDOW|MAP|GLASS moved)|NOTES (START|LAYOUT)" /tmp/zdesktop.log' > "$out/zdesktop.log"
guest "$stop_all" >/dev/null
[ $status -eq 0 ] && echo "p138: PASS" || echo "p138: FAIL"
exit $status
