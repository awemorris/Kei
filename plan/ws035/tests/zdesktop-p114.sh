#!/bin/sh
# ws035-p114: the terminal's scrollback, a selection that follows its text as output scrolls, and a selection
# dragged past the window's edge scrolling the scrollback, on the Venus guest (the lean image,
# plan/tools/files/build-files-image.sh).  zdesktop --glass at 1280x800 and terminal; 60 lines "row-1".."row-60"
# are printed first, so the scrollback holds the first lines:
#  1. follow.png: "row-50" is double clicked while a background job is about to print 6 lines; after they scroll
#     the screen, Ctrl+Shift+C copies the range, which still holds "row-50" (the copy is pasted into a file).
#  2. wheel.png: three wheel notches up show the scrollback (ZTERM VIEW how=scroll back=9); a key returns to the
#     live screen (how=key back=0).
#  3. edge.png: a press on the grid's first row dragged above the window scrolls the view back on its own
#     (ZTERM VIEW how=edge, several steps); the release selects from the top of what it reached down to the
#     press, and the copy starts at a line of the scrollback.
#  4. back.png: from there, a drag below the window's bottom scrolls toward the live screen (how=edge back=0).
#
#   plan/tools/files/files-guest.sh start     (the guest must be up)
#   plan/ws035/tests/zdesktop-p114.sh [OUTDIR] [SHOTS PREFIX]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws035-p114}
prefix=${2:-}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1 </dev/null; }
. plan/tools/guest/zwl-clients.sh
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; sleep 0.8; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[t]erminal" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[t]erminal" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
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

# Fails the run unless a guest file holds exactly a text.
expect_file() {
	got=$(guest "cat $1" | tail -1)
	if [ "$got" = "$2" ]; then
		echo "file: $1 = '$2' ok"
	else
		echo "file: $1 = '$got', want '$2'"
		status=1
	fi
}

shot() {
	check "$out/$1" >/dev/null
	[ -n "$prefix" ] && cp "$out/$1" "$prefix$1"
}

# The screen point of a cell's middle.
cx() { echo $((wx + 8 + $1 * cw + cw / 2)); }
cy() { echo $((wy + 8 + $1 * ch + ch / 2)); }

# Pastes the clipboard into a guest file: echo "<clipboard>" > FILE.
paste_to() {
	keys 'echo "'
	keys '<ctrl-shift-v>'
	keys "\" > $1" '\n'
	sleep 1
}

guest "$stop_all" >/dev/null
guest 'rm -f /tmp/copied-*.txt; export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0; picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=600 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & for w in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30; do grep -q ZWL.READY /tmp/zdesktop.log 2>/dev/null && break; sleep 0.5; done; sleep 1
/bin/terminal --token=t1 --timeout-s=500 > /tmp/t.log 2>&1 </dev/null & sleep 6; echo started' >/dev/null
zwl_app_clients
set -- $(guest "grep 'ZWL MAP client=$zc1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2/p')
wx=${1:-0}; wy=${2:-0}
set -- $(guest "grep 'ZTERM START' /tmp/t.log | tail -1" | sed -n 's/.* cell=\([0-9]*\)x\([0-9]*\) .*/\1 \2/p')
cw=${1:-10}; ch=${2:-20}
rows=$(guest "grep -E 'ZTERM (START|RESIZE)' /tmp/t.log | tail -1" | sed -n 's/.* rows=\([0-9]*\) .*/\1/p')
rows=${rows:-28}
echo "terminal at $wx,$wy cell ${cw}x$ch rows $rows"

# 60 numbered lines, then the prompt: the first lines scroll into the scrollback.
pointer move "$(cx 30)" "$(cy 10)" sleep 200 down sleep 60 up sleep 300
keys 'clear; i=1; while [ $i -le 60 ]; do echo row-$i; i=$((i+1)); done' '\n'
sleep 2

# 1. A word selected, then output scrolls the screen: the range follows its text.
keys '(sleep 5; for w in a b c d e f; do echo tick-$w; done) &' '\n'
sleep 1
# The screen now ends with row-60, the prompt, the job line and a prompt: row-50 is 13 rows above the last row.
target=$((rows - 1 - 13))
pointer move "$(cx 2)" "$(cy $target)" sleep 300 down sleep 50 up sleep 120 down sleep 50 up sleep 800
expect_log /tmp/t.log 'ZTERM SELECT how=word from=0,[0-9]+ to=5,[0-9]+ bytes=6'
sleep 6
shot follow.png
keys '<ctrl-shift-c>'
expect_log /tmp/t.log 'ZTERM COPY bytes=6'
paste_to /tmp/copied-follow.txt
expect_file /tmp/copied-follow.txt row-50

# 2. The wheel shows the scrollback; a key returns to the live screen.
pointer move "$(cx 30)" "$(cy 10)" sleep 200 wheel-up sleep 150 wheel-up sleep 150 wheel-up sleep 800
expect_log /tmp/t.log 'ZTERM VIEW how=scroll back=9 '
shot wheel.png
keys 'x'
expect_log /tmp/t.log 'ZTERM VIEW how=key back=0 '
keys '<backspace>'

# 3. A press on the first row dragged above the window: the view scrolls back by itself.
pointer move "$(cx 4)" "$(cy 0)" sleep 300 down sleep 100 move "$(cx 4)" "$(cy 0)" sleep 100 move "$(cx 4)" $((wy - 12)) sleep 1500
expect_log /tmp/t.log 'ZTERM VIEW how=edge back=[1-9]'
shot edge.png
pointer move "$(cx 4)" "$(cy 0)" sleep 200 up sleep 800
expect_log /tmp/t.log 'ZTERM SELECT how=drag from=4,[0-9]+ to=4,[0-9]+ bytes='
edges=$(guest "grep -c 'ZTERM VIEW how=edge' /tmp/t.log" | tail -1)
echo "edge steps: $edges"
[ "${edges:-0}" -ge 3 ] 2>/dev/null || { echo "edge: fewer than 3 steps"; status=1; }
last=$(guest "grep 'ZTERM SELECT how=drag' /tmp/t.log | tail -1" | tail -1)
view=$(guest "grep 'ZTERM VIEW how=edge' /tmp/t.log | tail -1" | tail -1)
echo "selection: $last"
echo "view: $view"
scrolled=$(echo "$view" | sed -n 's/.* scrolled=\([0-9]*\).*/\1/p')
from_line=$(echo "$last" | sed -n 's/.* from=[0-9]*,\([0-9]*\) .*/\1/p')
if [ -n "$scrolled" ] && [ -n "$from_line" ] && [ "$from_line" -lt "$scrolled" ]; then
	echo "selection starts in the scrollback: line $from_line < $scrolled ok"
else
	echo "selection does not start in the scrollback"
	status=1
fi

# 4. With the view back, a drag below the bottom scrolls toward the live screen.
pointer move "$(cx 10)" "$(cy $((rows - 1)))" sleep 300 down sleep 100 move "$(cx 10)" $((wy + 8 + rows * ch + 30)) sleep 2500
expect_log /tmp/t.log 'ZTERM VIEW how=edge back=0 '
shot back.png
pointer move "$(cx 10)" "$(cy $((rows - 1)))" sleep 200 up sleep 600

errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
guest 'cat /tmp/t.log' > "$out/t.log"
guest "$stop_all" >/dev/null
[ $status = 0 ] && echo "zdesktop-p114: PASS" || echo "zdesktop-p114: FAIL"
exit $status
