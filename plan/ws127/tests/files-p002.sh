#!/bin/sh
# ws127-p002: the Files improvements on the Venus guest (plan/tools/files/build-files-image.sh; start the guest first,
# e.g. plan/ws127/tests/files-guest-p1.sh start IMAGE).  zdesktop --glass at 1280x800, files at 1000x640 on the
# sample home (/tmp/fhome).
#  1. Move To: a right click on Budget.csv in Documents has the Move To submenu; its Downloads row moves it there
#     (CONTEXT move-to, TASK move done, the file in Downloads): moveto.png.
#  2. Open in New Window: a right click on the folder zedBSD in Projects; the row starts a second window on it
#     (CONTEXT new-window, ZWL MAP client=2, two files processes): newwindow.png.
#  3. The overlay scroll bar on a folder of 300 files: after wheel notches it shows thin (scroll-thin.png); the
#     pointer at the right edge makes it thick (scroll-thick.png); a press on the thumb and a drag down scroll the
#     content (SCROLLBAR press dragging=1, SCROLLBAR release with a larger scroll); it fades (scroll-faded.png).
#  4. A PDF's thumbnail (a real one-page PDF, plan/ws127/tests/make-pdf.py): THUMB error=0 cached=0, and from the
#     disk cache in a second window run (cached=1): pdf.png.
#  5. BUG-141: a click on an item, the pointer over another one (hover-on.png: it is lit), then to the desktop's
#     corner; hover.png shows that item no longer lit.
#   sh plan/ws127/tests/files-p002.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws127-p002}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; sleep 0.8; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[f]iles|[t]erminal" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[f]iles|[t]erminal" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
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

# The middle (y) of the latest popup row of an item, and the latest popup's left edge (depth 1 or 2).
row_y() {
	guest "grep 'MENU row item=$1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* y=\([0-9]*\) height=\([0-9]*\).*/\1 \2/p' | { read y h; echo $(( ${y:-0} + ${h:-0} / 2 )); }
}
popup_x() {
	guest "grep 'MENU open .* depth=${1:-0} ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([0-9]*\) y=.*/\1/p'
}

# Clicks a screen point; rclick right-clicks a point of the window's body (from its top left).
click() {
	pointer move $(($1 - 2)) "$2" sleep 150 move "$1" "$2" sleep 300 down sleep 60 up sleep "${3:-700}"
}
rclick() {
	pointer move $((wx + $1 - 2)) $((wy + $2)) sleep 150 move $((wx + $1)) $((wy + $2)) sleep 300 right-down sleep 60 right-up sleep 900
}
shot() {
	pointer move ${2:-1270} ${3:-790} sleep 400
	check "$out/$1" >/dev/null
}


# Starts zdesktop and files on a folder, and finds the window (client 1).
start() {
	guest "$stop_all" >/dev/null
	guest "export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0
/bin/wayland --timeout=900 --width=1280 --height=800 --glass --wallpaper=/usr/share/keiland/wallpaper.ppm > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4
HOME=/tmp/fhome /bin/files --token=f1 --timeout-s=800 --width=1000 --height=640 $1 > /tmp/f.log 2>&1 </dev/null & sleep 6; echo started" >/dev/null
	set -- $(guest "grep 'ZWL MAP client=1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* surface=\([0-9]*\) x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2 \3/p')
	surface=${1:-0}; wx=${2:-0}; wy=${3:-0}
	echo "files: surface $surface at $wx,$wy"
}
expect_guest() {
	if [ "$(guest "$1 && echo yes" | tail -1)" = yes ]; then echo "guest: $2 ok"; else echo "guest: $2 MISSING"; status=1; fi
}

guest "$stop_all" >/dev/null
guest 'rm -f /tmp/files.clipboard; rm -rf /tmp/fhome; sh /usr/share/files-tests/make-home.sh /tmp/fhome >/dev/null; mkdir -p /tmp/fhome/Big; i=0; while [ $i -lt 300 ]; do : > /tmp/fhome/Big/item-$i.txt; i=$((i+1)); done; echo made' >/dev/null
python3 plan/ws127/tests/make-pdf.py "$out/Red.pdf"
timeout 60 python3 plan/ws005/phase024/guest-ports.py put "$out/Red.pdf" /tmp/fhome/Documents/Red.pdf >/dev/null 2>&1

# 1. Move To.
start /tmp/fhome/Documents
expect_log /tmp/f.log 'ZFILES LOCATION kind=folder path=/tmp/fhome/Documents items=7 error=0'
rclick 446 140
expect_log /tmp/zdesktop.log 'ZWL MENU row item=105 depth=1 '
click $(( $(popup_x 1) + 60 )) "$(row_y 105)" 900
expect_log /tmp/zdesktop.log 'ZWL MENU row item=1503 depth=2 '
shot moveto.png $(( $(popup_x 2) + 60 )) "$(row_y 1503)"
click $(( $(popup_x 2) + 60 )) "$(row_y 1503)" 1500
expect_log /tmp/f.log 'ZFILES CONTEXT move-to place=3 path=/tmp/fhome/Downloads count=1'
expect_log /tmp/f.log 'ZFILES TASK done id=[0-9]+ kind=move state=done files=1 '
expect_guest '[ -f /tmp/fhome/Downloads/Budget.csv ] && [ ! -e /tmp/fhome/Documents/Budget.csv ]' 'Budget.csv moved to Downloads'

# 4. The PDF's thumbnail (Documents shown), then again from the cache.
expect_log /tmp/f.log 'ZFILES THUMB path=/tmp/fhome/Documents/Red.pdf error=0 width=[0-9]+ height=256 cached=0'
shot pdf.png
start /tmp/fhome/Documents
expect_log /tmp/f.log 'ZFILES THUMB path=/tmp/fhome/Documents/Red.pdf error=0 width=[0-9]+ height=256 cached=1'

# 5. BUG-141: a click on an item, then the pointer to the desktop's corner (shot moves it there).
click $((wx + 446)) $((wy + 140)) 900
pointer move $((wx + 670)) $((wy + 140)) sleep 300 move $((wx + 680)) $((wy + 145)) sleep 500
check "$out/hover-on.png" >/dev/null
shot hover.png

# 2. Open in New Window.
start /tmp/fhome/Projects
expect_log /tmp/f.log 'ZFILES LOCATION kind=folder path=/tmp/fhome/Projects items=2 error=0'
rclick 335 130
expect_log /tmp/zdesktop.log 'ZWL MENU row item=1058 depth=1 '
click $(( $(popup_x 1) + 60 )) "$(row_y 1058)" 6000
expect_log /tmp/f.log 'ZFILES CONTEXT new-window path=/tmp/fhome/Projects/[a-zA-Z]+$'
expect_log /tmp/zdesktop.log 'ZWL MAP client=2 '
running=$(guest "ps -A -o args | grep -c '[/]bin/files'" | tail -1)
[ "${running:-0}" -ge 2 ] && echo "new window: two files ok" || { echo "new window: two files MISSING"; status=1; }
shot newwindow.png

# 3. The overlay scroll bar.
start /tmp/fhome/Big
expect_log /tmp/f.log 'ZFILES LOCATION kind=folder path=/tmp/fhome/Big items=300 error=0'
pointer move $((wx + 600)) $((wy + 300)) sleep 200 wheel-down sleep 60 wheel-down sleep 60 wheel-down sleep 300
check "$out/scroll-thin.png" >/dev/null
pointer move $((wx + 993)) $((wy + 120)) sleep 400
check "$out/scroll-thick.png" >/dev/null
pointer move $((wx + 993)) $((wy + 40)) sleep 200 down sleep 150 move $((wx + 993)) $((wy + 200)) sleep 150 move $((wx + 993)) $((wy + 400)) sleep 200 up sleep 400
expect_log /tmp/f.log 'ZFILES SCROLLBAR press y=[0-9]+ scroll=[0-9]+ dragging=1'
expect_log /tmp/f.log 'ZFILES SCROLLBAR release scroll=[1-9][0-9]+'
pointer move $((wx + 600)) $((wy + 300)) sleep 2500
check "$out/scroll-faded.png" >/dev/null

errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
guest "$stop_all" >/dev/null
[ $status = 0 ] && echo "files-p002: PASS" || echo "files-p002: FAIL"
exit $status
