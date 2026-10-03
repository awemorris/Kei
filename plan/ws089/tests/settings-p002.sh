#!/bin/sh
# ws089-p002: Settings' skeleton on the Venus guest (the lean image, build-settings-image.sh).
# zdesktop --glass at 1280x800 with the wallpaper and the demonstration's App Home (/etc/keiland/apps.conf).
#  1. home.png: settings opens on Home (READY glass=1, the titlebar's controls and the menus ready).
#  2. network.png: the list's Network row clicked (PAGE network; a page that is coming later).
#  3. The titlebar's Back and Forward walk the history (PAGE home back, PAGE network forward).
#  4. about.png: the Down key walks the list to About (PAGE about); the list scrolls it into sight.
#  5. nosidebar.png: the titlebar's Sidebar control hides the list (SIDEBAR shown=0), and shows it again.
#  6. The breadcrumb's first part (Settings) goes Home; a tile of Home opens its page (PAGE wifi).
#  7. apphome.png, apphome-settings.png: App Home lists Settings, and its icon starts it (HOME LAUNCH).
#  8. No ERROR line in zdesktop's log.
#
#   plan/ws089/tests/settings-guest.sh start     (the guest must be up)
#   plan/ws089/tests/settings-p002.sh [OUTDIR]   (default build/ws089-shots/p002)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws089-run}"
export GUEST_RUNTIME
out=${1:-build/ws089-shots/p002}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; sleep 0.7; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[s]ettings" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[s]ettings" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
status=0
. plan/ws089/tests/settings-wait.sh

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

# Clicks a point of the window's body (x, y from its top left) and waits.
click() {
	pointer move $((wx + $1 - 2)) $((wy + $2)) sleep 150 move $((wx + $1)) $((wy + $2)) sleep 300 down sleep 60 up sleep "${3:-800}"
}

# A titlebar control's place as zdesktop logged it: "x y width height".
place() {
	guest "grep 'ZWL TITLEBAR control client=$1 .* where=floating id=$2 ' /tmp/zdesktop.log | tail -1" |
	    sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\) width=\([0-9]*\) height=\([0-9]*\).*/\1 \2 \3 \4/p'
}

# Clicks a titlebar control of Settings' client (find_window), at its centre or at an offset from its left edge.
control() {
	set -- $(place "${wclient:-1}" "$1") "${2:-}"
	if [ -n "${5:-}" ]; then
		set -- $((${1:-0} + $5)) $((${2:-0} + ${4:-0} / 2))
	else
		set -- $((${1:-0} + ${3:-0} / 2)) $((${2:-0} + ${4:-0} / 2))
	fi
	pointer move $(($1 - 2)) "$2" sleep 150 move "$1" "$2" sleep 300 down sleep 60 up sleep 900
}

# A picture of the screen with the pointer out of the way.
shot() {
	pointer move 1270 790 sleep 500
	check "$out/$1" >/dev/null
	echo "shot: $out/$1"
}

guest "$stop_all" >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0
picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=900 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4
HOME=/root /bin/settings --timeout-s=800 > /tmp/s.log 2>&1 </dev/null & sleep 5; echo started' >/dev/null
find_window
echo "settings: window at $wx,$wy"

# 1. Home.
expect_log /tmp/s.log 'ZSETTINGS READY width=[0-9]+ height=[0-9]+ glass=1 page=home'
expect_log /tmp/s.log 'ZSETTINGS TITLEBAR ready controls=6'
expect_log /tmp/s.log 'ZSETTINGS MENU ready'
expect_log /tmp/s.log 'ZSETTINGS GLASS panels count=2'
guest "grep -E 'READY|ABOUT' /tmp/s.log"
shot home.png

# 2. Network from the list (the fifth row: 10 + 4 x 36 + 17).
click 120 171
expect_log /tmp/s.log 'ZSETTINGS PAGE network$'
expect_log /tmp/s.log 'ZSETTINGS TITLEBAR state back=1 forward=0 parts=2 last=Network'
shot network.png

# 3. Back and Forward.
control 1
expect_log /tmp/s.log 'ZSETTINGS PAGE home back'
control 2
expect_log /tmp/s.log 'ZSETTINGS PAGE network forward'

# 4. Down through the list to About (18 rows after Network).
keys '<down>' '<down>' '<down>' '<down>' '<down>' '<down>' '<down>' '<down>' '<down>' '<down>' '<down>' '<down>' '<down>' '<down>' '<down>' '<down>' '<down>' '<down>'
expect_log /tmp/s.log 'ZSETTINGS PAGE about$'
shot about.png

# 5. The list hidden and shown by the Sidebar control (ID 6 since ws089-p008 put Search at 5).
control 6
expect_log /tmp/s.log 'ZSETTINGS SIDEBAR shown=0'
shot nosidebar.png
control 6
expect_log /tmp/s.log 'ZSETTINGS SIDEBAR shown=1'

# 6. The breadcrumb's first part goes Home; Home's first tile (Wi-Fi) opens its page.
control 4 12
expect_log /tmp/s.log 'ZSETTINGS TITLEBAR activated id=4 detail=0'
expect_log /tmp/s.log 'ZSETTINGS TITLEBAR state back=1 forward=0 parts=1 last=Settings'
shot home-again.png
click 380 175
expect_log /tmp/s.log 'ZSETTINGS PAGE wifi$'
shot wifi.png

# 7. App Home: Settings is listed and starts from its icon.
guest "$stop_all" >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp HOME=/root; rm -f /tmp/wayland-0
picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=300 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4; echo started' >/dev/null
pointer move 23 17 sleep 300 down sleep 60 up sleep 1500
pointer move 700 500 sleep 400
check "$out/apphome.png" >/dev/null
expect_log /tmp/zdesktop.log 'ZWL HOME icon name="Settings"'
set -- $(guest "grep 'ZWL HOME icon name=\"Settings\"' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([0-9]*\) y=\([0-9]*\).*/\1 \2/p')
echo "Settings icon at ${1:-?},${2:-?}"
pointer move ${1:-0} ${2:-0} sleep 400 down sleep 60 up sleep 8000
shot apphome-settings.png
expect_log /tmp/zdesktop.log 'ZWL HOME launch name=Settings pid='
expect_log /tmp/zdesktop.log 'ZWL TITLEBAR control client=[0-9]+ .* where=floating id=1 .* shown=1'

# 8. zdesktop saw no error.
errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; guest "grep ERROR /tmp/zdesktop.log | head -5"; status=1; }
guest "$stop_all" >/dev/null
[ $status = 0 ] && echo "settings-p002: PASS" || echo "settings-p002: FAIL"
exit $status
