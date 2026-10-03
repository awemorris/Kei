#!/bin/sh
# ws071-p011: files from App Home on the Venus guest (the lean image, build-files-image.sh).
# zdesktop --glass at 1280x800 with no /etc/keiland/apps.conf (the built-in list):
#  1. home.png: the launcher opens Home with the built-in list, Files among them (at least 7 tiles; the list has
#     grown since ws071, e.g. 11 on 2026-10-03 with Lock Screen and Log Out, so ws127-p002 checks no exact count).
#  2. files.png: the Files icon starts files (HOME LAUNCH name=Files), which maps its window and gives
#     its titlebar the controls (ZWL TITLEBAR control ... where=floating id=1 ... shown=1).
#
#   plan/tools/files/files-guest.sh start     (the guest must be up)
#   plan/tools/files/files-p011.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws071-p011}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
. plan/tools/guest/zwl-clients.sh
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[f]iles" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[f]iles" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
status=0

# Fails the run unless zdesktop's log has a line matching a pattern (within a few seconds).
expect_log() {
	tries=0
	found=0
	while [ $tries -lt 8 ]; do
		found=$(guest "grep -cE '$1' /tmp/zdesktop.log" | tail -1)
		[ "${found:-0}" -gt 0 ] 2>/dev/null && break
		tries=$((tries + 1))
		sleep 1
	done
	if [ "${found:-0}" -gt 0 ] 2>/dev/null; then
		echo "log: $1 ok"
	else
		echo "log: $1 MISSING"
		status=1
	fi
}

# An icon's centre on Home as zdesktop logged it: "x y".
icon() {
	guest "grep 'ZWL HOME icon name=\"$1\"' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([0-9]*\) y=\([0-9]*\).*/\1 \2/p'
}

guest "$stop_all; rm -f /etc/keiland/apps.conf" >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0
picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=300 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4; echo started' >/dev/null

# 1. Home.
pointer move 23 17 sleep 300 down sleep 60 up sleep 1500
pointer move 700 500 sleep 400
check "$out/home.png" >/dev/null
expect_log 'ZWL HOME opened apps=([7-9]|[1-9][0-9]) '
expect_log 'ZWL HOME icon name="Files"'

# 2. Files.
set -- $(icon Files)
echo "Files icon at ${1:-?},${2:-?}"
pointer move ${1:-0} ${2:-0} sleep 400 down sleep 60 up sleep 8000
pointer move 1250 780 sleep 400
check "$out/files.png" >/dev/null
expect_log 'ZWL HOME launch name=Files pid='
zwl_app_clients
expect_log "ZWL MAP client=$zc1 "
expect_log "ZWL TITLEBAR control client=$zc1 .* where=floating id=1 .* shown=1"

errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
guest "$stop_all" >/dev/null
[ $status = 0 ] && echo "files-p011: PASS" || echo "files-p011: FAIL"
exit $status
