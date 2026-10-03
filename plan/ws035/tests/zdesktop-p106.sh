#!/bin/sh
# ws035-p106 (F-041, the collision part): files asks what to do with a name the destination already has, on
# the Venus guest (the lean files image, plan/tools/files/build-files-image.sh).  zdesktop --glass at 1280x800;
# files at 1000x640 on the sample home, opened on Documents.  Downloads is given an older Budget.csv and
# Report.pdf first.  The six items of Documents are copied (Ctrl+A, Ctrl+C) and pasted into Downloads:
#  1. dialog.png: the question for the first taken name, with "Apply to all 2 conflicts".  Replace.
#  2. The second: Skip.  The copy runs: the replaced file has Documents' contents, the skipped one is still the
#     older one, the four others are copied, nothing gets a "2" name.
#  3. all.png: pasted again, all six names are taken; "Apply to all 6 conflicts" is ticked, Keep Both: six
#     "name 2" copies, no more questions.
#  4. Pasted again, Esc stops the operation (COLLISION cancel, no task).
#  5. A drop: Report.pdf dragged from Documents onto the sidebar's Downloads (a move there), Skip: nothing moves.
#
#   GUEST_RUNTIME=... plan/tools/files/files-guest.sh start build/<x>/hdd-image.img
#   plan/ws035/tests/zdesktop-p106.sh [OUTDIR] [SHOTS PREFIX]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws035-p106}
prefix=${2:-}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1 </dev/null; }
. plan/tools/guest/zwl-clients.sh
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[f]iles" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[f]iles" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
status=0

# Fails the run unless a log has at least N lines matching a pattern (within a few seconds).
expect_log() {
	tries=0
	found=0
	while [ $tries -lt 8 ]; do
		found=$(guest "grep -cE '$2' $1" | tail -1)
		[ "${found:-0}" -ge "${3:-1}" ] 2>/dev/null && break
		tries=$((tries + 1))
		sleep 1
	done
	if [ "${found:-0}" -ge "${3:-1}" ] 2>/dev/null; then
		echo "log: $2 ok"
	else
		echo "log: $2 MISSING"
		status=1
	fi
}

# Fails the run unless a guest test command succeeds.
expect_guest() {
	if guest "$1 && echo YES" | grep -q YES; then
		echo "guest: $2 ok"
	else
		echo "guest: $2 FAILED"
		status=1
	fi
}

# Clicks a point of the window's body (x, y from its top left) and waits.
click() {
	pointer move $((wx + $1 - 2)) $((wy + $2)) sleep 150 move $((wx + $1)) $((wy + $2)) sleep 300 down sleep 60 up sleep "${3:-700}"
}
shot() {
	pointer move 1270 790 sleep 400
	check "$out/$1" >/dev/null
	[ -n "$prefix" ] && cp "$out/$1" "$prefix$1"
}
# The name the last question asked about.
asked() {
	guest "grep 'ZFILES DIALOG collision' /tmp/f.log | tail -1" | sed -n 's/.* name=\(.*\) left=.*/\1/p'
}

# The dialog's card is centred in the 1000x640 body: 460x200 at (270, 220); its buttons' middles.
skip_x=$((270 + 156)); keep_x=$((270 + 270)); replace_x=$((270 + 384)); buttons_y=$((220 + 160))
all_x=$((270 + 40)); all_y=$((220 + 118))

guest "$stop_all" >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0 /tmp/files.clipboard; rm -rf /tmp/fhome; sh /usr/share/files-tests/make-home.sh /tmp/fhome >/dev/null
printf older > /tmp/fhome/Downloads/Budget.csv; printf older > /tmp/fhome/Downloads/Report.pdf
picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=900 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & for w in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30; do grep -q ZWL.READY /tmp/zdesktop.log 2>/dev/null && break; sleep 0.5; done; sleep 1
HOME=/tmp/fhome /bin/files --token=f1 --timeout-s=800 --width=1000 --height=640 /tmp/fhome/Documents > /tmp/f.log 2>&1 </dev/null & sleep 5; echo started' >/dev/null
zwl_app_clients
set -- $(guest "grep 'ZWL MAP client=$zc1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* surface=\([0-9]*\) x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2 \3/p')
surface=${1:-0}; wx=${2:-0}; wy=${3:-0}
echo "files: surface $surface at $wx,$wy"
expect_log /tmp/f.log 'ZFILES LOCATION kind=folder path=/tmp/fhome/Documents items=6 error=0'

# 1. Copy the six items, paste into Downloads: the first question.
click 600 400
keys '<ctrl-a>'
sleep 0.5
keys '<ctrl-c>'
sleep 0.5
expect_log /tmp/f.log 'ZFILES CLIPBOARD mode=1 items=6'
click 100 155
expect_log /tmp/f.log 'ZFILES LOCATION kind=folder path=/tmp/fhome/Downloads'
keys '<ctrl-v>'
sleep 1
expect_log /tmp/f.log 'ZFILES DIALOG collision index=[0-9]+ name=(Budget.csv|Report.pdf) left=2'
first=$(asked)
echo "first asked: $first"
shot dialog.png
click $replace_x $buttons_y

# 2. The second: Skip.
expect_log /tmp/f.log 'ZFILES COLLISION answer=replace'
expect_log /tmp/f.log 'ZFILES DIALOG collision index=[0-9]+ name=(Budget.csv|Report.pdf) left=1'
second=$(asked)
echo "second asked: $second"
click $skip_x $buttons_y 1500
expect_log /tmp/f.log 'ZFILES COLLISION answer=skip'
expect_log /tmp/f.log 'ZFILES TASK done id=1 kind=copy state=done files=[0-9]+ bytes=[0-9]+ errors=0'
expect_guest "! grep -q older '/tmp/fhome/Downloads/$first'" "$first replaced"
expect_guest "grep -q older '/tmp/fhome/Downloads/$second'" "$second skipped (still the older one)"
expect_guest "[ -f '/tmp/fhome/Downloads/Meeting notes.txt' ] && [ -f '/tmp/fhome/Downloads/Plan v3.key' ]" 'the others copied'
expect_guest "[ ! -e '/tmp/fhome/Downloads/Report 2.pdf' ] && [ ! -e '/tmp/fhome/Downloads/Budget 2.csv' ]" 'no "2" names'
shot copied.png

# 3. Again: six conflicts, apply to all, Keep Both.
keys '<ctrl-v>'
sleep 1
expect_log /tmp/f.log 'ZFILES DIALOG collision index=0 name=.* left=6'
click $all_x $all_y
expect_log /tmp/f.log 'ZFILES COLLISION all=1'
shot all.png
click $keep_x $buttons_y 1500
expect_log /tmp/f.log 'ZFILES COLLISION answer=keep index=0 all=1'
expect_log /tmp/f.log 'ZFILES TASK done id=2 kind=copy state=done files=6 bytes=[0-9]+ errors=0'
expect_guest "[ -f '/tmp/fhome/Downloads/Report 2.pdf' ] && [ -f '/tmp/fhome/Downloads/Budget 2.csv' ] && [ -f '/tmp/fhome/Downloads/Meeting notes 2.txt' ]" 'six "2" copies'
questions=$(guest "grep -c 'ZFILES DIALOG collision' /tmp/f.log" | tail -1)
[ "${questions:-0}" = 3 ] && echo "questions: 3 ok" || { echo "questions: $questions FAIL"; status=1; }

# 4. Again, and Esc.
keys '<ctrl-v>'
sleep 1
expect_log /tmp/f.log 'ZFILES DIALOG collision index=0' 2
keys '<esc>'
sleep 1
expect_log /tmp/f.log 'ZFILES COLLISION cancel index=0'
tasks=$(guest "grep -c 'ZFILES TASK start' /tmp/f.log" | tail -1)
[ "${tasks:-0}" = 2 ] && echo "tasks after cancel: 2 ok" || { echo "tasks after cancel: $tasks FAIL"; status=1; }

# 5. A drop of Report.pdf from Documents onto the sidebar's Downloads (a move), Skip.
click 100 125
expect_log /tmp/f.log 'ZFILES LOCATION kind=folder path=/tmp/fhome/Documents items=6'
pointer move $((wx + 777)) $((wy + 120)) sleep 150 move $((wx + 779)) $((wy + 120)) sleep 300 down sleep 120 \
    move $((wx + 767)) $((wy + 124)) sleep 80 move $((wx + 440)) $((wy + 138)) sleep 80 \
    move $((wx + 103)) $((wy + 156)) sleep 80 move $((wx + 100)) $((wy + 155)) sleep 600 up sleep 1200
expect_log /tmp/f.log 'ZFILES DRAG drop operation=move items=1 destination=/tmp/fhome/Downloads$'
expect_log /tmp/f.log 'ZFILES DIALOG collision index=0 name=Report.pdf left=1'
shot drop.png
click $skip_x $buttons_y 1500
expect_log /tmp/f.log 'ZFILES TASK done id=[0-9]+ kind=move state=done files=0 '
expect_guest "[ -f /tmp/fhome/Documents/Report.pdf ]" 'the skipped move left Report.pdf where it was'

errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
guest 'cat /tmp/f.log' > "$out/f.log"
guest "$stop_all" >/dev/null
[ $status = 0 ] && echo "zdesktop-p106: PASS" || echo "zdesktop-p106: FAIL"
exit $status
