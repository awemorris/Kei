#!/bin/sh
# ws071-p008: the menus of files (the System Menu, WS070) on the Venus guest (the lean
# image, build-files-image.sh).  zdesktop --glass at 1280x800; files (f1) at 1000x640 on
# the sample home (/tmp/fhome), opened on Documents.
#  Since ws071-p014 the window's titlebar shows its controls, and File Edit View Go Window Help are
#  in the titlebar's "..." popup (WS070's decision A); menu N below opens "..." and then the menu.
#  1. floating.png: the controls and "..." in the floating title bar (MENU ready, TITLEBAR ready).
#  2. file-menu.png: File by the pointer (Open, Open With pale: nothing selected).
#  3. view-menu.png: View by the pointer; List chosen (ACTION 16, the list view).
#  4. Meeting notes.txt selected; open-with.png: File > Open With lists Record and the built-in ways.
#  5. Ctrl+I is Get Info's shortcut (zdesktop activates it, via=shortcut): the card; Esc.
#  6. tags-menu.png: Edit > Tags; Work chosen (TAG tag=Work on=1).
#  7. help-shortcuts.png: Help > Keyboard Shortcuts (HELP open card=2); Esc.
#  8. Ctrl+Shift+D goes to Desktop, Alt+Left back to Documents (both via the menus' shortcuts).
#  9. two-windows.png: Ctrl+N starts a second window (f1-new, READY) on Documents; its
#     Ctrl+Shift+W closes it (DONE reason=close), the first stays.
# 10. Ctrl+M minimizes the first window (WINDOW minimize).
#
#   plan/tools/files/files-guest.sh start     (the guest must be up)
#   plan/tools/files/files-p008.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws071-p008}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
. plan/tools/guest/zwl-clients.sh
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; sleep 0.8; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[f]iles|[t]erminal" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[f]iles|[t]erminal" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
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

# The middle (y) of the latest popup row of an item, and the latest popup's left edge.
row_y() {
	guest "grep 'MENU row item=$1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* y=\([0-9]*\) height=\([0-9]*\).*/\1 \2/p' | { read y h; echo $(( ${y:-0} + ${h:-0} / 2 )); }
}
popup_x() {
	guest "grep 'MENU open ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([0-9]*\) y=.*/\1/p'
}

# Clicks a screen point; wclick clicks a point of the window's body (from its top left).
click() {
	pointer move $(($1 - 2)) "$2" sleep 150 move "$1" "$2" sleep 300 down sleep 60 up sleep "${3:-700}"
}
wclick() {
	click $((wx + $1)) $((wy + $2)) "${3:-700}"
}

# Opens a top-level menu (its item) from the titlebar's "..." (its place from zdesktop's log).
menu() {
	item=$1
	zwl_app_clients
	set -- $(guest "grep 'ZWL TITLEBAR control client=$zc1 .* where=floating id=0 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\) width=\([0-9]*\) height=\([0-9]*\).*/\1 \2 \3 \4/p')
	click $((${1:-0} + ${3:-0} / 2)) $((${2:-0} + ${4:-0} / 2)) 900
	click $(( $(popup_x) + 60 )) "$(row_y "$item")" 900
}
shot() {
	pointer move ${2:-1270} ${3:-790} sleep 400
	check "$out/$1" >/dev/null
}

guest "$stop_all" >/dev/null
guest 'rm -f /tmp/wayland-0 /tmp/files.clipboard; rm -rf /tmp/fhome; sh /usr/share/files-tests/make-home.sh /tmp/fhome >/dev/null'
guest 'export XDG_RUNTIME_DIR=/tmp
picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=900 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4
HOME=/tmp/fhome /bin/files --token=f1 --timeout-s=800 --width=1000 --height=640 /tmp/fhome/Documents > /tmp/f.log 2>&1 </dev/null & sleep 6; echo started' >/dev/null
zwl_app_clients
set -- $(guest "grep 'ZWL MAP client=$zc1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* surface=\([0-9]*\) x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2 \3/p')
surface=${1:-0}; wx=${2:-0}; wy=${3:-0}
echo "files: surface $surface at $wx,$wy"

# 1. The controls and "..." in the floating title bar.
expect_log /tmp/f.log 'ZFILES MENU ready items='
expect_log /tmp/f.log 'ZFILES TITLEBAR ready controls='
expect_log /tmp/zdesktop.log "ZWL TITLEBAR control client=$zc1 surface=$surface where=floating id=0 "
shot floating.png

# 2. File by the pointer.
menu 1
expect_log /tmp/zdesktop.log "MENU open client=$zc1 surface=$surface item=1 depth="
shot file-menu.png
keys '<esc>' '<esc>'

# 3. View, then List.
menu 3
shot view-menu.png
click $(( $(popup_x) + 60 )) "$(row_y 1016)" 900
expect_log /tmp/f.log 'ZFILES ACTION action=16'

# 4. Meeting notes.txt (the third row), then File > Open With.
wclick 400 170
expect_log /tmp/f.log 'ZFILES MENU state selection=1 .* openers=[1-9]'
menu 1
click $(( $(popup_x) + 60 )) "$(row_y 10)" 900
shot open-with.png
keys '<esc>' '<esc>' '<esc>'

# 5. Get Info by its shortcut.
keys '<ctrl-i>'
expect_log /tmp/zdesktop.log "MENU activate client=$zc1 .*item=1004 action=4 .*via=shortcut"
expect_log /tmp/f.log 'ZFILES INFO path=/tmp/fhome/Documents/Meeting notes.txt '
keys '<esc>'
expect_log /tmp/f.log 'ZFILES INFO close'

# 6. Edit > Tags > Work.
menu 2
click $(( $(popup_x) + 60 )) "$(row_y 11)" 900
shot tags-menu.png
click $(( $(popup_x) + 60 )) "$(row_y 1300)" 900
expect_log /tmp/f.log 'ZFILES TAG tag=Work on=1 items=1'

# 7. Help > Keyboard Shortcuts.
menu 6
click $(( $(popup_x) + 60 )) "$(row_y 1039)" 900
expect_log /tmp/f.log 'ZFILES HELP open card=2'
shot help-shortcuts.png
keys '<esc>'
expect_log /tmp/f.log 'ZFILES HELP close'

# 8. Go > Desktop and Back by their shortcuts.
keys '<ctrl-shift-d>'
expect_log /tmp/f.log 'ZFILES LOCATION kind=folder path=/tmp/fhome/Desktop '
expect_log /tmp/zdesktop.log "MENU activate client=$zc1 .*item=1028 action=28 .*via=shortcut"
keys '<alt-left>'
back=$(guest "grep -c 'LOCATION kind=folder path=/tmp/fhome/Documents ' /tmp/f.log" | tail -1)
[ "${back:-0}" -ge 2 ] && echo "back: ok" || { echo "back: MISSING"; status=1; }

# 9. A second window, then closed by its own shortcut.
keys '<ctrl-n>'
expect_log /tmp/f.log 'ZFILES SPAWN program=/bin/files'
expect_log /tmp/f.log 'ZFILES READY .* token=f1-new'
expect_log /tmp/zdesktop.log "ZWL MAP client=$zc2 "
sleep 2
shot two-windows.png
keys '<ctrl-shift-w>'
expect_log /tmp/f.log 'ZFILES DONE reason=close'
windows=$(guest "i=0; while [ \$(ps -A -o args | grep -c '[f]iles') -gt 1 ] && [ \$i -lt 50 ]; do sleep 0.2; i=\$((i+1)); done; ps -A -o args | grep -c '[f]iles'" | tail -1)
[ "${windows:-0}" = 1 ] && echo "second window: closed, the first stays" || { echo "second window: $windows running"; status=1; }

# 10. Minimize the first window.
wclick 700 448
keys '<ctrl-m>'
expect_log /tmp/f.log 'ZFILES WINDOW minimize'
shot minimized.png

errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
guest "$stop_all" >/dev/null
guest 'cat /tmp/f.log' > "$out/f.log"
guest 'cat /tmp/zdesktop.log' > "$out/zdesktop.log"
[ $status = 0 ] && echo "files-p008: PASS" || echo "files-p008: FAIL"
exit $status
