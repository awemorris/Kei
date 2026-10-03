#!/bin/sh
# ws134-p003: the System Monitor's 3D parts and motion on the Venus guest (plan/ws134/tests/build-monitor-image.sh; start
# the guest with plan/tools/files/files-guest.sh start IMAGE).  zdesktop --glass at 1280x800.
#  1. The recordings with the clock stopped at 130 s (/usr/share/monitor-tests/): calm8 (Normal), warning (CPU 88% from
#     60 s: ZMON LEVEL Elevated then Warning, the state drawn "Warning"), critical (CPU 97% and the disk latency 60 ms:
#     Elevated, Warning, Critical, the state drawn "Critical").  calm.png, warning.png, critical.png: the core, the CPU
#     relief, the flows, the raised plates' amber or coral edges.
#  2. The simulation (16 CPUs, 2 GPUs) moving for 20 seconds with the pointer moved across the window (the parallax):
#     frames at 15 fps or more (ZMON FRAME), no failure.  sim.png.
# Judged by the monitor's log (/tmp/monitor.log in the guest, read over SSH) and the pictures, not the console.
#
#   plan/ws134/tests/monitor-p003.sh [OUTDIR]
# Prints "monitor-p003: PASS" or "monitor-p003: FAIL".
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws134-p003}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[m]onitor" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[m]onitor" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
status=0

# Fails the run unless the monitor's log has a line matching a pattern (within a few seconds).
expect_log() {
	tries=0
	found=0
	while [ $tries -lt 15 ]; do
		found=$(guest "grep -cE '$1' /tmp/monitor.log" | tail -1)
		[ "${found:-0}" -gt 0 ] 2>/dev/null && break
		tries=$((tries + 1))
		sleep 1
	done
	if [ "${found:-0}" -gt 0 ] 2>/dev/null; then
		echo "log: $1 ok"
	else
		echo "log: $1 MISSING"
		status=1
	fi
}

# Starts zdesktop, then the monitor with options, its log in /tmp/monitor.log.
start() {
	guest "$stop_all" >/dev/null
	guest "export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0
picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=600 --width=1280 --height=800 --glass \$picture > /tmp/zdesktop.log 2>&1 </dev/null & for w in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20; do grep -q ZWL.READY /tmp/zdesktop.log 2>/dev/null && break; sleep 0.5; done; sleep 1
/bin/monitor --timeout-s=300 $1 > /tmp/monitor.log 2>&1 </dev/null & echo started" >/dev/null
}

# Checks that the levels came in order (the names in the order given) and the state drawn last.
expect_levels() {
	levels=$(guest "grep 'ZMON LEVEL' /tmp/monitor.log | sed -n 's/.* level=\([A-Za-z]*\) .*/\1/p' | tr '\n' ' '" | tail -1)
	if [ "$levels" = "$1 " ]; then
		echo "levels: $levels ok"
	else
		echo "levels: '$levels' (want '$1 ') MISSING"
		status=1
	fi
	expect_log "ZMON TEXT plate=state value=\"$2\""
}

# 1. The recordings.
for name in calm8 warning critical; do
	start "--source=replay:/usr/share/monitor-tests/$name.txt --clock=fixed:130000 --range=0 --token=$name"
	expect_log "ZMON READY .* source=replay cpus=8 gpus=1"
	case $name in
	calm8) expect_log 'ZMON TEXT plate=state value="Normal"' ;;
	warning) expect_levels "Elevated Warning" Warning ;;
	critical) expect_levels "Elevated Warning Critical" Critical ;;
	esac
	sleep 3
	pointer move 1270 790 sleep 300
	check "$out/$name.png" >/dev/null
	guest 'cat /tmp/monitor.log' > "$out/$name.log"
done

# 2. The simulation, moving, the pointer across the window.
start '--seed=9 --cpus=16 --gpus=2 --token=s'
expect_log 'ZMON READY .* source=sim cpus=16 gpus=2'
pointer move 200 200 sleep 2000 move 1100 300 sleep 2000 move 640 600 sleep 2000 move 300 700 sleep 2000
sleep 10
check "$out/sim.png" >/dev/null
fps=$(guest "grep 'ZMON FRAME' /tmp/monitor.log | tail -1" | sed -n 's/.*fps=\([0-9]*\).*/\1/p')
[ "${fps:-0}" -ge 15 ] 2>/dev/null && echo "sim: $fps fps ok" || { echo "sim: ${fps:-?} fps (want 15 or more) MISSING"; status=1; }
failed=$(guest "grep -cE 'ZMON (FAILED|DISCONNECTED)|failed' /tmp/monitor.log" | tail -1)
[ "${failed:-1}" = 0 ] && echo "sim: no failure ok" || { echo "sim: failure lines MISSING"; status=1; }
guest 'grep -E "ZMON (READY|FRAME|MEM|LEVEL|VISIBLE)" /tmp/monitor.log' > "$out/sim.log"
errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
guest "$stop_all" >/dev/null
[ $status -eq 0 ] && echo "monitor-p003: PASS" || echo "monitor-p003: FAIL"
exit $status
