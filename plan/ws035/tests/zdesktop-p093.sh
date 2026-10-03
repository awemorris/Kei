#!/bin/sh
# ws035-p093: selecting text in terminal with the pointer, and dragging it out, on the Venus guest (the
# lean image, plan/tools/files/build-files-image.sh, or the browser image).  zdesktop --glass at 1280x800 and
# terminal; the screen is cleared and "alpha beta-gamma delta" is printed on its first row:
#  1. word.png: a double click on "gamma" selects the word "beta-gamma" (ZTERM SELECT how=word from=6,0 to=15,0
#     bytes=10); Ctrl+Shift+C copies it (ZTERM COPY bytes=10).
#  2. A triple click selects the line (how=line bytes=22).
#  3. drag.png: a press on "a" of alpha moved to its last "a" selects "alpha" (how=drag bytes=5).
#  4. dropped.png: a press inside the selection moved away drags its text out (ZTERM DRAG start bytes=5); dropped
#     on the terminal's own rows below, it is pasted into the shell (ZTERM DROP bytes=5) and the drag ends dropped.
#
#   plan/tools/files/files-guest.sh start     (the guest must be up)
#   plan/ws035/tests/zdesktop-p093.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws035-p093}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1 </dev/null; }
. plan/tools/guest/zwl-clients.sh
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; sleep 0.8; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[t]erminal" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[t]erminal" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
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

shot() {
	pointer move 1270 790 sleep 400
	check "$out/$1" >/dev/null
}

# The screen point of a cell's middle.
cx() { echo $((wx + 8 + $1 * cw + cw / 2)); }
cy() { echo $((wy + 8 + $1 * ch + ch / 2)); }

guest "$stop_all" >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0; picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=600 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & for w in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30; do grep -q ZWL.READY /tmp/zdesktop.log 2>/dev/null && break; sleep 0.5; done; sleep 1
/bin/terminal --token=t1 --timeout-s=500 > /tmp/t.log 2>&1 </dev/null & sleep 6; echo started' >/dev/null
zwl_app_clients
set -- $(guest "grep 'ZWL MAP client=$zc1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2/p')
wx=${1:-0}; wy=${2:-0}
set -- $(guest "grep 'ZTERM START' /tmp/t.log | tail -1" | sed -n 's/.* cell=\([0-9]*\)x\([0-9]*\) .*/\1 \2/p')
cw=${1:-10}; ch=${2:-20}
echo "terminal at $wx,$wy cell ${cw}x$ch"

# The text on the first row.
pointer move "$(cx 30)" "$(cy 10)" sleep 200 down sleep 60 up sleep 300
keys 'printf "\\033[H\\033[2J"; echo alpha beta-gamma delta' '\n'
sleep 1

# 1. A double click: the word, copied.
pointer move "$(cx 12)" "$(cy 0)" sleep 300 down sleep 50 up sleep 120 down sleep 50 up sleep 800
expect_log /tmp/t.log 'ZTERM SELECT how=word from=6,0 to=15,0 bytes=10'
shot word.png
keys '<ctrl-shift-c>'
expect_log /tmp/t.log 'ZTERM COPY bytes=10'

# 2. A triple click: the line.
sleep 1
pointer move "$(cx 2)" "$(cy 0)" sleep 300 down sleep 50 up sleep 120 down sleep 50 up sleep 120 down sleep 50 up sleep 800
expect_log /tmp/t.log 'ZTERM SELECT how=line from=0,0 to=[0-9]+,0 bytes=22'

# 3. A click inside the line clears it; then a press moved: "alpha".
sleep 1
pointer move "$(cx 20)" "$(cy 0)" sleep 300 down sleep 50 up sleep 800
pointer move "$(cx 0)" "$(cy 0)" sleep 300 down sleep 100 move "$(cx 2)" "$(cy 0)" sleep 100 move "$(cx 4)" "$(cy 0)" sleep 200 up sleep 800
expect_log /tmp/t.log 'ZTERM SELECT how=drag from=0,0 to=4,0 bytes=5'
shot drag.png

# 4. A press inside the selection moved away: the text dragged out, dropped on the rows below.
sleep 1
pointer move "$(cx 2)" "$(cy 0)" sleep 300 down sleep 100 move "$(cx 4)" "$(cy 2)" sleep 150 move "$(cx 10)" "$(cy 6)" sleep 150 move "$(cx 12)" "$(cy 8)" sleep 700 up sleep 1500
expect_log /tmp/t.log 'ZTERM DRAG start bytes=5'
expect_log /tmp/t.log 'ZTERM DROP bytes=5 uris=0'
expect_log /tmp/t.log 'ZTERM DRAG done dropped=1'
shot dropped.png

errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
guest 'cat /tmp/t.log' > "$out/t.log"
guest 'grep -E "ZWL DATA" /tmp/zdesktop.log' > "$out/zdesktop-data.log"
guest "$stop_all" >/dev/null
[ $status = 0 ] && echo "zdesktop-p093: PASS" || echo "zdesktop-p093: FAIL"
exit $status
