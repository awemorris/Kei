#!/bin/sh
# ws068-p023: cube maps on the Venus guest (the zdesktop image, plan/ws035/tests/build-zdesktop-image.sh, run by
# plan/ws035/tests/zdesktop-guest.sh start).
# egltest --scene=cube draws six squares, each sampling one axis direction of a cube map whose faces are one colour
# each: +X red, -X green, +Y blue from glTexImage2D; -Y yellow, +Z magenta cleared as a framebuffer object's
# attachment; -Z cyan copied with glCopyTexSubImage2D from a framebuffer object's 2D texture; then glGenerateMipmap
# (which reads the drawn faces back) and a mipmapping filter.  The first frame is read back (EGLTEST CHECK) and the
# screen is photographed and read.
#  1. display.png: without zdesktop, the whole screen.
#  2. wayland.png: in a zdesktop --glass window of 640x400.
#
#   plan/ws068/tests/egl-p023.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
export GUEST_RUNTIME="${GUEST_RUNTIME:-$PWD/build/ws035-sq-run}"
out=${1:-build/ws068-p023}
mkdir -p "$out"
guest() { timeout 120 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
. plan/tools/guest/zwl-clients.sh
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
stop_all='for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[e]gltest" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[e]gltest" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
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

# The squares' colours on the screen, for a window at X,Y of W x H (screen coordinates, from the top).
cube_expect() {
	x=$1; y=$2; w=$3; h=$4
	echo "--expect $((x + w / 5)),$((y + h / 4)),ff0000" \
	     "--expect $((x + w / 2)),$((y + h / 4)),00ff00" \
	     "--expect $((x + w * 4 / 5)),$((y + h / 4)),0000ff" \
	     "--expect $((x + w / 5)),$((y + h * 3 / 4)),ffff00" \
	     "--expect $((x + w / 2)),$((y + h * 3 / 4)),ff00ff" \
	     "--expect $((x + w * 4 / 5)),$((y + h * 3 / 4)),00ffff" \
	     "--expect $((x + w / 2)),$((y + h / 2)),202020"
}

# 1. The whole screen, without a compositor.
guest "$stop_all" >/dev/null
guest '/bin/egltest --platform=display --scene=cube --frames=200 --delay-ms=30 --token=d > /tmp/egl-d.log 2>&1 </dev/null & sleep 5; echo started' >/dev/null
check "$out/display.png" $(cube_expect 0 0 1280 800) || status=1
guest 'i=0; while ! grep -q EGLTEST.DONE /tmp/egl-d.log && [ $i -lt 60 ]; do sleep 1; i=$((i+1)); done; cat /tmp/egl-d.log' > "$out/display.txt"
cat "$out/display.txt"
expect_log /tmp/egl-d.log 'EGLTEST CUBE face 3 fbo status=0x8cd5'
expect_log /tmp/egl-d.log 'EGLTEST CUBE face 4 fbo status=0x8cd5'
expect_log /tmp/egl-d.log 'EGLTEST CUBE copy fbo status=0x8cd5'
expect_log /tmp/egl-d.log 'EGLTEST CUBE ready'
expect_log /tmp/egl-d.log 'EGLTEST CHECK run=d failures=0 glerror=0x0'
expect_log /tmp/egl-d.log 'EGLTEST DONE run=d frames=200 glerror=0x0 failures=0'

# 2. In a zdesktop window.
guest "$stop_all" >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0; /bin/wayland --timeout=600 --width=1280 --height=800 --glass --log-frames > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4
/bin/egltest --display=/tmp/wayland-0 --size=640x400 --scene=cube --frames=300 --delay-ms=30 --token=w > /tmp/egl-w.log 2>&1 </dev/null & sleep 6; echo started' >/dev/null
zwl_app_clients
set -- $(guest "grep 'ZWL MAP client=$zc1 ' /tmp/zdesktop.log" | sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2/p')
wx=${1:-0}; wy=${2:-0}
pointer move 1250 780 sleep 400
check "$out/wayland.png" $(cube_expect "$wx" "$wy" 640 400) || status=1
guest 'cat /tmp/egl-w.log' > "$out/wayland.txt"
cat "$out/wayland.txt"
expect_log /tmp/egl-w.log 'EGLTEST CHECK run=w failures=0 glerror=0x0'
guest "$stop_all" >/dev/null
[ $status -eq 0 ] && echo "egl-p023: PASS" || echo "egl-p023: FAIL"
exit $status
