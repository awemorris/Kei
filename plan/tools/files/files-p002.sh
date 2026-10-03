#!/bin/sh
# ws071-p002: files' window on the Venus guest (the lean image, build-files-image.sh).
# zdesktop --glass runs at 1280x800 with the wallpaper; files opens at 1000x640 on a
# sample home (make-home.sh in /tmp/fhome).  Every step checks the file manager's log, and the
# screens are kept for reading:
#  1. home.png: the window: toolbar, sidebar, the home dashboard.
#  2. documents.png: the sidebar's Documents clicked (LOCATION .../Documents).
#  3. folder.png: back home by the Back button, then Documents opened by its card on the dashboard.
#  4. The Back and Forward buttons move through the history (LOCATION lines).
#  5. The program ends (SIGTERM); no ERROR line in zdesktop's log.
#
#   plan/tools/files/files-guest.sh start     (the guest must be up)
#   plan/tools/files/files-p002.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws071-p002}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
. plan/tools/guest/zwl-clients.sh
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[f]iles" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[f]iles" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
status=0

# Fails the run unless a log has a line matching a pattern.
expect_log() {
	found=$(guest "grep -cE '$2' $1" | tail -1)
	if [ "${found:-0}" -gt 0 ] 2>/dev/null; then
		echo "log: $2 ok"
	else
		echo "log: $2 MISSING"
		status=1
	fi
}

# Clicks a point of the window's body (x, y from its top left) and waits.
click() {
	pointer move $((wx + $1 - 2)) $((wy + $2)) sleep 150 move $((wx + $1)) $((wy + $2)) sleep 300 down sleep 60 up sleep "${3:-700}"
}
double() {
	pointer move $((wx + $1)) $((wy + $2)) sleep 300 down sleep 50 up sleep 80 down sleep 50 up sleep "${3:-900}"
}

# Clicks a control of the window's titlebar (drawn by zdesktop; its place from zdesktop's log, ws071-p014).
control() {
	zwl_app_clients
	set -- $(guest "grep 'ZWL TITLEBAR control client=$zc1 .* where=floating id=$1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\) width=\([0-9]*\) height=\([0-9]*\).*/\1 \2 \3 \4/p')
	set -- $((${1:-0} + ${3:-0} / 2)) $((${2:-0} + ${4:-0} / 2))
	pointer move $(($1 - 2)) "$2" sleep 150 move "$1" "$2" sleep 300 down sleep 60 up sleep 900
}

guest "$stop_all" >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0; rm -rf /tmp/fhome; sh /usr/share/files-tests/make-home.sh /tmp/fhome >/dev/null
picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=900 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4
HOME=/tmp/fhome /bin/files --token=f1 --timeout-s=800 --width=1000 --height=640 > /tmp/f.log 2>&1 </dev/null & sleep 5; echo started' >/dev/null
zwl_app_clients
set -- $(guest "grep 'ZWL MAP client=$zc1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* surface=\([0-9]*\) x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2 \3/p')
surface=${1:-0}; wx=${2:-0}; wy=${3:-0}
echo "files: surface $surface at $wx,$wy"

# 1. The window.
expect_log /tmp/f.log 'ZFILES READY width=1000 height=640 token=f1'
expect_log /tmp/f.log 'ZFILES LOCATION kind=home path=/tmp/fhome items=0 error=0'
pointer move 1270 790 sleep 400
check "$out/home.png" >/dev/null

# 2. Documents from the sidebar.
click 100 125
expect_log /tmp/f.log 'ZFILES LOCATION kind=folder path=/tmp/fhome/Documents items=6 error=0'
pointer move 1270 790 sleep 400
check "$out/documents.png" >/dev/null

# 3. Back home, then Documents by its card (the first of the dashboard).
control 1
expect_log /tmp/f.log 'ZFILES LOCATION kind=home path=/tmp/fhome items=0'
click 340 310
pointer move 1270 790 sleep 400
check "$out/folder.png" >/dev/null
found=$(guest "grep -c 'LOCATION kind=folder path=/tmp/fhome/Documents ' /tmp/f.log" | tail -1)
[ "${found:-0}" -ge 2 ] && echo "card: ok" || { echo "card: MISSING"; status=1; }

# 4. Back and Forward.
control 1
control 2
found=$(guest "grep -c 'LOCATION kind=folder path=/tmp/fhome/Documents ' /tmp/f.log" | tail -1)
[ "${found:-0}" -ge 3 ] && echo "forward: ok" || { echo "forward: MISSING"; status=1; }

# 5. The close request ends the program.
guest 'kill $(ps -A -o pid,args | grep "[f]iles" | awk "{print \$1}") 2>/dev/null; sleep 1' >/dev/null
errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
guest "$stop_all" >/dev/null
guest 'cat /tmp/f.log' > "$out/f.log"
[ $status = 0 ] && echo "files-p002: PASS" || echo "files-p002: FAIL"
exit $status
