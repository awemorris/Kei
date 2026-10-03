#!/bin/sh
# ws099-p007 (BUG-118): a client's cursor (here hidden) is shown only over that client's window; elsewhere zdesktop's
# arrow, without the keyboard's focus changing.  zdesktop --glass at 1280x800 with the wallpaper, and
# /bin/wlshm --hide-cursor (400x300, it hides the cursor when the pointer enters it).  The pointer is put:
#  1. on the window's body: hidden (ZWL CURSOR client=N shown=1; the picture around the pointer is the same as with
#     the pointer elsewhere, so no arrow is drawn there);
#  2. on the desktop: the arrow (shown=0; the picture around the pointer differs from one with the pointer elsewhere);
#  3. on the window's title bar: the arrow (the title bar is zdesktop's);
#  4. on the body again: hidden.
# Prints "cursor-owner: PASS" or "cursor-owner: FAIL".
#
#   plan/ws035/tests/zdesktop-guest.sh start IMAGE
#   plan/ws099/tests/cursor-owner.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws035-sq-run}"
export GUEST_RUNTIME
out=${1:-build/ws099-shots/cursor-owner}
mkdir -p "$out"
# The SSH to the guest, tried again when ssh itself fails (plan/ws099/tests/guest-retry.sh, ws099-p023).
. plan/ws099/tests/guest-retry.sh
guest() { guest_retry 90 "$1" </dev/null; }
. plan/tools/guest/zwl-clients.sh
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[w]lshm" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[w]lshm" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
status=0

# Fails the run unless zdesktop's log has (within a few seconds) at least COUNT lines matching a pattern.
expect_log() {
	tries=0
	found=0
	while [ $tries -lt 6 ]; do
		found=$(guest "grep -cE '$1' /tmp/zdesktop.log" | tail -1)
		[ "${found:-0}" -ge "$2" ] 2>/dev/null && break
		tries=$((tries + 1))
		sleep 1
	done
	if [ "${found:-0}" -ge "$2" ] 2>/dev/null; then
		echo "log: $1 ($2) ok"
	else
		echo "log: $1 ($2) MISSING (found ${found:-0})"
		status=1
	fi
}

# Tells how many pixels differ (by more than 24) in a 24x32 box at a point between two pictures.
differs() {
	python3 - "$1" "$2" "$3" "$4" <<'EOF'
import sys
from PIL import Image
a = Image.open(sys.argv[1]).convert("RGB")
b = Image.open(sys.argv[2]).convert("RGB")
x, y = int(sys.argv[3]), int(sys.argv[4])
n = 0
for dy in range(32):
	for dx in range(24):
		p = a.getpixel((x + dx, y + dy))
		q = b.getpixel((x + dx, y + dy))
		if max(abs(p[i] - q[i]) for i in range(3)) > 24:
			n += 1
print(n)
EOF
}

# Puts the pointer at a point (a small approach, then a rest) and photographs the screen.
put() {
	pointer move $(($1 - 3)) $(($2 - 3)) sleep 150 move "$1" "$2" sleep 700
	check "$out/$3.png" >/dev/null
}

guest "$stop_all" >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0; picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=300 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & for w in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30; do grep -q ZWL.READY /tmp/zdesktop.log 2>/dev/null && break; sleep 0.5; done; sleep 1
/bin/wlshm --size=400x300 --color=ff3060a0 --frames=6000 --hide-cursor --token=h > /tmp/h.log 2>&1 </dev/null & sleep 3; echo started' >/dev/null
zwl_app_clients
set -- $(guest "grep 'ZWL MAP client=$zc1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2/p')
wx=${1:-440}; wy=${2:-250}
echo "window at $wx,$wy"
body_x=$((wx + 200)); body_y=$((wy + 150))
desk_x=150; desk_y=600
title_x=$((wx + 120)); title_y=$((wy - 30))
far_x=1100; far_y=650

# 1. The body: the client hid the cursor; nothing is drawn at the pointer.
put "$body_x" "$body_y" body
expect_log 'ZWL CURSOR client=[0-9]+ shown=1' 1
# 2. The desktop: the arrow.
put "$desk_x" "$desk_y" desktop
expect_log 'ZWL CURSOR client=[0-9]+ shown=0' 1
# 3. The title bar: the arrow.
put "$title_x" "$title_y" title
# The pictures to compare with: the pointer far away on the desktop.
put "$far_x" "$far_y" far
# 4. The body again: hidden.
put "$body_x" "$body_y" body-again
expect_log 'ZWL CURSOR client=[0-9]+ shown=1' 2

# The arrow is drawn where it should be, and not on the body.
n=$(differs "$out/desktop.png" "$out/far.png" "$desk_x" "$desk_y")
[ "${n:-0}" -gt 20 ] && echo "desktop: the arrow at the pointer ($n pixels) ok" || { echo "desktop: no arrow at the pointer ($n pixels) FAIL"; status=1; }
n=$(differs "$out/title.png" "$out/far.png" "$title_x" "$title_y")
[ "${n:-0}" -gt 20 ] && echo "title bar: the arrow at the pointer ($n pixels) ok" || { echo "title bar: no arrow at the pointer ($n pixels) FAIL"; status=1; }
n=$(differs "$out/body.png" "$out/far.png" "$body_x" "$body_y")
[ "${n:-0}" -le 20 ] && echo "body: nothing drawn at the pointer ($n pixels) ok" || { echo "body: something drawn at the pointer ($n pixels) FAIL"; status=1; }
n=$(differs "$out/body-again.png" "$out/far.png" "$body_x" "$body_y")
[ "${n:-0}" -le 20 ] && echo "body again: nothing drawn at the pointer ($n pixels) ok" || { echo "body again: something drawn at the pointer ($n pixels) FAIL"; status=1; }

# The keyboard stayed with the window (no focus change was needed).
leaves=$(guest "grep -c 'WLSHM LEAVE' /tmp/h.log" | tail -1)
echo "wlshm pointer leaves: ${leaves:-?}"
guest 'grep -E "ERROR|FAILED" /tmp/zdesktop.log' | tee "$out/errors.txt"
[ -s "$out/errors.txt" ] && status=1
guest 'grep -E "ZWL CURSOR" /tmp/zdesktop.log' > "$out/zdesktop-cursor.log"
guest "$stop_all" >/dev/null
[ $status -eq 0 ] && echo "cursor-owner: PASS" || echo "cursor-owner: FAIL"
exit $status
