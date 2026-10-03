#!/bin/sh
# ws070-p011: the TABS presentation on the Venus guest (the lean image).  zdesktop --glass shows
# /bin/titlebar-probe --show --mode=tabs (README.md, main.c active, 日本語.txt wanting attention, "+"; the probe
# makes a chosen tab active, drops a closed one, adds "Untitled N" for "+"):
#  1. tabs.png: the strip in the floating titlebar (ZWL TITLEBAR strip ... where=floating), "+" after it.
#  2. README.md clicked (event tab id=1, then active: flags=1); 日本語.txt clicked (active, its dot gone:
#     flags=5), its close button clicked (event close id=3; main.c active again); "+" clicked (event new; tab 4
#     active); after.png.
#  3. docked.png: docked by a double click on the title, the strip in the system bar (where=docked); main.c
#     clicked there; restored.
#  4. A second probe with 6 tabs, 860 pixels wide: the tabs narrow (all shown, none as wide as it would like,
#     no arrows); narrow.png.
#  5. A third probe with 14 tabs, 640 pixels wide: the strip scrolls (arrows, tabs out of sight shown=0,
#     "..."); scroll.png; the right arrow moves it (strip scroll first=1); "..." lists the tabs out of sight
#     and one chosen there becomes active (event tab); strip-overflow.png.
#  6. A fourth probe with --switch=2: the mode goes between tabs and controls in single commits that keep both
#     models (every commit of it has controls=8 tabs=3); switch-controls.png and switch-tabs.png.
#
#   plan/tools/files/files-guest.sh start     (the guest must be up; GUEST_RUNTIME as for it)
#   plan/tools/titlebar/titlebar-p011.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws070-p011}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
. plan/tools/guest/zwl-clients.sh

# A point of the system bar's empty title right of everything the docked window put there (its tabs, buttons and
# controls take a press, so the undock's double click goes 40 pixels past the rightmost of them; 190 when none is logged).
docked_free_x() {
	guest "grep -E 'ZWL TITLEBAR (strip|control) client=$(zwl_app_client $1) .* where=docked .* x=[-0-9]+ y=[-0-9]+ width=[0-9]+' /tmp/zdesktop.log" |
	    sed -n 's/.* x=\([-0-9]*\) y=[-0-9]* width=\([0-9]*\).*/\1 \2/p' |
	    awk 'BEGIN { right = 150 } { if ($1 + $2 > right) right = $1 + $2 } END { print right + 40 }'
}
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
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

# The latest logged line of a tab (client, place, ID) or of a strip button (client, place, name).
tab_line() {
	guest "grep 'ZWL TITLEBAR strip client=$(zwl_app_client $1) .* where=$2 id=$3 ' /tmp/zdesktop.log | tail -1"
}
button_line() {
	guest "grep 'ZWL TITLEBAR strip client=$(zwl_app_client $1) .* where=$2 button=$3 ' /tmp/zdesktop.log | tail -1"
}

# The centre of a logged rectangle ("x y" from a line with x=, y=, width=, height=), and a tab's close button's.
centre() {
	sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\) width=\([0-9]*\) height=\([0-9]*\).*/\1 \2 \3 \4/p' |
	    { read x y w h; echo "$(( ${x:-0} + ${w:-0} / 2 )) $(( ${y:-0} + ${h:-0} / 2 ))"; }
}
close_centre() {
	sed -n 's/.* y=\([-0-9]*\) width=[0-9]* height=\([0-9]*\) .* close=\([-0-9]*\).*/\3 \1 \2/p' |
	    { read c y h; echo "$(( ${c:-0} + 10 )) $(( ${y:-0} + ${h:-0} / 2 ))"; }
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
	pointer move $(($1 - 2)) "$2" sleep 150 move "$1" "$2" sleep 300 down sleep 60 up sleep "${3:-900}"
}
double() {
	pointer move "$1" "$2" sleep 300 down sleep 50 up sleep 80 down sleep 50 up sleep "${3:-1200}"
}
shot() {
	pointer move 1270 790 sleep 500
	check "$out/$1" >/dev/null
}

# Starts a probe (its log, its options) and waits for it.
probe() {
	guest "export XDG_RUNTIME_DIR=/tmp; /bin/titlebar-probe $2 > $1 2>&1 </dev/null & sleep 5; echo started" >/dev/null
}

guest "$stop_all" >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0
picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=700 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4; echo started' >/dev/null
probe /tmp/probe.log '--show=Editor --mode=tabs --seconds=500'
zwl_app_clients
set -- $(guest "grep 'ZWL MAP client=$zc1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* surface=\([0-9]*\) x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2 \3/p')
surface=${1:-0}; wx=${2:-0}; wy=${3:-0}
echo "probe: surface $surface at $wx,$wy"

# 1. The strip.
expect_log /tmp/probe.log 'TITLEBARPROBE show ready mode=tabs'
expect_log /tmp/zdesktop.log "ZWL TITLEBAR strip client=$zc1 surface=$surface where=floating id=2 .* shown=1 flags=5 close=[0-9]+"
expect_log /tmp/zdesktop.log "ZWL TITLEBAR strip client=$zc1 surface=$surface where=floating id=3 .* shown=1 flags=6 close=-1"
expect_log /tmp/zdesktop.log "ZWL TITLEBAR strip client=$zc1 surface=$surface where=floating button=new "
shot tabs.png

# 2. README.md, 日本語.txt and its close button, "+".
set -- $(tab_line 1 floating 1 | centre); click "$1" "$2"
expect_log /tmp/probe.log 'TITLEBARPROBE event=tab id=1 '
expect_log /tmp/zdesktop.log "ZWL TITLEBAR tab client=$zc1 id=1 event=activated"
expect_log /tmp/zdesktop.log "ZWL TITLEBAR strip client=$zc1 surface=$surface where=floating id=1 .* flags=1 close=-1"
set -- $(tab_line 1 floating 3 | centre); click "$1" "$2"
expect_log /tmp/probe.log 'TITLEBARPROBE event=tab id=3 '
expect_log /tmp/zdesktop.log "ZWL TITLEBAR strip client=$zc1 surface=$surface where=floating id=3 .* flags=5 close=[0-9]+"
set -- $(tab_line 1 floating 3 | close_centre); click "$1" "$2"
expect_log /tmp/probe.log 'TITLEBARPROBE event=close id=3'
expect_log /tmp/zdesktop.log "ZWL TITLEBAR strip client=$zc1 surface=$surface where=floating id=2 .* flags=5 "
set -- $(button_line 1 floating new | centre); click "$1" "$2"
expect_log /tmp/probe.log 'TITLEBARPROBE event=new '
expect_log /tmp/zdesktop.log "ZWL TITLEBAR strip client=$zc1 surface=$surface where=floating id=4 .* shown=1 flags=5 "
shot after.png

# 3. Docked, main.c there, restored.
set -- $(tab_line 1 floating 1 | centre)
double $((wx + 40)) "$2"
expect_log /tmp/zdesktop.log "GLASS dock surface=$surface"
expect_log /tmp/zdesktop.log "ZWL TITLEBAR strip client=$zc1 surface=$surface where=docked id=2 .* shown=1 "
shot docked.png
set -- $(tab_line 1 docked 2 | centre); click "$1" "$2"
activations=$(guest "grep -c 'TITLEBARPROBE event=tab id=2 ' /tmp/probe.log" | tail -1)
[ "${activations:-0}" -ge 1 ] && echo "docked main.c: ok" || { echo "docked main.c: MISSING"; status=1; }
double "$(docked_free_x 1)" 17
expect_log /tmp/zdesktop.log "GLASS undock surface=$surface"

# 4. Six tabs narrowed.
probe /tmp/probe2.log '--show=Six --mode=tabs --tabs=6 --width=860 --seconds=300'
expect_log /tmp/zdesktop.log "ZWL TITLEBAR strip client=$zc2 .* where=floating id=6 .* shown=1 "
hidden=$(guest "grep -c 'ZWL TITLEBAR strip client=$zc2 .* shown=0 ' /tmp/zdesktop.log" | tail -1)
arrows=$(guest "grep -c 'ZWL TITLEBAR strip client=$zc2 .* button=left ' /tmp/zdesktop.log" | tail -1)
[ "${hidden:-1}" = 0 ] && [ "${arrows:-1}" = 0 ] && echo "narrow: all shown, no arrows ok" || { echo "narrow: hidden=$hidden arrows=$arrows MISSING"; status=1; }
shot narrow.png

# 5. Fourteen tabs scrolled.
probe /tmp/probe3.log '--show=Many --mode=tabs --tabs=14 --width=640 --seconds=300'
expect_log /tmp/zdesktop.log "ZWL TITLEBAR strip client=$zc3 .* where=floating button=left "
expect_log /tmp/zdesktop.log "ZWL TITLEBAR strip client=$zc3 .* where=floating id=14 .* shown=0 "
expect_log /tmp/zdesktop.log "ZWL TITLEBAR strip client=$zc3 .* where=floating button=overflow "
shot scroll.png
set -- $(button_line 3 floating right | centre); click "$1" "$2"
expect_log /tmp/zdesktop.log "ZWL TITLEBAR strip scroll client=$zc3 surface=[0-9]+ first=1"
set -- $(button_line 3 floating overflow | centre); click "$1" "$2" 1000
expect_log /tmp/zdesktop.log "ZWL MENU open client=$zc3 "
check "$out/strip-overflow.png" >/dev/null
click $(( $(popup_x) + 60 )) "$(row_y 4026531854)" 1000
expect_log /tmp/probe3.log 'TITLEBARPROBE event=tab id=14 '
expect_log /tmp/zdesktop.log "ZWL TITLEBAR strip client=$zc3 .* where=floating id=14 .* shown=1 flags=5 "

# 6. The mode switched in single commits.
guest "$stop_all" >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0
picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=300 --width=1280 --height=800 --glass $picture > /tmp/zdesktop2.log 2>&1 </dev/null & sleep 4; echo started' >/dev/null
probe /tmp/probe4.log '--show=Switch --mode=tabs --switch=2 --seconds=200'
expect_log /tmp/probe4.log 'TITLEBARPROBE switch mode=controls'
zwl_app_clients /tmp/zdesktop2.log
expect_log /tmp/zdesktop2.log "ZWL TITLEBAR control client=$zc1 .* where=floating id=5 .* shown=1"
check "$out/switch-controls.png" >/dev/null
expect_log /tmp/probe4.log 'TITLEBARPROBE switch mode=tabs'
check "$out/switch-tabs.png" >/dev/null
set -- $(guest "grep 'ZWL TITLEBAR commit client=$zc1 ' /tmp/zdesktop2.log > /tmp/commits.txt; grep -c . /tmp/commits.txt; grep -vc ' controls=8 tabs=3 ' /tmp/commits.txt" | tail -2)
commits=${1:-0}; partial=${2:-1}
[ "$commits" -ge 3 ] && [ "$partial" = 0 ] && echo "switch: $commits commits, each whole ok" || { echo "switch: commits=$commits partial=$partial MISSING"; status=1; }

errors=$(guest "grep -c ERROR /tmp/zdesktop.log /tmp/zdesktop2.log | grep -v ':0' | wc -l" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
guest 'grep -E "ZWL (TITLEBAR|MENU)" /tmp/zdesktop.log /tmp/zdesktop2.log' > "$out/zdesktop-titlebar.log"
guest 'cat /tmp/probe.log /tmp/probe2.log /tmp/probe3.log /tmp/probe4.log' > "$out/probe.log"
guest "$stop_all" >/dev/null
[ $status = 0 ] && echo "titlebar-p011: PASS" || echo "titlebar-p011: FAIL"
exit $status
