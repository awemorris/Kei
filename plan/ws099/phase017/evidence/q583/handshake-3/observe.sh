#!/bin/sh
# ws035-p076: xdg_popup, xdg_positioner, the xdg_toplevel requests and the ping, on the Venus guest,
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
cd /home/awe/zedBSD-worktrees/b3
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws035-sq-run}"
export GUEST_RUNTIME
out=${1:-build/ws035-p076}
mkdir -p "$out"
guest() { /usr/bin/python3 /home/awe/zedBSD-worktrees/b3/plan/ws099/tests/p017-popup-timeline.py --event "$out" guest -- python3 plan/tools/guest/guest.py run "$1" 2>&1; }
check() { /usr/bin/python3 /home/awe/zedBSD-worktrees/b3/plan/ws099/tests/p017-popup-timeline.py --capture "$out" -- "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { /usr/bin/python3 /home/awe/zedBSD-worktrees/b3/plan/ws099/tests/p017-popup-timeline.py --event "$out" pointer -- python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
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
	guest 'cat /tmp/zdesktop.log' > "$out/frames.log"
	guest 'grep -E "ERROR|FAILED|protocol error" /tmp/zdesktop.log /tmp/p.log' | tee "$out/errors.txt"
	[ -s "$out/errors.txt" ] && status=1
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

far_maps=$(count_log /tmp/zdesktop.log '^ZWL POPUP map surface=')
far_configures=$(count_log /tmp/p.log 'POPUPPROBE configure menu x=360 y=60 width=360 height=180')
far_focuses=$(count_log /tmp/p.log 'POPUPPROBE focus menu')
case "$far_maps:$far_configures:$far_focuses" in
*[!0-9:]*|:*|*::|*:) echo 'far-menu: invalid initial count'; status=1; finish ;;
esac
click $((wx + 360)) $((wy + 60)) 1200
far_reply=$(guest "i=0; while [ \$i -lt 6 ]; do m=\$(grep -cE '^ZWL POPUP map surface=' /tmp/zdesktop.log); c=\$(grep -cE 'POPUPPROBE configure menu x=360 y=60 width=360 height=180' /tmp/p.log); f=\$(grep -cE 'POPUPPROBE focus menu' /tmp/p.log); if [ \$m -gt $far_maps ] && [ \$c -gt $far_configures ] && [ \$f -gt $far_focuses ]; then grep '^ZWL POPUP map surface=' /tmp/zdesktop.log | tail -1; echo P017_FAR_READY=1; exit 0; fi; sleep 0.5; i=\$((i + 1)); done; echo P017_FAR_READY=0; exit 1")
far_status=$?
printf '%s\n' "$far_reply"
if [ "$far_status" -ne 0 ] || ! printf '%s\n' "$far_reply" | grep -q '^P017_FAR_READY=1$'; then
    echo 'far-menu: fresh map/configure/focus missing; no submenu click sent'
    status=1
    finish
fi
click $((wx + 360 + 100)) $((wy + 60 + 130)) 1200
expect_log /tmp/p.log 'POPUPPROBE configure submenu x=-360 y=120 width=360 height=120'
check "$out/flipped.png" --expect $((wx + 360 - 360 + 100)),$((wy + 60 + 120 + 45)),e2a04a || status=1

# Saves the first partial-test verdict and complete application logs.
finish
