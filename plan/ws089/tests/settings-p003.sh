#!/bin/sh
# ws089-p003: the network pages of Settings on the Venus guest (the lean image with the networkd stand-in,
# build-settings-image.sh).  zdesktop --glass at 1280x800; settings runs as root (its key store is /etc/wifi.conf).
#  1. The real networkd (QEMU: a wired interface, no Wi-Fi radio): the Network page shows the wired connection,
#     its address and the DNS servers (network-real.png); Ethernet shows the interface (ethernet.png); Wi-Fi says
#     there is no radio (wifi-absent.png).
#  2. The networkd stand-in with a Wi-Fi radio (network-probe; networkd's socket moved aside meanwhile), with a key
#     saved beforehand for "Kei Lab" (net wifi add, against the real networkd):
#     a. the Wi-Fi page scans (NETWORK scan count=3) and lists Kei Lab as Saved (wifi-list.png);
#     b. a click on Kei Lab joins it (probe: op=35 ssid=Kei Lab; Connected) (wifi-joined.png);
#     c. a click on Neighbor 5G opens the key's line; a key typed and Enter saves it (NETWORK save-key ok, the SSID
#        in /etc/wifi.conf), tells the daemon (op=37) and joins (op=35 ssid=Neighbor 5G; Connected)
#        (wifi-key.png, wifi-key-joined.png);
#     d. Disconnect (op=36), and the switch turns the Wi-Fi off (op=33; Wi-Fi is off) (wifi-off.png);
#     e. the Network page with the stand-in's Wi-Fi back on (network-probe.png).
#  3. networkd's socket back; no ERROR line in zdesktop's log.
# Part 2 is QEMU-only faking: the radio, the scan and the joins are the stand-in's.
#
#   plan/ws089/tests/settings-guest.sh start     (the guest must be up)
#   plan/ws089/tests/settings-p003.sh [OUTDIR]   (default build/ws089-shots/p003)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws089-run}"
export GUEST_RUNTIME
out=${1:-build/ws089-shots/p003}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1 </dev/null; }
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; sleep 0.7; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[s]ettings|[n]etwork-probe" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[s]ettings|[n]etwork-probe" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
start_desktop='export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0; picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=900 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4; echo started'
status=0
. plan/ws089/tests/settings-wait.sh

# Fails the run unless a log has a line matching a pattern (within a few seconds).
expect_log() {
	tries=0
	found=0
	while [ $tries -lt 10 ]; do
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

# Starts settings on a page (its log in /tmp/s.log) and finds its window.
start_settings() {
	guest "export XDG_RUNTIME_DIR=/tmp HOME=/root; /bin/settings --timeout-s=800 $1 > /tmp/s.log 2>&1 </dev/null & sleep 5; echo started" >/dev/null
	find_window
	echo "settings: window at $wx,$wy"
}

# Clicks a control of the page by its index, where settings last logged it (window coordinates).
control() {
	which=$1
	delay=${2:-1200}
	set -- $(guest "grep 'ZSETTINGS CONTROL index=$which ' /tmp/s.log | tail -1" | sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\) width=\([0-9]*\) height=\([0-9]*\).*/\1 \2 \3 \4/p')
	if [ -z "${1:-}" ]; then
		echo "control $which: not found"
		status=1
		return
	fi
	cx=$((wx + $1 + $3 / 2)); cy=$((wy + $2 + $4 / 2))
	pointer move $((cx - 2)) "$cy" sleep 150 move "$cx" "$cy" sleep 300 down sleep 60 up sleep "$delay"
}

# A picture of the screen with the pointer out of the way.
shot() {
	pointer move 1270 790 sleep 600
	check "$out/$1" >/dev/null
	echo "shot: $out/$1"
}

# 1. The real networkd.
guest "$stop_all" >/dev/null
guest "$start_desktop" >/dev/null
start_settings network
expect_log /tmp/s.log 'ZSETTINGS NETWORK open links=[1-9]'
expect_log /tmp/s.log 'ZSETTINGS NETWORK state reachable=1 connected=1 kind=1 interface=[a-z]+[0-9]+ wifi=0'
guest "grep -E 'NETWORK (open|state)' /tmp/s.log | tail -2"
shot network-real.png
control 7
expect_log /tmp/s.log 'ZSETTINGS PAGE ethernet$'
shot ethernet.png
guest "pid=\$(ps -A -o pid,args | grep '[s]ettings' | awk '{print \$1}'); kill \$pid" >/dev/null
sleep 1
start_settings wifi
expect_log /tmp/s.log 'ZSETTINGS PAGE wifi'
shot wifi-absent.png

# 2. The stand-in, with a key saved for Kei Lab first (against the real networkd).
guest "$stop_all" >/dev/null
guest 'rm -f /etc/wifi.conf; net wifi add "Kei Lab" --password keilab-2026 --auto yes; echo saved' >/dev/null
guest 'mv /run/networkd.sock /run/networkd.sock.real; /bin/network-probe 600 > /tmp/probe.log 2>&1 </dev/null & sleep 1; echo started' >/dev/null
expect_log /tmp/probe.log 'NETPROBE listening'
guest "$start_desktop" >/dev/null
start_settings wifi

# a. The scan and the list.
expect_log /tmp/s.log 'ZSETTINGS NETWORK scan count=3'
expect_log /tmp/s.log 'ZSETTINGS CONTROL index=102 '
shot wifi-list.png

# b. Kei Lab, saved, joins at once.
control 100
expect_log /tmp/probe.log 'NETPROBE request op=35 ssid=Kei Lab'
expect_log /tmp/s.log 'ZSETTINGS NETWORK state .*wifi=4 ssid=Kei Lab'
expect_log /tmp/s.log 'ZSETTINGS NETWORK message bad=0 text=Connected to Kei Lab.'
shot wifi-joined.png

# c. Neighbor 5G: the key's line, a key, Enter.
control 102
expect_log /tmp/s.log 'ZSETTINGS NETWORK key-form ssid=Neighbor 5G'
keys 'osc-demo-2026'
shot wifi-key.png
keys '<ret>'
expect_log /tmp/s.log 'ZSETTINGS NETWORK save-key ok'
expect_log /tmp/probe.log 'NETPROBE request op=37'
expect_log /tmp/probe.log 'NETPROBE request op=35 ssid=Neighbor 5G'
expect_log /tmp/s.log 'ZSETTINGS NETWORK message bad=0 text=Connected to Neighbor 5G.'
saved=$(guest "grep -ciE '4e65696768626f72203547|Neighbor 5G' /etc/wifi.conf" | tail -1)
if [ "${saved:-0}" -gt 0 ] 2>/dev/null; then echo "store: Neighbor 5G saved ok"; else echo "store: Neighbor 5G MISSING"; status=1; fi
shot wifi-key-joined.png

# d. Disconnect, and the switch off.
control 3
expect_log /tmp/probe.log 'NETPROBE request op=36'
control 1
expect_log /tmp/probe.log 'NETPROBE request op=33'
expect_log /tmp/s.log 'ZSETTINGS NETWORK state .*wifi=1 '
shot wifi-off.png

# e. The Wi-Fi on again, and the Network page.
control 1
expect_log /tmp/probe.log 'NETPROBE request op=32'
guest "pid=\$(ps -A -o pid,args | grep '[s]ettings' | awk '{print \$1}'); kill \$pid" >/dev/null
sleep 1
start_settings network
expect_log /tmp/s.log 'ZSETTINGS NETWORK scan count=3'
shot network-probe.png

# 3. zdesktop saw no error; networkd's socket back.
errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; guest "grep ERROR /tmp/zdesktop.log | head -5"; status=1; }
guest 'cat /tmp/s.log' > "$out/settings.log"
guest 'cat /tmp/probe.log' > "$out/probe.log"
guest "$stop_all" >/dev/null
guest 'rm -f /run/networkd.sock; mv /run/networkd.sock.real /run/networkd.sock; rm -f /etc/wifi.conf; net show' > "$out/net-show.txt"
grep -q 'online' "$out/net-show.txt" && echo "networkd: back" || { echo "networkd: not back"; status=1; }
[ $status = 0 ] && echo "settings-p003: PASS" || echo "settings-p003: FAIL"
exit $status
