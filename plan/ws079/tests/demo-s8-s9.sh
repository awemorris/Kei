#!/bin/sh
# ws079-p016: the demonstration's S8 and S9 (plan/master.md, the demo script) on the Venus guest with the injected
# touch screen and pen (config-amd64-demo.mk), zdesktop --glass at 1280x800.  Pictures and program logs only (read
# over SSH); nothing reads the console.
#  S8  a finger's swipe from the top-right corner brings Notes fullscreen (NOTES START ... fullscreen=1); the pen
#      writes a stroke (NOTES STROKE); Esc leaves fullscreen for a window (s8-fullscreen.png, s8-written.png,
#      s8-window.png).
#  S9  PDF Viewer on the A4 document of ten pages (make-a4-document.sh):
#      a. the scroll mode, the Right key: nine pages on and one back (ten turns, "TURN done ... ms=");
#      b. the page mode, the finger: nine swipes on and one back (ten turns, their longest frame "frame_ms=");
#      c. a double tap zooms in (TOUCH double-tap zoom=), a pinch zooms (TOUCH pinch end scale=).
#      The longest turn of a (from the key to its page's frame shown) and the longest frame of b are compared with
#      TURN_LIMIT_MS (200: the WS079 L2 target).
#   plan/ws079/tests/demo-s8-s9.sh [IMAGE] [OUTDIR]   (IMAGE default: built with config-amd64-demo.mk into build/amd64)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
image=${1:-}
out=${2:-build/ws079-shots/p016}
limit=${TURN_LIMIT_MS:-200}
mkdir -p "$out"
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws079/demo-run}"
export GUEST_RUNTIME
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1 </dev/null; }
put() { timeout 60 python3 plan/tools/guest/guest.py put "$1" "$2" >/dev/null 2>&1 </dev/null; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; sleep 0.8; }
shot() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" move 1270 790 sleep 300 >/dev/null 2>&1; python3 plan/ws035/tests/zdesktop-check.py "$out/$1" --runtime "$GUEST_RUNTIME" >/dev/null 2>&1; echo "shot: $out/$1"; }
status=0

# Records a verdict.
verdict() {
	if [ "$1" = ok ]; then
		echo "$2 ok"
	else
		echo "$2 FAIL"
		status=1
	fi
}

# The number of lines of a guest file matching a pattern.
count() {
	guest "grep -cE '$2' $1" | tail -1
}

# Waits until a guest file has more than N lines matching a pattern (within some seconds).
expect_more() {
	tries=0
	found=0
	while [ $tries -lt "$4" ]; do
		found=$(count "$1" "$2")
		[ "${found:-0}" -gt "$3" ] 2>/dev/null && break
		tries=$((tries + 1))
		sleep 1
	done
	if [ "${found:-0}" -gt "$3" ] 2>/dev/null; then
		echo "log: $2 ok"
		return 0
	fi
	echo "log: $2 MISSING"
	status=1
	return 1
}

# Replays touch frames (commands separated by '|') on the 1280x800 output with touchinject.
touches() {
	printf 'size 1279 799 2\nwait 1500\n%s\n' "$1" | tr '|' '\n' > "$out/touch.script"
	put "$out/touch.script" /tmp/touch.script
	guest 'timeout 60 /bin/touchinject /tmp/touch.script; echo replay=$?' | grep -q '^replay=0$' || { echo "touchinject: FAILED"; status=1; }
}

# The largest number after KEY= in the last N lines of a guest file matching a pattern.
largest() {
	guest "grep -E '$2' $1 | tail -$3" | sed -n "s/.* $4=\([0-9]*\).*/\1/p" | sort -n | tail -1
}

# 0. The image, the guest, the document, and zdesktop.
if [ -z "$image" ]; then
	extra=$(python3 plan/tools/guest/guest.py extra-files | sed -n "s/^ZEDBSD_TEST_EXTRA_FILES='\(.*\)'$/\1/p")
	timeout 3000 make -j"$(nproc)" ZEDBSD_CONFIG=plan/ws079/tests/config-amd64-demo.mk BUILD=build/amd64 \
	    "ZEDBSD_TEST_EXTRA_FILES=$extra" disk-image > "$out/image.log" 2>&1 || { echo "image: FAIL"; exit 1; }
	mkdir -p build/ws079
	cp --reflink=auto build/amd64/hdd-image.img build/ws079/demo.img
	image=build/ws079/demo.img
fi
sh plan/ws079/tests/make-a4-document.sh "$out/a4.pdf" || exit 1
sh plan/ws035/tests/zdesktop-guest.sh stop >/dev/null 2>&1
timeout 180 sh plan/ws035/tests/zdesktop-guest.sh start "$image" >/dev/null 2>&1
# ws128-p001 (F-062): waits for the guest's SSH (at most three minutes) rather than a fixed 20 seconds, which a slower
# boot outran (every step then failed with MISSING).
timeout 200 python3 plan/tools/guest/guest.py wait --timeout 180 >/dev/null 2>&1 || { echo "guest: no answer"; status=1; }
put "$out/a4.pdf" /root/a4.pdf
guest 'service stop greeter >/dev/null 2>&1; rm -rf /root/Documents/Notes /root/.local/share/keiland/notes; export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0; picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=1200 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4; echo started' >/dev/null
expect_more /tmp/zdesktop.log 'ZWL READY' 0 20
expect_more /tmp/zdesktop.log 'ZWL CORNER zone' 0 20
sleep 3

# S8. The corner's swipe, the pen, Esc.
touches 'down 1 1272 6|swipe -160 160 8 30|up 1|wait 300'
expect_more /tmp/zdesktop.log 'NOTES START width=1280 height=800 fullscreen=1' 0 15
sleep 2
shot s8-fullscreen.png
python3 - "$out/pen.pen" <<'EOF'
import math, sys
def raw(x, y):
    return round(x * 21600 / 1279), round(y * 13500 / 799)
lines = ["size 21600 13500", "wait 2000", "tool pen"]
x, y = raw(400, 380)
lines += ["hover %d %d" % (x, y), "wait 200", "down %d %d 1500" % (x, y)]
for step in range(1, 61):
    x, y = raw(400 + 480 * step / 60, 380 + 60 * math.sin(step / 60 * 2 * math.pi))
    lines += ["move %d %d %d" % (x, y, 1500 + round(2000 * math.sin(step / 60 * math.pi))), "wait 15"]
lines += ["up", "wait 800"]
open(sys.argv[1], "w").write("\n".join(lines) + "\n")
EOF
put "$out/pen.pen" /tmp/pen.pen
strokes=$(count /tmp/zdesktop.log 'NOTES STROKE')
guest 'timeout 60 /bin/peninject /tmp/pen.pen; echo peninject=$?' | tail -1
expect_more /tmp/zdesktop.log 'NOTES STROKE' "${strokes:-0}" 10
shot s8-written.png
layouts=$(count /tmp/zdesktop.log 'NOTES LAYOUT window=')
keys '<esc>'
sleep 1.5
expect_more /tmp/zdesktop.log 'NOTES LAYOUT window=' "${layouts:-0}" 10
shot s8-window.png
keys '<ctrl-w>'
sleep 2

# S9 a. The scroll mode and the Right key: nine pages on, one back.
guest 'export XDG_RUNTIME_DIR=/tmp HOME=/root; /bin/pdfviewer /root/a4.pdf > /tmp/pv.log 2>&1 </dev/null & sleep 5; echo started' >/dev/null
expect_more /tmp/pv.log 'READY .*pages=10' 0 15
sleep 3
shot s9-first.png
turns=$(count /tmp/pv.log 'TURN done')
for page in 1 2 3 4 5 6 7 8 9; do
	keys '<right>'
	sleep 0.7
done
keys '<left>'
sleep 1
got=$(( $(count /tmp/pv.log 'TURN done') - ${turns:-0} ))
[ "$got" -eq 10 ] && verdict ok "scroll mode: ten turns logged" || verdict no "scroll mode: ten turns logged ($got)"
guest "grep -E 'TURN done' /tmp/pv.log | tail -10" > "$out/turns-scroll.txt"
cat "$out/turns-scroll.txt"
scroll_ms=$(largest /tmp/pv.log 'TURN done' 10 ms)
echo "scroll mode: the longest turn ${scroll_ms:-?} ms (limit $limit)"
[ "${scroll_ms:-99999}" -le "$limit" ] && verdict ok "scroll mode: every turn within $limit ms" || verdict no "scroll mode: every turn within $limit ms (${scroll_ms:-?})"
shot s9-scroll-page9.png
keys '<ctrl-q>'
sleep 2

# S9 b. The page mode and the finger: nine swipes on, one back.
guest 'export XDG_RUNTIME_DIR=/tmp HOME=/root; /bin/pdfviewer --mode=page /root/a4.pdf > /tmp/pv2.log 2>&1 </dev/null & sleep 5; echo started' >/dev/null
expect_more /tmp/pv2.log 'READY .*pages=10' 0 15
sleep 3
swipes=""
for page in 1 2 3 4 5 6 7 8 9; do
	swipes="$swipes|down 1 900 420|swipe -500 0 10 16|up 1|wait 1200"
done
swipes="$swipes|down 1 400 420|swipe 500 0 10 16|up 1|wait 1200"
touches "${swipes#|}"
sleep 1
got=$(count /tmp/pv2.log 'TURN done')
[ "${got:-0}" -ge 10 ] && verdict ok "page mode: ten turns by the finger" || verdict no "page mode: ten turns by the finger (${got:-0})"
guest "grep -E 'TURN done' /tmp/pv2.log | tail -10" > "$out/turns-page.txt"
cat "$out/turns-page.txt"
frame_ms=$(largest /tmp/pv2.log 'TURN done' 10 frame_ms)
page_ms=$(largest /tmp/pv2.log 'TURN done' 10 ms)
echo "page mode: the longest frame ${frame_ms:-?} ms (limit $limit), the longest turn with its slide ${page_ms:-?} ms"
[ "${frame_ms:-99999}" -le "$limit" ] && verdict ok "page mode: every turn's frames within $limit ms" || verdict no "page mode: every turn's frames within $limit ms (${frame_ms:-?})"
shot s9-page-page9.png

# S9 c. Zoom by the finger: a double tap, then a pinch out.
touches 'down 1 640 420|wait 60|up 1|wait 120|down 1 640 420|wait 60|up 1|wait 900'
expect_more /tmp/pv2.log 'TOUCH double-tap zoom=' 0 5
shot s9-double-tap.png
touches 'down 1 640 420|wait 60|up 1|wait 120|down 1 640 420|wait 60|up 1|wait 900|down 1 600 420; down 2 680 420|wait 60|move 1 580 420; move 2 700 420|wait 30|move 1 560 420; move 2 720 420|wait 30|move 1 540 420; move 2 740 420|wait 30|move 1 520 420; move 2 760 420|wait 30|move 1 500 420; move 2 780 420|wait 30|move 1 480 420; move 2 800 420|wait 300|up 1; up 2|wait 800'
expect_more /tmp/pv2.log 'TOUCH pinch end scale=' 0 5
shot s9-pinch.png
guest "grep -E 'TOUCH (double-tap|pinch end)' /tmp/pv2.log" > "$out/zoom.txt"
cat "$out/zoom.txt"

# The logs, and no errors.
guest 'grep -E "ZWL (CORNER|TOUCH|ERROR)|NOTES (START|STROKE|LAYOUT)" /tmp/zdesktop.log' > "$out/zdesktop.log"
guest 'cat /tmp/pv.log' > "$out/pv-scroll.log"
guest 'cat /tmp/pv2.log' > "$out/pv-page.log"
errors=$(count /tmp/zdesktop.log 'ZWL ERROR')
[ "${errors:-1}" = 0 ] && verdict ok "no ZWL ERROR" || verdict no "ZWL ERROR ($errors)"
sh plan/ws035/tests/zdesktop-guest.sh stop >/dev/null 2>&1
echo "RESULT scroll_turn_ms=${scroll_ms:-?} page_frame_ms=${frame_ms:-?} page_turn_ms=${page_ms:-?} limit=$limit"
[ $status -eq 0 ] && echo "demo-s8-s9: PASS" || echo "demo-s8-s9: FAIL"
exit $status
