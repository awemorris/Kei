#!/bin/sh
# ws005-p029 (BUG-160): the key field of a network that asks for a key stands under that network's row, in the system
# bar's network menu, on the Venus guest of the Settings image (zdesktop and network-probe; plan/ws089/tests/build-settings-image.sh).
# networkd's socket is moved aside and network-probe stands in (three networks: Kei Lab, Cafe Guest, Neighbor 5G;
# Neighbor 5G asks for a key and has none saved).  The run:
#  1. The menu, the scan, a click on Neighbor 5G: the key field opens (ZWL NETWORK key open ssid=Neighbor 5G) and the
#     menu's rows put "Key for Neighbor 5G", the field and "Enter: join   Esc: cancel" right after the Neighbor 5G row
#     (the row indexes follow one another), before the wired line (inline.png).
#  2. A key of 4 characters and Enter: "The key must be 8 to 63 characters" is the row right after the field's note
#     (short.png).  Esc closes the field.
#  networkd's socket back and root's /etc/wifi.conf removed at the end; no ERROR line in zdesktop's log.
#
#   plan/ws089/tests/settings-guest.sh start     (the guest must be up, the Settings image)
#   plan/ws005/tests/inline-key-bug160.sh [OUTDIR]   (default build/ws005-shots/bug160)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws089-run}"
export GUEST_RUNTIME
out=${1:-build/ws005-shots/bug160}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1 </dev/null; }
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; sleep 1; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[s]ettings|[n]etwork-probe" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[s]ettings|[n]etwork-probe" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
start_desktop='export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0; picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=600 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & for w in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20; do grep -q ZWL.READY /tmp/zdesktop.log 2>/dev/null && break; sleep 0.5; done; sleep 1; echo started'
status=0

# Fails the run unless a log has a line matching a pattern (within a few seconds).
expect_log() {
	tries=0
	found=0
	while [ $tries -lt 8 ]; do
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
click() {
	pointer move $(($1 - 2)) "$2" sleep 150 move "$1" "$2" sleep 300 down sleep 60 up sleep "${3:-900}"
}
shot() {
	check "$out/$1" >/dev/null
	echo "shot: $out/$1"
}
icon() {
	guest "grep -a 'ZWL NETWORK icon' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([0-9]*\) y=\([0-9]*\) width=\([0-9]*\) height=\([0-9]*\).*/\1 \2 \3 \4/p'
}
row() {
	guest "grep -a 'ZWL NETWORK row .*text=$1\$' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([0-9]*\) y=\([0-9]*\) width=\([0-9]*\) height=\([0-9]*\).*/\1 \2 \3 \4/p'
}
# The index of the last laid-out row whose text is $1 (from the last layout logged).
row_index() {
	guest "grep -a 'ZWL NETWORK row ' /tmp/zdesktop.log | tail -40" | grep "text=$1\$" | tail -1 | sed -n 's/.* index=\([0-9]*\) .*/\1/p'
}

# 0. The stand-in and zdesktop (no key saved for Neighbor 5G).
guest "$stop_all" >/dev/null
guest 'rm -f /tmp/wifi.conf.bug160; [ -f /etc/wifi.conf ] && mv /etc/wifi.conf /tmp/wifi.conf.bug160; mv /run/networkd.sock /run/networkd.sock.real; /bin/network-probe 600 > /tmp/probe.log 2>&1 </dev/null & sleep 1; echo started' >/dev/null
expect_log /tmp/probe.log 'NETPROBE listening'
guest "$start_desktop" >/dev/null
expect_log /tmp/zdesktop.log 'ZWL NETWORK state reachable=1 '

# 1. The menu and a click on Neighbor 5G: the field right under its row.
set -- $(icon)
click $(($1 + $3 / 2)) $(($2 + $4 / 2)) 1500
expect_log /tmp/zdesktop.log 'ZWL NETWORK scan count=3'
expect_log /tmp/zdesktop.log 'ZWL NETWORK row .*text=Neighbor 5G$'
set -- $(row 'Neighbor 5G')
click $(($1 + 150)) $(($2 + $4 / 2)) 1500
expect_log /tmp/zdesktop.log 'ZWL NETWORK key open ssid=Neighbor 5G'
expect_log /tmp/zdesktop.log 'ZWL NETWORK row .*text=Key for Neighbor 5G'
ap=$(row_index 'Neighbor 5G'); note=$(row_index 'Key for Neighbor 5G'); finish=$(row_index 'Enter: join   Esc: cancel')
wired=$(guest "grep -a 'ZWL NETWORK row ' /tmp/zdesktop.log | tail -40" | grep 'text=Wired' | tail -1 | sed -n 's/.* index=\([0-9]*\) .*/\1/p')
echo "rows: Neighbor 5G=$ap note=$note finish=$finish wired=$wired"
[ -n "$ap" ] && [ "${note:-0}" = $((ap + 1)) ] && [ "${finish:-0}" = $((ap + 3)) ] && [ "${wired:-0}" -gt "${finish:-99}" ] &&
    echo "inline: ok" || { echo "inline: FAIL"; status=1; }
pointer move 1100 500 sleep 500
shot inline.png

# 2. A short key: the refusal right under the field's note.
keys 'p160' '\n'
expect_log /tmp/zdesktop.log 'ZWL NETWORK row .*text=The key must be 8 to 63 characters'
short=$(row_index 'The key must be 8 to 63 characters'); finish=$(row_index 'Enter: join   Esc: cancel')
[ -n "$short" ] && [ "$short" = $((finish + 1)) ] && echo "refusal under the field: ok" || { echo "refusal under the field: FAIL ($short after $finish)"; status=1; }
shot short.png
keys '<esc>'
expect_log /tmp/zdesktop.log 'ZWL NETWORK key cancel'

# zdesktop saw no error; networkd's socket and root's store back.
errors=$(guest "grep -ac ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
guest 'grep -a "ZWL NETWORK" /tmp/zdesktop.log' > "$out/zdesktop-network.log"
guest "$stop_all" >/dev/null
guest 'rm -f /run/networkd.sock; mv /run/networkd.sock.real /run/networkd.sock; rm -f /etc/wifi.conf; [ -f /tmp/wifi.conf.bug160 ] && mv /tmp/wifi.conf.bug160 /etc/wifi.conf; net show' > "$out/net-show.txt"
grep -q 'online' "$out/net-show.txt" && echo "networkd: back" || { echo "networkd: not back"; status=1; }
[ $status = 0 ] && echo "inline-key-bug160: PASS" || echo "inline-key-bug160: FAIL"
exit $status
