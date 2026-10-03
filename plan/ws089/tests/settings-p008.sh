#!/bin/sh
# ws089-p008: Settings' search and Home's tiles on the Venus guest (the lean image, build-settings-image.sh), with the
# real networkd (QEMU: a wired interface, no Wi-Fi radio).  zdesktop --glass at 1280x800.
#  1. Home: the tiles show the state now (Ethernet "Connected · <if>", Network "Online · <address>", Wi-Fi
#     "No Wi-Fi radio", About "Kei · x86_64") (home.png).
#  2. Ctrl+F (the Edit menu's Find, taken by zdesktop's menus) gives the titlebar's search field the keyboard
#     (ZSETTINGS SEARCH focus, TITLEBAR state ... focus=1); "wi" typed lists 4 results (Wi-Fi, Ethernet, Wi-Fi radio, and ws089-p004's Window opacity) (search-wi.png); Enter opens
#     the first (SEARCH open page=wifi).
#  3. Ctrl+F, "dns" typed (2 results: the Network page and its "DNS servers"); a click on the second opens the
#     Network page (SEARCH open page=network) (search-dns.png, search-dns-open.png).
#  4. Ctrl+F, "zzz" typed: nothing matches (search-none.png); Esc ends the search and the page comes back
#     (SEARCH end page=network).
#  5. No ERROR line in zdesktop's log.
#
#   plan/ws089/tests/settings-guest.sh start     (the guest must be up)
#   plan/ws089/tests/settings-p008.sh [OUTDIR]   (default build/ws089-shots/p008)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws089-run}"
export GUEST_RUNTIME
out=${1:-build/ws089-shots/p008}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1 </dev/null; }
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; sleep 0.7; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[s]ettings" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[s]ettings" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
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

# Starts settings (its log in /tmp/s.log) and finds its window.
start_settings() {
	guest "export XDG_RUNTIME_DIR=/tmp HOME=/root; /bin/settings --timeout-s=800 $1 > /tmp/s.log 2>&1 </dev/null & sleep 5; echo started" >/dev/null
	find_window
	echo "settings: window at $wx,$wy"
}

# Clicks a search result by its index, where settings last logged it (window coordinates).
result() {
	which=$1
	set -- $(guest "grep 'ZSETTINGS RESULT index=$which ' /tmp/s.log | tail -1" | sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\) width=\([0-9]*\) height=\([0-9]*\).*/\1 \2 \3 \4/p')
	if [ -z "${1:-}" ]; then
		echo "result $which: not found"
		status=1
		return
	fi
	cx=$((wx + $1 + $3 / 2)); cy=$((wy + $2 + $4 / 2))
	pointer move $((cx - 2)) "$cy" sleep 150 move "$cx" "$cy" sleep 300 down sleep 60 up sleep 1200
}

# A picture of the screen with the pointer out of the way.
shot() {
	pointer move 1270 790 sleep 600
	check "$out/$1" >/dev/null
	echo "shot: $out/$1"
}

# 1. Home with the real networkd.
guest "$stop_all" >/dev/null
guest "$start_desktop" >/dev/null
start_settings ""
expect_log /tmp/s.log 'ZSETTINGS PAGE home'
expect_log /tmp/s.log 'ZSETTINGS NETWORK state reachable=1 connected=1 kind=1'
sleep 2
shot home.png

# 2. Ctrl+F, "wi", Enter.
keys '<ctrl-f>'
expect_log /tmp/s.log 'ZSETTINGS SEARCH focus'
expect_log /tmp/s.log 'ZSETTINGS TITLEBAR state .*focus=1'
keys 'wi'
expect_log /tmp/s.log 'ZSETTINGS SEARCH query=wi results=4'
shot search-wi.png
keys '<ret>'
expect_log /tmp/s.log 'ZSETTINGS SEARCH open page=wifi'
expect_log /tmp/s.log 'ZSETTINGS PAGE wifi'

# 3. Ctrl+F, "dns", a click on the second result.
keys '<ctrl-f>'
keys 'dns'
expect_log /tmp/s.log 'ZSETTINGS SEARCH query=dns results=2'
expect_log /tmp/s.log 'ZSETTINGS RESULT index=1 '
shot search-dns.png
result 1
expect_log /tmp/s.log 'ZSETTINGS SEARCH open page=network'
expect_log /tmp/s.log 'ZSETTINGS PAGE network'
shot search-dns-open.png

# 4. Ctrl+F, "zzz", Esc.
keys '<ctrl-f>'
keys 'zzz'
expect_log /tmp/s.log 'ZSETTINGS SEARCH query=zzz results=0'
shot search-none.png
keys '<esc>'
expect_log /tmp/s.log 'ZSETTINGS SEARCH end page=network'
shot search-ended.png

# 5. zdesktop saw no error.
errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; guest "grep ERROR /tmp/zdesktop.log | head -5"; status=1; }
guest 'cat /tmp/s.log' > "$out/settings.log"
guest "$stop_all" >/dev/null
[ $status = 0 ] && echo "settings-p008: PASS" || echo "settings-p008: FAIL"
exit $status
