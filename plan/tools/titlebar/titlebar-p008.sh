#!/bin/sh
# ws070-p008: the Titlebar Presentation's protocol errors, model and libkeiland's own checks, on the
# Venus guest (the lean image).  zdesktop --glass runs, and /bin/titlebar-probe
# (userland/tests/titlebar-probe) sends each case on its own connection; every case must print
# ok; zdesktop must log each protocol error with its object and code, take the good case's model
# (three commits: controls with six controls and one tab, then tabs, then controls again) and go on
# serving (a terminal started afterwards shows its menus: the MENU mode is unchanged).
#
#   plan/tools/files/files-guest.sh start     (the guest must be up; GUEST_RUNTIME as for it)
#   plan/tools/titlebar/titlebar-p008.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws070-p008}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
stop_all='for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[t]erminal|[f]iles" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[t]erminal|[f]iles" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
status=0

guest "$stop_all" >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0
/bin/wayland --timeout=300 --width=1280 --height=800 --glass > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4
/bin/titlebar-probe > /tmp/probe.log 2>&1; echo probe-exit=$?
/bin/terminal --token=t2 --timeout-s=20 > /tmp/t2.log 2>&1 </dev/null & sleep 6; echo started' | tee "$out/run.txt"
guest 'cat /tmp/probe.log' | tee "$out/probe.log"
guest 'grep -E "ZWL (ERROR|TITLEBAR)" /tmp/zdesktop.log' | tee "$out/zdesktop-titlebar.log"
grep -q 'probe-exit=0' "$out/run.txt" || { echo "probe: exit"; status=1; }
grep -q 'TITLEBARPROBE DONE failures=0' "$out/probe.log" || { echo "probe: failures"; status=1; }
errors=$(grep -c 'ZWL ERROR client=[0-9]* object=[0-9]* code=' "$out/zdesktop-titlebar.log")
[ "$errors" -eq 12 ] || { echo "zdesktop: $errors protocol errors (want 12)"; status=1; }
grep -qE 'ZWL TITLEBAR commit client=[0-9]+ titlebar=[0-9]+ serial=1 mode=1 controls=6 tabs=1 ' "$out/zdesktop-titlebar.log" || { echo "commit 1: MISSING"; status=1; }
grep -qE 'ZWL TITLEBAR commit client=[0-9]+ titlebar=[0-9]+ serial=2 mode=2 controls=6 tabs=1 ' "$out/zdesktop-titlebar.log" || { echo "commit 2: MISSING"; status=1; }
grep -qE 'ZWL TITLEBAR commit client=[0-9]+ titlebar=[0-9]+ serial=3 mode=1 controls=6 tabs=1 ' "$out/zdesktop-titlebar.log" || { echo "commit 3: MISSING"; status=1; }
guest 'grep -c "ZTERM MENU ready" /tmp/t2.log' | tail -1 | grep -q '^1$' || { echo "terminal: menus MISSING"; status=1; }
guest "$stop_all" >/dev/null
[ $status -eq 0 ] && echo "titlebar-p008: PASS" || echo "titlebar-p008: FAIL"
exit $status
