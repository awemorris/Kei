#!/bin/sh
# ws089-p010: a picture of every page of Settings on the Venus guest at one output size, for the walk through the
# pages (the lean image, build-settings-image.sh).  zdesktop --glass at WIDTHxHEIGHT with the session's wallpaper;
# Settings starts at Home and the Down key goes through all 23 pages (NN-WORD.png, as settings-p006.sh step 2), then
# Home's tiles scrolled to the end (home-end.png) and the sidebar's end (about.png is the last page).
# The guest's Venus output must have the size: start it with VENUS_SIZE=WIDTHxHEIGHT (zdesktop-guest.sh).
# The window is the last one mapped by a client that is not the input method (find_window, BUG-146).
#
#   VENUS_SIZE=1920x1080 plan/ws089/tests/settings-guest.sh start IMAGE
#   plan/ws089/tests/settings-pages.sh WIDTHxHEIGHT [OUTDIR]   (default build/ws089-shots/pages-WIDTHxHEIGHT)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws089-run}"
export GUEST_RUNTIME
size=${1:-1280x800}
width=${size%x*}
height=${size#*x}
out=${2:-build/ws089-shots/pages-$size}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1 </dev/null; }
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py --width "$width" --height "$height" "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; sleep 0.7; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[s]ettings" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[s]ettings" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
start_desktop="export XDG_RUNTIME_DIR=/tmp HOME=/root; rm -f /tmp/wayland-0
/bin/wayland --timeout=900 --width=$width --height=$height --glass --wallpaper=/usr/share/keiland/wallpaper.ppm > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4; echo started"
conf=/root/.config/keiland/desktop.conf
status=0
. plan/ws089/tests/settings-wait.sh

# A picture of the screen with the pointer out of the way.
shot() {
	pointer move $((width - 10)) $((height - 10)) sleep 600
	check "$out/$1" >/dev/null
	echo "shot: $out/$1"
}

wait_guest
guest "$stop_all" >/dev/null
guest "rm -f $conf" >/dev/null
guest "$start_desktop" >/dev/null
wait_desktop
guest "export XDG_RUNTIME_DIR=/tmp HOME=/root; /bin/settings --timeout-s=800 > /tmp/s.log 2>&1 </dev/null & sleep 5; echo started" >/dev/null

# The window: Settings' client, not the input method's (BUG-146, find_window in settings-wait.sh).
find_window
echo "settings: client ${wclient:-?} window at $wx,$wy"
grep_home=$(guest "grep -c 'ZSETTINGS PAGE home' /tmp/s.log" | tail -1)
[ "${grep_home:-0}" -gt 0 ] 2>/dev/null || { echo "settings: no Home page"; status=1; }
pointer move $((wx + 700)) $((wy + 60)) sleep 200
shot 00-home.png

# Home's tiles to the end (the wheel over the page pane).
pointer move $((wx + 700)) $((wy + 300)) sleep 200 wheel-down wheel-down wheel-down wheel-down wheel-down wheel-down wheel-down wheel-down wheel-down wheel-down sleep 800
check "$out/00-home-end.png" >/dev/null
echo "shot: $out/00-home-end.png"

number=1
while [ $number -le 23 ]; do
	keys '<down>'
	word=$(guest "grep 'ZSETTINGS PAGE ' /tmp/s.log | tail -1" | sed -n 's/.*ZSETTINGS PAGE \([a-z]*\).*/\1/p' | tail -1)
	shot "$(printf '%02d-%s.png' "$number" "${word:-unknown}")"
	number=$((number + 1))
done
pages=$(guest "grep -c 'ZSETTINGS PAGE ' /tmp/s.log" | tail -1)
echo "pages shown: $pages"
[ "${pages:-0}" -ge 24 ] 2>/dev/null || { echo "pages: fewer than 24"; status=1; }
last=$(guest "grep 'ZSETTINGS PAGE ' /tmp/s.log | tail -1" | sed -n 's/.*ZSETTINGS PAGE \([a-z]*\).*/\1/p' | tail -1)
[ "$last" = about ] || { echo "last page: $last (not about)"; status=1; }

errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; guest "grep ERROR /tmp/zdesktop.log | head -5"; status=1; }
guest 'cat /tmp/s.log' > "$out/settings.log"
guest "grep -E 'ZWL (MAP|CLIENT)' /tmp/zdesktop.log" > "$out/zdesktop-map.log"
guest "$stop_all" >/dev/null
guest "rm -f $conf" >/dev/null
[ $status = 0 ] && echo "settings-pages: PASS" || echo "settings-pages: FAIL"
exit $status
