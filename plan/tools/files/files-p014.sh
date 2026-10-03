#!/bin/sh
# ws071-p014: files' navigation in the window's titlebar (WS070's controls) on the Venus guest
# (the lean image, build-files-image.sh).  zdesktop --glass at 1280x800; files (f1) at
# 1000x640 on the sample home (/tmp/fhome), opened on Documents.  The controls' places come from
# zdesktop's log (ZWL TITLEBAR control ...):
#  1. floating.png: back, forward, home, the path (Home > Documents), the search field, Icons/List,
#     Preview and "..." in the floating titlebar (TITLEBAR ready; no toolbar in the window).
#  2. The path's first part goes to the home folder; Back returns to Documents.
#  3. preview.png: Preview shows the preview pane (id 8), clicked again it goes.
#  4. search.png: Ctrl+F gives the search field the keyboard (focus id=5), "report" finds Report.pdf;
#     Esc ends the search and goes back to Documents.
#  5. location.png: Ctrl+L makes the path a field (focus id=4 edit=1); /tmp/fhome/Pictures and Enter
#     go there.
#  6. docked.png: the window maximized (a double click on its title): the controls in the system
#     bar (where=docked); Home there goes to the dashboard; a double click brings the window back.
#  7. narrow.png: a second window 420 pixels wide: its secondary controls give way (shown=0) to "...";
#     overflow.png: "..." holds them and the menus (File's row); its List row sets the list view.
#
#   plan/tools/files/files-guest.sh start     (the guest must be up)
#   plan/tools/files/files-p014.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws071-p014}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
. plan/tools/guest/zwl-clients.sh
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; sleep 0.7; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[f]iles" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[f]iles" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
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

# Fails the run unless a log has at least a number of lines matching a pattern.
expect_count() {
	found=$(guest "grep -cE '$2' $1" | tail -1)
	if [ "${found:-0}" -ge "$3" ] 2>/dev/null; then
		echo "count: $2 >= $3 ok"
	else
		echo "count: $2 >= $3 MISSING (${found:-0})"
		status=1
	fi
}

# A control's place (client, where, ID) as zdesktop last logged it: "x y width height".
place() {
	guest "grep 'ZWL TITLEBAR control client=$(zwl_app_client $1) .* where=$2 id=$3 ' /tmp/zdesktop.log | tail -1" |
	    sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\) width=\([0-9]*\) height=\([0-9]*\).*/\1 \2 \3 \4/p'
}

# The middle (y) of the latest popup row of an item, and the latest popup's left edge.
row_y() {
	guest "grep 'MENU row item=$1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* y=\([0-9]*\) height=\([0-9]*\).*/\1 \2/p' | { read y h; echo $(( ${y:-0} + ${h:-0} / 2 )); }
}
popup_x() {
	guest "grep 'MENU open ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([0-9]*\) y=.*/\1/p'
}

# Clicks and double-clicks a screen point; control clicks a control's centre (client, where, ID).
click() {
	pointer move $(($1 - 2)) "$2" sleep 150 move "$1" "$2" sleep 300 down sleep 60 up sleep "${3:-900}"
}
double() {
	pointer move "$1" "$2" sleep 300 down sleep 50 up sleep 80 down sleep 50 up sleep "${3:-1500}"
}
control() {
	set -- $(place "$1" "$2" "$3")
	click $((${1:-0} + ${3:-0} / 2)) $((${2:-0} + ${4:-0} / 2))
}
shot() {
	pointer move 1270 790 sleep 500
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

# 1. The controls in the floating titlebar.
expect_log /tmp/f.log 'ZFILES TITLEBAR ready controls=8'
expect_log /tmp/f.log 'ZFILES TITLEBAR state back=0 forward=0 parts=2 last=Documents '
expect_log /tmp/zdesktop.log "ZWL TITLEBAR control client=$zc1 surface=$surface where=floating id=4 .* shown=1"
expect_log /tmp/zdesktop.log "ZWL TITLEBAR control client=$zc1 surface=$surface where=floating id=8 .* shown=1"
expect_log /tmp/zdesktop.log "ZWL TITLEBAR control client=$zc1 surface=$surface where=floating id=0 "
shot floating.png

# 2. The path's first part (Home, or the "..." before Documents), then Back.
set -- $(place 1 floating 4)
click $((${1:-0} + 10)) $((${2:-0} + ${4:-0} / 2))
expect_log /tmp/f.log 'ZFILES TITLEBAR kind=0 id=4 detail=0 '
expect_log /tmp/f.log 'ZFILES LOCATION kind=folder path=/tmp/fhome items=7 error=0'
control 1 floating 1
expect_count /tmp/f.log 'ZFILES LOCATION kind=folder path=/tmp/fhome/Documents ' 2

# 3. The preview pane on and off.
control 1 floating 8
expect_log /tmp/f.log 'ZFILES TITLEBAR state .* preview=1 '
shot preview.png
control 1 floating 8
expect_log /tmp/f.log 'ZFILES TITLEBAR kind=0 id=8 detail=0 '

# 4. Ctrl+F, a search, Esc.
keys '<ctrl-f>'
expect_log /tmp/zdesktop.log "ZWL TITLEBAR focus client=$zc1 surface=$surface id=5 edit=0"
keys 'report'
sleep 2
expect_log /tmp/f.log 'ZFILES SEARCH done query=report results=[1-9]'
expect_log /tmp/f.log 'ZFILES TITLEBAR state .* query=report '
shot search.png
keys '<esc>'
expect_log /tmp/f.log 'ZFILES TITLEBAR kind=2 id=5 detail=1 '
expect_count /tmp/f.log 'ZFILES LOCATION kind=folder path=/tmp/fhome/Documents ' 3

# 5. Ctrl+L and a path.
keys '<ctrl-l>'
expect_log /tmp/zdesktop.log "ZWL TITLEBAR focus client=$zc1 surface=$surface id=4 edit=1"
keys '<ctrl-a>' '/tmp/fhome/Pictures'
shot location.png
keys '<ret>'
expect_log /tmp/f.log 'ZFILES TITLEBAR kind=2 id=4 detail=0 text=/tmp/fhome/Pictures'
expect_log /tmp/f.log 'ZFILES LOCATION kind=folder path=/tmp/fhome/Pictures items=4 error=0'

# 6. Maximized: the controls in the system bar; Home there; back to floating.
set -- $(place 1 floating 1)
double $((wx + 60)) $((${2:-0} + ${4:-0} / 2))
expect_log /tmp/zdesktop.log "GLASS dock surface=$surface"
expect_log /tmp/zdesktop.log "ZWL TITLEBAR control client=$zc1 surface=$surface where=docked id=6 .* shown=1"
shot docked.png
control 1 docked 3
expect_log /tmp/f.log 'ZFILES LOCATION kind=home path=/tmp/fhome '
# The docked bar's name "Files" (120,17) undocks on a double click; (190,17) is the Forward control now (ws127-p001 T1).
double 120 17
expect_log /tmp/zdesktop.log "GLASS undock surface=$surface"

# 7. A narrow window: its controls give way to "...", which holds them and the menus.
guest 'export XDG_RUNTIME_DIR=/tmp; HOME=/tmp/fhome /bin/files --token=f2 --timeout-s=400 --width=420 --height=400 /tmp/fhome/Documents > /tmp/f2.log 2>&1 </dev/null & sleep 6; echo started' >/dev/null
expect_log /tmp/f2.log 'ZFILES TITLEBAR ready controls=8'
expect_log /tmp/zdesktop.log "ZWL TITLEBAR control client=$zc2 .* where=floating id=7 .* shown=0"
shot narrow.png
control 2 floating 0
expect_log /tmp/zdesktop.log "ZWL MENU open client=$zc2 "
expect_log /tmp/zdesktop.log 'ZWL MENU row item=1 depth=1 '
expect_log /tmp/zdesktop.log 'ZWL MENU row item=4026531847 depth=1 '
shot overflow.png
click $(( $(popup_x) + 60 )) "$(row_y 4026531847)"
expect_log /tmp/f2.log 'ZFILES TITLEBAR kind=0 id=7 detail=0 '
expect_log /tmp/f2.log 'ZFILES TITLEBAR state .* view=1 '

errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
guest "$stop_all" >/dev/null
guest 'cat /tmp/f.log' > "$out/f.log"
guest 'cat /tmp/f2.log' > "$out/f2.log"
guest 'grep -E "ZWL (TITLEBAR|MENU)" /tmp/zdesktop.log' > "$out/zdesktop-titlebar.log"
[ $status = 0 ] && echo "files-p014: PASS" || echo "files-p014: FAIL"
exit $status
