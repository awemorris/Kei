#!/bin/sh
# ws089-p012: Settings with the keys and a finger, on the Venus guest of the touch image (config-amd64-settings-touch.mk:
# the Settings image with the test touch screen of /dev/input-inject and touchinject).  zdesktop --glass at 1280x800.
#  1. The search's results by the keys: Ctrl+F, "wi", Tab (the titlebar's field is left; zdesktop's field takes Up and
#     Down itself), Down chooses the second result (SEARCH chosen index=1 page=ethernet, search-chosen.png), Enter opens
#     it (SEARCH open page=ethernet).  Then Ctrl+F, "wall", Enter in the field opens the first (page=wallpaper).
#  2. The list's keys and the history: from Home, Down three times (wifi, ethernet, bluetooth), Up (ethernet); Alt+Left
#     goes back to bluetooth, Alt+Right forward to ethernet.
#  3. A finger: on Home, a finger dragged up 300 pixels over the page scrolls it (TOUCH scroll start pane=page, then
#     end with scroll > 0) and opens no page; a tap on a row of the list opens its page (a tap is still a click); a
#     finger dragged up over the list scrolls the list (pane=list) (touch-before.png, touch-after.png).
#  4. No ERROR line in zdesktop's log.
#
#   plan/ws089/tests/settings-guest.sh start build/<W>-settings-touch/hdd-image.img
#   plan/ws089/tests/settings-p012.sh [OUTDIR]   (default build/ws089-shots/p012)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws089-run}"
export GUEST_RUNTIME
out=${1:-build/ws089-shots/p012}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1 </dev/null; }
put() { timeout 120 python3 plan/tools/guest/guest.py put "$1" "$2" >/dev/null 2>&1 </dev/null || { echo "put $1: FAILED"; status=1; }; }
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@" >/dev/null; sleep 0.7; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[s]ettings" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[s]ettings" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
start_desktop='export XDG_RUNTIME_DIR=/tmp HOME=/root; rm -f /tmp/wayland-0
/bin/wayland --timeout=900 --width=1280 --height=800 --glass --wallpaper=/usr/share/keiland/wallpaper.ppm > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4; echo started'
conf=/root/.config/keiland/desktop.conf
status=0
. plan/ws089/tests/settings-wait.sh

# Fails the run unless a log has a line matching a pattern (within a few seconds).
expect_log() {
	tries=0
	found=0
	while [ $tries -lt 10 ]; do
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

# Starts settings on a page (its log in /tmp/s.log) and finds its window.
start_settings() {
	guest "export XDG_RUNTIME_DIR=/tmp HOME=/root; /bin/settings --timeout-s=800 $1 > /tmp/s.log 2>&1 </dev/null & sleep 5; echo started" >/dev/null
	find_window
	echo "settings: window at $wx,$wy"
}

# A picture of the screen with the pointer out of the way.
shot() {
	pointer move 1270 790 sleep 600
	check "$out/$1" >/dev/null
	echo "shot: $out/$1"
}

# Replays a touch script on the injected touch screen, in screen pixels of 1280x800.
touch_replay() {
	printf '%s\n' "$2" > "$out/$1.script"
	put "$out/$1.script" "/tmp/$1.script"
	result=$(guest "/bin/touchinject /tmp/$1.script 2>&1; echo replay=\$?")
	printf '%s\n' "$result" | grep -q '^replay=0$' || { echo "touchinject $1: FAILED ($result)"; status=1; }
}

wait_guest
guest "$stop_all" >/dev/null
guest "rm -f $conf" >/dev/null
guest "$start_desktop" >/dev/null
wait_desktop

# 1. The search's results by the keys.
start_settings ""
pointer move $((wx + 700)) $((wy + 300)) sleep 200 down sleep 60 up sleep 600
keys '<ctrl-f>'
expect_log /tmp/s.log 'ZSETTINGS SEARCH focus'
sleep 2
keys 'wi'
expect_log /tmp/s.log 'ZSETTINGS SEARCH query=wi results=[1-9]'
keys '<tab>'
sleep 1
keys '<down>'
expect_log /tmp/s.log 'ZSETTINGS SEARCH chosen index=1 page=ethernet'
shot search-chosen.png
keys '<ret>'
expect_log /tmp/s.log 'ZSETTINGS SEARCH open page=ethernet'
expect_log /tmp/s.log 'ZSETTINGS PAGE ethernet$'
keys '<ctrl-f>'
sleep 2
keys 'wall'
expect_log /tmp/s.log 'ZSETTINGS SEARCH query=wall results=[1-9]'
keys '<ret>'
expect_log /tmp/s.log 'ZSETTINGS SEARCH open page=wallpaper'

# 2. The list's keys and the history.
guest "$stop_all" >/dev/null
guest "$start_desktop" >/dev/null
wait_desktop
start_settings ""
pointer move $((wx + 700)) $((wy + 300)) sleep 200 down sleep 60 up sleep 600
keys '<down>'
keys '<down>'
keys '<down>'
expect_log /tmp/s.log 'ZSETTINGS PAGE bluetooth$'
keys '<up>'
expect_log /tmp/s.log 'ZSETTINGS PAGE ethernet$'
keys '<alt-left>'
expect_log /tmp/s.log 'ZSETTINGS PAGE bluetooth back'
keys '<alt-right>'
expect_log /tmp/s.log 'ZSETTINGS PAGE ethernet forward'

# 3. A finger: Home's page dragged up, a tap on a row, the list dragged up -- one touch screen for all of them (each
#    run of touchinject declares a new one).  A tap on the page's title comes first: the first finger after a touch
#    screen appears may reach Settings as the pointer (before it took wl_touch) and does nothing there.
guest "$stop_all" >/dev/null
guest "$start_desktop" >/dev/null
wait_desktop
start_settings ""
shot touch-before.png
px=$((wx + 700)); py=$((wy + 560))
rx=$((wx + 120)); ry=$((wy + 27))
lx=$((wx + 120)); ly=$((wy + 450))
hx=$((wx + 500)); hy=$((wy + 40))
touch_replay gestures "size 1279 799 2
wait 2600
down 1 $hx $hy
wait 80
up 1
wait 3000
down 1 $px $py
swipe 0 -300 12 16
up 1
wait 3000
down 1 $rx $ry
wait 80
up 1
wait 3000
down 1 $lx $ly
swipe 0 -250 10 16
up 1
hold 3000"
expect_log /tmp/s.log 'ZSETTINGS TOUCH scroll start pane=page'
expect_log /tmp/s.log 'ZSETTINGS TOUCH scroll end pane=page scroll=[1-9]'
expect_log /tmp/s.log 'ZSETTINGS PAGE wifi$'
expect_log /tmp/s.log 'ZSETTINGS TOUCH scroll end pane=list scroll=[1-9]'
opened=$(guest "grep -c 'ZSETTINGS PAGE ' /tmp/s.log" | tail -1)
[ "${opened:-0}" = 2 ] && echo "drags: no page opened ok" || { echo "drags: $opened PAGE lines (home and wifi expected)"; status=1; }
shot touch-after.png

# 4. zdesktop saw no error.
errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; guest "grep ERROR /tmp/zdesktop.log | head -5"; status=1; }
guest 'cat /tmp/s.log' > "$out/settings.log"
guest "grep -E 'ZWL (TOUCH|INPUT|SEAT)' /tmp/zdesktop.log" > "$out/zdesktop-touch.log"
guest "$stop_all" >/dev/null
guest "rm -f $conf" >/dev/null
[ $status = 0 ] && echo "settings-p012: PASS" || echo "settings-p012: FAIL"
exit $status
