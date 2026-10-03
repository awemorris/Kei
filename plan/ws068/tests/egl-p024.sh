#!/bin/sh
# ws068-p024: OpenGL ES 3.0's buffer, vertex array and uniform buffer API on the Venus guest (the zdesktop image,
# plan/ws035/tests/build-zdesktop-image.sh, or the lean one, plan/ws068/tests/build-glsl-image.sh; run by
# plan/ws035/tests/zdesktop-guest.sh start).
# egltest --scene=es3 (an OpenGL ES 3 context, GLSL ES 3.00) draws from four vertex array objects: A red
# (glDrawRangeElements, the colour index a glVertexAttribI4i current value), B green (an interleaved buffer, an
# unsigned byte index read by glVertexAttribIPointer), C blue and yellow (one triangle strip split by the fixed restart
# index, the gap between them the background), D four instances placed by gl_InstanceID (colours per instance, gains per
# two instances: magenta, cyan, 999999, 999900).  The colours come from a std140 uniform block bound with
# glBindBufferRange at the offset alignment (written through glMapBufferRange and glCopyBufferSubData) and a tint block
# at binding point 2 (glUniformBlockBinding); a uint uniform gates them.  The start checks what the API reports (EGLTEST
# ES3 check lines), the first frame is read back (EGLTEST CHECK) and the screen is photographed and read.
#  1. display.png: without zdesktop, the whole screen.
#  2. wayland.png: in a zdesktop --glass window of 640x400.
#
#   plan/ws068/tests/egl-p024.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
export GUEST_RUNTIME="${GUEST_RUNTIME:-$PWD/build/ws035-sq-run}"
out=${1:-build/ws068-p024}
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

# The shapes' colours on the screen, for a window at X,Y of W x H (screen coordinates, from the top).
es3_expect() {
	x=$1; y=$2; w=$3; h=$4
	echo "--expect $((x + w * 15 / 100)),$((y + h / 5)),ff0000" \
	     "--expect $((x + w * 40 / 100)),$((y + h / 5)),00ff00" \
	     "--expect $((x + w * 625 / 1000)),$((y + h / 5)),0000ff" \
	     "--expect $((x + w * 725 / 1000)),$((y + h / 5)),202020" \
	     "--expect $((x + w * 85 / 100)),$((y + h / 5)),ffff00" \
	     "--expect $((x + w * 1375 / 10000)),$((y + h * 4 / 5)),ff00ff" \
	     "--expect $((x + w * 3625 / 10000)),$((y + h * 4 / 5)),00ffff" \
	     "--expect $((x + w * 5875 / 10000)),$((y + h * 4 / 5)),999999" \
	     "--expect $((x + w * 8125 / 10000)),$((y + h * 4 / 5)),999900"
}

# 1. The whole screen, without a compositor.
guest "$stop_all" >/dev/null
guest '/bin/egltest --platform=display --scene=es3 --frames=200 --delay-ms=30 --token=d > /tmp/egl-d.log 2>&1 </dev/null & sleep 5; echo started' >/dev/null
check "$out/display.png" $(es3_expect 0 0 1280 800) || status=1
guest 'i=0; while ! grep -q EGLTEST.DONE /tmp/egl-d.log && [ $i -lt 60 ]; do sleep 1; i=$((i+1)); done; cat /tmp/egl-d.log' > "$out/display.txt"
cat "$out/display.txt"
expect_log /tmp/egl-d.log 'EGLTEST ES3 ready failures=0'
expect_log /tmp/egl-d.log 'EGLTEST CHECK run=d failures=0 glerror=0x0'
expect_log /tmp/egl-d.log 'EGLTEST DONE run=d frames=200 glerror=0x0 failures=0'

# 2. In a zdesktop window.
guest "$stop_all" >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0; /bin/wayland --timeout=600 --width=1280 --height=800 --glass --log-frames > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4
/bin/egltest --display=/tmp/wayland-0 --size=640x400 --scene=es3 --frames=300 --delay-ms=30 --token=w > /tmp/egl-w.log 2>&1 </dev/null & sleep 6; echo started' >/dev/null
zwl_app_clients
set -- $(guest "grep 'ZWL MAP client=$zc1 ' /tmp/zdesktop.log" | sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2/p')
wx=${1:-0}; wy=${2:-0}
pointer move 1250 780 sleep 400
check "$out/wayland.png" $(es3_expect "$wx" "$wy" 640 400) || status=1
guest 'cat /tmp/egl-w.log' > "$out/wayland.txt"
cat "$out/wayland.txt"
expect_log /tmp/egl-w.log 'EGLTEST ES3 ready failures=0'
expect_log /tmp/egl-w.log 'EGLTEST CHECK run=w failures=0 glerror=0x0'
guest "$stop_all" >/dev/null
[ $status -eq 0 ] && echo "egl-p024: PASS" || echo "egl-p024: FAIL"
exit $status
