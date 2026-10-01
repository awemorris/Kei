#!/bin/sh
# ws070-p002: the System Menu's protocol errors and libkeiland's own checks, on the Venus guest.
# zdesktop --glass runs, and /bin/menu-probe (userland/tests/menu-probe) sends each case on its own
# connection; every case must print ok, and zdesktop must log each protocol error with its object and
# code and go on serving (a terminal started afterwards shows its menus).
#
#   plan/tools/titlebar/menu-guest.sh start     (the guest must be up)
#   plan/tools/titlebar/menu-p002.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws070-run}"
export GUEST_RUNTIME
out=${1:-build/ws070-p002}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
stop_all='for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[t]erminal" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[t]erminal" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
status=0

guest "$stop_all" >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0
/bin/wayland --timeout=300 --width=1280 --height=800 --glass > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4
/bin/menu-probe > /tmp/probe.log 2>&1; echo probe-exit=$?
/bin/terminal --token=t2 --timeout-s=20 > /tmp/t2.log 2>&1 </dev/null & sleep 6; echo started' | tee "$out/run.txt"
guest 'cat /tmp/probe.log' | tee "$out/probe.log"
guest 'grep -E "ZWL (ERROR|MENU commit)" /tmp/zdesktop.log' | tee "$out/zdesktop-errors.log"
grep -q 'probe-exit=0' "$out/run.txt" || status=1
grep -q 'MENUPROBE DONE failures=0' "$out/probe.log" || status=1
[ "$(grep -c 'ZWL ERROR client=[0-9]* object=[0-9]* code=' "$out/zdesktop-errors.log")" -eq 10 ] || status=1
guest 'grep -c "ZTERM MENU ready" /tmp/t2.log' | tail -1 | grep -q '^1$' || status=1
guest "$stop_all" >/dev/null
[ $status -eq 0 ] && echo "menu-p002: PASS" || echo "menu-p002: FAIL"
exit $status
