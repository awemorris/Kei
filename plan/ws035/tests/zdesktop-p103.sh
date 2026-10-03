#!/bin/sh
# ws035-p103: the primary selection between X and Wayland clients through xserver, on the Venus guest
# (the lean image, the files image, or the login image with the greeter stopped).  zdesktop --glass at 1280x800
# with terminal (a Wayland client) and xserver with zterm (an X client):
#  1. Wayland to X: "beta-gamma" double-clicked in the terminal is the desktop's primary selection; zterm, clicked,
#     gets the keyboard and the server owns PRIMARY for it; a middle click in zterm asks for it, the server reads it
#     from the desktop and zterm pastes it (ZTERM-X PASTE bytes=10): x-paste.png.
#  2. X to Wayland: "xprimary-word" double-clicked in zterm is PRIMARY (the server offers it to the desktop as its
#     primary selection); a middle click in the terminal asks for it, the server asks zterm and writes its answer
#     to the terminal (ZTERM PRIMARY paste received bytes=13): w-paste.png.
#
#   plan/tools/files/files-guest.sh start     (the guest must be up)
#   plan/ws035/tests/zdesktop-p103.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws035-p103}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1 </dev/null; }
. plan/tools/guest/zwl-clients.sh
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; sleep 0.8; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[t]erminal|[x]server|[z]term" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[t]erminal|[x]server|[z]term" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
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

# A window's place from zdesktop's log: "x y" of client N's map.
window() {
	guest "grep 'ZWL MAP client=$(zwl_app_client $1) ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2/p'
}
click() {
	pointer move $(($1 - 2)) "$2" sleep 150 move "$1" "$2" sleep 300 down sleep 60 up sleep "${3:-900}"
}
shot() {
	pointer move 1270 790 sleep 400
	check "$out/$1" >/dev/null
}

guest "$stop_all" >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0 /tmp/.X11-unix/X0; picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=600 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & for w in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30; do grep -q ZWL.READY /tmp/zdesktop.log 2>/dev/null && break; sleep 0.5; done; sleep 1
/bin/terminal --token=t1 --timeout-s=500 --columns=56 --rows=12 > /tmp/t.log 2>&1 </dev/null & sleep 5; echo started' >/dev/null

# The terminal where zdesktop put it (no window is moved: the cells' places come from the maps).
set -- $(window 1)
tx=${1:-0}; ty=${2:-0}
set -- $(guest "grep 'ZTERM START' /tmp/t.log | tail -1" | sed -n 's/.* cell=\([0-9]*\)x\([0-9]*\) window=\([0-9]*\)x\([0-9]*\).*/\1 \2 \3 \4/p')
cw=${1:-10}; ch=${2:-20}; tw=${3:-576}; th=${4:-280}
echo "terminal at $tx,$ty size ${tw}x$th cell ${cw}x$ch"

# 1. Wayland to X: "beta-gamma" in the terminal, double-clicked.
click $((tx + tw / 2)) $((ty + th / 2))
keys 'printf "\\033[H\\033[2J"; echo alpha beta-gamma delta' '\n'
sleep 1
bx=$((tx + 8 + 12 * cw + cw / 2)); by=$((ty + 8 + ch / 2))
pointer move "$bx" "$by" sleep 300 down sleep 50 up sleep 120 down sleep 50 up sleep 800
expect_log /tmp/t.log 'ZTERM PRIMARY set bytes=10'

# zterm, started now, on top where zdesktop puts it.
guest 'export XDG_RUNTIME_DIR=/tmp; DISPLAY=:0 /bin/xserver --size 640x420 > /tmp/x11server.log 2>&1 </dev/null & sleep 3; DISPLAY=:0 /bin/zterm -geometry 60x16 > /tmp/zterm.log 2>&1 </dev/null & sleep 5; echo started' >/dev/null
set -- $(window 2)
xx=${1:-0}; xy=${2:-0}
echo "zterm at $xx,$xy (480x256)"
click $((xx + 240)) $((xy + 200)) 1500
expect_log /tmp/x11server.log 'X11 PRIMARY selection text=1'
expect_log /tmp/x11server.log 'X11 SELECTION owner selection=PRIMARY window=root client=desktop'
pointer move $((xx + 240)) $((xy + 200)) sleep 200 middle-down sleep 60 middle-up sleep 1500
expect_log /tmp/zterm.log 'ZTERM-X PRIMARY paste asked'
expect_log /tmp/x11server.log 'X11 PRIMARY read bytes=10'
expect_log /tmp/zterm.log 'ZTERM-X PASTE bytes=10'
sleep 1
shot x-paste.png

# 2. X to Wayland: "xprimary-word" in zterm, double-clicked (zterm's cells are 8x16, from its corner).
keys '\n' 'clear; echo xprimary-word' '\n'
sleep 1
pointer move $((xx + 3 * 8 + 4)) $((xy + 8)) sleep 300 down sleep 50 up sleep 120 down sleep 50 up sleep 800
expect_log /tmp/zterm.log 'ZTERM-X PRIMARY set bytes=13'
expect_log /tmp/x11server.log 'X11 PRIMARY own'

# The terminal, at a point of it zterm does not cover, gets the keyboard and pastes with the middle button.
px=$((tx + 20)); py=$((ty + th - 20))
for spot in "$((tx + 20)) $((ty + 20))" "$((tx + tw - 20)) $((ty + 20))" "$((tx + 20)) $((ty + th - 20))" "$((tx + tw - 20)) $((ty + th - 20))"; do
	set -- $spot
	if [ "$1" -lt "$xx" ] || [ "$1" -ge $((xx + 480)) ] || [ "$2" -lt "$xy" ] || [ "$2" -ge $((xy + 256)) ]; then
		px=$1; py=$2
		break
	fi
done
echo "terminal clicked at $px,$py"
click "$px" "$py" 1500
expect_log /tmp/t.log 'ZTERM PRIMARY offer text=1'
pointer move "$px" "$py" sleep 200 middle-down sleep 60 middle-up sleep 1500
expect_log /tmp/x11server.log 'X11 PRIMARY send mime=text/plain;charset=utf-8'
expect_log /tmp/zterm.log 'ZTERM-X SELECTION answered requestor=0x1 bytes=13'
expect_log /tmp/x11server.log 'X11 SELECTION sent bytes=13'
expect_log /tmp/t.log 'ZTERM PRIMARY paste received bytes=13'
sleep 1
shot w-paste.png

errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
guest 'cat /tmp/x11server.log' > "$out/x11server.log"
guest 'cat /tmp/zterm.log' > "$out/zterm.log"
guest 'cat /tmp/t.log' > "$out/t.log"
guest "$stop_all" >/dev/null
[ $status = 0 ] && echo "zdesktop-p103: PASS" || echo "zdesktop-p103: FAIL"
exit $status
