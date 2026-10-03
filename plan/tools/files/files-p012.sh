#!/bin/sh
# ws071-p012: opening files and the information card of files on the Venus guest (the lean
# image, build-files-image.sh).  zdesktop --glass at 1280x800; files at 1000x640 on the
# sample home (/tmp/fhome), opened on Documents in the list view.  The sample home's
# ~/.config/keiland/open-with sends PDFs and plain text to "Record" (echo %f >> ~/.opened).
#  1. Report.pdf double-clicked: the user's list opens it (OPEN app=Record), ~/.opened has its path.
#  2. Meeting notes.txt, Ctrl+I: the card (INFO ... openers=), info.png; Compute: the SHA-256 of the
#     file (CHECKSUM done), info-sum.png; Esc closes it (INFO close).
#  3. Sunset.ppm in Pictures double-clicked: Quick Look (OPEN app=Quick Look, LOOK open); Esc.
#  4. Budget.csv (text/csv, not in the user's list) double-clicked: the built-in default for text, Text Editor
#     (since WS128 made it the text default; ws127-p002 updated this check from "Terminal (less)"), starts
#     (LAUNCH, a second client's window in zdesktop): terminal.png.
#
#   plan/tools/files/files-guest.sh start     (the guest must be up)
#   plan/tools/files/files-p012.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws071-p012}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
. plan/tools/guest/zwl-clients.sh
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; sleep 0.7; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[f]iles|[t]erminal|[l]ess " | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[f]iles|[t]erminal" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
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
shot() {
	pointer move 1270 790 sleep 400
	check "$out/$1" >/dev/null
}

guest "$stop_all" >/dev/null
guest 'rm -f /tmp/wayland-0 /tmp/files.clipboard; rm -rf /tmp/fhome; sh /usr/share/files-tests/make-home.sh /tmp/fhome >/dev/null'
guest 'export XDG_RUNTIME_DIR=/tmp
picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=900 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4
HOME=/tmp/fhome /bin/files --token=f1 --timeout-s=800 --width=1000 --height=640 /tmp/fhome/Documents > /tmp/f.log 2>&1 </dev/null & sleep 6; echo started' >/dev/null
zwl_app_clients
set -- $(guest "grep 'ZWL MAP client=$zc1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* surface=\([0-9]*\) x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2 \3/p')
surface=${1:-0}; wx=${2:-0}; wy=${3:-0}
echo "files: surface $surface at $wx,$wy"
expect_log /tmp/f.log 'ZFILES LOCATION kind=folder path=/tmp/fhome/Documents items=6 error=0'
keys '<ctrl-2>'

# 1. Report.pdf (the fifth row) through the user's list.
double 400 226 1500
expect_log /tmp/f.log 'ZFILES OPEN path=/tmp/fhome/Documents/Report.pdf app=Record error=0'
opened=$(guest 'cat /tmp/fhome/.opened' | tail -1)
[ "$opened" = /tmp/fhome/Documents/Report.pdf ] && echo "record: ok" || { echo "record: MISSING ($opened)"; status=1; }

# 2. The information of Meeting notes.txt (the third row), its checksum, closed.
click 400 170
keys '<ctrl-i>'
expect_log /tmp/f.log 'ZFILES INFO path=/tmp/fhome/Documents/Meeting notes.txt mode=0644 '
shot info.png
click 409 453 1500
expect_log /tmp/f.log 'ZFILES CHECKSUM done path=/tmp/fhome/Documents/Meeting notes.txt sha256=1369d0d55862634309fac10a53383d6b29b3914ac1784a2c91b8f134761c3259'
shot info-sum.png
keys '<esc>'
expect_log /tmp/f.log 'ZFILES INFO close'

# 3. Sunset.ppm (the third row of Pictures) opens in Quick Look.
click 100 185
expect_log /tmp/f.log 'ZFILES LOCATION kind=folder path=/tmp/fhome/Pictures items=4 error=0'
double 400 170
expect_log /tmp/f.log 'ZFILES OPEN path=/tmp/fhome/Pictures/Sunset.ppm app=Quick Look'
expect_log /tmp/f.log 'ZFILES LOOK open path=/tmp/fhome/Pictures/Sunset.ppm'
keys '<esc>'

# 4. Budget.csv (the second row of Documents) in a terminal.
click 100 125
double 400 142 4000
expect_log /tmp/f.log 'ZFILES LAUNCH name=Text Editor command=/bin/textedit ./tmp/fhome/Documents/Budget.csv.$'
expect_log /tmp/zdesktop.log "ZWL MAP client=$zc2 "
running=$(guest "ps -A -o args | grep -c '[t]extedit'" | tail -1)
[ "${running:-0}" -ge 1 ] && echo "textedit: running" || { echo "textedit: MISSING"; status=1; }
shot terminal.png

errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
guest "$stop_all" >/dev/null
guest 'cat /tmp/f.log' > "$out/f.log"
[ $status = 0 ] && echo "files-p012: PASS" || echo "files-p012: FAIL"
exit $status
