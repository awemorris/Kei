#!/bin/sh
# ws035-p086: terminal's tabs in the titlebar's TABS mode, on the Venus guest (the lean image,
# plan/tools/files/build-files-image.sh).  zdesktop --glass at 1280x800, terminal with its shell:
#  1. The terminal binds xdg-shell 4 (ZWL BOUNDS client=1) and shows its menus with one tab (ZTERM TABS count=1
#     mode=0); "echo one" typed in it.
#  2. Ctrl+Shift+T (Shell > New Tab): a second shell (TAB new id=2 count=2), the titlebar shows the tabs
#     (TABS count=2 mode=2, the strip in zdesktop's log); "echo two" typed there; tabs.png.
#  3. Tab 1 clicked in the strip: it is active again and shows its own screen (tab1.png).  Ctrl+W typed there
#     is the shell's (no tab event in zdesktop's log).
#  4. Ctrl+Tab (zdesktop's tab key): tab 2 active.  "+" clicked: a third tab; its close button: closed.
#  5. Ctrl+Shift+W (Shell > Close Tab): back to one tab and the menus (TABS count=1 mode=0); one.png.
#  6. "exit" in the last tab ends the terminal (ZTERM DONE reason=shell-exited).
#
#   plan/tools/files/files-guest.sh start     (the guest must be up)
#   plan/ws035/tests/zdesktop-p086.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws035-p086}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
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

# The latest logged rectangle of a tab or of a strip button of client 1 (floating), and its centre.
tab_line() {
	zwl_app_clients
	guest "grep 'ZWL TITLEBAR strip client=$zc1 .* where=floating id=$1 ' /tmp/zdesktop.log | tail -1"
}
button_line() {
	zwl_app_clients
	guest "grep 'ZWL TITLEBAR strip client=$zc1 .* where=floating button=$1 ' /tmp/zdesktop.log | tail -1"
}
centre() {
	sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\) width=\([0-9]*\) height=\([0-9]*\).*/\1 \2 \3 \4/p' |
	    { read x y w h; echo "$(( ${x:-0} + ${w:-0} / 2 )) $(( ${y:-0} + ${h:-0} / 2 ))"; }
}
close_centre() {
	sed -n 's/.* y=\([-0-9]*\) width=[0-9]* height=\([0-9]*\) .* close=\([-0-9]*\).*/\3 \1 \2/p' |
	    { read c y h; echo "$(( ${c:-0} + 10 )) $(( ${y:-0} + ${h:-0} / 2 ))"; }
}
click() {
	pointer move $(($1 - 2)) "$2" sleep 150 move "$1" "$2" sleep 300 down sleep 60 up sleep "${3:-900}"
}
shot() {
	pointer move 1270 790 sleep 400
	check "$out/$1" >/dev/null
}

guest "$stop_all" >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0; picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=600 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & for w in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30; do grep -q ZWL.READY /tmp/zdesktop.log 2>/dev/null && break; sleep 0.5; done; sleep 1
/bin/terminal --token=t1 --timeout-s=500 > /tmp/t.log 2>&1 </dev/null & sleep 6; echo started' >/dev/null
zwl_app_clients
set -- $(guest "grep 'ZWL MAP client=$zc1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* surface=\([0-9]*\) x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2 \3/p')
wx=${2:-0}; wy=${3:-0}
echo "terminal at $wx,$wy"

# 1. One tab, the menus.
expect_log /tmp/zdesktop.log "ZWL BOUNDS client=$zc1 surface=[0-9]+ width=1256 height=690"
expect_log /tmp/t.log 'ZTERM TABS count=1 active=1 mode=0'
keys 'echo one' '\n'

# 2. A new tab.
keys '<ctrl-shift-t>'
expect_log /tmp/t.log 'ZTERM TAB new run=t1 id=2 count=2'
expect_log /tmp/t.log 'ZTERM TABS count=2 active=2 mode=2'
expect_log /tmp/zdesktop.log "ZWL TITLEBAR strip client=$zc1 .* where=floating id=2 .* shown=1 flags=5 "
keys 'echo two' '\n'
shot tabs.png

# 3. Tab 1 by the pointer; Ctrl+W is the shell's.
set -- $(tab_line 1 | centre); click "$1" "$2"
expect_log /tmp/t.log 'ZTERM TAB active id=1'
keys '<ctrl-w>'
closes=$(guest "grep -c 'ZWL TITLEBAR tab client=$zc1 .*event=close' /tmp/zdesktop.log" | tail -1)
[ "${closes:-1}" = 0 ] && echo "ctrl-w: the shell's ok" || { echo "ctrl-w: $closes tab closes MISSING"; status=1; }
shot tab1.png

# 4. Ctrl+Tab, "+", and the third tab's close button.
keys '<ctrl-tab>'
expect_log /tmp/zdesktop.log "ZWL TITLEBAR tab client=$zc1 id=2 event=activated"
expect_log /tmp/t.log 'ZTERM TABS count=2 active=2 mode=2'
set -- $(button_line new | centre); click "$1" "$2" 1500
expect_log /tmp/t.log 'ZTERM TAB new run=t1 id=3 count=3'
sleep 1
set -- $(tab_line 3 | close_centre); click "$1" "$2" 1500
expect_log /tmp/t.log 'ZTERM TAB closed run=t1 id=3 count=2'

# 5. Close Tab by its key: one tab and the menus again.
keys '<ctrl-shift-w>'
expect_log /tmp/t.log 'ZTERM TABS count=1 .* mode=0'
shot one.png

# 6. The last shell's exit ends the terminal.
keys 'exit' '\n'
expect_log /tmp/t.log 'ZTERM DONE run=t1 reason=shell-exited'

errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
guest 'cat /tmp/t.log' > "$out/t.log"
guest 'grep -E "ZWL (TITLEBAR (tab|strip)|BOUNDS)" /tmp/zdesktop.log' > "$out/zdesktop-tabs.log"
guest "$stop_all" >/dev/null
[ $status = 0 ] && echo "zdesktop-p086: PASS" || echo "zdesktop-p086: FAIL"
exit $status
