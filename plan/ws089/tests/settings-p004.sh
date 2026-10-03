#!/bin/sh
# ws089-p004: the look's pages of Settings on the Venus guest (the lean image with the generated wallpapers, build-settings-image.sh).  zdesktop --glass at 1280x800 with the session's wallpaper; Settings and zdesktop share
# root's home (/root/.config/keiland/desktop.conf, removed before and after).
#  1. Wallpaper: six tiles (Kei (default) and the five of generate.py) (wallpaper.png); a click on Aurora writes the key
#     (LOOK set key=wallpaper value=.../Aurora.ppm) and zdesktop shows it (ZWL PREFERENCES key=wallpaper applied)
#     (wallpaper-aurora.png); a click on the default removes the key and zdesktop goes back (wallpaper-default.png).
#  2. Appearance: the slider dragged to the left end writes window.opacity=85 and zdesktop applies it
#     (appearance-85.png); dragged to the right end removes the key (opacity 100 again).
#  3. Display (display.png) and Storage (storage.png) are shown; Home's tiles show the look's state (home.png).
#  4. No ERROR line in zdesktop's log.
#
#   plan/ws089/tests/settings-guest.sh start     (the guest must be up)
#   plan/ws089/tests/settings-p004.sh [OUTDIR]   (default build/ws089-shots/p004)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws089-run}"
export GUEST_RUNTIME
out=${1:-build/ws089-shots/p004}
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

# Drags the slider (control 1) from its middle to one end (left or right).
slide() {
	find_control 1
	if [ -z "$cx0" ]; then
		echo "slider: not found"
		status=1
		return
	fi
	cy=$((wy + cy0 + ch / 2)); start=$((wx + cx0 + cw / 2))
	end=$((wx + cx0 + 4))
	[ "$1" = right ] && end=$((wx + cx0 + cw - 4))
	pointer move "$start" "$cy" sleep 200 down sleep 100 move $(((start + end) / 2)) "$cy" sleep 100 move "$end" "$cy" sleep 200 up sleep 1500
}

# A picture of the screen with the pointer out of the way.
shot() {
	pointer move 1270 790 sleep 600
	check "$out/$1" >/dev/null
	echo "shot: $out/$1"
}

# 1. Wallpaper (once the guest answers, and zdesktop is ready).
wait_guest
guest "$stop_all" >/dev/null
guest "rm -f $conf" >/dev/null
guest "$start_desktop" >/dev/null
wait_desktop
start_settings wallpaper
expect_log /tmp/s.log 'ZSETTINGS LOOK pictures count=6'
# BUG-152: the small copies are read by a thread after the page is shown; the shot waits for every tile.
expect_log /tmp/s.log 'ZSETTINGS LOOK pictures ready count=6'
expect_log /tmp/s.log 'ZSETTINGS CONTROL index=102 '
shot wallpaper.png
control 101
expect_log /tmp/s.log 'ZSETTINGS LOOK set key=wallpaper value=/usr/share/keiland/wallpapers/Aurora.ppm error=0'
expect_log /tmp/zdesktop.log 'ZWL GLASS wallpaper path=/usr/share/keiland/wallpapers/Aurora.ppm'
guest "cat $conf"
shot wallpaper-aurora.png
control 100
expect_log /tmp/s.log 'ZSETTINGS LOOK set key=wallpaper value= error=0'
expect_log /tmp/zdesktop.log 'ZWL GLASS wallpaper path=/usr/share/keiland/wallpaper.ppm'
shot wallpaper-default.png

# 2. Appearance.
guest "pid=\$(ps -A -o pid,args | grep '[s]ettings' | awk '{print \$1}'); kill \$pid" >/dev/null
sleep 1
start_settings appearance
expect_log /tmp/s.log 'ZSETTINGS CONTROL index=1 '
slide left
expect_log /tmp/s.log 'ZSETTINGS LOOK set key=window.opacity value=85 error=0'
expect_log /tmp/zdesktop.log 'ZWL PREFERENCES key=window.opacity applied value=85'
shot appearance-85.png
slide right
expect_log /tmp/s.log 'ZSETTINGS LOOK set key=window.opacity value=100 error=0'
expect_log /tmp/zdesktop.log 'ZWL PREFERENCES key=window.opacity applied value=100'

# 3. Display, Storage and Home.
guest "pid=\$(ps -A -o pid,args | grep '[s]ettings' | awk '{print \$1}'); kill \$pid" >/dev/null
sleep 1
start_settings display
shot display.png
guest "pid=\$(ps -A -o pid,args | grep '[s]ettings' | awk '{print \$1}'); kill \$pid" >/dev/null
sleep 1
start_settings storage
shot storage.png
guest "pid=\$(ps -A -o pid,args | grep '[s]ettings' | awk '{print \$1}'); kill \$pid" >/dev/null
sleep 1
start_settings ""
shot home.png

# 4. zdesktop saw no error.
errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; guest "grep ERROR /tmp/zdesktop.log | head -5"; status=1; }
guest 'cat /tmp/s.log' > "$out/settings.log"
guest 'grep -E "PREFERENCES|GLASS wallpaper" /tmp/zdesktop.log' > "$out/zdesktop-preferences.log"
guest "$stop_all" >/dev/null
guest "rm -f $conf" >/dev/null
[ $status = 0 ] && echo "settings-p004: PASS" || echo "settings-p004: FAIL"
exit $status
