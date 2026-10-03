#!/bin/sh
# BUG-142 (ws099-p029): a key pressed before a click is applied before it even when both wait while zdesktop is
# busy.  On the lean image (plan/tools/files/build-files-image.sh), zdesktop --glass 1280x800 and files on Documents:
# Report.pdf is selected; zdesktop is stopped (SIGSTOP, standing in for a busy compositor); Ctrl+C is pressed and
# then Downloads in the sidebar is clicked; zdesktop goes on (SIGCONT).  Files must copy before it moves:
# ZFILES CLIPBOARD mode=1 items=1 comes before ZFILES LOCATION ... Downloads in its log, and Ctrl+V in Downloads
# pastes Report.pdf.  Before the fix zdesktop read its devices one after another, so a click waiting on the pointer
# device could be applied before a key waiting on the keyboard.  Judged by the logs and the guest's files only.
#
#   plan/tools/files/files-guest.sh start IMAGE     (the guest must be up)
#   plan/ws099/tests/bug142-order.sh [OUTDIR]
# Prints "bug142: PASS" or "bug142: FAIL".
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws127-bug142}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
. plan/tools/guest/zwl-clients.sh
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; sleep 0.8; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[f]iles" | awk "{print \$1}"); do kill -CONT $p; kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[f]iles" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
status=0

# Fails the run unless a log has a line matching a pattern (within a few seconds).
expect_log() {
	tries=0
	found=0
	while [ $tries -lt 5 ]; do
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

# Clicks a point of the window's body (x, y from its top left) and waits.
click() {
	pointer move $((wx + $1 - 2)) $((wy + $2)) sleep 150 move $((wx + $1)) $((wy + $2)) sleep 300 down sleep 60 up sleep "${3:-700}"
}

guest "$stop_all" >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0 /tmp/files.clipboard; rm -rf /tmp/fhome; sh /usr/share/files-tests/make-home.sh /tmp/fhome >/dev/null
picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=600 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4
HOME=/tmp/fhome /bin/files --token=f1 --timeout-s=500 --width=1000 --height=640 /tmp/fhome/Documents > /tmp/f.log 2>&1 </dev/null & sleep 5; echo started' >/dev/null
zwl_app_clients
set -- $(guest "grep 'ZWL MAP client=$zc1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* surface=\([0-9]*\) x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2 \3/p')
surface=${1:-0}; wx=${2:-0}; wy=${3:-0}
echo "files: surface $surface at $wx,$wy"
expect_log /tmp/f.log 'ZFILES LOCATION kind=folder path=/tmp/fhome/Documents items=6 error=0'
expect_log /tmp/f.log 'ZFILES MENU ready items='

# Report.pdf selected, then Ctrl+C and the click on Downloads while zdesktop is stopped.
click 779 120
expect_log /tmp/f.log 'ZFILES MENU state selection=1 '
sleep 1
compositor=$(guest "ps -A -o pid,args | grep -E '[/]bin/wayland( |\$)' | awk '{print \$1}' | head -1" | tail -1)
echo "zdesktop: pid $compositor"
guest "kill -STOP $compositor" >/dev/null
sleep 1
keys '<ctrl-c>'
click 100 155 300
sleep 1
guest "kill -CONT $compositor" >/dev/null
sleep 3

# The copy came before the move, and Ctrl+V pastes the file in Downloads.
expect_log /tmp/f.log 'ZFILES CLIPBOARD mode=1 items=1'
expect_log /tmp/f.log 'ZFILES LOCATION kind=folder path=/tmp/fhome/Downloads '
order=$(guest "grep -nE 'ZFILES (CLIPBOARD mode=1 items=1|LOCATION kind=folder path=/tmp/fhome/Downloads )' /tmp/f.log | head -1" | tail -1)
case $order in
*CLIPBOARD*) echo "order: copy before the move ok" ;;
*) echo "order: the move came first ($order) FAIL"; status=1 ;;
esac
keys '<ctrl-v>'
sleep 2
if guest '[ -f /tmp/fhome/Downloads/Report.pdf ] && echo YES' | grep -q YES; then
	echo "guest: pasted ok"
else
	echo "guest: pasted FAILED"
	status=1
fi
guest 'grep -E "ZFILES (CLIPBOARD|LOCATION|ACTION|PASTE|TASK)" /tmp/f.log' > "$out/f-order.log"
guest "$stop_all" >/dev/null
[ $status -eq 0 ] && echo "bug142: PASS" || echo "bug142: FAIL"
exit $status
