#!/bin/sh
# ws070-p010: the CONTROLS presentation on the Venus guest (the lean image).  zdesktop --glass shows
# /bin/titlebar-probe --show --mode=controls (a file manager's controls: back, forward (disabled), home,
# a breadcrumb Home > Projects > 日本語, a search field, Icons/List, Preview):
#  1. floating.png: the controls in the floating titlebar (ZWL TITLEBAR control ... where=floating).
#  2. Back clicked (event activated id=1); the breadcrumb's "..." (the nearest part left out) and its last
#     part (detail=2); the search field
#     clicked, "aBc" typed (text events, a capital with Shift), Enter (done how=0 text=aBc): search.png while typing; List
#     clicked (id=7).
#  3. docked.png: docked by a double click on the title, the controls in the system bar
#     (where=docked); List clicked there too; restored by a double click on the docked title.
#  4. narrow.png: a second probe 380 pixels wide: controls give way (shown=0) to "..." (id=0);
#     overflow.png: "..." opened (its rows are the hidden controls); List's row chosen (activated id=7).
#
#   plan/tools/files/files-guest.sh start     (the guest must be up; GUEST_RUNTIME as for it)
#   plan/tools/titlebar/titlebar-p010.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws070-p010}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
. plan/tools/guest/zwl-clients.sh

# A point of the system bar's empty title: midway between the rightmost thing the docked window put there (its tabs,
# strip buttons and controls take a press) and the left edge of the bar's minimize button (from the latest
# "GLASS dock ... buttons=close,restore,minimize" line; a button is 30 wide).  The input method's indicator, when
# installed, moves the buttons left, so a fixed offset can land on minimize (titlebar-p010 in T1-036).
docked_free_x() {
	guest "grep -E 'ZWL TITLEBAR (strip|control) client=$(zwl_app_client $1) .* where=docked .* x=[-0-9]+ y=[-0-9]+ width=[0-9]+|ZWL GLASS dock surface=' /tmp/zdesktop.log" |
	    awk 'BEGIN { right = 150; minimize = 0 }
		/GLASS dock surface=/ { for (i = 1; i <= NF; i++) if ($i ~ /^buttons=/) { split(substr($i, 9), b, ","); minimize = b[3] - 15 } next }
		{ x = ""; w = ""; for (i = 1; i <= NF; i++) { if ($i ~ /^x=/) x = substr($i, 3); if ($i ~ /^width=/) w = substr($i, 7) }
		  if (x != "" && w != "" && x + w > right) right = x + w }
		END { if (minimize > right + 2) print int((right + minimize) / 2); else print right + 40 }'
}
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; sleep 0.7; }
stop_all='for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[t]itlebar-probe" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[t]itlebar-probe" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
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

# The centre of a control (client, place, ID) as zdesktop last logged it: "x y".
control() {
	guest "grep 'ZWL TITLEBAR control client=$(zwl_app_client $1) .* where=$2 id=$3 ' /tmp/zdesktop.log | tail -1" |
	    sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\) width=\([0-9]*\) height=\([0-9]*\).*/\1 \2 \3 \4/p' |
	    { read x y w h; echo "$(( ${x:-0} + ${w:-0} / 2 )) $(( ${y:-0} + ${h:-0} / 2 ))"; }
}

# The middle (y) of the latest popup row of an item, and the latest popup's left edge.
row_y() {
	guest "grep 'MENU row item=$1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* y=\([0-9]*\) height=\([0-9]*\).*/\1 \2/p' | { read y h; echo $(( ${y:-0} + ${h:-0} / 2 )); }
}
popup_x() {
	guest "grep 'MENU open ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([0-9]*\) y=.*/\1/p'
}

# Clicks a screen point, and double-clicks one.
click() {
	pointer move $(($1 - 2)) "$2" sleep 150 move "$1" "$2" sleep 300 down sleep 60 up sleep "${3:-700}"
}
double() {
	pointer move "$1" "$2" sleep 300 down sleep 50 up sleep 80 down sleep 50 up sleep "${3:-1200}"
}
shot() {
	pointer move 1270 790 sleep 500
	check "$out/$1" >/dev/null
}

guest "$stop_all" >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0
picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=600 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4
/bin/titlebar-probe --show=Files --mode=controls --seconds=400 > /tmp/probe.log 2>&1 </dev/null & sleep 5; echo started' >/dev/null
zwl_app_clients
set -- $(guest "grep 'ZWL MAP client=$zc1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* surface=\([0-9]*\) x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2 \3/p')
surface=${1:-0}; wx=${2:-0}; wy=${3:-0}
echo "probe: surface $surface at $wx,$wy"

# 1. The controls in the floating titlebar.
expect_log /tmp/probe.log 'TITLEBARPROBE show ready mode=controls'
expect_log /tmp/zdesktop.log "ZWL TITLEBAR control client=$zc1 surface=$surface where=floating id=5 .* shown=1"
shot floating.png

# 2. Back, the breadcrumb's first part, the search field, List.
set -- $(control 1 floating 1); click "$1" "$2"
expect_log /tmp/probe.log 'TITLEBARPROBE event=activated id=1 detail=0'
set -- $(guest "grep 'ZWL TITLEBAR control client=$zc1 .* where=floating id=4 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\) width=\([0-9]*\).*/\1 \2 \3/p')
click $(( $1 + 8 )) $(( $2 + 15 ))
expect_log /tmp/probe.log 'TITLEBARPROBE event=activated id=4 detail=[01] '
click $(( $1 + 60 )) $(( $2 + 15 ))
expect_log /tmp/probe.log 'TITLEBARPROBE event=activated id=4 detail=2 '
set -- $(control 1 floating 5); click "$1" "$2"
expect_log /tmp/zdesktop.log "ZWL TITLEBAR focus client=$zc1 surface=[0-9]+ id=5 edit=0"
keys 'aBc'
shot search.png
expect_log /tmp/probe.log 'TITLEBARPROBE event=text id=5 text=aBc'
keys '<ret>'
expect_log /tmp/probe.log 'TITLEBARPROBE event=done id=5 how=0 text=aBc'
set -- $(control 1 floating 5); click "$1" "$2"
keys 'x'
keys '<esc>'
expect_log /tmp/probe.log 'TITLEBARPROBE event=done id=5 how=1 '
set -- $(control 1 floating 7); click "$1" "$2"
expect_log /tmp/probe.log 'TITLEBARPROBE event=activated id=7 detail=0'

# 3. Docked: the controls in the system bar, List there, then restored.
set -- $(control 1 floating 1)
double $((wx + 60)) "$2"
expect_log /tmp/zdesktop.log "GLASS dock surface=$surface"
expect_log /tmp/zdesktop.log "ZWL TITLEBAR control client=$zc1 surface=$surface where=docked id=7 .* shown=1"
shot docked.png
set -- $(control 1 docked 7); click "$1" "$2"
activations=$(guest "grep -c 'TITLEBARPROBE event=activated id=7 ' /tmp/probe.log" | tail -1)
[ "${activations:-0}" -ge 2 ] && echo "docked List: ok" || { echo "docked List: MISSING"; status=1; }
double "$(docked_free_x 1)" 17
expect_log /tmp/zdesktop.log "GLASS undock surface=$surface"

# 4. A narrow window: the controls give way to "...", which holds them.
guest 'export XDG_RUNTIME_DIR=/tmp; /bin/titlebar-probe --show=Narrow --mode=controls --width=380 --seconds=200 > /tmp/probe2.log 2>&1 </dev/null & sleep 5; echo started' >/dev/null
expect_log /tmp/zdesktop.log "ZWL TITLEBAR control client=$zc2 .* where=floating id=7 .* shown=0"
expect_log /tmp/zdesktop.log "ZWL TITLEBAR control client=$zc2 .* where=floating id=0 "
shot narrow.png
set -- $(control 2 floating 0); click "$1" "$2" 1000
expect_log /tmp/zdesktop.log "ZWL MENU open client=$zc2 "
shot overflow.png
click $(( $(popup_x) + 60 )) "$(row_y 4026531847)" 1000
expect_log /tmp/probe2.log 'TITLEBARPROBE event=activated id=7 detail=0'
expect_log /tmp/zdesktop.log "ZWL TITLEBAR activate client=$zc2 id=7 detail=0 serial=[0-9]+ via=overflow"

errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
guest 'grep -E "ZWL (TITLEBAR|MENU)" /tmp/zdesktop.log' > "$out/zdesktop-titlebar.log"
guest 'cat /tmp/probe.log /tmp/probe2.log' > "$out/probe.log"
guest "$stop_all" >/dev/null
[ $status = 0 ] && echo "titlebar-p010: PASS" || echo "titlebar-p010: FAIL"
exit $status
