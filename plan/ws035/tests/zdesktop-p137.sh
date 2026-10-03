#!/bin/sh
# ws035-p137 (BUG-113): when the front window closes, the keyboard goes to the next window below it on the
# desktop shown, on the Venus guest.  zdesktop --glass at 1280x800; /bin/popup-probe windows (each logs
# "POPUPPROBE focus window" on keyboard enter and "POPUPPROBE key K" for each key it hears).
#  1. Closed by its client's end: probes a then b (b in front, with the keyboard); b's process is killed.  a hears
#     enter again, and the key x (45) reaches a.
#  2. Closed by its close button (xdg_toplevel.close, the probe ends): probe c over a; c's close button pressed.
#     a hears enter again, and the key x reaches a.
#  3. On another desktop: desktop 2 shown (Ctrl+Alt+Right), probe d opened there and closed.  a (on desktop 1)
#     does not take the keyboard while desktop 2 is shown; back on desktop 1 (Ctrl+Alt+Left) it does.
#  4. The last window closed: a killed; nothing has the keyboard, and zdesktop logs no error.
#
#   plan/ws035/tests/zdesktop-guest.sh start IMAGE     (the guest must be up)
#   plan/ws035/tests/zdesktop-p137.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws035-sq-run}"
export GUEST_RUNTIME
out=${1:-build/ws035-shots/p137}
mkdir -p "$out"
# The SSH to the guest, tried again when ssh itself fails (plan/ws099/tests/guest-retry.sh, ws099-p023).
. plan/ws099/tests/guest-retry.sh
guest() { guest_retry 90 "$1" </dev/null; }
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; sleep 0.8; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[p]opup-probe" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[p]opup-probe" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
status=0

# Prints how many lines of a log match a pattern now.
count() {
	guest "grep -cE '$2' $1" | tail -1
}

# Fails the run unless a log has (within a few seconds) as many lines matching a pattern as asked (default 1).
expect_log() {
	tries=0
	found=0
	while [ $tries -lt 6 ]; do
		found=$(count "$1" "$2")
		[ "${found:-0}" -ge "${3:-1}" ] 2>/dev/null && break
		tries=$((tries + 1))
		sleep 1
	done
	if [ "${found:-0}" -ge "${3:-1}" ] 2>/dev/null; then
		echo "log: $1 $2 (${3:-1}) ok"
	else
		echo "log: $1 $2 (${3:-1}) MISSING (found ${found:-0})"
		status=1
	fi
}

# Fails the run if a log has more lines matching a pattern than a number.
expect_at_most() {
	found=$(count "$1" "$2")
	if [ "${found:-0}" -le "$3" ] 2>/dev/null; then
		echo "log: $1 $2 at most $3 ok"
	else
		echo "log: $1 $2 at most $3 FAIL (found $found)"
		status=1
	fi
}

# Starts a probe with a token; its log is /tmp/probe-TOKEN.log, its pid in /tmp/probe-TOKEN.pid (the guest's
# ps shows no arguments, so a probe is found by its pid, not by its token).
start_probe() {
	guest "export XDG_RUNTIME_DIR=/tmp; /bin/popup-probe --timeout-s=400 --token=$1 > /tmp/probe-$1.log 2>&1 </dev/null & echo \$! > /tmp/probe-$1.pid; sleep 3; echo started" >/dev/null
	expect_log "/tmp/probe-$1.log" "POPUPPROBE ready run=$1"
}

# Ends a probe's process, and fails the run if it is still there.
kill_probe() {
	gone=$(guest "kill \$(cat /tmp/probe-$1.pid); sleep 2; kill -0 \$(cat /tmp/probe-$1.pid) 2>/dev/null && echo alive || echo gone" | tail -1)
	echo "probe $1 killed: $gone"
	[ "$gone" = gone ] || status=1
}

guest "$stop_all" >/dev/null
guest 'rm -f /tmp/probe-*.log /tmp/probe-*.pid; export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0; picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=600 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & for w in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30; do grep -q ZWL.READY /tmp/zdesktop.log 2>/dev/null && break; sleep 0.5; done; sleep 1; echo started' >/dev/null
pointer move 5 790 sleep 300

# 1. a, then b over it; b's process ends.
start_probe a
expect_log /tmp/probe-a.log 'POPUPPROBE focus window' 1
start_probe b
expect_log /tmp/probe-b.log 'POPUPPROBE focus window' 1
expect_log /tmp/probe-a.log 'POPUPPROBE unfocus window' 1
kill_probe b
expect_log /tmp/probe-a.log 'POPUPPROBE focus window' 2
keys 'x'
expect_log /tmp/probe-a.log 'POPUPPROBE key 45 state=1' 1
check "$out/killed.png" >/dev/null

# 2. c over a; c's close button (the title bar's right end; c is centred at 440,250 like a, 400 wide).
start_probe c
expect_log /tmp/probe-c.log 'POPUPPROBE focus window' 1
set -- $(guest "grep 'ZWL MAP client=' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2/p')
cx=${1:-440}; cy=${2:-250}
echo "c at $cx,$cy"
pointer move $((cx + 400 - 26)) $((cy - 8 - 22)) sleep 400 down sleep 60 up sleep 1500
expect_log /tmp/probe-c.log 'POPUPPROBE DONE run=c' 1
expect_log /tmp/probe-a.log 'POPUPPROBE focus window' 3
pointer move 5 790 sleep 300
keys 'x'
expect_log /tmp/probe-a.log 'POPUPPROBE key 45 state=1' 2
check "$out/closed.png" >/dev/null

# 3. Desktop 2, d opened and closed there: a (desktop 1) stays without the keyboard until desktop 1 is shown.
keys '<ctrl-alt-right>'
sleep 1.5
start_probe d
expect_log /tmp/probe-d.log 'POPUPPROBE focus window' 1
before=$(count /tmp/probe-a.log 'POPUPPROBE focus window')
kill_probe d
sleep 1
after=$(count /tmp/probe-a.log 'POPUPPROBE focus window')
if [ "${after:-0}" = "${before:-0}" ]; then
	echo "a on desktop 1 did not take the keyboard while desktop 2 is shown ok"
else
	echo "a took the keyboard on another desktop ($before -> $after) FAIL"
	status=1
fi
check "$out/desktop2.png" >/dev/null
keys '<ctrl-alt-left>'
sleep 1.5
expect_log /tmp/probe-a.log 'POPUPPROBE focus window' $((before + 1))

# 4. The last window closed.
kill_probe a
sleep 1
check "$out/empty.png" >/dev/null

# Nothing failed.
guest 'grep -E "ERROR|FAILED|protocol error" /tmp/zdesktop.log' | tee "$out/errors.txt"
[ -s "$out/errors.txt" ] && status=1
guest 'cat /tmp/probe-a.log' > "$out/probe-a.log"
guest 'grep -E "MAP|UNMAP|GLASS (desktop|move)" /tmp/zdesktop.log' > "$out/zdesktop.log"
guest "$stop_all" >/dev/null
[ $status -eq 0 ] && echo "p137: PASS" || echo "p137: FAIL"
exit $status
