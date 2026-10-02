#!/bin/sh
# q577 diagnostic, canonical p076 plus frame logging and optional direct geometry path; ws035-p076: xdg_popup, xdg_positioner, the xdg_toplevel requests and the ping, on the Venus guest,
# with /bin/popup-probe (userland/tests/popup-probe) in wide mode (menus 360 wide).
# zdesktop --glass runs at 1280x800; the probe's window (400x300, dark, limits 200x150 to 800x600) is centred.
#  1. A press in the window opens the menu (blue) there, with the grab: the menu gets the keyboard;
#     the press pinged the client, which answered.  menu.png: the menu over the window at the press.
#  2. A press in the menu's lower half opens the submenu (orange) at the menu's right; the keyboard stays
#     with the menu until the submenu is shown.  submenu.png.
#  3. A press in the submenu chooses an item; both popups close and the window has the keyboard again.
#  4. A menu opened far right: its submenu does not fit on the right and flips to the left (x=-360).
#  5. A press on the desktop: zdesktop closes the popups (popup_done); the press reaches nobody.
#  6. The menu again and the key r: xdg_popup.reposition to window-local (10,40) (repositioned, configure).
#  7. A press and drag in the window's top strip moves it (xdg_toplevel.move).
#  8. Resizes (xdg_toplevel.resize): the bottom-right corner +150,+100 (550x400, resizing state while it
#     goes); the left edge +400 (clamped to the minimum width 200, the right edge stays); the left edge
#     -800 (clamped to the maximum width 800, the right edge stays).
#  9. The key p stops the probe's pongs; a right press asks for the window menu and pings; unanswered
#     for 5 s, the client is not responding (its title says so: unresponsive.png); p again answers.
# 10. Keys m, u, n: maximize (docks), unmaximize (undocks), minimize.
#
#   plan/ws035/tests/zdesktop-guest.sh start     (the guest must be up; plan/tools/titlebar/menu-guest.sh for the lean image)
#   plan/ws035/tests/zdesktop-p076.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws035-sq-run}"
export GUEST_RUNTIME
out=${1:-build/p8-q577/framed}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[p]opup-probe" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[p]opup-probe" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
status=0

# Fails the run unless a log has (within a few seconds) as many lines matching a pattern as asked (default 1).
expect_log() {
	tries=0
	found=0
	while [ $tries -lt 5 ]; do
		found=$(guest "grep -cE '$2' $1" | tail -1)
		[ "${found:-0}" -ge "${3:-1}" ] 2>/dev/null && break
		tries=$((tries + 1))
		sleep 1
	done
	if [ "${found:-0}" -ge "${3:-1}" ] 2>/dev/null; then
		echo "log: $2 ok"
	else
		echo "log: $2 MISSING (found ${found:-0})"
		status=1
	fi
}

# Prints how many lines of a log match a pattern now.
count_log() {
	guest "grep -cE '$2' $1" | tail -1
}

# Requires a new accepted request while the button remains held, within six half-second polls.
# A ten-second host deadline also bounds a stalled SSH command; an old matching line cannot pass.
wait_request() {
	request_reply=$(timeout 10 python3 plan/tools/guest/guest.py run "i=0; n=0; while [ \$i -lt 6 ]; do n=\$(grep -cE '$2' '$1'); if [ \$n -gt $3 ]; then echo P076_REQUEST=\$n; exit 0; fi; sleep 0.5; i=\$((i + 1)); done; echo P076_REQUEST=\$n; exit 1" 2>&1)
	request_status=$?
	request_seen=$(printf '%s\n' "$request_reply" | sed -n 's/^P076_REQUEST=\([0-9][0-9]*\)$/\1/p' | tail -1)
	if [ "$request_status" -eq 0 ] && [ "${request_seen:-0}" -gt "$3" ]; then
		echo "request: $2 count $3 -> $request_seen ok"
		return 0
	fi
	echo "request: $2 MISSING new count (before $3, found ${request_seen:-0}, status $request_status)"
	status=1
	return 1
}

# Starts the gesture only after counting accepted requests, so a previous drag cannot satisfy it.
press_request() {
	request_before=$(timeout 10 python3 plan/tools/guest/guest.py run "grep -cE '$3' /tmp/zdesktop.log" 2>&1 | tail -1)
	case "$request_before" in
	''|*[!0-9]*) echo "request: invalid initial count ($request_before)"; status=1; return 1 ;;
	esac
	pointer move $(($1 - 2)) "$2" sleep 150 move "$1" "$2" sleep 300 down sleep 200
	if wait_request /tmp/zdesktop.log "$3" "$request_before"; then
		return 0
	fi
	# A missing request ends this attempt before any drag motion is sent.
	pointer up
	return 1
}

# Saves the ordinary verdict and evidence, also when a request handshake stops a gesture early.
finish() {
	guest 'grep -E "ERROR|FAILED|protocol error" /tmp/zdesktop.log /tmp/p.log' | tee "$out/errors.txt"
	[ -s "$out/errors.txt" ] && status=1
	guest 'cat /tmp/zdesktop.log' > "$out/frames.log"
	guest 'cat /tmp/p.log' > "$out/probe.log"
	guest 'grep -E "POPUP|PING|RESIZE|GLASS (request|moved|dock|undock|minimize)" /tmp/zdesktop.log' > "$out/zdesktop.log"
	guest "$stop_all" >/dev/null
	[ $status -eq 0 ] && echo "p076: PASS" || echo "p076: FAIL"
	exit $status
}

# Clicks a point (with a small approach, so that the motion is seen before the press).
click() {
	pointer move $(($1 - 2)) "$2" sleep 150 move "$1" "$2" sleep 300 down sleep 60 up sleep "${3:-800}"
}

# Presses at a point, drags by (dx, dy) in three steps, and lets go.
drag() {
	press_request "$1" "$2" '^ZWL RESIZE start surface=' || finish
	pointer move $(($1 + $3 / 3)) $(($2 + $4 / 3)) sleep 200 \
	    move $(($1 + 2 * $3 / 3)) $(($2 + 2 * $4 / 3)) sleep 200 \
	    move $(($1 + $3)) $(($2 + $4)) sleep 400 up sleep 1000
}

guest "$stop_all" >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0; picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --log-frames --timeout=400 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & for w in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30; do grep -q ZWL.READY /tmp/zdesktop.log 2>/dev/null && break; sleep 0.5; done; sleep 1
/bin/popup-probe --wide --timeout-s=300 --token=p > /tmp/p.log 2>&1 </dev/null & sleep 4; echo started' >/dev/null
expect_log /tmp/p.log 'POPUPPROBE ready run=p'
set -- $(guest "grep 'ZWL MAP client=1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2/p')
wx=${1:-0}; wy=${2:-0}
echo "window at $wx,$wy"

if [ "${P076_GEOMETRY_ONLY:-0}" != 1 ]; then
# 1. The menu at a press in the window (window-local 60,60); the press pinged the client.
click $((wx + 60)) $((wy + 60)) 1200
expect_log /tmp/zdesktop.log 'ZWL POPUP grab popup='
expect_log /tmp/zdesktop.log 'ZWL POPUP map surface=[0-9]+ x=[0-9]+ y=[0-9]+ grab=1'
expect_log /tmp/zdesktop.log 'ZWL PING pong client=1 '
expect_log /tmp/p.log 'POPUPPROBE ping serial=[0-9]+ answered'
expect_log /tmp/p.log 'POPUPPROBE configure menu x=60 y=60 width=360 height=180'
expect_log /tmp/p.log 'POPUPPROBE focus menu'
check "$out/menu.png" --expect $((wx + 60 + 100)),$((wy + 60 + 45)),4a90e2 || status=1

# 2. The submenu from the menu's lower half (menu-local 100,130: the row at 120), beside the menu;
#    the keyboard goes from the menu to the submenu directly (never back to the window).
click $((wx + 60 + 100)) $((wy + 60 + 130)) 1200
expect_log /tmp/p.log 'POPUPPROBE button menu state=1 x=100 y=130'
expect_log /tmp/p.log 'POPUPPROBE configure submenu x=360 y=120 width=360 height=120'
expect_log /tmp/p.log 'POPUPPROBE focus submenu'
back=$(count_log /tmp/p.log 'POPUPPROBE focus window')
[ "${back:-1}" = 1 ] || { echo "the keyboard went back to the window while the submenu opened ($back)"; status=1; }
check "$out/submenu.png" --expect $((wx + 60 + 360 + 100)),$((wy + 60 + 120 + 45)),e2a04a || status=1

# 3. A press in the submenu chooses; both close and the window has the keyboard again.
click $((wx + 60 + 360 + 100)) $((wy + 60 + 120 + 45)) 1200
expect_log /tmp/p.log 'POPUPPROBE choose submenu row=1'
expect_log /tmp/p.log 'POPUPPROBE close menu'
expect_log /tmp/p.log 'POPUPPROBE focus window' 2
check "$out/chosen.png" --expect $((wx + 60 + 100)),$((wy + 60 + 45)),2b3444 || status=1

# 4. A menu far right (window-local 360,60): its submenu flips to the left of it.
click $((wx + 360)) $((wy + 60)) 1200
click $((wx + 360 + 100)) $((wy + 60 + 130)) 1200
expect_log /tmp/p.log 'POPUPPROBE configure submenu x=-360 y=120 width=360 height=120'
check "$out/flipped.png" --expect $((wx + 360 - 360 + 100)),$((wy + 60 + 120 + 45)),e2a04a || status=1

# 5. A press on the desktop closes the popups (popup_done) and reaches nobody.
before=$(count_log /tmp/p.log 'POPUPPROBE button other state=1')
click 40 700 1200
expect_log /tmp/zdesktop.log 'ZWL POPUP dismiss client='
expect_log /tmp/p.log 'POPUPPROBE done submenu'
expect_log /tmp/p.log 'POPUPPROBE close menu' 2
after=$(count_log /tmp/p.log 'POPUPPROBE button other state=1')
[ "${after:-1}" = "${before:-0}" ] || { echo "a press outside reached the client"; status=1; }
check "$out/dismissed.png" --expect $((wx + 380)),$((wy + 200)),2b3444 || status=1

# 6. The menu again, and r: it is placed again at window-local (10,40).
click $((wx + 60)) $((wy + 60)) 1200
keys 'r'
expect_log /tmp/p.log 'POPUPPROBE repositioned token=1'
expect_log /tmp/p.log 'POPUPPROBE configure menu x=10 y=40 width=360 height=180'
check "$out/repositioned.png" --expect $((wx + 30)),$((wy + 50)),4a90e2 || status=1
click 40 700 1200
expect_log /tmp/p.log 'POPUPPROBE close menu' 3

fi

# 7. A move from the window's top strip.
press_request $((wx + 200)) $((wy + 10)) '^ZWL GLASS request move surface=[0-9]+$' || finish
pointer move $((wx + 240)) $((wy + 40)) sleep 150 move $((wx + 300)) $((wy + 110)) sleep 300 up sleep 800
expect_log /tmp/p.log 'POPUPPROBE move'
expect_log /tmp/zdesktop.log 'ZWL GLASS request move surface='
expect_log /tmp/zdesktop.log "ZWL GLASS moved surface=[0-9]+ x=$((wx + 100)) y=$((wy + 100))"
check "$out/moved.png" --expect $((wx + 100 + 200)),$((wy + 100 + 155)),2b3444 || status=1
mx=$((wx + 100)); my=$((wy + 100))

# 8. Resizes: the corner, the left edge (to the minimum width), the corner again (to the maximum).
drag $((mx + 390)) $((my + 290)) 150 100
expect_log /tmp/p.log 'POPUPPROBE resize bottom-right'
expect_log /tmp/zdesktop.log 'ZWL RESIZE start surface=[0-9]+ edges=10 width=400 height=300'
expect_log /tmp/p.log 'POPUPPROBE configure window width=[0-9]+ height=[0-9]+ states=2 resizing=1'
expect_log /tmp/zdesktop.log 'ZWL RESIZE end surface=[0-9]+ width=550 height=400'
expect_log /tmp/p.log 'POPUPPROBE configure window width=550 height=400 states=1 resizing=0'
check "$out/resized.png" --expect $((mx + 540)),$((my + 385)),2b3444 || status=1
drag $((mx + 5)) $((my + 200)) 400 0
expect_log /tmp/p.log 'POPUPPROBE resize left'
expect_log /tmp/zdesktop.log 'ZWL RESIZE end surface=[0-9]+ width=200 height=400'
expect_log /tmp/zdesktop.log "ZWL RESIZE settled surface=[0-9]+ x=$((mx + 350)) y=$my width=200 height=400"
check "$out/narrowed.png" --expect $((mx + 350 + 100)),$((my + 385)),2b3444 || status=1
drag $((mx + 355)) $((my + 200)) -800 0
expect_log /tmp/zdesktop.log 'ZWL RESIZE end surface=[0-9]+ width=800 height=400'
expect_log /tmp/zdesktop.log "ZWL RESIZE settled surface=[0-9]+ x=$((mx - 250)) y=$my width=800 height=400"
check "$out/widened.png" --expect $((mx - 250 + 30)),$((my + 385)),2b3444 || status=1

# 9. Not responding: pongs off, a right press (the window menu) pings, 5 s pass; then pongs on.
keys 'p'
expect_log /tmp/p.log 'POPUPPROBE pong off'
pointer move $((mx - 250 + 400)) $((my + 100)) sleep 300 right-down sleep 60 right-up sleep 800
expect_log /tmp/p.log 'POPUPPROBE window-menu'
expect_log /tmp/p.log 'POPUPPROBE ping serial=[0-9]+ ignored'
sleep 6
expect_log /tmp/zdesktop.log 'ZWL PING unresponsive client=1'
check "$out/unresponsive.png" >/dev/null
keys 'p'
expect_log /tmp/p.log 'POPUPPROBE pong on answered=[1-9]'
expect_log /tmp/zdesktop.log 'ZWL PING responsive client=1'

# 10. Maximize, unmaximize and minimize from the keyboard.
keys 'm'
expect_log /tmp/zdesktop.log 'ZWL GLASS dock surface=[0-9]+ via=request'
keys 'u'
expect_log /tmp/zdesktop.log 'ZWL GLASS undock surface=[0-9]+ via=request'
sleep 1
keys 'n'
expect_log /tmp/zdesktop.log 'ZWL GLASS minimize surface='

# Nothing failed.
finish
