#!/bin/sh
# ws070-p013: the TABS hardening on the Venus guest (the lean image).  zdesktop --glass shows
# /bin/titlebar-probe --show --mode=tabs (README.md, main.c active, 日本語.txt, "+"; the probe makes a chosen tab
# active, drops a closed one, adds "Untitled N" for "+"; it has no menu, so the tab keys are zdesktop's):
#  1. keys.png: Ctrl+Tab, Ctrl+Tab (around the end), Ctrl+Shift+Tab, Ctrl+PageUp, Ctrl+PageDown give the probe,
#     in order: tab 3, tab 1, tab 3, tab 2, tab 3.  Ctrl+W and Ctrl+T stay the application's since ws035-p086
#     (a terminal's shell needs them): the probe (without a menu) hears no close and no new tab from them.
#  2. dock-1.png, dock-2.png: pictures taken while the window docks (the strip on its way), docked.png after;
#     restored.
#  3. title.png: a second probe with a long title and 5 tabs, 860 pixels wide: the title gives way (the first
#     tab starts within 150 pixels of the window's left edge) and the strip does not scroll.
#  4. wheel.png: a third probe with 14 tabs, 640 pixels wide: a wheel notch down over the strip scrolls it
#     (strip scroll ... first=1 by=wheel), a notch up back (first=0 by=wheel).
#
#   plan/tools/files/files-guest.sh start     (the guest must be up; GUEST_RUNTIME as for it)
#   plan/tools/titlebar/titlebar-p013.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws070-p013}
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
input() { python3 plan/tools/files/qmp-input.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
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

# The latest logged line of a tab (client, place, ID).
tab_line() {
	guest "grep 'ZWL TITLEBAR strip client=$(zwl_app_client $1) .* where=$2 id=$3 ' /tmp/zdesktop.log | tail -1"
}

# The centre of a logged rectangle ("x y" from a line with x=, y=, width=, height=), and its left edge.
centre() {
	sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\) width=\([0-9]*\) height=\([0-9]*\).*/\1 \2 \3 \4/p' |
	    { read x y w h; echo "$(( ${x:-0} + ${w:-0} / 2 )) $(( ${y:-0} + ${h:-0} / 2 ))"; }
}
left() {
	sed -n 's/.* x=\([-0-9]*\) y=.*/\1/p'
}

# Clicks a screen point.
click() {
	pointer move $(($1 - 2)) "$2" sleep 150 move "$1" "$2" sleep 300 down sleep 60 up sleep "${3:-900}"
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
/bin/wayland --timeout=600 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4; echo started' >/dev/null
probe /tmp/probe.log '--show=Editor --mode=tabs --seconds=500'
zwl_app_clients
set -- $(guest "grep 'ZWL MAP client=$zc1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* surface=\([0-9]*\) x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2 \3/p')
surface=${1:-0}; wx=${2:-0}; wy=${3:-0}
echo "probe: surface $surface at $wx,$wy"
expect_log /tmp/probe.log 'TITLEBARPROBE show ready mode=tabs'

# 1. The keys (the new window has the keyboard; the probe answers no ping, so its body is not clicked).
input hold ctrl key tab sleep 700 key tab sleep 700 hold shift key tab free shift sleep 700 key pgup sleep 700 key pgdn sleep 700 key w sleep 700 key t sleep 700 free ctrl sleep 500
got=$(guest "grep -E 'TITLEBARPROBE event=(tab|close|new)' /tmp/probe.log" | sed -n 's/.*event=\(tab\|close\|new\)\( id=\([0-9]*\)\)\{0,1\}.*/\1\3/p' | tr '\n' ' ')
want='tab3 tab1 tab3 tab2 tab3 '
[ "$got" = "$want" ] && echo "keys: $got ok" || { echo "keys: got '$got' want '$want' MISSING"; status=1; }
shot keys.png

# 2. Docking, pictured on its way (the double click's second release starts it), and back.
set -- $(tab_line 1 floating 2 | centre)
pointer move $((wx + 40)) "$2" sleep 300 down sleep 50 up sleep 80 down sleep 50 up sleep 1500 &
sleep 0.45
check "$out/dock-1.png" >/dev/null
check "$out/dock-2.png" >/dev/null
wait
expect_log /tmp/zdesktop.log "GLASS dock surface=$surface"
shot docked.png
pointer move "$(docked_free_x 1)" 17 sleep 300 down sleep 50 up sleep 80 down sleep 50 up sleep 1500
expect_log /tmp/zdesktop.log "GLASS undock surface=$surface"

# 3. A long title gives way to six tabs.
probe /tmp/probe2.log '--show=A_rather_long_window_title_that_would_crowd_the_tabs --mode=tabs --tabs=5 --width=860 --seconds=300'
expect_log /tmp/zdesktop.log "ZWL TITLEBAR strip client=$zc2 .* where=floating id=[0-9]+ .* shown=1 "
set -- $(guest "grep 'ZWL MAP client=$zc2 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([-0-9]*\) y=.*/\1/p')
w2x=${1:-0}
first=$(guest "grep 'ZWL TITLEBAR strip client=$zc2 .* where=floating id=' /tmp/zdesktop.log | head -1" | left)
[ $(( ${first:-9999} - w2x )) -le 150 ] && echo "title: first tab at +$(( ${first:-0} - w2x )) ok" || { echo "title: first tab at +$(( ${first:-9999} - w2x )) MISSING"; status=1; }
arrows=$(guest "grep -c 'ZWL TITLEBAR strip client=$zc2 .* button=left ' /tmp/zdesktop.log" | tail -1)
[ "${arrows:-1}" = 0 ] && echo "title: no arrows ok" || { echo "title: arrows=$arrows MISSING"; status=1; }
shot title.png

# 4. The wheel over a scrolling strip.
probe /tmp/probe3.log '--show=Many --mode=tabs --tabs=14 --width=640 --seconds=300'
expect_log /tmp/zdesktop.log "ZWL TITLEBAR strip client=$zc3 .* where=floating button=left "
set -- $(tab_line 3 floating 2 | centre)
pointer move "$1" "$2" sleep 400 wheel-down sleep 600
expect_log /tmp/zdesktop.log "ZWL TITLEBAR strip scroll client=$zc3 surface=[0-9]+ first=1 by=wheel"
check "$out/wheel.png" >/dev/null
pointer wheel-up sleep 600
expect_log /tmp/zdesktop.log "ZWL TITLEBAR strip scroll client=$zc3 surface=[0-9]+ first=0 by=wheel"

errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
guest 'grep -E "ZWL (TITLEBAR|MENU|GLASS)" /tmp/zdesktop.log' > "$out/zdesktop-titlebar.log"
guest 'cat /tmp/probe.log /tmp/probe2.log /tmp/probe3.log' > "$out/probe.log"
guest "$stop_all" >/dev/null
[ $status = 0 ] && echo "titlebar-p013: PASS" || echo "titlebar-p013: FAIL"
exit $status
