#!/bin/sh
# BUG-141 (ws099-p029): an item lit under the pointer goes dark when zdesktop takes the pointer (its menus here).
# On the lean image (plan/tools/files/build-files-image.sh), zdesktop --glass 1280x800 and files at 1000x640 on
# Documents:
#  1. base.png: an item clicked, the pointer then on the window's empty bottom (no item lit).
#  2. hover-on.png: the pointer on another item: the box around it differs from base.png (it is lit: 600 pixels
#     or more, more than the cursor alone).
#  3. menu.png: F10 opens the first menu and the pointer goes onto its rows (the motion is zdesktop's): the box is
#     as in base.png again (files heard the pointer leave).  Before the fix files heard nothing and kept it lit.
# Judged by the pictures (the Venus display) and the logs, not the console.
#
#   plan/tools/files/files-guest.sh start IMAGE     (the guest must be up)
#   plan/ws099/tests/bug141-hover.sh [OUTDIR]
# Prints "bug141: PASS" or "bug141: FAIL".
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws099-bug141}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
. plan/tools/guest/zwl-clients.sh
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; sleep 0.8; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[f]iles" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[f]iles" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
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

# The middle (y) of the latest popup row of an item, and the latest popup's left edge.
row_y() {
	guest "grep 'MENU row item=$1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* y=\([0-9]*\) height=\([0-9]*\).*/\1 \2/p' | { read y h; echo $(( ${y:-0} + ${h:-0} / 2 )); }
}

guest "$stop_all" >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0 /tmp/files.clipboard; rm -rf /tmp/fhome; sh /usr/share/files-tests/make-home.sh /tmp/fhome >/dev/null
picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=600 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4
HOME=/tmp/fhome /bin/files --token=f1 --timeout-s=500 --width=1000 --height=640 /tmp/fhome/Documents > /tmp/f.log 2>&1 </dev/null & sleep 5; echo started' >/dev/null
zwl_app_clients
set -- $(guest "grep 'ZWL MAP client=$zc1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* surface=\([0-9]*\) x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2 \3/p')
wx=${2:-0}; wy=${3:-0}
echo "files at $wx,$wy"
expect_log /tmp/f.log 'ZFILES LOCATION kind=folder path=/tmp/fhome/Documents items=6 error=0'

# 1. An item clicked, the pointer on the empty bottom of the window.
pointer move $((wx + 444)) $((wy + 140)) sleep 150 move $((wx + 446)) $((wy + 140)) sleep 300 down sleep 60 up sleep 900
pointer move $((wx + 600)) $((wy + 600)) sleep 800
check "$out/base.png" >/dev/null

# 2. The pointer on another item.
pointer move $((wx + 670)) $((wy + 140)) sleep 300 move $((wx + 680)) $((wy + 145)) sleep 800
check "$out/hover-on.png" >/dev/null

# 3. F10's menu, the pointer onto its rows (the first popup row the log names).
keys '<f10>'
expect_log /tmp/zdesktop.log 'MENU open '
row=$(guest "grep 'MENU row item=' /tmp/zdesktop.log | tail -1" | sed -n 's/.* item=\([0-9]*\) .*/\1/p')
popup=$(guest "grep 'MENU open ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([0-9]*\) y=.*/\1/p')
y=$(row_y "${row:-0}")
pointer move $(( ${popup:-200} + 50 )) $(( y - 4 )) sleep 200 move $(( ${popup:-200} + 60 )) "$y" sleep 900
check "$out/menu.png" >/dev/null
keys '<esc>'

# The box around the hovered item: lit in hover-on.png, as in base.png in menu.png.
python3 - "$out" "$((wx + 680))" "$((wy + 145))" <<'PY' || status=1
import sys
from PIL import Image
out, cx, cy = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
base = Image.open(out + "/base.png").convert("RGB")
def differing(name):
	other = Image.open(out + "/" + name).convert("RGB")
	count = 0
	for y in range(cy - 50, cy + 50):
		for x in range(cx - 60, cx + 60):
			p, q = base.getpixel((x, y)), other.getpixel((x, y))
			if max(abs(p[i] - q[i]) for i in range(3)) > 12:
				count += 1
	return count
lit, menu = differing("hover-on.png"), differing("menu.png")
status = 0
if lit >= 600:
	print("hover: the item is lit (%d pixels differ from base) ok" % lit)
else:
	print("hover: the item is not lit (%d pixels) -- the test did not light it MISSING" % lit); status = 1
if menu <= 50:
	print("menu: the item is dark again (%d pixels differ) ok" % menu)
else:
	print("menu: the item stays lit (%d pixels differ) FAIL" % menu); status = 1
sys.exit(status)
PY
errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
guest "$stop_all" >/dev/null
[ $status -eq 0 ] && echo "bug141: PASS" || echo "bug141: FAIL"
exit $status
