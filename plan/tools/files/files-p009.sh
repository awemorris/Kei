#!/bin/sh
# ws071-p009: the context menus of files on the Venus guest (the lean image, build-files-image.sh):
# WS070's System Menu protocol version 2 (xdg_menu_manager_v1.get_context_menu, xdg_context_menu_v1) and
# libkeiland's keiland_menu_popup.  zdesktop --glass at 1280x800; files at 1000x640 on the
# sample home (/tmp/fhome), opened on Documents.
#  1. A right click on Budget.csv: zdesktop opens the context menu at the click (ZWL MENU context), items.png;
#     Get Info chosen: the action comes back (CONTEXT-MENU item=1004 action=4), the information opens,
#     and the context menu is done.
#  2. A right click on the empty part: the empty menu; View > as List chosen through the submenu
#     (action 16), empty.png with the submenu open.
#  3. A right click on Downloads in the sidebar: Open in New Tab (action 49) makes a second tab.
#  4. A right click, then Esc: the context menu closes without a choice (done).
#
#   plan/tools/files/files-guest.sh start     (the guest must be up)
#   plan/tools/files/files-p009.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws071-p009}
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

# The middle (y) of the latest popup row of an item, and the latest popup's left edge (depth 1 or 2).
row_y() {
	guest "grep 'MENU row item=$1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* y=\([0-9]*\) height=\([0-9]*\).*/\1 \2/p' | { read y h; echo $(( ${y:-0} + ${h:-0} / 2 )); }
}
popup_x() {
	guest "grep 'MENU open .* depth=${1:-0} ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([0-9]*\) y=.*/\1/p'
}

# Clicks a screen point; rclick right-clicks a point of the window's body (from its top left).
click() {
	pointer move $(($1 - 2)) "$2" sleep 150 move "$1" "$2" sleep 300 down sleep 60 up sleep "${3:-700}"
}
rclick() {
	pointer move $((wx + $1 - 2)) $((wy + $2)) sleep 150 move $((wx + $1)) $((wy + $2)) sleep 300 right-down sleep 60 right-up sleep 900
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
expect_log /tmp/f.log 'ZFILES LOCATION kind=folder path=/tmp/fhome/Documents items=6 error=0'

# 1. Budget.csv: the items' menu, and Get Info.
rclick 446 140
expect_log /tmp/f.log 'ZFILES CONTEXT-MENU open rows=[0-9]+ x=446 y=140 '
expect_log /tmp/zdesktop.log "ZWL MENU context client=$zc1 context=[0-9]+ surface=$surface x=$((wx + 446)) y=$((wy + 140)) rows="
expect_log /tmp/zdesktop.log 'ZWL MENU row item=1004 depth=1 '
shot items.png $((wx + 446)) $((wy + 140))
click $(( $(popup_x 1) + 60 )) "$(row_y 1004)" 1200
expect_log /tmp/zdesktop.log "ZWL MENU context-activate client=$zc1 context=[0-9]+ item=1004 action=4 "
expect_log /tmp/f.log 'ZFILES CONTEXT-MENU item=1004 action=4 '
expect_log /tmp/f.log 'ZFILES INFO path=/tmp/fhome/Documents/Budget.csv '
expect_log /tmp/f.log 'ZFILES CONTEXT-MENU done'
keys '<esc>'

# 2. The empty part: View > as List.
rclick 700 520
expect_log /tmp/zdesktop.log 'ZWL MENU row item=102 depth=1 '
click $(( $(popup_x 1) + 60 )) "$(row_y 102)" 900
expect_log /tmp/zdesktop.log 'ZWL MENU row item=1016 depth=2 '
shot empty.png $(( $(popup_x 2) + 60 )) "$(row_y 1016)"
click $(( $(popup_x 2) + 60 )) "$(row_y 1016)" 1200
expect_log /tmp/f.log 'ZFILES CONTEXT-MENU item=1016 action=16 '
expect_log /tmp/f.log 'ZFILES ACTION action=16'

# 3. Downloads in the sidebar: Open in New Tab.
rclick 100 155
expect_log /tmp/zdesktop.log 'ZWL MENU row item=1049 depth=1 '
click $(( $(popup_x 1) + 60 )) "$(row_y 1049)" 1200
expect_log /tmp/f.log 'ZFILES TABS new index=1 count=2'
expect_log /tmp/f.log 'ZFILES LOCATION kind=folder path=/tmp/fhome/Downloads '

# 4. A right click, then Esc: done without a choice.
before=$(guest "grep -c 'ZWL MENU context-done' /tmp/zdesktop.log" | tail -1)
rclick 700 520
keys '<esc>'
after=$(guest "grep -c 'ZWL MENU context-done' /tmp/zdesktop.log" | tail -1)
[ "${after:-0}" -gt "${before:-0}" ] 2>/dev/null && echo "esc: done ok" || { echo "esc: done MISSING ($before -> $after)"; status=1; }

errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
guest "$stop_all" >/dev/null
guest 'cat /tmp/f.log' > "$out/f.log"
guest 'grep "MENU" /tmp/zdesktop.log' > "$out/zdesktop-menu.log"
[ $status = 0 ] && echo "files-p009: PASS" || echo "files-p009: FAIL"
exit $status
