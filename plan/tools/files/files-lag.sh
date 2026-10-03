#!/bin/sh
# ws071: checks whether the last frame files presents is on the screen promptly.
# A rubber band is dragged in steps and the screen is taken a second after each step
# (lag1.png .. lag3.png); each picture must show the band up to the pointer.
#
#   plan/tools/files/files-guest.sh start     (the guest must be up)
#   plan/tools/files/files-lag.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws071-lag}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
. plan/tools/guest/zwl-clients.sh
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
input() { python3 plan/tools/files/qmp-input.py "$GUEST_RUNTIME/qmp.sock" $1; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[f]iles" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[f]iles" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
guest "$stop_all" >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0; rm -rf /tmp/fhome; sh /usr/share/files-tests/make-home.sh /tmp/fhome >/dev/null
/bin/wayland --timeout=900 --width=1280 --height=800 --glass > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4
HOME=/tmp/fhome /bin/files --token=f1 --timeout-s=800 --width=1000 --height=640 /tmp/fhome/Projects > /tmp/f.log 2>&1 </dev/null & sleep 5; echo started' >/dev/null
zwl_app_clients
set -- $(guest "grep 'ZWL MAP client=$zc1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* surface=\([0-9]*\) x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2 \3/p')
wx=${2:-0}; wy=${3:-0}
input "move $((wx + 900)) $((wy + 600)) sleep 300 down sleep 100 move $((wx + 800)) $((wy + 500)) sleep 1000"
check "$out/lag1.png" >/dev/null
input "move $((wx + 600)) $((wy + 400)) sleep 1000"
check "$out/lag2.png" >/dev/null
input "move $((wx + 400)) $((wy + 300)) sleep 1000"
check "$out/lag3.png" >/dev/null
input "up sleep 300"
guest "$stop_all" >/dev/null
echo "files-lag: pictures in $out"
