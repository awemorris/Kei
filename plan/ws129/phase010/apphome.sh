#!/bin/sh
# ws129-p010: every application of App Home's default list on the CI configuration's image (config/ci/config-amd64.mk
# with the guest harness's files only), from plan/ws035/tests/zdesktop-p090.sh: Home's list is the image's own
# /etc/keiland/apps.conf (installed with the compositor), which is checked to be there and is kept.
# zdesktop --glass at 1280x800 with the wallpaper; Home opens from the launcher (top left) and each application
# starts from its icon, one after another; a screenshot after each (NN-NAME.png) makes the sheet:
#  00-home.png, then one per application in Home's order, and 99-all.png with every window.
# Checks: every launch maps a window (ZWL MAP), zdesktop logs no ERROR; the pictures are judged by eye.
#
#   plan/tools/files/files-guest.sh start IMAGE     (the guest must be up; GUEST_RUNTIME names its runtime)
#   plan/ws129/phase010/apphome.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws129-p010-home}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1 </dev/null; }
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
stop_all='service stop greeter >/dev/null 2>&1; ps -A -o pid,comm | awk "{ n = \$2; sub(\".*/\", \"\", n) } n ~ /^(wayland|xserver|files|terminal|notes|pdfviewer|imageview|textedit|settings|browser|zgears|zterm|mview)$/ {print \$1}" | while read p; do kill $p; done; sleep 1'
status=0

# The centre of an icon, from zdesktop's log.
icon() {
	guest "grep 'ZWL HOME icon name=\"$1\"' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([0-9]*\) y=\([0-9]*\).*/\1 \2/p'
}

# How many windows zdesktop has mapped.
maps() {
	guest "grep -c 'ZWL MAP ' /tmp/zdesktop.log" | tail -1
}

shot() {
	pointer move 1270 790 sleep 400
	check "$out/$1" >/dev/null
}

# The home as the demo session makes it (plan/ws035/demo/run-zdesktop.sh): /root with the usual folders.  (Home's
# applications get the passwd home, not zdesktop's HOME: they lose zdesktop's environment, BUG-079.)
guest "$stop_all" >/dev/null
guest 'for folder in Desktop Documents Downloads Pictures Music Movies; do mkdir -p /root/$folder; done' >/dev/null
timeout 60 python3 plan/tools/guest/guest.py get /etc/keiland/apps.conf "$out/apps.conf" >/dev/null 2>&1 </dev/null
if cmp -s "$out/apps.conf" userland/desktop/wayland/apps.conf; then
	echo "apps.conf: the image's is the default ok"
else
	echo "apps.conf: MISSING or not the default"
	status=1
fi
guest 'export XDG_RUNTIME_DIR=/tmp HOME=/root; rm -f /tmp/wayland-0 /tmp/x11server.pid; rmdir /tmp/x11server.lock 2>/dev/null
/bin/wayland --timeout=900 --width=1280 --height=800 --glass --wallpaper=/usr/share/keiland/wallpaper.ppm > /tmp/zdesktop.log 2>&1 </dev/null & for w in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30; do grep -q ZWL.READY /tmp/zdesktop.log 2>/dev/null && break; sleep 0.5; done; sleep 1; echo started' >/dev/null

# Home, and its icons.
pointer move 23 17 sleep 300 down sleep 60 up sleep 1500
shot 00-home.png
names=$(guest "grep -o 'ZWL HOME icon name=\"[^\"]*\"' /tmp/zdesktop.log | sed 's/.*name=\"//; s/\"\$//' | awk '!seen[\$0]++'")
keys '<esc>'
sleep 1
echo "Home: $(echo "$names" | tr '\n' '|')"

# Each application from its icon.
n=1
echo "$names" | while IFS= read -r name; do
	[ -n "$name" ] || continue
	before=$(maps)
	pointer move 23 17 sleep 300 down sleep 60 up sleep 1500
	set -- $(icon "$name")
	pointer move "${1:-0}" "${2:-0}" sleep 400 down sleep 60 up sleep 9000
	after=$(maps)
	file=$(printf '%02d-%s.png' "$n" "$(echo "$name" | tr 'A-Z ' 'a-z-')")
	shot "$file"
	if [ "${after:-0}" -gt "${before:-0}" ] 2>/dev/null; then
		echo "launch: $name mapped ($file)"
	else
		echo "launch: $name NO WINDOW ($file)"
		touch "$out/failed"
	fi
	n=$((n + 1))
done
[ -f "$out/failed" ] && status=1
rm -f "$out/failed"
shot 99-all.png

errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
guest 'grep -E "ZWL (MAP|HOME|TITLE|CLIENT|BOUNDS|ERROR)|ZWL GLASS (placed|moved)" /tmp/zdesktop.log' > "$out/zdesktop.log"
guest "$stop_all" >/dev/null
[ $status = 0 ] && echo "apphome: PASS (and judge the sheet)" || echo "apphome: FAIL"
exit $status
