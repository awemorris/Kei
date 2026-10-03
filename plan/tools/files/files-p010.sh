#!/bin/sh
# ws071-p010: PNG thumbnails (libpng-compat, libz-compat) and dragging items within files, on the
# Venus guest (the lean image, build-files-image.sh).  zdesktop --glass at 1280x800; files at
# 1000x640 on the sample home (/tmp/fhome).
#  1. Opened on Desktop: the thumbnails of Logo.png (64x64, a palette with a transparent colour) and
#     Screenshot.png (120x80 RGB) are made (THUMB error=0); thumbs.png.
#  2. Logo.png dragged onto Pictures in the sidebar: the drag starts, Pictures is the target (drag.png taken
#     with the button still held), the release moves the file there (TASK move done, the file in Pictures).
#  3. Opened on Projects/zedBSD: README.md dragged onto the folder docs moves it into docs; Makefile dragged
#     toward src and Esc pressed before the release: DRAG cancel, nothing moves.
#  4. The favorite Pictures dragged onto Desktop in the sidebar goes first in the list (drag-place.png,
#     reordered.png).
#
#   plan/tools/files/files-guest.sh start     (the guest must be up)
#   plan/tools/files/files-p010.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws071-p010}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; sleep 0.7; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[f]iles" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[f]iles" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
status=0
surface=0
wx=0
wy=0

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

# Fails the run unless a shell test holds in the guest.
expect_guest() {
	if [ "$(guest "$1 && echo yes" | tail -1)" = yes ]; then
		echo "guest: $2 ok"
	else
		echo "guest: $2 MISSING"
		status=1
	fi
}

# Starts zdesktop and files on a folder, and finds the window.
start() {
	guest "$stop_all" >/dev/null
	guest "export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0
/bin/wayland --timeout=900 --width=1280 --height=800 --glass --wallpaper=/usr/share/keiland/wallpaper.ppm > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4
HOME=/tmp/fhome /bin/files --token=f1 --timeout-s=800 --width=1000 --height=640 $1 > /tmp/f.log 2>&1 </dev/null & sleep 6; echo started" >/dev/null
	set -- $(guest "grep 'ZWL MAP client=1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* surface=\([0-9]*\) x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2 \3/p')
	surface=${1:-0}; wx=${2:-0}; wy=${3:-0}
	echo "files: surface $surface at $wx,$wy"
}

# Presses a point of the window's body (x, y from its top left) and moves, with the button held, to another
# in a few steps; the button stays down.
drag() {
	pointer move $((wx + $1 - 2)) $((wy + $2)) sleep 150 move $((wx + $1)) $((wy + $2)) sleep 300 down sleep 120 \
	    move $((wx + $1 - 12)) $((wy + $2 + 4)) sleep 80 \
	    move $((wx + ($1 + $3) / 2)) $((wy + ($2 + $4) / 2)) sleep 80 \
	    move $((wx + $3 + 3)) $((wy + $4 + 1)) sleep 80 \
	    move $((wx + $3)) $((wy + $4)) sleep 600
}
release() {
	pointer up sleep 1200
}
shot() {
	pointer move 1270 790 sleep 400
	check "$out/$1" >/dev/null
}

guest 'rm -f /tmp/files.clipboard; rm -rf /tmp/fhome; sh /usr/share/files-tests/make-home.sh /tmp/fhome >/dev/null'

# 1. The PNG thumbnails on Desktop.
start /tmp/fhome/Desktop
expect_log /tmp/f.log 'ZFILES LOCATION kind=folder path=/tmp/fhome/Desktop items=2 error=0'
expect_log /tmp/f.log 'ZFILES THUMB path=/tmp/fhome/Desktop/Logo.png error=0 width=64 height=64'
expect_log /tmp/f.log 'ZFILES THUMB path=/tmp/fhome/Desktop/Screenshot.png error=0 width=120 height=80'
shot thumbs.png

# 2. Logo.png (the first item) onto Pictures in the sidebar; the picture while the button is held.
drag 330 110 100 173
expect_log /tmp/f.log 'ZFILES DRAG start items=1$'
expect_log /tmp/f.log 'ZFILES DRAG target kind=folder path=/tmp/fhome/Pictures$'
check "$out/drag.png" >/dev/null
release
expect_log /tmp/f.log 'ZFILES DRAG drop operation=move items=1 destination=/tmp/fhome/Pictures$'
expect_log /tmp/f.log 'ZFILES TASK done id=[0-9]+ kind=move state=done files=1 '
expect_guest '[ -f /tmp/fhome/Pictures/Logo.png ] && [ ! -e /tmp/fhome/Desktop/Logo.png ]' 'Logo.png moved to Pictures'

# 3. Projects/zedBSD: README.md (the fourth item) onto docs (the first); Makefile toward src, then Esc.
start /tmp/fhome/Projects/zedBSD
expect_log /tmp/f.log 'ZFILES LOCATION kind=folder path=/tmp/fhome/Projects/zedBSD items=4 error=0'
drag 666 110 330 110
expect_log /tmp/f.log 'ZFILES DRAG target kind=folder path=/tmp/fhome/Projects/zedBSD/docs$'
check "$out/drag-folder.png" >/dev/null
release
expect_log /tmp/f.log 'ZFILES DRAG drop operation=move items=1 destination=/tmp/fhome/Projects/zedBSD/docs$'
expect_guest '[ -f /tmp/fhome/Projects/zedBSD/docs/README.md ] && [ ! -e /tmp/fhome/Projects/zedBSD/README.md ]' 'README.md moved to docs'
shot moved.png
# The drag's hold on docs (the screenshot with the button down) springs docs open (ws127-p002, F-039): the
# window goes back to Projects/zedBSD for the next drag.
start /tmp/fhome/Projects/zedBSD
expect_log /tmp/f.log 'ZFILES LOCATION kind=folder path=/tmp/fhome/Projects/zedBSD items=3 error=0'
drag 554 110 442 110
keys '<esc>'
release
expect_log /tmp/f.log 'ZFILES DRAG cancel$'
cancelled=$(guest "grep -c 'DRAG drop' /tmp/f.log" | tail -1)
# The window was started again before this drag, so its log has no drop at all.
[ "${cancelled:-0}" = 0 ] && echo "cancel: no drop ok" || { echo "cancel: drops $cancelled MISSING"; status=1; }
expect_guest '[ -f /tmp/fhome/Projects/zedBSD/Makefile ] && [ ! -e /tmp/fhome/Projects/zedBSD/src/Makefile ]' 'Makefile stayed'

# 4. Pictures (the fifth place) dragged onto Desktop (the second) in the sidebar: first of the Favorites.
drag 100 173 100 83
expect_log /tmp/f.log 'ZFILES DRAG target kind=place place=1$'
check "$out/drag-place.png" >/dev/null
release
expect_log /tmp/f.log 'ZFILES DRAG drop operation=reorder place=4 to=1 error=0$'
expect_guest 'head -1 /tmp/fhome/.config/files/sidebar | grep -qx /tmp/fhome/Pictures' 'Pictures first in the sidebar list'
shot reordered.png

errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
guest "$stop_all" >/dev/null
guest 'cat /tmp/f.log' > "$out/f.log"
[ $status = 0 ] && echo "files-p010: PASS" || echo "files-p010: FAIL"
exit $status
