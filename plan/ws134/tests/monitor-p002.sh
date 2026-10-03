#!/bin/sh
# ws134-p002: the System Monitor's skeleton on the Venus guest (plan/ws134/tests/build-monitor-image.sh; start the guest
# with plan/tools/files/files-guest.sh start IMAGE).  zdesktop --glass at 1280x800.
#  1. A recording with the clock stopped (/usr/share/monitor-tests/normal.txt, --clock=fixed:2000): ZMON READY, and the
#     values drawn (ZMON TEXT) are the recording's: CPU 37%, GPU 21%, Memory 2.1 GiB (of 8 GiB), Network 4.2 Mb/s, Disk 12 MB/s,
#     latency 0.8 ms, state Normal.  replay.png.
#  2. The simulation for 25 seconds: samples every second (ZMON SAMPLE), frames drawn (ZMON FRAME fps > 0), no failure.
#     sim.png, taken after 20 seconds.
# Judged by the monitor's log (/tmp/monitor.log in the guest, read over SSH) and the pictures, not the console.
#
#   plan/ws134/tests/monitor-p002.sh [OUTDIR]
# Prints "monitor-p002: PASS" or "monitor-p002: FAIL".
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws134-p002}
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
	while [ $tries -lt 10 ]; do
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

# 1. The recording, the clock stopped.
start '--source=replay:/usr/share/monitor-tests/normal.txt --clock=fixed:2000 --range=0 --token=r'
expect_log 'ZMON READY width=[0-9]+ height=[0-9]+ source=replay cpus=4 gpus=1'
expect_log 'ZMON TEXT plate=cpu value="37%"'
expect_log 'ZMON TEXT plate=gpu value="21%"'
expect_log 'ZMON TEXT plate=memory value="2.1 GiB"'
expect_log 'ZMON TEXT plate=network value="4.2 Mb/s"'
expect_log 'ZMON TEXT plate=disk value="12 MB/s"'
expect_log 'ZMON TEXT plate=latency value="0.8 ms"'
expect_log 'ZMON TEXT plate=state value="Normal"'
sleep 2
pointer move 1270 790 sleep 300
check "$out/replay.png" >/dev/null
guest 'cat /tmp/monitor.log' > "$out/replay.log"

# 2. The simulation, moving.
start '--seed=3 --token=s'
expect_log 'ZMON READY .* source=sim'
sleep 20
pointer move 1270 790 sleep 300
check "$out/sim.png" >/dev/null
sleep 6
samples=$(guest "grep -c 'ZMON SAMPLE' /tmp/monitor.log" | tail -1)
[ "${samples:-0}" -ge 20 ] 2>/dev/null && echo "sim: $samples samples ok" || { echo "sim: $samples samples (want 20 or more) MISSING"; status=1; }
expect_log 'ZMON FRAME fps=[1-9]'
failed=$(guest "grep -cE 'ZMON (FAILED|DISCONNECTED)|failed' /tmp/monitor.log" | tail -1)
[ "${failed:-1}" = 0 ] && echo "sim: no failure ok" || { echo "sim: failure lines MISSING"; status=1; }
guest 'grep -E "ZMON (READY|FRAME|MEM|LEVEL|VISIBLE)" /tmp/monitor.log' > "$out/sim.log"
errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
guest "$stop_all" >/dev/null
[ $status -eq 0 ] && echo "monitor-p002: PASS" || echo "monitor-p002: FAIL"
exit $status
