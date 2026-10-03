#!/bin/sh
# ws071-p007: the preview pane, Quick Look and the thumbnails of files on the Venus guest
# (the lean image, build-files-image.sh).  zdesktop --glass at 1280x800; files at 1000x640
# on the sample home (/tmp/fhome), opened on Pictures.
#  1. The thumbnails of Sunset.ppm (256x160), Ramp.pgm (PGM) and Tiny.ppm are made (THUMB error=0).
#  2. Ctrl+Alt+P shows the preview pane; a click on Sunset.ppm previews it (PEEK): preview.png.
#  3. Space opens Quick Look on it (LOOK open, PEEK picture 256x160): look.png.  Right goes to
#     Tiny.ppm (LOOK path=...Tiny.ppm); Esc closes it (LOOK close).
#  4. Documents in the list view: Meeting notes.txt previewed as text (PEEK type=text/plain lines=4):
#     text.png.  Space: look-text.png; a click on the dimmed window closes it (a second LOOK close).
#
#   plan/tools/files/files-guest.sh start     (the guest must be up)
#   plan/tools/files/files-p007.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws071-p007}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
. plan/tools/guest/zwl-clients.sh
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; sleep 0.7; }
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
shot() {
	pointer move 1270 790 sleep 400
	check "$out/$1" >/dev/null
}

guest "$stop_all" >/dev/null
guest 'rm -f /tmp/wayland-0 /tmp/files.clipboard; rm -rf /tmp/fhome; sh /usr/share/files-tests/make-home.sh /tmp/fhome >/dev/null'
guest 'export XDG_RUNTIME_DIR=/tmp
picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=900 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4
HOME=/tmp/fhome /bin/files --token=f1 --timeout-s=800 --width=1000 --height=640 /tmp/fhome/Pictures > /tmp/f.log 2>&1 </dev/null & sleep 6; echo started' >/dev/null
zwl_app_clients
set -- $(guest "grep 'ZWL MAP client=$zc1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* surface=\([0-9]*\) x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2 \3/p')
surface=${1:-0}; wx=${2:-0}; wy=${3:-0}
echo "files: surface $surface at $wx,$wy"

# 1. The thumbnails.
expect_log /tmp/f.log 'ZFILES LOCATION kind=folder path=/tmp/fhome/Pictures items=4 error=0'
expect_log /tmp/f.log 'ZFILES THUMB path=/tmp/fhome/Pictures/Sunset.ppm error=0 width=256 height=160'
expect_log /tmp/f.log 'ZFILES THUMB path=/tmp/fhome/Pictures/Ramp.pgm error=0 width=64 height=48'
expect_log /tmp/f.log 'ZFILES THUMB path=/tmp/fhome/Pictures/Tiny.ppm error=0 width=2 height=2'

# 2. The preview pane, and Sunset.ppm in it (the third item).
keys '<ctrl-alt-p>'
click 530 136
expect_log /tmp/f.log 'ZFILES PEEK path=/tmp/fhome/Pictures/Sunset.ppm type=image/x-portable-pixmap lines=0'
shot preview.png

# 3. Quick Look: open, the next item, closed.
keys '<spc>'
expect_log /tmp/f.log 'ZFILES LOOK open path=/tmp/fhome/Pictures/Sunset.ppm'
expect_log /tmp/f.log 'ZFILES PEEK picture path=/tmp/fhome/Pictures/Sunset.ppm error=0 width=256 height=160'
shot look.png
keys '<right>'
expect_log /tmp/f.log 'ZFILES LOOK path=/tmp/fhome/Pictures/Tiny.ppm'
keys '<esc>'
expect_log /tmp/f.log 'ZFILES LOOK close'

# 4. Text: Documents in the list view, Meeting notes.txt (the third row), then Quick Look.
click 100 125
expect_log /tmp/f.log 'ZFILES LOCATION kind=folder path=/tmp/fhome/Documents items=6 error=0'
keys '<ctrl-2>'
click 400 170
expect_log /tmp/f.log 'ZFILES PEEK path=/tmp/fhome/Documents/Meeting notes.txt type=text/plain lines=4'
shot text.png
keys '<spc>'
expect_log /tmp/f.log 'ZFILES LOOK open path=/tmp/fhome/Documents/Meeting notes.txt'
shot look-text.png
click 30 568
closed=$(guest "grep -c 'ZFILES LOOK close' /tmp/f.log" | tail -1)
[ "${closed:-0}" -ge 2 ] && echo "look: closed by a click on the window: ok" || { echo "look: click close MISSING"; status=1; }

errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
guest "$stop_all" >/dev/null
guest 'cat /tmp/f.log' > "$out/f.log"
[ $status = 0 ] && echo "files-p007: PASS" || echo "files-p007: FAIL"
exit $status
