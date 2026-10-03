#!/bin/sh
# ws089-p006: the demonstration's walk through Settings on the Venus guest (the lean image, build-settings-image.sh).
# zdesktop --glass at 1280x800 with the session's wallpaper; root's home.  The test waits for the guest's SSH and for
# zdesktop's READY first (settings-wait.sh).
#  1. App Home: the Settings tile (the cog) is there; a click starts Settings (ZWL HOME launch name=Settings) and its
#     window maps (apphome.png, launched.png).
#  2. Every page in the list's order: Settings at Home, then Down key by key through all 23 pages, a picture of each
#     (NN-WORD.png) and the page's line in the log (ZSETTINGS PAGE WORD).
#  3. The search: Ctrl+F, "wall", Enter opens Wallpaper.  Back (Alt+Left) twice goes back through the history.
#  4. No ERROR line in zdesktop's log.
#
#   plan/ws089/tests/settings-guest.sh start     (the guest may still be booting)
#   plan/ws089/tests/settings-p006.sh [OUTDIR]   (default build/ws089-shots/p006)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws089-run}"
export GUEST_RUNTIME
out=${1:-build/ws089-shots/p006}
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

keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; sleep 0.7; }

# 1. App Home.
wait_guest
guest "$stop_all" >/dev/null
guest "rm -f $conf" >/dev/null
guest "$start_desktop" >/dev/null
wait_desktop
pointer move 23 17 sleep 300 down sleep 60 up sleep 1500
expect_log /tmp/zdesktop.log 'ZWL HOME icon name="Settings"'
shot apphome.png
set -- $(guest "grep 'ZWL HOME icon name=\"Settings\"' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([0-9]*\) y=\([0-9]*\).*/\1 \2/p')
if [ -n "${1:-}" ]; then
	pointer move "$1" "$2" sleep 400 down sleep 60 up sleep 3000
else
	echo "no Settings icon"
	status=1
fi
expect_log /tmp/zdesktop.log 'ZWL HOME launch name=Settings pid=[0-9]+'
expect_log /tmp/zdesktop.log 'ZWL MAP client=[0-9]+ '
shot launched.png
guest "pid=\$(ps -A -o pid,args | grep '[s]ettings' | awk '{print \$1}'); kill \$pid" >/dev/null
sleep 1

# 2. Every page, from Home down the list.
start_settings ""
expect_log /tmp/s.log 'ZSETTINGS PAGE home'
pointer move $((wx + 700)) $((wy + 60)) sleep 200
shot 00-home.png
number=1
while [ $number -le 23 ]; do
	keys '<down>'
	word=$(guest "grep 'ZSETTINGS PAGE ' /tmp/s.log | tail -1" | sed -n 's/.*ZSETTINGS PAGE \([a-z]*\).*/\1/p' | tail -1)
	name=$(printf '%02d-%s.png' "$number" "${word:-unknown}")
	shot "$name"
	number=$((number + 1))
done
expect_log /tmp/s.log 'ZSETTINGS PAGE about'
pages=$(guest "grep -c 'ZSETTINGS PAGE ' /tmp/s.log" | tail -1)
echo "pages shown: $pages"
[ "${pages:-0}" -ge 24 ] 2>/dev/null || { echo "pages: fewer than 24"; status=1; }

# 3. The search, and Back.
keys '<ctrl-f>'
keys 'wall'
expect_log /tmp/s.log 'ZSETTINGS SEARCH query=wall results=[1-9]'
shot search-wall.png
keys '<ret>'
expect_log /tmp/s.log 'ZSETTINGS SEARCH open page=wallpaper'
keys '<alt-left>'
expect_log /tmp/s.log 'ZSETTINGS PAGE about back'
shot back.png

# 4. zdesktop saw no error.
errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; guest "grep ERROR /tmp/zdesktop.log | head -5"; status=1; }
guest 'cat /tmp/s.log' > "$out/settings.log"
guest "$stop_all" >/dev/null
guest "rm -f $conf" >/dev/null
[ $status = 0 ] && echo "settings-p006: PASS" || echo "settings-p006: FAIL"
exit $status
