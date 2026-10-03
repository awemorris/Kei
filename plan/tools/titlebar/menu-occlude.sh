#!/bin/sh
# ws070-p003: a title bar's menu covered by another window cannot be pressed through it.
# zdesktop --glass runs the terminal (its menus in its title bar) and a wl_shm window, which is dragged by
# its title bar to the top of the space, over the terminal's title bar.  A press where the terminal's Shell
# item is (under the other window's body) must open no menu; the covered.png screen shows the red body there.
# Then the terminal is raised by a press on its body, and the same press opens Shell (opened.png).
#
#   plan/tools/titlebar/menu-guest.sh start     (the guest must be up)
#   plan/tools/titlebar/menu-occlude.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws070-run}"
export GUEST_RUNTIME
out=${1:-build/ws070-occlude}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
. plan/tools/guest/zwl-clients.sh
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
stop_all='for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[t]erminal|[w]lshm" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[t]erminal|[w]lshm" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
status=0

# The number of lines of zdesktop's log that match a pattern.
count() {
	guest "grep -cE '$1' /tmp/zdesktop.log" | tail -1
}

guest "$stop_all" >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0
/bin/wayland --timeout=300 --width=1280 --height=800 --glass > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4
/bin/terminal --token=t3 --timeout-s=250 > /tmp/t3.log 2>&1 </dev/null & sleep 6
/bin/wlshm --size=520x340 --color=ffd04040 --frames=20000 --token=a > /tmp/a.log 2>&1 </dev/null & sleep 3; echo started' >/dev/null
zwl_app_clients
set -- $(guest "grep 'ZWL MAP client=$zc1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2/p')
tx=${1:-0}; ty=${2:-0}
set -- $(guest "grep 'ZWL MAP client=$zc2 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2/p')
ax=${1:-0}; ay=${2:-0}
shell=$(guest "grep 'MENU bar client=$zc1 .* where=floating item=1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* offset=\([0-9]*\) top=[0-9]* width=\([0-9]*\).*/\1 \2/p')
set -- $shell
sx=$((tx + ${1:-0} + ${2:-0} / 2)); sy=$((ty - 30))
echo "terminal at $tx,$ty (Shell at $sx,$sy); red window at $ax,$ay"

# The red window's title bar dragged so that its body's top left is at (Shell - 60, the highest a body may be).
pointer move $((ax + 100)) $((ay - 30)) sleep 300 down sleep 100 move $((ax + 80)) $((ay - 60)) sleep 150 \
    move $((sx - 60 + 100)) $((86 - 30)) sleep 300 up sleep 600
before=$(count "MENU open client=$zc1 ")
pointer move $sx $((sy + 8)) sleep 300 down sleep 60 up sleep 600 move 1250 780 sleep 400
after=$(count "MENU open client=$zc1 ")
check "$out/covered.png" --expect $sx,$((sy + 8)),d04040 || status=1
if [ "${after:-0}" -eq "${before:-0}" ] 2>/dev/null; then echo "covered press opens no menu: ok"; else echo "covered press opens no menu: FAIL"; status=1; fi

# The terminal raised by its body; then Shell opens.
pointer move $((tx + 700)) $((ty + 500)) sleep 300 down sleep 60 up sleep 600
pointer move $sx $sy sleep 300 down sleep 60 up sleep 600
check "$out/opened.png" >/dev/null
now=$(count "MENU open client=$zc1 ")
if [ "${now:-0}" -gt "${after:-0}" ] 2>/dev/null; then echo "uncovered press opens Shell: ok"; else echo "uncovered press opens Shell: FAIL"; status=1; fi
guest "$stop_all" >/dev/null
[ $status -eq 0 ] && echo "menu-occlude: PASS (and judge the screens)" || echo "menu-occlude: FAIL"
exit $status
