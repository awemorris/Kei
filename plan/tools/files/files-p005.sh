#!/bin/sh
# ws071-p005: tags, search, recent files and the sidebar's favorites of files on the
# Venus guest (the lean image, build-files-image.sh).  zdesktop --glass at 1280x800;
# files at 1000x640 on the sample home (/tmp/fhome), opened on Documents.
#  1. tagged.png: Alt+1 and Alt+3 tag Report.pdf Work and Ideas (TAG lines; the dots on its icon).
#  2. tag.png: the sidebar's Work lists it (LOCATION kind=tag path=Work items=1).
#  3. search.png: "note" typed in the search field finds Meeting notes.txt (SEARCH done results=1);
#     the Computer chip searches the whole computer (a second SEARCH start with base=/).
#  4. Down and Enter open the result (OPEN), and the sidebar's Recents lists it (items=1).
#  (FILES_ON_UFS=1: the same on a home on the root UFS, linked from /tmp/fhome.)
#  5. favorite.png: Ctrl+L to Projects, Ctrl+Alt+T adds it to Favorites (FAVORITE add); its
#     row's button takes it off again (FAVORITE remove).
#
#   plan/tools/files/files-guest.sh start     (the guest must be up)
#   plan/tools/files/files-p005.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws071-p005}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
. plan/tools/guest/zwl-clients.sh
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[f]iles" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[f]iles" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
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

# Clicks a control of the window's titlebar (drawn by zdesktop; its place from zdesktop's log, ws071-p014).
control() {
	zwl_app_clients
	set -- $(guest "grep 'ZWL TITLEBAR control client=$zc1 .* where=floating id=$1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\) width=\([0-9]*\) height=\([0-9]*\).*/\1 \2 \3 \4/p')
	set -- $((${1:-0} + ${3:-0} / 2)) $((${2:-0} + ${4:-0} / 2))
	pointer move $(($1 - 2)) "$2" sleep 150 move "$1" "$2" sleep 300 down sleep 60 up sleep 900
}

guest "$stop_all" >/dev/null
# With FILES_ON_UFS=1 the sample home lives on the root file system (UFS) and /tmp/fhome links to it,
# so that the tags (extended attributes) are kept by UFS rather than tmpfs.
make_home='sh /usr/share/files-tests/make-home.sh /tmp/fhome >/dev/null'
[ "${FILES_ON_UFS:-0}" = 1 ] && make_home='rm -rf /fhome; sh /usr/share/files-tests/make-home.sh /fhome >/dev/null; ln -s /fhome /tmp/fhome'
guest "rm -f /tmp/wayland-0 /tmp/files.clipboard; rm -rf /tmp/fhome; $make_home"
guest 'export XDG_RUNTIME_DIR=/tmp
picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=900 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4
HOME=/tmp/fhome /bin/files --token=f1 --timeout-s=800 --width=1000 --height=640 /tmp/fhome/Documents > /tmp/f.log 2>&1 </dev/null & sleep 5; echo started' >/dev/null
zwl_app_clients
set -- $(guest "grep 'ZWL MAP client=$zc1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* surface=\([0-9]*\) x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2 \3/p')
surface=${1:-0}; wx=${2:-0}; wy=${3:-0}
echo "files: surface $surface at $wx,$wy"
expect_log /tmp/f.log 'ZFILES LOCATION kind=folder path=/tmp/fhome/Documents items=6 error=0'

# 1. Tags.
click 779 120
keys '<alt-1>'
sleep 0.8
keys '<alt-3>'
sleep 0.8
expect_log /tmp/f.log 'ZFILES TAG tag=Work on=1 items=1'
expect_log /tmp/f.log 'ZFILES TAG tag=Ideas on=1 items=1'
shot tagged.png

# 2. The tag's place.
click 100 441
expect_log /tmp/f.log 'ZFILES LOCATION kind=tag path=Work items=1 error=0'
shot tag.png

# 3. Search, then the whole computer.
control 5
keys 'note'
sleep 2
expect_log /tmp/f.log 'ZFILES SEARCH done query=note results=1 '
shot search.png
click 922 45 1500
expect_log /tmp/f.log 'ZFILES SEARCH start query=note base=/$'
sleep 3

# 4. Open the result; Recents lists it.
keys '<ret>' '<down>' '<ret>'
sleep 1
expect_log /tmp/f.log 'ZFILES OPEN path=.*/Documents/Meeting notes.txt'
click 100 313
expect_log /tmp/f.log 'ZFILES LOCATION kind=recents path= items=1 error=0'

# 5. A favorite added and taken off.
keys '<ctrl-l>'
sleep 0.5
keys '<ctrl-a>' '/tmp/fhome/Projects' '<ret>'
sleep 1
keys '<ctrl-alt-t>'
sleep 1
expect_log /tmp/f.log 'ZFILES FAVORITE add path=/tmp/fhome/Projects'
pointer move $((wx + 100)) $((wy + 263)) sleep 500
check "$out/favorite.png" >/dev/null
click 188 263
expect_log /tmp/f.log 'ZFILES FAVORITE remove path=/tmp/fhome/Projects'

errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
guest "$stop_all" >/dev/null
guest 'cat /tmp/f.log' > "$out/f.log"
[ $status = 0 ] && echo "files-p005: PASS" || echo "files-p005: FAIL"
exit $status
