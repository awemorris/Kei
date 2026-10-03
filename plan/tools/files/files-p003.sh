#!/bin/sh
# ws071-p003: selection, the keyboard, the list view, sorting, Ctrl+L and the rubber band of
# files on the Venus guest (the lean image, build-files-image.sh).  zdesktop --glass
# at 1280x800; files at 1000x640 on the sample home (/tmp/fhome), opened on Documents.
#  1. keys.png: Down selects the first item, Shift+Right twice extends to three (SELECT count=3).
#  2. click.png: Ctrl+click adds the fifth item (count=4); a plain click selects one (count=1).
#  3. list.png: the list view (the toolbar's list button) with the header.
#  4. sorted.png: the Size title sorts by size (SORT key=2 reverse=0), again reverses (reverse=1).
#  5. location.png: Ctrl+L, the path typed, Enter goes there (LOCATION .../Projects).
#  6. band.png: a rubber band from the empty ground over both rows selects them (count=2).
#  7. Backspace goes back to Documents; Ctrl+Up goes to the home folder.
#
#   plan/tools/files/files-guest.sh start     (the guest must be up)
#   plan/tools/files/files-p003.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws071-p003}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
. plan/tools/guest/zwl-clients.sh
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
input() { python3 plan/tools/files/qmp-input.py "$GUEST_RUNTIME/qmp.sock" $1; }
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
HOME=/tmp/fhome /bin/files --token=f1 --timeout-s=800 --width=1000 --height=640 /tmp/fhome/Documents > /tmp/f.log 2>&1 </dev/null & sleep 5; echo started' >/dev/null
zwl_app_clients
set -- $(guest "grep 'ZWL MAP client=$zc1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* surface=\([0-9]*\) x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2 \3/p')
surface=${1:-0}; wx=${2:-0}; wy=${3:-0}
echo "files: surface $surface at $wx,$wy"
expect_log /tmp/f.log 'ZFILES LOCATION kind=folder path=/tmp/fhome/Documents items=6 error=0'

# 1. The keyboard.
keys '<down>' '<shift-right>' '<shift-right>'
sleep 1
expect_log /tmp/f.log 'ZFILES SELECT count=3 cursor=2 '
shot keys.png

# 2. Ctrl+click adds the fifth cell, a plain click selects the second alone.
input "move $((wx + 779)) $((wy + 120)) sleep 300 hold ctrl click sleep 100 free ctrl sleep 600"
expect_log /tmp/f.log 'ZFILES SELECT count=4 cursor=4 '
click 443 120
expect_log /tmp/f.log 'ZFILES SELECT count=1 cursor=1 '
shot click.png

# 3. The list view.
control 7
shot list.png

# 4. Sorting by size, and reversed.
click 760 85
expect_log /tmp/f.log 'ZFILES SORT key=2 reverse=0'
click 760 85
expect_log /tmp/f.log 'ZFILES SORT key=2 reverse=1'
shot sorted.png

# 5. Ctrl+L and a path.
keys '<ctrl-l>'
sleep 0.5
keys '<ctrl-a>' '/tmp/fhome/Projects'
sleep 0.5
shot location.png
keys '<ret>'
sleep 1
expect_log /tmp/f.log 'ZFILES LOCATION kind=folder path=/tmp/fhome/Projects items=2 error=0'

# 6. A rubber band over both rows, from the empty ground below them.
pointer move $((wx + 600)) $((wy + 348)) sleep 300 down sleep 100 move $((wx + 500)) $((wy + 248)) sleep 100 move $((wx + 300)) $((wy + 113)) sleep 300
check "$out/band.png" >/dev/null
pointer up sleep 500
expect_log /tmp/f.log 'ZFILES SELECT count=2 '

# 7. Back, and up to the home folder.
keys '<backspace>'
sleep 1
found=$(guest "grep -c 'LOCATION kind=folder path=/tmp/fhome/Documents ' /tmp/f.log" | tail -1)
[ "${found:-0}" -ge 2 ] && echo "back: ok" || { echo "back: MISSING"; status=1; }
keys '<ctrl-up>'
sleep 1
expect_log /tmp/f.log 'ZFILES LOCATION kind=folder path=/tmp/fhome items=7'

errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
guest "$stop_all" >/dev/null
guest 'cat /tmp/f.log' > "$out/f.log"
[ $status = 0 ] && echo "files-p003: PASS" || echo "files-p003: FAIL"
exit $status
