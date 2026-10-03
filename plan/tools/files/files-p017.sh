#!/bin/sh
# ws071-p017: files' glass cards line up with the floating titlebar, and its controls dock into
# the system bar and back, on the Venus guest (the lean image, build-files-image.sh).  zdesktop --glass at
# 1280x800; files at 1000x640 on the sample home (/tmp/fhome), opened on Documents, two tabs.
#  1. floating.png: the cards reach the window's edges (the sidebar's card from x 0, the content's to x 1000,
#     from y 0 to 640; 8 pixels apart, the gap between the titlebar and the window); the titlebar is as
#     wide as the window (zdesktop's floating_title; its controls, back to "...", inside x .. x+1000).
#  2. Docked by a double click on the title (GLASS dock): the controls in the system bar (where=docked),
#     the cards 8 pixels in from the screen's edges; docked.png.  There: Go > Downloads, then the
#     docked Back returns to Documents; the docked search field clicked, "report" typed, found
#     (SEARCH done), Esc; the docked "..." opened with the menus (docked-overflow.png), View > List chosen
#     (action 16).
#  3. Restored by a double click on the docked title (GLASS undock): the controls floating again, the cards
#     at the window's edges again; restored.png.
#
#   plan/tools/files/files-guest.sh start     (the guest must be up)
#   plan/tools/files/files-p017.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws071-p017}
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

# A control's place (client, where, ID) as zdesktop last logged it: "x y width height".
place() {
	guest "grep 'ZWL TITLEBAR control client=$(zwl_app_client $1) .* where=$2 id=$3 ' /tmp/zdesktop.log | tail -1" |
	    sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\) width=\([0-9]*\) height=\([0-9]*\).*/\1 \2 \3 \4/p'
}

# The middle (y) of the latest popup row of an item, and the latest popup's left edge at a depth.
row_y() {
	guest "grep 'MENU row item=$1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* y=\([0-9]*\) height=\([0-9]*\).*/\1 \2/p' | { read y h; echo $(( ${y:-0} + ${h:-0} / 2 )); }
}
popup_x() {
	guest "grep 'MENU open .* depth=${1:-1} ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([0-9]*\) y=.*/\1/p'
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
/bin/wayland --timeout=900 --width=1280 --height=800 --glass --wallpaper=/usr/share/keiland/wallpaper.ppm > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4
HOME=/tmp/fhome /bin/files --token=f1 --timeout-s=800 --width=1000 --height=640 /tmp/fhome/Documents > /tmp/f.log 2>&1 </dev/null & sleep 6; echo started' >/dev/null
zwl_app_clients
set -- $(guest "grep 'ZWL MAP client=$zc1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* surface=\([0-9]*\) x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2 \3/p')
surface=${1:-0}; wx=${2:-0}; wy=${3:-0}
echo "files: surface $surface at $wx,$wy"

# 1. Floating, two tabs: the cards at the window's edges, the titlebar as wide as the window.
keys '<ctrl-t>'
expect_log /tmp/f.log 'ZFILES TABS new index=1 count=2'
expect_log /tmp/zdesktop.log "ZWL GLASS client=[0-9]+ surface=$surface panels=2 card:0,0,212,640,16 card:220,0,780,640,16\$"
set -- $(place 1 floating 1); first=${1:-0}
set -- $(place 1 floating 0); last=$(( ${1:-0} + ${3:-0} ))
if [ "$first" -ge "$wx" ] && [ "$last" -le $((wx + 1000)) ]; then
	echo "titlebar: controls from $first to $last in $wx..$((wx + 1000)) ok"
else
	echo "titlebar: controls from $first to $last, window $wx..$((wx + 1000)) MISSING"
	status=1
fi
shot floating.png

# 2. Docked: the controls in the system bar, the cards in from the screen's edges.
set -- $(place 1 floating 1)
double $((wx + 60)) $((${2:-0} + ${4:-0} / 2))
expect_log /tmp/zdesktop.log "GLASS dock surface=$surface"
expect_log /tmp/zdesktop.log "ZWL TITLEBAR control client=$zc1 surface=$surface where=docked id=1 .* shown=1"
expect_log /tmp/zdesktop.log "ZWL GLASS client=[0-9]+ surface=$surface panels=2 card:8,8,212,[0-9]+,16 card:228,8,[0-9]+,[0-9]+,16\$"
shot docked.png

# Downloads (Go > Downloads, Ctrl+Shift+L), then the docked Back.
keys '<ctrl-shift-l>'
expect_log /tmp/f.log 'ZFILES LOCATION kind=folder path=/tmp/fhome/Downloads '
control 1 docked 1
expect_log /tmp/f.log 'ZFILES LOCATION kind=folder path=/tmp/fhome/Documents items=6'

# The docked search field.
control 1 docked 5
expect_log /tmp/zdesktop.log "ZWL TITLEBAR focus client=$zc1 surface=$surface id=5 edit=0"
keys 'report'
sleep 2
expect_log /tmp/f.log 'ZFILES SEARCH done query=report results=[1-9]'
shot docked-search.png
keys '<esc>'
expect_log /tmp/f.log 'ZFILES LOCATION kind=folder path=/tmp/fhome/Documents items=6'

# The docked "..." with the menus: View > List.
control 1 docked 0
expect_log /tmp/zdesktop.log 'ZWL MENU row item=3 depth=1 '
click $(( $(popup_x 1) + 60 )) "$(row_y 3)"
expect_log /tmp/zdesktop.log 'ZWL MENU row item=1016 depth=2 '
shot docked-overflow.png
click $(( $(popup_x 2) + 60 )) "$(row_y 1016)" 1200
expect_log /tmp/f.log 'ZFILES ACTION action=16'

# 3. Restored.
# The docked bar's name "Files" (120,17) undocks on a double click; (190,17) is the Forward control now (ws127-p001 T1).
double 120 17
expect_log /tmp/zdesktop.log "GLASS undock surface=$surface"
expect_log /tmp/zdesktop.log "ZWL TITLEBAR control client=$zc1 surface=$surface where=floating id=1 .* shown=1"
floating=$(guest "grep -c 'ZWL GLASS client=[0-9]* surface=$surface panels=2 card:0,0,212,640,16 card:220,0,780,640,16' /tmp/zdesktop.log" | tail -1)
[ "${floating:-0}" -ge 2 ] && echo "restored cards: ok" || { echo "restored cards: $floating MISSING"; status=1; }
shot restored.png

errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
guest "$stop_all" >/dev/null
guest 'cat /tmp/f.log' > "$out/f.log"
guest 'grep -E "GLASS|TITLEBAR control|MENU" /tmp/zdesktop.log' > "$out/zdesktop.log"
[ $status = 0 ] && echo "files-p017: PASS" || echo "files-p017: FAIL"
exit $status
