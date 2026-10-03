#!/bin/sh
# ws035-p057: the glass shows the windows under it, blurred (backdrop.c), on the Venus guest (the lean
# image of plan/tools/files/build-files-image.sh).  zdesktop --glass at 1280x800 with the wallpaper;
# /bin/extras-probe --body-viewport (a 600x300 window of green and white with a red sub-surface) first,
# then files (1000x640, glass cards) over it.
#  1. zdesktop makes the backdrop when a window is over another (ZWL BACKDROP ready).
#  2. over.png: a point of the file manager's content card that lies over the probe's window; gone.png:
#     the same point after the probe has closed.  The glass there differs by 12 or more (the blurred
#     window under it shows through; with only the blurred wallpaper both pictures would agree), and it
#     leans to the probe's red or green by 10 or more.
#
#   plan/tools/files/files-guest.sh start     (the guest must be up)
#   plan/ws035/tests/zdesktop-p057.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws035-p057}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
. plan/tools/guest/zwl-clients.sh
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[f]iles|[e]xtras-probe" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[f]iles|[e]xtras-probe" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
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

# The place of a client's window (x y) from zdesktop's log.
place() {
	guest "grep 'ZWL MAP client=$(zwl_app_client $1) ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2/p'
}

guest "$stop_all" >/dev/null
guest 'rm -f /tmp/wayland-0; rm -rf /tmp/fhome; sh /usr/share/files-tests/make-home.sh /tmp/fhome >/dev/null'
guest 'export XDG_RUNTIME_DIR=/tmp
/bin/wayland --timeout=600 --width=1280 --height=800 --glass --wallpaper=/usr/share/keiland/wallpaper.ppm > /tmp/zdesktop.log 2>&1 </dev/null & for w in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30; do grep -q ZWL.READY /tmp/zdesktop.log 2>/dev/null && break; sleep 0.5; done; sleep 1
/bin/extras-probe --timeout-s=500 --token=v --body-viewport > /tmp/v.log 2>&1 </dev/null & sleep 4
HOME=/tmp/fhome /bin/files --token=f1 --timeout-s=500 --width=1000 --height=640 /tmp/fhome/Documents > /tmp/f.log 2>&1 </dev/null & sleep 6; echo started' >/dev/null
set -- $(place 1); px=${1:-0}; py=${2:-0}
set -- $(place 2); fx=${1:-0}; fy=${2:-0}
echo "probe at $px,$py; files at $fx,$fy"
expect_log /tmp/zdesktop.log 'ZWL BACKDROP ready width=160 height=100'

# A point of the file manager's content card (x 254..968, y 60..608 of its body) over the probe's red
# sub-surface (its middle, 120,70 of the probe's body), else over its green (the first of those inside the card).
set -- $(python3 -c "
import sys
px, py, fx, fy = map(int, sys.argv[1:])
for x, y in ((px + 120, py + 70), (px + 300, py + 60), (px + 500, py + 60)):
	if fx + 254 <= x <= fx + 968 and fy + 60 <= y <= fy + 608:
		print(x, y)
		break
else:
	print('0 0')
" "$px" "$py" "$fx" "$fy")
x=${1:-0}; y=${2:-0}
[ "$x" -gt 0 ] && echo "point: $x,$y ok" || { echo "point: the windows do not overlap MISSING"; status=1; }

# The glass over the probe, then over what is left when the probe has gone.
pointer move 20 790 sleep 800
check "$out/over.png" >/dev/null
guest 'for p in $(ps -A -o pid,args | grep -E "[e]xtras-probe" | awk "{print \$1}"); do kill $p; done; sleep 2' >/dev/null
pointer move 22 790 sleep 800
check "$out/gone.png" >/dev/null
python3 - "$out/over.png" "$out/gone.png" "$x" "$y" <<'EOF' || status=1
import sys
from PIL import Image
over = Image.open(sys.argv[1]).convert("RGB")
gone = Image.open(sys.argv[2]).convert("RGB")
x, y = int(sys.argv[3]), int(sys.argv[4])
a = over.getpixel((x, y))
b = gone.getpixel((x, y))
change = max(abs(p - q) for p, q in zip(a, b))
tinted = (a[0] - a[1]) - (b[0] - b[1]) >= 10 or (a[1] - a[0]) - (b[1] - b[0]) >= 10
if change >= 12 and tinted:
	print("glass: %s over the probe, %s without it (%d) ok" % (a, b, change))
else:
	print("glass: %s over the probe, %s without it (%d) MISSING" % (a, b, change))
	sys.exit(1)
EOF

errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
guest "$stop_all" >/dev/null
guest 'grep -E "BACKDROP|MAP" /tmp/zdesktop.log' > "$out/zdesktop.log"
[ $status = 0 ] && echo "zdesktop-p057: PASS" || echo "zdesktop-p057: FAIL"
exit $status
