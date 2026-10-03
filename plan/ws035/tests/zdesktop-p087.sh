#!/bin/sh
# ws035-p087: the clipboard between X and Wayland clients through xserver, on the Venus guest (the
# lean image, plan/tools/files/build-files-image.sh).  zdesktop --glass at 1280x800 with terminal (a
# Wayland client) and xserver with zterm (an X client):
#  1. Wayland to X: the terminal copies its screen (Select All, Copy); zterm, clicked, gets the keyboard and the
#     server owns CLIPBOARD for the desktop's text; Ctrl+Shift+V in zterm asks for it: the server reads it from
#     the desktop and zterm pastes it (ZTERM-X PASTE bytes=N); x-paste.png.
#  2. X to Wayland: zterm copies its screen (Ctrl+Shift+C: it owns CLIPBOARD, the server offers it to the
#     desktop); the terminal, clicked, pastes (Ctrl+Shift+V): the server asks zterm (SelectionRequest), zterm
#     answers in the root window's property and SelectionNotify, the server writes it to the terminal
#     (X11 SELECTION sent bytes=N, ZTERM PASTE bytes=N); w-paste.png.
#
#   plan/tools/files/files-guest.sh start     (the guest must be up)
#   plan/ws035/tests/zdesktop-p087.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws035-p087}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
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

# The terminal to the left by its titlebar, then zterm (started now) to the right.
set -- $(window 1)
tx=${1:-0}; ty=${2:-0}
pointer move $((tx + 60)) $((ty - 30)) sleep 300 down sleep 100 move $((tx - 100)) $((ty - 60)) sleep 150 move $((tx - 300)) $((ty - 90)) sleep 400 up sleep 800
tx=$((tx - 300)); ty=$((ty - 90))
guest 'export XDG_RUNTIME_DIR=/tmp; DISPLAY=:0 /bin/xserver --size 640x420 > /tmp/x11server.log 2>&1 </dev/null & sleep 3; DISPLAY=:0 /bin/zterm > /tmp/zterm.log 2>&1 </dev/null & sleep 5; echo started' >/dev/null
set -- $(window 2)
xx=${1:-0}; xy=${2:-0}
pointer move $((xx + 60)) $((xy - 30)) sleep 300 down sleep 100 move $((xx + 150)) $((xy + 30)) sleep 150 move $((xx + 300)) $((xy + 60)) sleep 400 up sleep 800
xx=$((xx + 300)); xy=$((xy + 60))
echo "terminal at $tx,$ty  zterm at $xx,$xy"
shot two.png

# 1. Wayland to X.
click $((tx + 100)) $((ty + 60))
keys 'echo FROM-WAYLAND' '\n'
keys '<ctrl-shift-a>' '<ctrl-shift-c>'
expect_log /tmp/t.log 'ZTERM COPY bytes=[1-9]'
click $((xx + 100)) $((xy + 60)) 1500
expect_log /tmp/x11server.log 'X11 CLIPBOARD selection text=1'
expect_log /tmp/x11server.log 'X11 SELECTION owner selection=CLIPBOARD window=root client=desktop'
keys '<ctrl-shift-v>'
expect_log /tmp/zterm.log 'ZTERM-X PASTE asked'
expect_log /tmp/x11server.log 'X11 CLIPBOARD read bytes=[1-9]'
expect_log /tmp/zterm.log 'ZTERM-X PASTE bytes=[1-9]'
sleep 1
shot x-paste.png

# 2. X to Wayland.
keys '\n' 'clear' '\n' 'echo FROM-X11' '\n'
sleep 1
keys '<ctrl-shift-c>'
expect_log /tmp/zterm.log 'ZTERM-X COPY bytes=[1-9]'
expect_log /tmp/x11server.log 'X11 CLIPBOARD own'
click $((tx + 100)) $((ty + 60)) 1500
keys '<ctrl-shift-v>'
expect_log /tmp/x11server.log 'X11 CLIPBOARD send mime=text/plain;charset=utf-8'
expect_log /tmp/zterm.log 'ZTERM-X SELECTION answered requestor=0x1 bytes=[1-9]'
expect_log /tmp/x11server.log 'X11 SELECTION sent bytes=[1-9]'
expect_log /tmp/t.log 'ZTERM PASTE bytes=[1-9]'
sleep 1
shot w-paste.png

errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
guest 'cat /tmp/x11server.log' > "$out/x11server.log"
guest 'cat /tmp/zterm.log' > "$out/zterm.log"
guest 'cat /tmp/t.log' > "$out/t.log"
guest "$stop_all" >/dev/null
[ $status = 0 ] && echo "zdesktop-p087: PASS" || echo "zdesktop-p087: FAIL"
exit $status
