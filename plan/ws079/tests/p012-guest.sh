#!/bin/sh
# ws079-p012: the test touch screen of /dev/input-inject on the pen test guest
# (plan/ws079/tests/config-amd64-pen.mk, started with pen-guest.sh start).
#
#  1. touchinject -c: the injector refuses bad touch setups and frames and
#     takes good ones.
#  2. The two-finger script (userland/tests/touchinject/two-fingers.touch)
#     replayed while touchinject -d reads the evdev node: the node's name,
#     its six axes and every event must be exactly p012-two-fingers.expected
#     (multitouch protocol B, a frame of three fingers split over two reports).
#  3. The pen still works: peninject -c, and the pen script's 182 events.
#  4. With the compositor running, the touch screen joins the seat (as a
#     touch screen since ws079-p013, which gave the compositor wl_touch; it was
#     an absolute pointer through ABS_X/ABS_Y before), leaves it when the
#     injector closes, and the compositor stays up.
#
#   plan/ws079/tests/pen-guest.sh start build/amd64/hdd-image.img
#   plan/ws079/tests/p012-guest.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
export GUEST_RUNTIME="${GUEST_RUNTIME:-$PWD/build/ws079-run}"
out=${1:-build/ws079-p012}
mkdir -p "$out"
guest() { timeout 120 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
status=0

# 1. The refusals.
guest '/bin/touchinject -c' > "$out/touchcheck.txt"
if grep -q 'TOUCHCHECK result=ok passed=14 failed=0' "$out/touchcheck.txt"; then
	echo "touchcheck: ok"
else
	echo "touchcheck: FAILED"
	status=1
fi

# 2. The two-finger script, read back from the evdev node.
guest '/bin/touchinject -d 6000 > /tmp/touchdump.txt 2>&1 & sleep 0.3; /bin/touchinject /usr/share/touchinject/two-fingers.touch; echo replay=$?; sleep 6; cat /tmp/touchdump.txt' > "$out/touchdump.txt"
grep -q '^replay=0$' "$out/touchdump.txt" || { echo "replay: FAILED"; status=1; }
grep -q 'TOUCHDUMP node=/dev/input/event[0-9]* name=Test touchscreen (input-inject)' "$out/touchdump.txt" || { echo "node: FAILED"; status=1; }
grep -E '^TOUCHDUMP (abs|event) ' "$out/touchdump.txt" > "$out/touchdump-events.txt"
if cmp -s "$out/touchdump-events.txt" plan/ws079/tests/p012-two-fingers.expected; then
	echo "two fingers: $(grep -c '^TOUCHDUMP event' "$out/touchdump-events.txt") events as expected ok"
else
	echo "two fingers: FAILED"
	diff plan/ws079/tests/p012-two-fingers.expected "$out/touchdump-events.txt" | head -20
	status=1
fi

# 3. The pen.
guest '/bin/peninject -c | tail -1; /bin/peninject -d 9000 > /tmp/pendump.txt 2>&1 & sleep 0.3; /bin/peninject /usr/share/peninject/stroke.pen; echo penreplay=$?; sleep 9; tail -1 /tmp/pendump.txt' > "$out/pen.txt"
if grep -q 'PENCHECK result=ok passed=15 failed=0' "$out/pen.txt" &&
   grep -q '^penreplay=0$' "$out/pen.txt" &&
   grep -q 'PENDUMP end events=182 reason=gone' "$out/pen.txt"; then
	echo "pen: ok"
else
	echo "pen: FAILED"
	status=1
fi

# 4. The compositor with the touch screen on the seat.
guest 'for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)" | awk "{print \$1}"); do kill $p; done; sleep 1
export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0
/bin/wayland --timeout=60 --width=1280 --height=800 --glass > /tmp/zdesktop-p012.log 2>&1 </dev/null &
i=0; while ! grep -q "ZWL MODE" /tmp/zdesktop-p012.log && [ $i -lt 60 ]; do sleep 0.5; i=$((i+1)); done
printf "size 1000 1000 2\nwait 5000\ndown 1 500 500; down 2 600 500\nswipe 0 -100 4 50\nup 1; up 2\nhold 1500\n" | /bin/touchinject; echo replay=$?; sleep 1
ps -A -o args | grep -cE "[w]ayland( |$)"
for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)" | awk "{print \$1}"); do kill $p; done; sleep 1
grep -E "ZWL INPUT|ERROR|FAILED" /tmp/zdesktop-p012.log | head -20' > "$out/compositor.txt"
if grep -q '^replay=0$' "$out/compositor.txt" &&
   grep -q '^1$' "$out/compositor.txt" &&
   grep -q 'ZWL INPUT device=/dev/input/event[0-9]* kind=touch abs=1' "$out/compositor.txt" &&
   grep -q 'ZWL INPUT_CLOSED' "$out/compositor.txt" &&
   ! grep -qE 'ERROR|FAILED' "$out/compositor.txt"; then
	echo "compositor with the touch screen: ok"
else
	echo "compositor with the touch screen: FAILED"
	status=1
fi

[ $status -eq 0 ] && echo "p012: PASS" || echo "p012: FAIL"
exit $status
