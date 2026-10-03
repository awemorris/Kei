#!/bin/sh
# ws035-p105 (F-044): mview and xserver's windows honor xdg-shell's configure_bounds (version 4), as files and
# terminal do.  On the Venus guest (the lean files image, plan/tools/files/build-files-image.sh), zdesktop --glass
# at 1280x800 tells a window the space for its body (1256x690):
#  1. mview.png: mview --windowed --size=1600x1000 takes the bounds (MVIEW WINDOW ... width=1256 height=690) and is
#     seen whole under its title bar.
#  2. zterm.png: zterm -geometry 300x100 on xserver (root 1280x800) makes an X window larger than the bounds;
#     (zterm keeps it within the root: 1240x720); xserver cuts it to the bounds (X11SERVER BOUNDS ... width=1240
#     height=690 was=1240x720) and zterm is seen whole.
#  3. small.png: an mview smaller than the bounds keeps its own size (640x460).
#
#   GUEST_RUNTIME=... plan/tools/files/files-guest.sh start build/<x>/hdd-image.img
#   plan/ws035/tests/zdesktop-p105.sh [OUTDIR] [SHOTS PREFIX]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws035-p105}
prefix=${2:-}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1 </dev/null; }
. plan/tools/guest/zwl-clients.sh
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[m]view|[x]server|[z]term" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[m]view|[x]server|[z]term" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
start='export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0
picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=400 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & for w in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30; do grep -q ZWL.READY /tmp/zdesktop.log 2>/dev/null && break; sleep 0.5; done; sleep 1; echo started'
status=0

# Fails the run unless a log has a line matching a pattern (within a few seconds).
expect_log() {
	tries=0
	found=0
	while [ $tries -lt 10 ]; do
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
shot() {
	pointer move 1270 790 sleep 500
	check "$out/$1" >/dev/null
	[ -n "$prefix" ] && cp "$out/$1" "$prefix$1"
}

# 1. mview larger than the bounds.
guest "$stop_all" >/dev/null
guest "$start" >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp; /bin/mview --windowed --size=1600x1000 --timeout-s=300 --token=big > /tmp/m1.log 2>&1 </dev/null & sleep 8; echo started' >/dev/null
zwl_app_clients
expect_log /tmp/zdesktop.log "ZWL BOUNDS client=$zc1 surface=[0-9]+ width=1256 height=690"
expect_log /tmp/m1.log 'MVIEW WINDOW run=big width=1256 height=690 bounds=1256x690'
shot mview.png
guest "$stop_all" >/dev/null

# 2. An X window larger than the bounds.
guest "$start" >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/.X11-unix/X0; DISPLAY=:0 /bin/xserver --size 1280x800 > /tmp/x11server.log 2>&1 </dev/null & sleep 3; DISPLAY=:0 /bin/zterm -geometry 300x100 > /tmp/zterm.log 2>&1 </dev/null & sleep 6; echo started' >/dev/null
expect_log /tmp/x11server.log 'X11SERVER BOUNDS window=0x[0-9a-f]+ width=1240 height=690 was=1240x720'
zwl_app_clients
expect_log /tmp/zdesktop.log "ZWL BOUNDS client=$zc1 surface=[0-9]+ width=1256 height=690"
shot zterm.png
guest "$stop_all" >/dev/null

# 3. mview smaller than the bounds keeps its size.
guest "$start" >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp; /bin/mview --windowed --size=640x460 --timeout-s=300 --token=small > /tmp/m2.log 2>&1 </dev/null & sleep 8; echo started' >/dev/null
expect_log /tmp/m2.log 'MVIEW WINDOW run=small width=640 height=460 bounds=1256x690'
shot small.png

errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
guest 'cat /tmp/x11server.log' > "$out/x11server.log"
guest "$stop_all" >/dev/null
[ $status = 0 ] && echo "zdesktop-p105: PASS" || echo "zdesktop-p105: FAIL"
exit $status
