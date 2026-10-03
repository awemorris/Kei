#!/bin/sh
# ws005-p026 (BUG-154): "Connecting..." while a Wi-Fi join is under way, in the system bar's network menu and in
# Settings' Wi-Fi page, on the Venus guest of the Settings image (plan/ws089/tests/build-settings-image.sh: zdesktop,
# Settings and network-probe).  networkd's socket is moved aside and network-probe stands in with a radio whose
# joins take 6 seconds (network-probe 600 6), the state left as it was meanwhile; "Kei Lab" has a key saved first.
#  1. The system bar's menu: a click on "Kei Lab" says at once "Connecting to Kei Lab..." under the switch and
#     "Connecting..." on the row (ZWL NETWORK connecting ssid=Kei Lab; bar-connecting.png), then it is joined and
#     checked (connecting cleared; bar-joined.png).
#  2. Switching: a click on "Cafe Guest" while on Kei Lab says "Connecting to Cafe Guest..." and Kei Lab loses its
#     check meanwhile (bar-switching.png), then Cafe Guest is joined (bar-switched.png).
#  3. Settings' Wi-Fi page: a click on Kei Lab says "Connecting to Kei Lab..." and the row "Connecting..."
#     (settings-connecting.png), then "Connected to Kei Lab." (settings-joined.png).
#  networkd's socket back and root's /etc/wifi.conf removed at the end; no ERROR line in zdesktop's log.
# The radio and the joins are the stand-in's (QEMU-only faking); the key is a made-up test key.
#
#   plan/ws089/tests/settings-guest.sh start     (the guest must be up, the Settings image)
#   plan/ws005/tests/connecting-bug154.sh [OUTDIR]   (default build/ws005-shots/bug154)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws089-run}"
export GUEST_RUNTIME
out=${1:-build/ws005-shots/bug154}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1 </dev/null; }
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[s]ettings|[n]etwork-probe" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[s]ettings|[n]etwork-probe" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
start_desktop='export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0; picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=900 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & for w in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20; do grep -q ZWL.READY /tmp/zdesktop.log 2>/dev/null && break; sleep 0.5; done; sleep 1; echo started'
status=0
. plan/ws089/tests/settings-wait.sh

# Fails the run unless a log has a line matching a pattern (within a few seconds).
expect_log() {
	tries=0
	found=0
	while [ $tries -lt ${3:-10} ]; do
		found=$(guest "grep -acE '$2' $1" | tail -1)
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
# Clicks a point and waits a little.
click() {
	pointer move $(($1 - 2)) "$2" sleep 150 move "$1" "$2" sleep 300 down sleep 60 up sleep "${3:-400}"
}
# A picture of the screen (the pointer left where it is, on the menu).
shot() {
	check "$out/$1" >/dev/null
	echo "shot: $out/$1"
}
# The middle of the network icon, from zdesktop's log.
icon() {
	guest "grep -a 'ZWL NETWORK icon' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([0-9]*\) y=\([0-9]*\) width=\([0-9]*\) height=\([0-9]*\).*/\1 \2 \3 \4/p'
}
# The place of the last laid-out menu row whose text is $1.
row() {
	guest "grep -a 'ZWL NETWORK row .*text=$1\$' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([0-9]*\) y=\([0-9]*\) width=\([0-9]*\) height=\([0-9]*\).*/\1 \2 \3 \4/p'
}
# Clicks a control of Settings' page by its index, where Settings last logged it.
control() {
	set -- $(guest "grep -a 'ZSETTINGS CONTROL index=$1 ' /tmp/s.log | tail -1" | sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\) width=\([0-9]*\) height=\([0-9]*\).*/\1 \2 \3 \4/p')
	if [ -z "${1:-}" ]; then
		echo "control: not found"
		status=1
		return
	fi
	click $((wx + $1 + $3 / 2)) $((wy + $2 + $4 / 2))
}

# 0. A key for Kei Lab (against the real networkd), then the slow stand-in and zdesktop.
guest "$stop_all" >/dev/null
guest 'rm -f /etc/wifi.conf; net wifi add "Kei Lab" --password keilab-2026 --auto yes; echo saved' >/dev/null
guest 'mv /run/networkd.sock /run/networkd.sock.real; /bin/network-probe 600 6 > /tmp/probe.log 2>&1 </dev/null & sleep 1; echo started' >/dev/null
expect_log /tmp/probe.log 'NETPROBE listening'
guest "$start_desktop" >/dev/null
expect_log /tmp/zdesktop.log 'ZWL NETWORK state reachable=1 .*wifi=disconnected'

# 1. The menu, the scan, and a click on Kei Lab: "Connecting" at once, joined after the stand-in's 6 seconds.
set -- $(icon)
click $(($1 + $3 / 2)) $(($2 + $4 / 2)) 1500
expect_log /tmp/zdesktop.log 'ZWL NETWORK scan count=3'
expect_log /tmp/zdesktop.log 'ZWL NETWORK row .*text=Kei Lab$'
set -- $(row 'Kei Lab')
click $(($1 + 150)) $(($2 + $4 / 2))
expect_log /tmp/zdesktop.log 'ZWL NETWORK connecting ssid=Kei Lab$' 3
expect_log /tmp/zdesktop.log 'ZWL NETWORK row .*text=Connecting to Kei Lab\.\.\.$' 3
shot bar-connecting.png
expect_log /tmp/probe.log 'NETPROBE join waits seconds=6 ssid=Kei Lab'
expect_log /tmp/zdesktop.log 'ZWL NETWORK state .*wifi=connected ssid=Kei Lab'
expect_log /tmp/zdesktop.log 'ZWL NETWORK connecting ssid=$'
sleep 1
shot bar-joined.png

# 2. Switching to Cafe Guest (open): "Connecting" again, then joined.
set -- $(row 'Cafe Guest')
click $(($1 + 150)) $(($2 + $4 / 2))
expect_log /tmp/zdesktop.log 'ZWL NETWORK connecting ssid=Cafe Guest$' 3
expect_log /tmp/zdesktop.log 'ZWL NETWORK row .*text=Connecting to Cafe Guest\.\.\.$' 3
shot bar-switching.png
expect_log /tmp/zdesktop.log 'ZWL NETWORK state .*wifi=connected ssid=Cafe Guest'
sleep 1
shot bar-switched.png
click 400 400 900
expect_log /tmp/zdesktop.log 'ZWL NETWORK close via=outside'

# 3. Settings' Wi-Fi page: Kei Lab again.
guest "export XDG_RUNTIME_DIR=/tmp HOME=/root; /bin/settings --timeout-s=600 wifi > /tmp/s.log 2>&1 </dev/null & sleep 5; echo started" >/dev/null
find_window
echo "settings: window at $wx,$wy"
expect_log /tmp/s.log 'ZSETTINGS NETWORK scan count=3'
expect_log /tmp/s.log 'ZSETTINGS CONTROL index=100 '
control 100
expect_log /tmp/s.log 'ZSETTINGS NETWORK message bad=0 text=Connecting to Kei Lab\.\.\.' 3
shot settings-connecting.png
expect_log /tmp/s.log 'ZSETTINGS NETWORK message bad=0 text=Connected to Kei Lab\.'
sleep 1
shot settings-joined.png

# zdesktop saw no error; networkd's socket back and the test key gone.
errors=$(guest "grep -ac ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
guest 'grep -a "ZWL NETWORK" /tmp/zdesktop.log' > "$out/zdesktop-network.log"
guest 'cat /tmp/s.log' > "$out/settings.log"
guest 'cat /tmp/probe.log' > "$out/probe.log"
guest "$stop_all" >/dev/null
guest 'rm -f /run/networkd.sock; mv /run/networkd.sock.real /run/networkd.sock; rm -f /etc/wifi.conf; net show' > "$out/net-show.txt"
grep -q 'online' "$out/net-show.txt" && echo "networkd: back" || { echo "networkd: not back"; status=1; }
[ $status = 0 ] && echo "connecting-bug154: PASS" || echo "connecting-bug154: FAIL"
exit $status
