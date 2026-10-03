#!/bin/sh
# ws005-p030 (BUG-148): the system bar's network menu puts the network it is on first, with a Disconnect button on its
# row (no "Disconnect from X" row at the bottom), and lists only the networks that fit under the bar, on the Venus guest
# of the Settings image (zdesktop and network-probe).  networkd's socket is moved aside and network-probe stands in
# (Kei Lab with a key saved first, Cafe Guest, Neighbor 5G).
#  1. 1280x800: the menu, a click on Kei Lab (joined): the first network row after the switch's lines is Kei Lab, the
#     button is logged (ZWL NETWORK disconnect ... ssid=Kei Lab) and no row reads "Disconnect from" (joined.png).
#  2. zdesktop again at 1280x230 (the stand-in stays connected): the menu lists Kei Lab and a note "2 more in Settings >
#     Wi-Fi" instead of the two others, and its height is within 230 - 34 - 6 - 8 = 182 (short.png).
#  3. A click on the button: the stand-in is told to disconnect (op=36) and the state says disconnected (left.png).
#  networkd's socket back and root's /etc/wifi.conf removed at the end; no ERROR line in zdesktop's log.
#
#   plan/ws089/tests/settings-guest.sh start     (the guest must be up, the Settings image)
#   plan/ws005/tests/menu-bug148.sh [OUTDIR]     (default build/ws005-shots/bug148)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws089-run}"
export GUEST_RUNTIME
out=${1:-build/ws005-shots/bug148}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1 </dev/null; }
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" --height "${PH:-800}" "$@"; }
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
	guest "grep 'ZWL NETWORK icon' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([0-9]*\) y=\([0-9]*\) width=\([0-9]*\) height=\([0-9]*\).*/\1 \2 \3 \4/p'
}
# The place of the last laid-out menu row whose text is $1.
row() {
	guest "grep 'ZWL NETWORK row .*text=$1\$' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([0-9]*\) y=\([0-9]*\) width=\([0-9]*\) height=\([0-9]*\).*/\1 \2 \3 \4/p'
}
# Clicks a control of Settings' page by its index, where Settings last logged it.
control() {
	set -- $(guest "grep 'ZSETTINGS CONTROL index=$1 ' /tmp/s.log | tail -1" | sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\) width=\([0-9]*\) height=\([0-9]*\).*/\1 \2 \3 \4/p')
	if [ -z "${1:-}" ]; then
		echo "control: not found"
		status=1
		return
	fi
	click $((wx + $1 + $3 / 2)) $((wy + $2 + $4 / 2))
}

# The rows of the last layout logged.
last_rows() {
	guest "grep -E 'ZWL NETWORK (menu|row|disconnect) ' /tmp/zdesktop.log" | awk '/ZWL NETWORK menu /{block=""} {block=block $0 "\n"} END{printf "%s", block}'
}

# 0. A key for Kei Lab (against the real networkd), then the stand-in and zdesktop.
guest "$stop_all" >/dev/null
guest 'rm -f /etc/wifi.conf; net wifi add "Kei Lab" --password keilab-2026 --auto yes; echo saved' >/dev/null
guest 'mv /run/networkd.sock /run/networkd.sock.real; /bin/network-probe 600 > /tmp/probe.log 2>&1 </dev/null & sleep 1; echo started' >/dev/null
expect_log /tmp/probe.log 'NETPROBE listening'
guest "$start_desktop" >/dev/null
expect_log /tmp/zdesktop.log 'ZWL NETWORK state reachable=1 '

# 1. Joined to Kei Lab: first, with its button.
set -- $(icon)
ix=$(($1 + $3 / 2)); iy=$(($2 + $4 / 2))
click $ix $iy 1500
expect_log /tmp/zdesktop.log 'ZWL NETWORK scan count=3'
set -- $(row 'Kei Lab')
click $(($1 + 150)) $(($2 + $4 / 2)) 1500
expect_log /tmp/zdesktop.log 'ZWL NETWORK state .*wifi=connected ssid=Kei Lab'
expect_log /tmp/zdesktop.log 'ZWL NETWORK disconnect .*ssid=Kei Lab'
last_rows > "$out/rows-joined.txt"
first=$(grep 'ZWL NETWORK row ' "$out/rows-joined.txt" | grep -E 'kind=(3|5) ' | head -1 | sed -n 's/.*text=//p')
[ "$first" = "Kei Lab" ] && echo "first network: Kei Lab ok" || { echo "first network: '$first' FAIL"; status=1; }
grep -q 'text=Disconnect from' "$out/rows-joined.txt" && { echo "a Disconnect from row: FAIL"; status=1; } || echo "no Disconnect from row: ok"
pointer move 1100 500 sleep 500
shot joined.png

# 2. A short screen: the list cut, the rest named.
# zdesktop alone ends (the stand-in keeps its connection to Kei Lab).
guest 'for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)" | awk "{print \$1}"); do kill $p; done; sleep 1; echo stopped' >/dev/null
PH=230
start_short=$(echo "$start_desktop" | sed 's/--height=800/--height=230/')
guest "$start_short" >/dev/null
expect_log /tmp/zdesktop.log 'ZWL NETWORK icon x='
set -- $(icon)
click $(($1 + $3 / 2)) $(($2 + $4 / 2)) 1500
expect_log /tmp/zdesktop.log 'ZWL NETWORK row .*text=[0-9]+ more in Settings > Wi-Fi'
last_rows > "$out/rows-short.txt"
height=$(grep 'ZWL NETWORK menu ' "$out/rows-short.txt" | tail -1 | sed -n 's/.* height=\([0-9]*\).*/\1/p')
[ "${height:-999}" -le 182 ] 2>/dev/null && echo "menu height $height fits: ok" || { echo "menu height ${height:-?}: FAIL"; status=1; }
shot short.png

# 3. The button disconnects.
set -- $(guest "grep 'ZWL NETWORK disconnect ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([0-9]*\) y=\([0-9]*\) width=\([0-9]*\) height=\([0-9]*\).*/\1 \2 \3 \4/p')
click $(($1 + $3 / 2)) $(($2 + $4 / 2)) 1500
expect_log /tmp/probe.log 'NETPROBE request op=36'
expect_log /tmp/zdesktop.log 'ZWL NETWORK state .*wifi=disconnected'
shot left.png

# zdesktop saw no error; networkd's socket back and the test key gone.
errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
guest 'grep "ZWL NETWORK" /tmp/zdesktop.log' > "$out/zdesktop-network.log"
guest "$stop_all" >/dev/null
guest 'rm -f /run/networkd.sock; mv /run/networkd.sock.real /run/networkd.sock; rm -f /etc/wifi.conf; net show' > "$out/net-show.txt"
grep -q 'online' "$out/net-show.txt" && echo "networkd: back" || { echo "networkd: not back"; status=1; }
[ $status = 0 ] && echo "menu-bug148: PASS" || echo "menu-bug148: FAIL"
exit $status
