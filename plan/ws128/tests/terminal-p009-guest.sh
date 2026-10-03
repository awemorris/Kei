#!/bin/sh
# ws128-p009: Terminal's View > "Treat Ambiguous-Width Characters as Wide" on the Venus guest
# (the settings image, plan/ws089/tests/build-settings-image.sh).  zdesktop runs at 1280x800, the
# terminal (t1) shows plan/ws128/tests/terminal-p009-sample.txt (Ambiguous samples, a box drawn
# for wide Ambiguous characters, a Japanese line) and the screens are kept for reading:
#  1. off.png: the setting off (the default; no terminal.conf): the sample as before.
#  2. view-off.png: the View menu, the item unchecked.
#  3. The item chosen: the terminal logs the change and saves it (terminal.conf has ambiguous-wide=1).
#  4. on.png: the sample shown again under the first one: the old lines unchanged, the new ones wide.
#  5. view-on.png: the View menu, the item checked.
#  6. tab.png: a new tab (Ctrl+Shift+T) shows the sample wide.
#  7. restart.png: the terminal ended and started again (t2): the setting read back, the sample wide.
#  8. The item chosen again in t2: off, saved (ambiguous-wide=0).
#
#   GUEST_RUNTIME=... plan/ws035/tests/zdesktop-guest.sh start IMAGE   (the guest must be up)
#   GUEST_RUNTIME=... plan/ws128/tests/terminal-p009-guest.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws128-p009-run}"
export GUEST_RUNTIME
out=${1:-build/ws128-p009-guest}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
. plan/tools/guest/zwl-clients.sh
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
stop_all='for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[t]erminal" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[t]erminal" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
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

# Fails the run unless the guest's terminal.conf is exactly a text.
expect_conf() {
	got=$(guest 'cat $HOME/.config/keiland/terminal.conf' | tail -1)
	if [ "$got" = "$1" ]; then
		echo "terminal.conf: $1 ok"
	else
		echo "terminal.conf: '$got' (want $1) MISSING"
		status=1
	fi
}

# The centre (x) of a top-level item of a client's floating bar.
item_x() {
	guest "grep 'MENU bar client=$(zwl_app_client $1) .* where=floating item=$2 ' /tmp/zdesktop.log | tail -1" |
	    sed -n 's/.* offset=\([-0-9]*\) top=[-0-9]* width=\([0-9]*\).*/\1 \2/p' | { read offset width; echo $(( ${3:-0} + ${offset:-0} + ${width:-0} / 2 )); }
}

# The middle (y) of the latest popup row of an item, and the latest popup's left edge.
row_y() {
	guest "grep 'MENU row item=$1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* y=\([0-9]*\) height=\([0-9]*\).*/\1 \2/p' | { read y h; echo $(( ${y:-0} + ${h:-0} / 2 )); }
}
popup_x() {
	guest "grep 'MENU open ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([0-9]*\) y=.*/\1/p'
}

# Clicks a point.
click() {
	pointer move $(($1 - 2)) "$2" sleep 150 move "$1" "$2" sleep 300 down sleep 60 up sleep "${3:-700}"
}

# Starts a terminal with a token and finds its window (client number $2).
start_terminal() {
	guest "export XDG_RUNTIME_DIR=/tmp; /bin/terminal --token=$1 --timeout-s=600 >> /tmp/t.log 2>&1 </dev/null & sleep 6; echo started" >/dev/null
	set -- $(guest "grep 'ZWL MAP client=$(zwl_app_client $2) ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* surface=\([0-9]*\) x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2 \3/p')
	wx=${2:-0}; wy=${3:-0}; bar=$((wy - 30))
	echo "terminal: surface ${1:-0} at $wx,$wy"
}

# Opens View of a client's bar and hovers away from the rows, for a picture.
view_menu() {
	click "$(item_x "$1" 3 $wx)" $bar
	sleep 0.5
}

guest "$stop_all" >/dev/null
timeout 60 python3 plan/tools/guest/guest.py put plan/ws128/tests/terminal-p009-sample.txt /tmp/p009.txt >/dev/null
guest 'rm -f $HOME/.config/keiland/terminal.conf; rm -f /tmp/t.log' >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0; picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=900 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4; echo started' >/dev/null
start_terminal t1 1

# 1. Off: the default.
expect_log /tmp/t.log 'ZTERM SETTINGS run=t1 ambiguous_wide=0'
expect_log /tmp/t.log 'ZTERM MENU ready items=31'
expect_log /tmp/t.log 'ZTERM MENU state .* ambiguous_wide=0'
keys 'cat /tmp/p009.txt' '\n'
sleep 1.5
pointer move 1250 780 sleep 400
check "$out/off.png" >/dev/null

# 2. View, unchecked.
view_menu 1
check "$out/view-off.png" >/dev/null
expect_log /tmp/zdesktop.log 'MENU row item=36 '

# 3. The item chosen.
click $(( $(popup_x) + 60 )) "$(row_y 36)" 1200
zwl_app_clients
expect_log /tmp/zdesktop.log "MENU activate client=$zc1 place=[0-9]+ item=36 action=21 .*via=pointer"
expect_log /tmp/t.log 'ZTERM AMBIGUOUS run=t1 wide=1 saved=0'
expect_log /tmp/t.log 'ZTERM MENU state .* ambiguous_wide=1'
expect_conf 'ambiguous-wide=1'

# 4. The sample again: the old lines as they were, the new ones wide.
keys 'cat /tmp/p009.txt' '\n'
sleep 1.5
pointer move 1250 780 sleep 400
check "$out/on.png" >/dev/null

# 5. View, checked.
view_menu 1
check "$out/view-on.png" >/dev/null
keys '<esc>'
sleep 0.5

# 6. A new tab follows the setting.
keys '<ctrl-shift-t>'
sleep 2
keys 'cat /tmp/p009.txt' '\n'
sleep 1.5
pointer move 1250 780 sleep 400
check "$out/tab.png" >/dev/null
expect_log /tmp/t.log 'ZTERM TAB new run=t1 id=2'

# 7. The terminal ends and starts again with the setting.
keys '<ctrl-shift-q>'
sleep 2
expect_log /tmp/t.log 'ZTERM DONE run=t1 reason=menu-close'
start_terminal t2 2
expect_log /tmp/t.log 'ZTERM SETTINGS run=t2 ambiguous_wide=1'
expect_log /tmp/t.log 'ZTERM MENU state .* ambiguous_wide=1'
keys 'cat /tmp/p009.txt' '\n'
sleep 1.5
pointer move 1250 780 sleep 400
check "$out/restart.png" >/dev/null

# 8. Off again from t2's menu.
view_menu 2
click $(( $(popup_x) + 60 )) "$(row_y 36)" 1200
expect_log /tmp/t.log 'ZTERM AMBIGUOUS run=t2 wide=0 saved=0'
expect_conf 'ambiguous-wide=0'
keys 'cat /tmp/p009.txt' '\n'
sleep 1.5
pointer move 1250 780 sleep 400
check "$out/off-again.png" >/dev/null
keys '<ctrl-shift-q>'
sleep 2
expect_log /tmp/t.log 'ZTERM DONE run=t2 reason=menu-close'

guest 'grep -E "FAILED|ERROR" /tmp/t.log /tmp/zdesktop.log' | tee "$out/errors.txt"
[ -s "$out/errors.txt" ] && status=1
guest 'cat /tmp/t.log' > "$out/t.log"
guest 'grep MENU /tmp/zdesktop.log' > "$out/zdesktop-menu.log"
guest "$stop_all" >/dev/null
[ $status -eq 0 ] && echo "terminal-p009-guest: PASS (and judge the screens)" || echo "terminal-p009-guest: FAIL"
exit $status
