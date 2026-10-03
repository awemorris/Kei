# ws089: waiting for the Venus guest before a Settings test, and finding its window (sourced by the settings-p0NN.sh tests).
#
# plan/ws089/tests/settings-guest.sh start returns before the guest's SSH answers; a test that starts at once
# loses its first commands (ws089-p004's first run).  wait_guest waits until a command runs in the guest (at most
# two minutes); wait_desktop waits until zdesktop, started by the test, says ZWL READY in /tmp/zdesktop.log.
# Both need guest() and set status=1 on a timeout.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib

wait_guest() {
	tries=0
	while [ $tries -lt 60 ]; do
		answer=$(guest 'echo guest-up' | tail -1)
		[ "$answer" = guest-up ] && { echo "guest: up"; return 0; }
		tries=$((tries + 1))
		sleep 2
	done
	echo "guest: no answer"
	status=1
	return 1
}

wait_desktop() {
	tries=0
	while [ $tries -lt 30 ]; do
		ready=$(guest 'grep -c "ZWL READY" /tmp/zdesktop.log 2>/dev/null' | tail -1)
		[ "${ready:-0}" -gt 0 ] 2>/dev/null && { echo "zdesktop: ready"; return 0; }
		tries=$((tries + 1))
		sleep 1
	done
	echo "zdesktop: not ready"
	status=1
	return 1
}

# Finds the window of the application under test (BUG-146): the last ZWL MAP line of a client that is not the input
# method.  zdesktop starts keiland-ime itself when it is installed, and the method connects first (its client is logged
# as "ZWL CLIENT client=N ... ime=1" and maps no window), so the window under test is not always client 1.
# Waits up to 15 seconds for the window to map (Settings' first frame on llvmpipe can take a few seconds, ws089-p012).
# Sets wclient, wsurface, wx and wy (0 0 when there is no window).
find_window() {
	tries=0
	line=
	while [ $tries -lt 15 ]; do
		line=$(guest "grep -E 'ZWL (MAP client=|CLIENT client=[0-9]+ .*ime=1)' /tmp/zdesktop.log" |
		    awk '/ime=1/ { split($3, part, "="); ime[part[2]] = 1; next }
		        /ZWL MAP client=/ { split($3, part, "="); if (!(part[2] in ime)) last = $0 }
		        END { print last }')
		[ -n "$line" ] && break
		tries=$((tries + 1))
		sleep 1
	done
	wclient=$(echo "$line" | sed -n 's/.*ZWL MAP client=\([0-9]*\) .*/\1/p')
	set -- $(echo "$line" | sed -n 's/.* surface=\([0-9]*\) x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2 \3/p')
	wsurface=${1:-}; wx=${2:-0}; wy=${3:-0}
}
