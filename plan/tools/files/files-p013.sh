#!/bin/sh
# ws071-p013: the tabs of files on the Venus guest (the lean image, build-files-image.sh).
# zdesktop --glass at 1280x800; files at 1000x640 on the sample home (/tmp/fhome), opened
# on Documents.  Ctrl+T, Ctrl+W and Ctrl+PageDown are the menus' shortcuts, so zdesktop takes them
# for the System Menu and they come back as menu actions; Ctrl+Tab stays the window's own key.
#  1. Ctrl+T: a second tab (TABS new index=1 count=2) and the tab bar, two.png.
#  2. Downloads clicked in the sidebar of the second tab; Ctrl+T again: three tabs, three.png.
#  3. A click on the first tab shows it (TABS select index=0); Ctrl+Tab the next (index=1);
#     Ctrl+PageDown the next again (index=2).
#  4. Ctrl+W closes the shown tab (TABS close index=2); the first tab's close button closes it
#     (TABS close index=0), closed.png.
#
#   plan/tools/files/files-guest.sh start     (the guest must be up)
#   plan/tools/files/files-p013.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws071-p013}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
. plan/tools/guest/zwl-clients.sh
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; sleep 0.7; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[f]iles|[t]erminal" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[f]iles|[t]erminal" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
status=0

# Fails the run unless a log has a line matching a pattern.
expect_log() {
	found=$(guest "grep -cE '$2' $1" | tail -1)
	if [ "${found:-0}" -gt 0 ] 2>/dev/null; then
		echo "log: $2 ok"
	else
		echo "log: $2 MISSING"
		status=1
	fi
}

# Clicks a point of the window's body (x, y from its top left) and waits.
click() {
	pointer move $((wx + $1 - 2)) $((wy + $2)) sleep 150 move $((wx + $1)) $((wy + $2)) sleep 300 down sleep 60 up sleep "${3:-700}"
}
shot() {
	pointer move 1270 790 sleep 400
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

# 1. A second tab.
keys '<ctrl-t>'
expect_log /tmp/f.log 'ZFILES TABS new index=1 count=2'
shot two.png

# 2. Downloads in the second tab (the sidebar keeps its place; the tabs are over the content), then a third tab.
click 100 155
expect_log /tmp/f.log 'ZFILES LOCATION kind=folder path=/tmp/fhome/Downloads '
keys '<ctrl-t>'
expect_log /tmp/f.log 'ZFILES TABS new index=2 count=3'
shot three.png

# 3. The first tab clicked, then Ctrl+Tab and Ctrl+PageDown.
click 350 15
expect_log /tmp/f.log 'ZFILES TABS select index=0 count=3'
keys '<ctrl-tab>'
expect_log /tmp/f.log 'ZFILES TABS select index=1 count=3'
keys '<ctrl-pgdn>'
expect_log /tmp/f.log 'ZFILES TABS select index=2 count=3'

# 4. Ctrl+W, then the first tab's close button.
keys '<ctrl-w>'
expect_log /tmp/f.log 'ZFILES TABS close index=2 count=2 shown=1'
click 594 15
expect_log /tmp/f.log 'ZFILES TABS close index=0 count=1 shown=0'
shot closed.png

errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
guest "$stop_all" >/dev/null
guest 'cat /tmp/f.log' > "$out/f.log"
[ $status = 0 ] && echo "files-p013: PASS" || echo "files-p013: FAIL"
exit $status
