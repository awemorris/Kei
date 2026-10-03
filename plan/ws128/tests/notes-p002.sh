#!/bin/sh
# ws128-p002: Notes' File > Open and Save As with libkeiui's file chooser, on a Venus guest of an image with Notes and
# libpdf (for example plan/ws089/tests/build-settings-image.sh's).  zdesktop --glass at 1280x800, root's home; the
# notebooks are in /root/Documents/Notes, where foreign.pdf (make-foreign-pdf.py: another program's PDF) is put.
#  1. A new notebook: a line with the pointer, Ctrl+S (NOTES SAVE ... path=A, A the new notebook).
#  2. Ctrl+O: the chooser at the notebook's folder (NOTES CHOOSER open mode=open); "f" and Enter open foreign.pdf,
#     which Notes writes on (NOTES OPEN ... kind=foreign, NOTES OPENED); a line, Ctrl+S (SAVE path=.../foreign.pdf).
#  3. Ctrl+O, "n" and Enter open A again: Notes' own notebook with its line (NOTES OPEN ... strokes=1 kind=notes).
#  4. Ctrl+Shift+S: the chooser to save as (mode=save), "copy" and Enter: SAVE reason=save-as path=.../copy.pdf; a
#     second line, Ctrl+S saves copy.pdf (strokes=2); A keeps its one line.
#  5. Ctrl+O and Esc: NOTES CHOOSER cancelled, the notebook stays copy.pdf.
#  6. Notes again on copy.pdf: both lines are there (NOTES OPEN ... strokes=2 kind=notes); foreign.pdf has its line.
#  7. No "not available" in Notes' log, no ERROR in zdesktop's.
#
#   plan/ws089/tests/settings-guest.sh start IMAGE
#   plan/ws128/tests/notes-p002.sh [OUTDIR]   (default build/ws128-shots/p002)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws089-run}"
export GUEST_RUNTIME
out=${1:-build/ws128-shots/p002}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1 </dev/null; }
put() { timeout 120 python3 plan/tools/guest/guest.py put "$1" "$2" >/dev/null 2>&1 </dev/null || { echo "put $1: FAILED"; status=1; }; }
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@" >/dev/null; sleep 0.8; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[n]otes( |$)" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[n]otes( |$)" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
start_desktop='export XDG_RUNTIME_DIR=/tmp HOME=/root; rm -f /tmp/wayland-0
/bin/wayland --timeout=900 --width=1280 --height=800 --glass --wallpaper=/usr/share/keiland/wallpaper.ppm > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4; echo started'
folder=/root/Documents/Notes
status=0
. plan/ws089/tests/settings-wait.sh

# Fails the run unless a log has a line matching a pattern (within a few seconds).
expect_log() {
	tries=0
	found=0
	while [ $tries -lt 15 ]; do
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

# Waits until a log has more lines matching a pattern than a count (the next save, the next open).
expect_more() {
	tries=0
	found=0
	while [ $tries -lt 15 ]; do
		found=$(guest "grep -cE '$2' $1" | tail -1)
		[ "${found:-0}" -gt "$3" ] 2>/dev/null && break
		tries=$((tries + 1))
		sleep 1
	done
	if [ "${found:-0}" -gt "$3" ] 2>/dev/null; then
		echo "log: $2 (more than $3) ok"
	else
		echo "log: $2 (more than $3) MISSING"
		status=1
	fi
}

count() { guest "grep -cE '$2' $1" | tail -1; }

# A picture of the screen with the pointer out of the way.
shot() {
	pointer move 1270 790 sleep 800
	check "$out/$1" >/dev/null
	echo "shot: $out/$1"
}

# Starts Notes on a file or none (its log appended to /tmp/n.log) and finds its window.
start_notes() {
	guest "export XDG_RUNTIME_DIR=/tmp HOME=/root; /bin/notes --width=1180 --height=700 $1 >> /tmp/n.log 2>&1 </dev/null & sleep 6; echo started" >/dev/null
	find_window
	echo "notes: window at $wx,$wy"
}

# Draws a line across the page at a share of its height (the page's place from Notes' last NOTES LAYOUT line).
line() {
	set -- $(guest "grep 'NOTES LAYOUT' /tmp/n.log | tail -1" | sed -n 's/.*page=\([0-9-]*\),\([0-9-]*\),\([0-9]*\),\([0-9]*\) .*/\1 \2 \3 \4/p' | tail -1) "$1"
	if [ -z "${4:-}" ]; then
		echo "layout: MISSING"
		status=1
		return
	fi
	x0=$((wx + $1 + $3 / 8)); x1=$((wx + $1 + $3 * 7 / 8)); y=$((wy + $2 + $4 * $5 / 100))
	pointer move "$x0" "$y" sleep 200 down sleep 60 move $(((x0 + x1) / 2)) $((y + 20)) sleep 60 move "$x1" "$y" sleep 60 up sleep 800
}

wait_guest
guest "$stop_all" >/dev/null
python3 plan/ws128/tests/make-foreign-pdf.py "$out/foreign.pdf"
put "$out/foreign.pdf" /tmp/foreign.pdf
guest "rm -rf $folder /root/.local/share/keiland/notes /tmp/n.log; mkdir -p $folder; cp /tmp/foreign.pdf $folder/foreign.pdf" >/dev/null
guest "$start_desktop" >/dev/null
wait_desktop

# 1. A new notebook, a line, saved.
start_notes ""
expect_log /tmp/n.log 'NOTES START '
line 30
keys '<ctrl-s>'
expect_log /tmp/n.log "NOTES SAVE reason=request .*strokes=1 .*path=$folder/note-"
first=$(guest "grep 'NOTES SAVE reason=request' /tmp/n.log | tail -1" | sed -n 's/.* path=\(.*\)$/\1/p' | tail -1)
echo "notebook: $first"

# 2. Open another program's PDF and write on it.
keys '<ctrl-o>'
expect_log /tmp/n.log "NOTES CHOOSER open mode=open folder=$folder"
sleep 2
shot open-chooser.png
keys 'f'
keys '<ret>'
expect_log /tmp/n.log "NOTES OPEN pages=1 strokes=0 kind=foreign path=$folder/foreign.pdf"
expect_log /tmp/n.log "NOTES OPENED pages=1 strokes=0 path=$folder/foreign.pdf"
sleep 2
line 50
keys '<ctrl-s>'
expect_more /tmp/n.log "NOTES SAVE reason=request .*strokes=1 .*path=$folder/foreign.pdf" 0
shot foreign.png

# 3. Notes' own notebook again.
keys '<ctrl-o>'
sleep 2
keys 'n'
keys '<ret>'
expect_log /tmp/n.log "NOTES OPEN pages=1 strokes=1 kind=notes path=$first"
sleep 2
shot reopened.png

# 4. Save As copy.pdf, then a second line saved there.
keys '<ctrl-shift-s>'
expect_log /tmp/n.log 'NOTES CHOOSER open mode=save'
sleep 2
shot save-chooser.png
keys 'copy'
keys '<ret>'
expect_log /tmp/n.log "NOTES SAVE reason=save-as .*strokes=1 .*path=$folder/copy.pdf"
line 60
keys '<ctrl-s>'
expect_log /tmp/n.log "NOTES SAVE reason=request .*strokes=2 .*path=$folder/copy.pdf"

# 5. Open cancelled.
opened=$(count /tmp/n.log 'NOTES OPENED')
keys '<ctrl-o>'
sleep 2
keys '<esc>'
expect_log /tmp/n.log 'NOTES CHOOSER cancelled'
now=$(count /tmp/n.log 'NOTES OPENED')
[ "${now:-0}" = "${opened:-x}" ] && echo "cancel: nothing opened ok" || { echo "cancel: something opened"; status=1; }

# 6. Notes again on copy.pdf: both lines; the first notebook keeps one.
keys '<ctrl-w>'
sleep 3
start_notes "$folder/copy.pdf"
expect_log /tmp/n.log "NOTES OPEN pages=1 strokes=2 kind=notes path=$folder/copy.pdf"
shot copy.png
keys '<ctrl-w>'
sleep 3
start_notes "$first"
expect_log /tmp/n.log "NOTES OPEN pages=1 strokes=1 kind=notes path=$first"
keys '<ctrl-w>'
sleep 3
start_notes "$folder/foreign.pdf"
expect_log /tmp/n.log "NOTES OPEN pages=1 strokes=1 kind=annotated path=$folder/foreign.pdf"
keys '<ctrl-w>'
sleep 2

# 7. No missing feature said, no error.
missing=$(count /tmp/n.log 'not available|chooser unsupported')
[ "${missing:-1}" = 0 ] && echo "notes: no missing feature ok" || { echo "notes: a missing feature was said"; status=1; }
errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; guest "grep ERROR /tmp/zdesktop.log | head -5"; status=1; }
guest 'cat /tmp/n.log' > "$out/notes.log"
guest "$stop_all" >/dev/null
[ $status = 0 ] && echo "notes-p002: PASS" || echo "notes-p002: FAIL"
exit $status
