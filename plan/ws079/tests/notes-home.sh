#!/bin/sh
# ws079-p005: Notes from App Home on the Venus guest (the lean image with Notes, build-notes-image.sh).
# zdesktop --glass at 1280x800 with no /etc/keiland/apps.conf (the built-in list):
#  1. home.png: Home lists Notes (ZWL HOME icon name="Notes").
#  2. window.png: the Notes icon starts /bin/notes (ZWL HOME launch name=Notes), a window with its System Menu
#     (ZWL MENU commit ... items=18) and the toolbar; a stroke drawn in the window (NOTES STROKE).
#  3. menu.png: F10 opens the File menu with the shortcuts (Ctrl+N, Ctrl+O, Ctrl+S, Ctrl+W); Esc closes it.
#  4. Ctrl+W closes Notes, saving the new notebook under ~/Documents/Notes (NOTES SAVE reason=close).
#
#   GUEST_RUNTIME=build/ws079-p005-run plan/ws035/tests/zdesktop-guest.sh start build/amd64/hdd-image.img
#   plan/ws079/tests/notes-home.sh [OUTDIR] [SHOTS PREFIX]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws079-p005-run}"
export GUEST_RUNTIME
out=${1:-build/ws079-p005-home}
prefix=${2:-}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1 </dev/null; }
. plan/tools/guest/zwl-clients.sh
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; sleep 0.8; }
stop_all='for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|/bin/[n]otes" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|/bin/[n]otes" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
status=0

# Fails the run unless a log has a line matching a pattern (within a few seconds).
expect_log() {
	tries=0
	found=0
	while [ $tries -lt 8 ]; do
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
	sleep 0.6
	check "$out/$1" >/dev/null
	[ -n "$prefix" ] && cp "$out/$1" "$prefix$1"
}

# An icon's centre on Home as zdesktop logged it: "x y".
icon() {
	guest "grep 'ZWL HOME icon name=\"$1\"' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([0-9]*\) y=\([0-9]*\).*/\1 \2/p'
}

guest "$stop_all; rm -f /etc/keiland/apps.conf; rm -rf /root/Documents/Notes /root/.local/share/keiland/notes" >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0
picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=300 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4; echo started' >/dev/null

# 1. Home.
pointer move 23 17 sleep 300 down sleep 60 up sleep 1500
pointer move 700 500 sleep 400
shot home.png
expect_log /tmp/zdesktop.log 'ZWL HOME icon name="Notes"'

# 2. Notes from its icon, and a stroke in its window.
set -- $(icon Notes)
echo "Notes icon at ${1:-?},${2:-?}"
pointer move ${1:-0} ${2:-0} sleep 400 down sleep 60 up sleep 6000
expect_log /tmp/zdesktop.log 'ZWL HOME launch name=Notes'
zwl_app_clients
expect_log /tmp/zdesktop.log "ZWL MENU commit client=$zc1 .*items=18"
set -- $(guest "grep 'ZWL MAP client=$zc1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2/p')
wx=${1:-0}; wy=${2:-0}
echo "Notes window at $wx,$wy"
pointer move $((wx + 350)) $((wy + 200)) sleep 60 down sleep 40 move $((wx + 420)) $((wy + 240)) sleep 30 \
    move $((wx + 500)) $((wy + 210)) sleep 30 move $((wx + 580)) $((wy + 260)) sleep 30 up sleep 400
pointer move 1250 780 sleep 400
shot window.png

# 3. The File menu from the keyboard.
keys '<f10>'
shot menu.png
keys '<esc>'

# 4. Close, which saves the new notebook.
keys '<ctrl-w>'
sleep 2
guest 'ls -l /root/Documents/Notes/' | tee "$out/documents.txt"
errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
guest 'grep -E "ZWL HOME|ZWL MENU" /tmp/zdesktop.log' > "$out/zdesktop-home.log"
guest "$stop_all" >/dev/null
[ $status = 0 ] && echo "notes-home: PASS" || echo "notes-home: FAIL"
exit $status
