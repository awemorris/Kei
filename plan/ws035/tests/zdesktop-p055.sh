#!/bin/sh
# ws035-p055: window mode draws only the damage (damage.c, compose.c) on the Venus guest (the lean image of
# plan/tools/files/build-files-image.sh).  zdesktop --log-frames at 1280x800 with /bin/wlshm --band (a
# 400x300 blue window with a yellow band moving down it, a new image every frame).
# In the glass look and then the plain look:
#  1. The window's new images are drawn in its body alone (ZWL DAMAGE lines of the body's size).
#  2. The pointer moved over the body is drawn in the cursor's places too (ZWL DAMAGE lines other than
#     the body's alone: the body and the cursor's places together).
#  3. band-<look>.png, taken while the band moves and the pointer rests on the body: down a column of the
#     body there is one band 20 rows high and the window's blue elsewhere -- no band left behind by an
#     image drawn in part -- and where the pointer was the blue has no cursor left behind.
#
#   plan/tools/files/files-guest.sh start     (the guest must be up)
#   plan/ws035/tests/zdesktop-p055.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws035-p055}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
. plan/tools/guest/zwl-clients.sh
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[w]lshm" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[w]lshm" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
status=0

# Fails the run unless a count read from the guest is at least a number.
expect_count() {
	found=$(guest "$2" | tail -1)
	if [ "${found:-0}" -ge "$3" ] 2>/dev/null; then
		echo "$1: $found ok"
	else
		echo "$1: $found (want $3 or more) MISSING"
		status=1
	fi
}

# One look (its zdesktop option: --glass or nothing).
run_look() {
	look=$1
	guest "$stop_all" >/dev/null
	guest "export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0
/bin/wayland --timeout=300 --width=1280 --height=800 --log-frames $2 > /tmp/zdesktop.log 2>&1 </dev/null & for w in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30; do grep -q ZWL.READY /tmp/zdesktop.log 2>/dev/null && break; sleep 0.5; done; sleep 1
/bin/wlshm --size=400x300 --color=ff3060c0 --band=ffe0e040 --frames=100000 --token=b > /tmp/b.log 2>&1 </dev/null & sleep 4; echo started" >/dev/null
	zwl_app_clients
	set -- $(guest "grep 'ZWL MAP client=$zc1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2/p')
	wx=${1:-0}; wy=${2:-0}
	echo "$look: window at $wx,$wy"

	# 1. The images, in the body.
	expect_count "$look: body damage" "grep -cE 'ZWL DAMAGE .* width=400 height=300' /tmp/zdesktop.log" 10

	# 2. The pointer across the body (a zigzag), then resting at its middle.
	before=$(guest "grep -c 'ZWL DAMAGE' /tmp/zdesktop.log" | tail -1)
	pointer move $((wx + 40)) $((wy + 40)) sleep 200 move $((wx + 120)) $((wy + 60)) sleep 100 move $((wx + 200)) $((wy + 40)) sleep 100 \
	    move $((wx + 280)) $((wy + 80)) sleep 100 move $((wx + 360)) $((wy + 40)) sleep 100 move $((wx + 200)) $((wy + 150)) sleep 800
	expect_count "$look: cursor damage" "grep -E 'ZWL DAMAGE' /tmp/zdesktop.log | grep -vc 'width=400 height=300'" 1

	# 3. The picture: one band down a column clear of the cursor, and no cursor where it passed.
	check "$out/band-$look.png" >/dev/null
	python3 - "$out/band-$look.png" "$wx" "$wy" <<'EOF' || status=1
import sys
from PIL import Image
shot = Image.open(sys.argv[1]).convert("RGB")
wx, wy = int(sys.argv[2]), int(sys.argv[3])
def near(p, q):
	return max(abs(a - b) for a, b in zip(p, q)) <= 24
blue, yellow = (0x30, 0x60, 0xc0), (0xe0, 0xe0, 0x40)
runs, rows, other, previous = 0, 0, 0, False
for y in range(wy + 2, wy + 298):
	p = shot.getpixel((wx + 330, y))
	band = near(p, yellow)
	if band:
		rows += 1
		if not previous:
			runs += 1
	elif not near(p, blue):
		other += 1
	previous = band
if runs == 1 and 16 <= rows <= 24 and other == 0:
	print("band: one band of %d rows, the rest blue ok" % rows)
else:
	print("band: %d bands, %d band rows, %d other rows MISSING" % (runs, rows, other))
	sys.exit(1)
ghosts = 0
for x, y in ((40, 40), (120, 60), (280, 80), (360, 40)):
	for dx in range(0, 12):
		for dy in range(0, 16):
			p = shot.getpixel((wx + x + dx, wy + y + dy))
			if not near(p, blue) and not near(p, yellow):
				ghosts += 1
if ghosts == 0:
	print("cursor: nothing left where it passed ok")
else:
	print("cursor: %d pixels left where it passed MISSING" % ghosts)
	sys.exit(1)
EOF
	errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
	[ "${errors:-1}" = 0 ] && echo "$look: zdesktop: no ERROR" || { echo "$look: zdesktop: ERROR lines"; status=1; }
	guest 'grep -c "ZWL DAMAGE" /tmp/zdesktop.log; grep -c "ZWL COMPOSE" /tmp/zdesktop.log' > "$out/frames-$look.txt"
}

run_look glass --glass
run_look plain ''
guest "$stop_all" >/dev/null
[ $status = 0 ] && echo "zdesktop-p055: PASS" || echo "zdesktop-p055: FAIL"
exit $status
