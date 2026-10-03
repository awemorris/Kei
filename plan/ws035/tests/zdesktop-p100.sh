#!/bin/sh
# ws035-p100: the primary selection between two terminals on the Venus guest (the lean image, the files
# image, or the login image), zdesktop --glass at 1280x800:
#  1. In the first terminal "alpha beta-gamma delta" is printed and "beta-gamma" double-clicked: it is the primary
#     selection (ZTERM PRIMARY set bytes=10; ZWL PRIMARY selection types=2).
#  2. A second terminal is started (it gets the keyboard and is told the primary selection: ZTERM PRIMARY offer
#     text=1); a middle click in it pastes the first one's text into its shell (ZWL PRIMARY receive, ZTERM PRIMARY
#     send bytes=10 in the first, ZTERM PRIMARY paste received bytes=10 in the second): pasted.png.
#  3. Back in the first terminal, a middle click pastes its own selection (ZTERM PRIMARY paste own bytes=10).
#
#   plan/tools/files/files-guest.sh start     (the guest must be up)
#   plan/ws035/tests/zdesktop-p100.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws035-p100}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1 </dev/null; }
. plan/tools/guest/zwl-clients.sh
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; sleep 0.8; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[t]erminal|[s]essiond" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[t]erminal" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
status=0

# Fails the run unless a log has a line matching a pattern (within a few seconds).
expect_log() {
	tries=0
	found=0
	while [ $tries -lt 6 ]; do
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

# A terminal's window place and cell size, from the logs.
window_of() {
	guest "grep 'ZWL MAP client=$(zwl_app_client $1) ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2/p'
}

guest "$stop_all" >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0; picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=600 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & for w in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30; do grep -q ZWL.READY /tmp/zdesktop.log 2>/dev/null && break; sleep 0.5; done; sleep 1
/bin/terminal --token=t1 --timeout-s=500 > /tmp/t1.log 2>&1 </dev/null & sleep 6; echo started' >/dev/null
set -- $(window_of 1)
wx=${1:-0}; wy=${2:-0}
set -- $(guest "grep 'ZTERM START' /tmp/t1.log | tail -1" | sed -n 's/.* cell=\([0-9]*\)x\([0-9]*\) .*/\1 \2/p')
cw=${1:-10}; ch=${2:-20}
cx() { echo $((wx + 8 + $1 * cw + cw / 2)); }
cy() { echo $((wy + 8 + $1 * ch + ch / 2)); }
echo "terminal 1 at $wx,$wy cell ${cw}x$ch"

# 1. The first terminal's text, and "beta-gamma" double-clicked.
pointer move "$(cx 30)" "$(cy 10)" sleep 200 down sleep 60 up sleep 300
keys 'printf "\\033[H\\033[2J"; echo alpha beta-gamma delta' '\n'
sleep 1
pointer move "$(cx 12)" "$(cy 0)" sleep 300 down sleep 50 up sleep 120 down sleep 50 up sleep 800
expect_log /tmp/t1.log 'ZTERM SELECT how=word .* bytes=10'
expect_log /tmp/t1.log 'ZTERM PRIMARY set bytes=10'
expect_log /tmp/zdesktop.log 'ZWL PRIMARY selection client=[0-9]+ source=[0-9]+ types=2'

# 2. The second terminal, and a middle click in it.
guest 'export XDG_RUNTIME_DIR=/tmp; /bin/terminal --token=t2 --timeout-s=400 > /tmp/t2.log 2>&1 </dev/null & sleep 6; echo started' >/dev/null
expect_log /tmp/t2.log 'ZTERM PRIMARY offer text=1'
set -- $(window_of 2)
x2=${1:-0}; y2=${2:-0}
echo "terminal 2 at $x2,$y2"
pointer move $((x2 + 300)) $((y2 + 200)) sleep 200 down sleep 60 up sleep 300
keys 'printf "\\033[H\\033[2J"; echo -n "pasted: "' '\n'
sleep 1
pointer move $((x2 + 300)) $((y2 + 200)) sleep 200 middle-down sleep 60 middle-up sleep 1500
expect_log /tmp/zdesktop.log 'ZWL PRIMARY receive client=[0-9]+ mime=text/plain;charset=utf-8 source=[0-9]+'
expect_log /tmp/t1.log 'ZTERM PRIMARY send bytes=10'
expect_log /tmp/t2.log 'ZTERM PRIMARY paste received bytes=10'
pointer move 1270 790 sleep 400
check "$out/pasted.png" >/dev/null

# 3. The first terminal (its visible left edge) pastes its own selection.
pointer move "$(cx 1)" "$(cy 14)" sleep 200 down sleep 60 up sleep 500 middle-down sleep 60 middle-up sleep 1000
expect_log /tmp/t1.log 'ZTERM PRIMARY paste own bytes=10'
check "$out/own.png" >/dev/null

guest "$stop_all" >/dev/null
echo "zdesktop-p100: status=$status"
exit $status
