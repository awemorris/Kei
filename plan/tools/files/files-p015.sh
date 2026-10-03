#!/bin/sh
# ws071-p015 / ws035-p083: files as a glass window on the Venus guest (the lean image,
# build-files-image.sh).  zdesktop --glass at 1280x800 with the wallpaper; files at 1000x640
# on the sample home (/tmp/fhome), opened on Documents.
#  1. The swapchain is see-through and zdesktop has glass: ZFILES GLASS on, two cards
#     (the sidebar and the content) in zdesktop's log with their places, one.png.
#  2. Between the sidebar and the content the desktop shows as it is: the picture's pixel there is the
#     wallpaper's, at most darkened evenly by the cards' shadows (each channel 0 to 60 darker, alike within 12).
#  3. Ctrl+Alt+P: the preview is a third card, preview.png.
#  4. Ctrl+T, Ctrl+W: the row of tabs is inside the content's card; the panels do not change, tabs.png.
#
#   plan/tools/files/files-guest.sh start     (the guest must be up)
#   plan/tools/files/files-p015.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws071-p015}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
. plan/tools/guest/zwl-clients.sh
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; sleep 0.7; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[f]iles|[t]erminal" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[f]iles|[t]erminal" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
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
shot() {
	pointer move 1270 790 sleep 400
	check "$out/$1" >/dev/null
}

guest "$stop_all" >/dev/null
guest 'rm -f /tmp/wayland-0 /tmp/files.clipboard; rm -rf /tmp/fhome; sh /usr/share/files-tests/make-home.sh /tmp/fhome >/dev/null'
guest 'export XDG_RUNTIME_DIR=/tmp
/bin/wayland --timeout=900 --width=1280 --height=800 --glass --wallpaper=/usr/share/keiland/wallpaper.ppm > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4
HOME=/tmp/fhome /bin/files --token=f1 --timeout-s=800 --width=1000 --height=640 /tmp/fhome/Documents > /tmp/f.log 2>&1 </dev/null & sleep 6; echo started' >/dev/null
zwl_app_clients
set -- $(guest "grep 'ZWL MAP client=$zc1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* surface=\([0-9]*\) x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2 \3/p')
surface=${1:-0}; wx=${2:-0}; wy=${3:-0}
echo "files: surface $surface at $wx,$wy"

# 1. Glass, and the two cards.
expect_log /tmp/f.log 'ZFILES GLASS on'
expect_log /tmp/f.log 'ZFILES GLASS panels count=2'
expect_log /tmp/zdesktop.log "ZWL GLASS client=[0-9]+ surface=$surface panels=2 card:0,0,212,640,16 card:220,0,780,640,16\$"
shot one.png

# 2. The desktop between the sidebar and the content, as the wallpaper has it.
wallpaper=build/ws035-wallpaper/wallpaper.ppm   # the file the image installs as /usr/share/keiland/wallpaper.ppm
python3 - "$out/one.png" "$wallpaper" $((wx + 216)) $((wy + 300)) <<'EOF' || status=1
import sys
from PIL import Image
shot = Image.open(sys.argv[1]).convert("RGB")
x, y = int(sys.argv[3]), int(sys.argv[4])
try:
	wall = Image.open(sys.argv[2]).convert("RGB").resize(shot.size)
except Exception as error:
	print("gap: no wallpaper to compare (%s) MISSING" % error)
	sys.exit(1)
seen = shot.getpixel((x, y))
wanted = wall.getpixel((x, y))
darker = [b - a for a, b in zip(seen, wanted)]
if min(darker) >= -4 and max(darker) <= 60 and max(darker) - min(darker) <= 12:
	print("gap: desktop shows at %d,%d %s ok" % (x, y, seen))
else:
	print("gap: %s at %d,%d, wallpaper %s MISSING" % (seen, x, y, wanted))
	sys.exit(1)
EOF

# 3. The preview (Ctrl+Alt+P): a third card, and the content narrower.
keys '<ctrl-alt-p>'
expect_log /tmp/f.log 'ZFILES GLASS panels count=3'
expect_log /tmp/zdesktop.log "ZWL GLASS client=[0-9]+ surface=$surface panels=3 card:0,0,212,640,16 card:220,0,508,640,16 card:736,0,264,640,16\$"
shot preview.png

# 4. A second tab: the row of tabs is inside the content's card, which keeps its place (no new panels).
keys '<ctrl-t>'
expect_log /tmp/f.log 'ZFILES TABS new index=1 count=2'
shot tabs.png
keys '<ctrl-w>'
expect_log /tmp/f.log 'ZFILES TABS close index=1 count=1'
lines=$(guest "grep -c 'ZWL GLASS client=' /tmp/zdesktop.log" | tail -1)
[ "${lines:-0}" = 2 ] && echo "log: two glass commits (the tabs change no panel) ok" || { echo "log: $lines glass commits, 2 wanted MISSING"; status=1; }

errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
guest "$stop_all" >/dev/null
guest 'cat /tmp/f.log' > "$out/f.log"
guest 'grep GLASS /tmp/zdesktop.log' > "$out/zdesktop-glass.log"
[ $status = 0 ] && echo "files-p015: PASS" || echo "files-p015: FAIL"
exit $status
