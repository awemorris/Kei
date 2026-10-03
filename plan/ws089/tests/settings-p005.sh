#!/bin/sh
# ws089-p005: the input and sound pages of Settings on the Venus guest (the lean image, build-settings-image.sh).
# zdesktop --glass at 1280x800; Settings and zdesktop share root's home (/root/.config/keiland/desktop.conf, removed
# before and after).  The test waits for the guest's SSH and for zdesktop's READY first (settings-wait.sh).
#  1. Mouse: the speed's slider dragged to the right end writes pointer.speed=300 and zdesktop applies it; the switch
#     of natural scrolling writes pointer.natural=1 and zdesktop applies it (mouse.png); back to the middle (100: the
#     key removed) and the switch off.
#  2. Keyboard: the rate's slider to the right end (60) and the delay's to the left end (150) are written and
#     applied (keyboard.png).
#  3. Sound: the lean image has no audiod; the page says the service is not running (sound.png).  Home (home.png).
#  4. No ERROR line in zdesktop's log.
# The QEMU guest's pointer is a tablet (absolute), so the speed's effect on a relative mouse is not seen here.
#
#   plan/ws089/tests/settings-guest.sh start     (the guest may still be booting)
#   plan/ws089/tests/settings-p005.sh [OUTDIR]   (default build/ws089-shots/p005)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws089-run}"
export GUEST_RUNTIME
out=${1:-build/ws089-shots/p005}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1 </dev/null; }
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
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

# Finds a control's rectangle in window coordinates (the last one logged): sets cx0 cy0 cw ch.
find_control() {
	set -- $(guest "grep 'ZSETTINGS CONTROL index=$1 ' /tmp/s.log | tail -1" | sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\) width=\([0-9]*\) height=\([0-9]*\).*/\1 \2 \3 \4/p')
	cx0=${1:-}; cy0=${2:-}; cw=${3:-}; ch=${4:-}
}

# Clicks a control of the page by its index.
control() {
	find_control "$1"
	if [ -z "$cx0" ]; then
		echo "control $1: not found"
		status=1
		return
	fi
	cx=$((wx + cx0 + cw / 2)); cy=$((wy + cy0 + ch / 2))
	pointer move $((cx - 2)) "$cy" sleep 150 move "$cx" "$cy" sleep 300 down sleep 60 up sleep 1200
}

# Drags a slider (its control) from its middle to one end (left, right) or back to the middle (middle).
slide() {
	find_control "$1"
	if [ -z "$cx0" ]; then
		echo "slider: not found"
		status=1
		return
	fi
	cy=$((wy + cy0 + ch / 2)); start=$((wx + cx0 + cw / 2))
	end=$((wx + cx0 + 4))
	[ "$2" = right ] && end=$((wx + cx0 + cw - 4))
	[ "$2" = middle ] && { start=$((wx + cx0 + 4)); end=$((wx + cx0 + 12 + (cw - 24) * 75 / 275)); }
	pointer move "$start" "$cy" sleep 200 down sleep 100 move $(((start + end) / 2)) "$cy" sleep 100 move "$end" "$cy" sleep 200 up sleep 1500
}

# A picture of the screen with the pointer out of the way.
shot() {
	pointer move 1270 790 sleep 600
	check "$out/$1" >/dev/null
	echo "shot: $out/$1"
}

# 1. Mouse (once the guest answers, and zdesktop is ready).
wait_guest
guest "$stop_all" >/dev/null
guest "rm -f $conf" >/dev/null
guest "$start_desktop" >/dev/null
wait_desktop
start_settings mouse
expect_log /tmp/s.log 'ZSETTINGS CONTROL index=3 '
slide 2 right
expect_log /tmp/s.log 'ZSETTINGS LOOK set key=pointer.speed value=300 error=0'
expect_log /tmp/zdesktop.log 'ZWL PREFERENCES key=pointer.speed applied value=300'
control 3
expect_log /tmp/s.log 'ZSETTINGS LOOK set key=pointer.natural value=1 error=0'
expect_log /tmp/zdesktop.log 'ZWL PREFERENCES key=pointer.natural applied value=1'
guest "cat $conf"
shot mouse.png
slide 2 middle
expect_log /tmp/s.log 'ZSETTINGS LOOK set key=pointer.speed value=100 error=0'
expect_log /tmp/zdesktop.log 'ZWL PREFERENCES key=pointer.speed applied value=100'
control 3
expect_log /tmp/s.log 'ZSETTINGS LOOK set key=pointer.natural value=0 error=0'
expect_log /tmp/zdesktop.log 'ZWL PREFERENCES key=pointer.natural applied value=0'

# 2. Keyboard.
guest "pid=\$(ps -A -o pid,args | grep '[s]ettings' | awk '{print \$1}'); kill \$pid" >/dev/null
sleep 1
start_settings keyboard
expect_log /tmp/s.log 'ZSETTINGS CONTROL index=5 '
slide 4 right
expect_log /tmp/s.log 'ZSETTINGS LOOK set key=keyboard.repeat.rate value=60 error=0'
expect_log /tmp/zdesktop.log 'ZWL PREFERENCES key=keyboard.repeat.rate applied value=60'
slide 5 left
expect_log /tmp/s.log 'ZSETTINGS LOOK set key=keyboard.repeat.delay value=150 error=0'
expect_log /tmp/zdesktop.log 'ZWL PREFERENCES key=keyboard.repeat.delay applied value=150'
shot keyboard.png

# 3. Sound and Home.
guest "pid=\$(ps -A -o pid,args | grep '[s]ettings' | awk '{print \$1}'); kill \$pid" >/dev/null
sleep 1
running=$(guest 'ls -l /run/audiod.sock 2>/dev/null | grep -c "^s"' | tail -1)
echo "audiod socket: ${running:-0}"
start_settings sound
shot sound.png
guest "pid=\$(ps -A -o pid,args | grep '[s]ettings' | awk '{print \$1}'); kill \$pid" >/dev/null
sleep 1
start_settings ""
shot home.png

# 4. zdesktop saw no error.
errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; guest "grep ERROR /tmp/zdesktop.log | head -5"; status=1; }
guest 'cat /tmp/s.log' > "$out/settings.log"
guest 'grep -E "PREFERENCES" /tmp/zdesktop.log' > "$out/zdesktop-preferences.log"
guest "$stop_all" >/dev/null
guest "rm -f $conf" >/dev/null
[ $status = 0 ] && echo "settings-p005: PASS" || echo "settings-p005: FAIL"
exit $status
