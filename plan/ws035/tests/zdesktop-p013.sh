#!/bin/sh
# ws035-p013: the network in the system bar, on the Venus guest (the network image,
# plan/ws035/tests/build-network-image.sh).  zdesktop --glass at 1280x800:
#  1. The real networkd (QEMU: a wired ue0, no Wi-Fi radio): the icon is the wired tree
#     (wired.png); a click opens the menu with "No Wi-Fi hardware" and the wired line
#     (wired-menu.png); a click outside closes it.
#  2. The networkd stand-in with a Wi-Fi radio (network-probe; networkd's socket is moved aside
#     meanwhile and put back at the end): the menu scans and lists three networks (list.png);
#     "Kei Lab" has a key in root's store (put there before the stand-in starts): a click joins
#     it at once, the icon becomes the Wi-Fi bars and the network is checked (joined.png).
#     "Neighbor 5G" asks for a key and has none saved: a click opens the key field in the menu
#     and sends no join (key.png); a key of 4 characters is refused (failed.png); the key
#     completed to 13 characters and Enter save it, tell the daemon and join (key-joined.png;
#     the three steps of ws005-p019, BUG-138).  The switch turns the Wi-Fi off (off.png).
# Part 2 is QEMU-only faking: the radio, the scan and the joins are the stand-in's, and the keys
# are made-up test keys.  root's /etc/wifi.conf is kept aside meanwhile and put back at the end.
#
#   GUEST_RUNTIME=... plan/ws035/tests/zdesktop-guest.sh start build/<x>/hdd-image.img
#   plan/ws035/tests/zdesktop-p013.sh [OUTDIR] [SHOTS PREFIX]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws035-sq-run}"
export GUEST_RUNTIME
out=${1:-build/ws035-p013}
prefix=${2:-}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1 </dev/null; }
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; sleep 1; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[n]etwork-probe" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[n]etwork-probe" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
start_desktop='export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0; picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=600 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & for w in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30; do grep -q ZWL.READY /tmp/zdesktop.log 2>/dev/null && break; sleep 0.5; done; sleep 1; echo started'
status=0

# Fails the run unless a log has a line matching a pattern (within a few seconds).
expect_log() {
	tries=0
	found=0
	while [ $tries -lt 8 ]; do
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
click() {
	pointer move $(($1 - 2)) "$2" sleep 150 move "$1" "$2" sleep 300 down sleep 60 up sleep "${3:-900}"
}
shot() {
	check "$out/$1" >/dev/null
	[ -n "$prefix" ] && cp "$out/$1" "$prefix$1"
}
# The middle of the icon, from zdesktop's log.
icon() {
	guest "grep 'ZWL NETWORK icon' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([0-9]*\) y=\([0-9]*\) width=\([0-9]*\) height=\([0-9]*\).*/\1 \2 \3 \4/p'
}
# The middle of the last laid-out row whose text is $1.
row() {
	guest "grep 'ZWL NETWORK row .*text=$1\$' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([0-9]*\) y=\([0-9]*\) width=\([0-9]*\) height=\([0-9]*\).*/\1 \2 \3 \4/p'
}

# 1. The real networkd.
guest "$stop_all" >/dev/null
guest "$start_desktop" >/dev/null
expect_log /tmp/zdesktop.log 'ZWL NETWORK state reachable=1 connected=1 kind=wired interface=[a-z]+[0-9]+ wifi=absent'
set -- $(icon)
ix=$(($1 + $3 / 2)); iy=$(($2 + $4 / 2))
echo "icon at $ix,$iy"
pointer move 700 400 sleep 400
shot wired.png
click "$ix" "$iy"
expect_log /tmp/zdesktop.log 'ZWL NETWORK open'
expect_log /tmp/zdesktop.log 'ZWL NETWORK row .*text=No Wi-Fi hardware'
expect_log /tmp/zdesktop.log 'ZWL NETWORK row .*text=Wired \([a-z]+[0-9]+\): connected'
pointer move 1100 300 sleep 400
shot wired-menu.png
click 400 400
expect_log /tmp/zdesktop.log 'ZWL NETWORK close via=outside'

# 2. The stand-in: networkd's socket aside, the stand-in in its place, zdesktop again.
guest "$stop_all" >/dev/null
# root's store (zdesktop runs as root here) holds only "Kei Lab", with a made-up key; it is saved
# while the real networkd still answers, so the stand-in is not told of it.
saved=$(guest 'rm -f /tmp/wifi.conf.p013; [ -f /etc/wifi.conf ] && mv /etc/wifi.conf /tmp/wifi.conf.p013; net wifi add "Kei Lab" --password p013-test-key >/dev/null 2>&1; echo "add=$?"' | tail -1)
[ "$saved" = add=0 ] && echo "store: Kei Lab saved" || { echo "store: Kei Lab not saved ($saved)"; status=1; }
guest 'mv /run/networkd.sock /run/networkd.sock.real; /bin/network-probe 240 > /tmp/probe.log 2>&1 </dev/null & sleep 1; echo started' >/dev/null
expect_log /tmp/probe.log 'NETPROBE listening'
guest "$start_desktop" >/dev/null
expect_log /tmp/zdesktop.log 'ZWL NETWORK state reachable=1 connected=1 kind=wired interface=em9 wifi=disconnected'
set -- $(icon)
ix=$(($1 + $3 / 2)); iy=$(($2 + $4 / 2))
click "$ix" "$iy" 1500
expect_log /tmp/zdesktop.log 'ZWL NETWORK request scan'
expect_log /tmp/zdesktop.log 'ZWL NETWORK scan count=3'
expect_log /tmp/zdesktop.log 'ZWL NETWORK row .*text=Kei Lab'
set -- $(row 'Kei Lab')
lx=$(($1 + 150)); ly=$(($2 + $4 / 2))
pointer move "$lx" "$ly" sleep 500
shot list.png

# Joining "Kei Lab", with its saved key: no key field.
click "$lx" "$ly" 1500
expect_log /tmp/probe.log 'NETPROBE request op=35 ssid=Kei Lab'
expect_log /tmp/zdesktop.log 'ZWL NETWORK state reachable=1 connected=1 kind=wifi interface=wlan0 wifi=connected ssid=Kei Lab'
expect_log /tmp/zdesktop.log 'ZWL NETWORK disconnect .*ssid=Kei Lab'
pointer move 1100 500 sleep 500
shot joined.png

# "Kei Lab" took no key field.
opened=$(guest "grep -c 'ZWL NETWORK key open ssid=Kei Lab' /tmp/zdesktop.log" | tail -1)
[ "${opened:-1}" = 0 ] && echo "Kei Lab: no key field ok" || { echo "Kei Lab: key field opened"; status=1; }

# "Neighbor 5G" asks for a key and has none saved: the key field, and no join yet.
set -- $(row 'Neighbor 5G')
click $(($1 + 150)) $(($2 + $4 / 2)) 1500
expect_log /tmp/zdesktop.log 'ZWL NETWORK key open ssid=Neighbor 5G'
expect_log /tmp/zdesktop.log 'ZWL NETWORK row .*text=Key for Neighbor 5G'
joins=$(guest "grep -c 'NETPROBE request op=35 ssid=Neighbor 5G' /tmp/probe.log" | tail -1)
[ "${joins:-1}" = 0 ] && echo "Neighbor 5G: no join before the key ok" || { echo "Neighbor 5G: joined before the key"; status=1; }
pointer move 1100 500 sleep 500
shot key.png

# A key of 4 characters is refused, and kept to be finished.
keys 'p013' '\n'
expect_log /tmp/zdesktop.log 'ZWL NETWORK row .*text=The key must be 8 to 63 characters'
shot failed.png

# The key finished (13 characters) and Enter: saved, the daemon told (37), then the join (35).
keys '-fake-key' '\n'
expect_log /tmp/zdesktop.log 'ZWL NETWORK key saved ssid=Neighbor 5G'
expect_log /tmp/probe.log 'NETPROBE request op=37'
expect_log /tmp/probe.log 'NETPROBE request op=35 ssid=Neighbor 5G'
expect_log /tmp/zdesktop.log 'ZWL NETWORK state reachable=1 connected=1 kind=wifi interface=wlan0 wifi=connected ssid=Neighbor 5G'
expect_log /tmp/zdesktop.log 'ZWL NETWORK disconnect .*ssid=Neighbor 5G'
pointer move 1100 500 sleep 500
shot key-joined.png

# The switch turns the Wi-Fi off; the menu closes.  The wired link (ue0) stays connected, so the icon is the
# wired tree (network.c: a wired connection is shown before the Wi-Fi); the struck-through bars show only with
# no wired link.
set -- $(row 'Wi-Fi')
click $(($1 + 150)) $(($2 + $4 / 2)) 1500
expect_log /tmp/probe.log 'NETPROBE request op=33'
expect_log /tmp/zdesktop.log 'ZWL NETWORK state .*wifi=off'
click 400 400
expect_log /tmp/zdesktop.log 'ZWL NETWORK close via=outside'
pointer move 700 400 sleep 500
shot off.png

errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
guest 'cat /tmp/zdesktop.log' > "$out/zdesktop.log"
guest 'cat /tmp/probe.log' > "$out/probe.log"

# networkd's socket back; net show answers again.
guest "$stop_all" >/dev/null
guest 'rm -f /run/networkd.sock; mv /run/networkd.sock.real /run/networkd.sock; net show' > "$out/net-show.txt"
# root's store as it was (the test keys go).
restored=$(guest 'rm -f /etc/wifi.conf; [ -f /tmp/wifi.conf.p013 ] && mv /tmp/wifi.conf.p013 /etc/wifi.conf; echo restored' | tail -1)
[ "$restored" = restored ] && echo "store: restored" || { echo "store: not restored"; status=1; }
grep -q 'online' "$out/net-show.txt" && echo "networkd: back" || { echo "networkd: not back"; status=1; }
[ $status = 0 ] && echo "zdesktop-p013: PASS" || echo "zdesktop-p013: FAIL"
exit $status
