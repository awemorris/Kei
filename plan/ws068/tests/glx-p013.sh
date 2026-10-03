#!/bin/sh
# ws068-p013: desktop OpenGL 3.0 through GLX on the Venus guest (the lean image, plan/ws068/tests/build-glsl-image.sh,
# or the zdesktop one; run by plan/ws035/tests/zdesktop-guest.sh start).  xserver runs under zdesktop;
# glxtest --gl3 makes its context with glXCreateContextAttribsARB (after checking that 3.3 is refused, that a
# glXCreateNewContext context reports 1.4 and that a forward-compatible 3.0 one reports its flag), checks
# OpenGL 3.0's calls at its start (GLXTEST GL3 check lines) and shows one square per outcome, green when it is the
# expected one, and a fixed-function square; the first frame is read back (EGLTEST CHECK) and the screen photographed.
#  1. gl3.png: the squares in the X window (a Wiseman window of 640x400).
#
#   plan/ws068/tests/glx-p013.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
export GUEST_RUNTIME="${GUEST_RUNTIME:-$PWD/build/ws068-run}"
out=${1:-build/ws068-p013}
mkdir -p "$out"
guest() { timeout 120 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
. plan/tools/guest/zwl-clients.sh
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
stop_all='for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[x]server|[g]lxtest|[e]gltest|[z]term" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[x]server|[g]lxtest|[e]gltest|[z]term" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
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

# The squares' colours on the screen, for a window at X,Y of W x H (screen coordinates, from the top): the eleven
# outcomes in a grid of four columns and the fixed-function square in the last cell, and the grey between them.
gl3_expect() {
	x=$1; y=$2; w=$3; h=$4
	for cell in 0 1 2 3 4 5 6 7 8 9 10 11; do
		column=$((cell % 4)); row=$((cell / 4))
		echo "--expect $((x + w * (125 + 250 * column) / 1000)),$((y + h * (17 + 33 * row) / 100)),00ff00"
	done
	echo "--expect $((x + w / 2)),$((y + h / 2)),202020"
}

# 1. zdesktop, xserver, glxtest --gl3.
guest "$stop_all" >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0 /tmp/.X11-unix/X0; /bin/wayland --timeout=600 --width=1280 --height=800 --glass --log-frames > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4
DISPLAY=:0 /bin/xserver > /tmp/x11server.log 2>&1 </dev/null & sleep 6
DISPLAY=:0 /bin/glxtest --gl3 --frames=600 --delay-ms=30 --token=g3 > /tmp/glx3.log 2>&1 </dev/null & i=0; while ! grep -q "EGLTEST CHECK" /tmp/glx3.log && [ $i -lt 60 ]; do sleep 1; i=$((i+1)); done; sleep 2; echo started' >/dev/null
zwl_app_clients
set -- $(guest "grep 'ZWL MAP client=$zc1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2/p')
wx=${1:-0}; wy=${2:-0}
echo "glxtest window at $wx,$wy"
pointer move 1250 780 sleep 400
check "$out/gl3.png" $(gl3_expect "$wx" "$wy" 640 400) || status=1
guest 'cat /tmp/glx3.log' > "$out/gl3.txt"
grep -vE "(PIXEL|check) .* ok$" "$out/gl3.txt"
expect_log /tmp/glx3.log 'GLXTEST GL3 contexts later-refused=1 legacy-version="1\.4 .*" legacy-major=1 forward-flags=1'
expect_log /tmp/glx3.log 'GLXTEST START run=g3 .* version="3\.0 .*" direct=1'
expect_log /tmp/glx3.log 'GLXTEST GL3 ready failures=0'
expect_log /tmp/glx3.log 'EGLTEST CHECK run=g3 failures=0 glerror=0x0'
guest "$stop_all" >/dev/null
[ $status -eq 0 ] && echo "glx-p013: PASS" || echo "glx-p013: FAIL"
exit $status
