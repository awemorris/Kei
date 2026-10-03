#!/bin/sh
# ws035-p091: the titles a shell sets (OSC 0 and 2) as terminal's tab titles and window title, on the
# Venus guest (the lean image, plan/tools/files/build-files-image.sh, or the browser image).  zdesktop --glass at
# 1280x800, terminal with its shell; plan/ws035/tests/p091-title.sh (copied to /tmp) sets the titles:
#  1. OSC 0 "Build logs" (BEL): the window's title (ZTERM TITLE tab=1 title=Build logs); one-tab.png.
#  2. Ctrl+Shift+T: tab 2 has no title of its own ("Shell 2"; the window's is "Terminal"); OSC 2 "日本語 notes"
#     (ESC \) there: its tab and the window take it; tabs.png shows both titles in the strip.
#  3. Tab 1 clicked: the window's title is tab 1's again.  OSC 2 of 150 bytes of "日": the title keeps 126
#     bytes (42 whole characters); OSC 1 (the icon's name) changes nothing; long.png.
#
#   plan/tools/files/files-guest.sh start     (the guest must be up)
#   plan/ws035/tests/zdesktop-p091.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws035-p091}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1 </dev/null; }
. plan/tools/guest/zwl-clients.sh
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; sleep 0.8; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[t]erminal" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[t]erminal" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
status=0

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

# Fails the run unless the terminal's latest title line matches a pattern (within a few seconds).
expect_title() {
	tries=0
	while [ $tries -lt 6 ]; do
		line=$(guest "grep 'ZTERM TITLE ' /tmp/t.log | tail -1")
		echo "$line" | grep -qE "$1" && break
		tries=$((tries + 1))
		sleep 1
	done
	if echo "$line" | grep -qE "$1"; then
		echo "title: $1 ok"
	else
		echo "title: $1 MISSING ($line)"
		status=1
	fi
}

tab_line() {
	zwl_app_clients
	guest "grep 'ZWL TITLEBAR strip client=$zc1 .* where=floating id=$1 ' /tmp/zdesktop.log | tail -1"
}
centre() {
	sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\) width=\([0-9]*\) height=\([0-9]*\).*/\1 \2 \3 \4/p' |
	    { read x y w h; echo "$(( ${x:-0} + ${w:-0} / 2 )) $(( ${y:-0} + ${h:-0} / 2 ))"; }
}
click() {
	pointer move $(($1 - 2)) "$2" sleep 150 move "$1" "$2" sleep 300 down sleep 60 up sleep "${3:-900}"
}
shot() {
	pointer move 1270 790 sleep 400
	check "$out/$1" >/dev/null
}

guest "$stop_all" >/dev/null
timeout 60 python3 plan/tools/guest/guest.py put plan/ws035/tests/p091-title.sh /tmp/p091-title.sh >/dev/null 2>&1 </dev/null
guest 'export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0; picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=600 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & for w in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30; do grep -q ZWL.READY /tmp/zdesktop.log 2>/dev/null && break; sleep 0.5; done; sleep 1
/bin/terminal --token=t1 --timeout-s=500 > /tmp/t.log 2>&1 </dev/null & sleep 6; echo started' >/dev/null

# 1. OSC 0 in the only tab: the window's title.
expect_log /tmp/t.log 'ZTERM TITLE tab=1 title=Terminal'
keys 'sh /tmp/p091-title.sh 1' '\n'
expect_title 'tab=1 title=Build logs$'
sleep 1
shot one-tab.png

# 2. A second tab, and OSC 2 (UTF-8, ended by ESC \) there.
keys '<ctrl-shift-t>'
expect_log /tmp/t.log 'ZTERM TABS count=2 active=2 mode=2'
expect_title 'tab=2 title=Terminal$'
keys 'sh /tmp/p091-title.sh 2' '\n'
expect_title 'tab=2 title=.+ notes$'
sleep 1
shot tabs.png

# 3. Tab 1 again; a title too long for the grid's store; OSC 1.
set -- $(tab_line 1 | centre); click "$1" "$2"
expect_title 'tab=1 title=Build logs$'
keys 'sh /tmp/p091-title.sh 3' '\n'
expect_title 'tab=1 title=[^B]'
bytes=$(guest "grep 'ZTERM TITLE tab=1' /tmp/t.log | tail -1" | tail -1 | LC_ALL=C sed 's/.*title=//' | tr -d '\n' | LC_ALL=C wc -c | tr -d ' ')
[ "$bytes" = 126 ] && echo "long title: 126 bytes ok" || { echo "long title: $bytes bytes MISSING"; status=1; }
before=$(guest "grep -c 'ZTERM TITLE ' /tmp/t.log" | tail -1)
keys 'sh /tmp/p091-title.sh 4' '\n'
sleep 4
after=$(guest "grep -c 'ZTERM TITLE ' /tmp/t.log" | tail -1)
[ "$before" = "$after" ] && echo "osc 1: no title ok" || { echo "osc 1: title changed MISSING"; status=1; }
shot long.png

errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
guest 'cat /tmp/t.log' > "$out/t.log"
guest 'grep -E "ZWL TITLEBAR (tab|strip)" /tmp/zdesktop.log' > "$out/zdesktop-tabs.log"
guest "$stop_all; rm -f /tmp/p091-title.sh" >/dev/null
[ $status = 0 ] && echo "zdesktop-p091: PASS" || echo "zdesktop-p091: FAIL"
exit $status
