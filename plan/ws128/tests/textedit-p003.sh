#!/bin/sh
# ws128-p003: Text Editor's Replace and Open Recent on a Venus guest of an image with the desktop's applications (for
# example plan/ws089/tests/build-settings-image.sh's).  zdesktop --glass at 1280x800, root's home.
#  1. Replace: /root/t.txt is "cat Cat dog cat"; Ctrl+H opens the panel (REPLACE open), "cat", Tab, "fox", Enter four
#     times (the first selects the first place, each next replaces it and selects the next), Esc, Ctrl+S: the file is
#     "fox fox dog fox" (read over SSH).  replace.png is the panel.
#  2. Replace All: Ctrl+H, the text to find "fox" and the replacement "ox", Tab Tab to Replace All, Enter
#     (REPLACE all count=3), Esc, Ctrl+S: "ox ox dog ox"; Ctrl+Z puts every place back in one step, Ctrl+S: "fox fox dog fox".
#  3. Open Recent: Text Editor quits and starts again without a file; its File > Open Recent lists /root/t.txt
#     (MENU recent count=N, N >= 1); F10 and Right open File, a click on Open Recent and on its first item opens it (RECENT open index=0, OPEN path=/root/t.txt).
#  4. No ERROR line in zdesktop's log.
#
#   plan/ws089/tests/settings-guest.sh start IMAGE
#   plan/ws128/tests/textedit-p003.sh [OUTDIR]   (default build/ws128-shots/p003)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws089-run}"
export GUEST_RUNTIME
out=${1:-build/ws128-shots/p003}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1 </dev/null; }
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@" >/dev/null; sleep 0.8; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[t]extedit" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[t]extedit" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
start_desktop='export XDG_RUNTIME_DIR=/tmp HOME=/root; rm -f /tmp/wayland-0
/bin/wayland --timeout=900 --width=1280 --height=800 --glass --wallpaper=/usr/share/keiland/wallpaper.ppm > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4; echo started'
status=0
. plan/ws089/tests/settings-wait.sh

# Fails the run unless a log has a line matching a pattern (within a few seconds).
expect_log() {
	tries=0
	found=0
	while [ $tries -lt 12 ]; do
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

# Fails the run unless the file holds a text (its first line), waiting a few seconds for the save.
expect_file() {
	tries=0
	have=
	while [ $tries -lt 8 ]; do
		have=$(guest "head -1 /root/t.txt" | tail -1)
		[ "$have" = "$1" ] && break
		tries=$((tries + 1))
		sleep 1
	done
	if [ "$have" = "$1" ]; then
		echo "file: \"$1\" ok"
	else
		echo "file: \"$have\" (expected \"$1\")"
		status=1
	fi
}

# The middle (y) of the latest popup row of a menu item, and the latest popup's left edge (zdesktop's log).
row_y() {
	guest "grep 'MENU row item=$1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* y=\([0-9]*\) height=\([0-9]*\).*/\1 \2/p' | { read y h; echo $(( ${y:-0} + ${h:-0} / 2 )); }
}
popup_x() {
	guest "grep 'MENU open ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([0-9]*\) y=.*/\1/p'
}

# Clicks a point.
click() {
	pointer move $(($1 - 2)) "$2" sleep 150 move "$1" "$2" sleep 300 down sleep 60 up sleep "${3:-900}"
}

# A picture of the screen with the pointer out of the way.
shot() {
	pointer move 1270 790 sleep 600
	check "$out/$1" >/dev/null
	echo "shot: $out/$1"
}

# Starts Text Editor (its log in /tmp/te.log, appended) on a file or none, and waits for its window.
start_textedit() {
	guest "export XDG_RUNTIME_DIR=/tmp HOME=/root; /bin/textedit $1 >> /tmp/te.log 2>&1 </dev/null & sleep 5; echo started" >/dev/null
	find_window
	echo "textedit: window at $wx,$wy"
	pointer move $((wx + 400)) $((wy + 300)) sleep 200 down sleep 60 up sleep 600
}

wait_guest
guest "$stop_all" >/dev/null
guest 'rm -f /tmp/te.log /root/.local/share/keiland/recent; printf "cat Cat dog cat\n" > /root/t.txt' >/dev/null
guest "$start_desktop" >/dev/null
wait_desktop

# 1. Replace.
start_textedit /root/t.txt
expect_log /tmp/te.log 'TEXTEDIT OPEN path=/root/t.txt'
keys '<ctrl-h>'
expect_log /tmp/te.log 'TEXTEDIT REPLACE open find=$'
keys 'cat'
keys '<tab>'
keys 'fox'
shot replace.png
keys '<ret>'
keys '<ret>'
keys '<ret>'
keys '<ret>'
replaced=$(guest "grep -c 'REPLACE one replaced=1' /tmp/te.log" | tail -1)
[ "${replaced:-0}" = 3 ] && echo "replace: 3 places ok" || { echo "replace: ${replaced:-0} places (expected 3)"; status=1; }
keys '<esc>'
expect_log /tmp/te.log 'TEXTEDIT REPLACE closed'
keys '<ctrl-s>'
expect_log /tmp/te.log 'TEXTEDIT SAVE path=/root/t.txt'
expect_file "fox fox dog fox"

# 2. Replace All, and one Undo.
keys '<ctrl-h>'
expect_log /tmp/te.log 'TEXTEDIT REPLACE open find=cat'
keys '<shift-tab>'
keys '<ctrl-a>'
keys 'fox'
keys '<tab>'
keys '<ctrl-a>'
keys 'ox'
keys '<tab>'
keys '<tab>'
keys '<ret>'
expect_log /tmp/te.log 'TEXTEDIT REPLACE all count=3'
shot replace-all.png
keys '<esc>'
keys '<ctrl-s>'
sleep 1
expect_file "ox ox dog ox"
keys '<ctrl-z>'
keys '<ctrl-s>'
sleep 1
expect_file "fox fox dog fox"

# 3. Open Recent, after a new start without a file.
keys '<ctrl-q>'
sleep 2
start_textedit ""
expect_log /tmp/te.log 'TEXTEDIT MENU recent count=[1-9]'
keys '<f10>'
keys '<right>'
click $(( $(popup_x) + 60 )) "$(row_y 18)"
shot recent-menu.png
click $(( $(popup_x) + 60 )) "$(row_y 70)"
expect_log /tmp/te.log 'TEXTEDIT RECENT open index=0 path=/root/t.txt'
expect_log /tmp/te.log 'TEXTEDIT OPEN path=/root/t.txt bytes=16'
shot recent-opened.png

# 4. zdesktop saw no error.
errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; guest "grep ERROR /tmp/zdesktop.log | head -5"; status=1; }
guest 'cat /tmp/te.log' > "$out/textedit.log"
guest "grep -E 'MENU' /tmp/zdesktop.log" > "$out/zdesktop-menu.log"
guest "$stop_all" >/dev/null
[ $status = 0 ] && echo "textedit-p003: PASS" || echo "textedit-p003: FAIL"
exit $status
