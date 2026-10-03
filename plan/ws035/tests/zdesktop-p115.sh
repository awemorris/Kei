#!/bin/sh
# ws035-p115 (F-050): files merges a folder into the folder with its name, on the Venus guest (the lean files
# image, plan/tools/files/build-files-image.sh).  zdesktop --glass at 1280x800; files at 1000x640 on the sample
# home, opened on Desktop, which holds two folders, Photos and Notes; Downloads has folders of the same names:
#   Desktop/Photos: a.txt (new), b.txt, Sub/deep.txt      Downloads/Photos: a.txt (old), c.txt, Sub/keep.txt
#   Desktop/Notes:  n1.txt                                Downloads/Notes:  n2.txt
#  1. Copy both into Downloads.  The folder question offers Merge (the default) beside Replace, Keep Both and
#     Skip (folder.png).  Photos: Merge (the button); its Sub: Enter (merge); its a.txt, a file: the three-answer
#     question, Replace (file.png); Notes: Enter (merge).  Downloads/Photos then holds a (new), b, c and
#     Sub/{deep,keep}, Notes n1 and n2, and no "2" names (merged.png).
#  2. Ctrl+Z: the copies go to the trash and the replaced a.txt comes back; the folders are as they were
#     (undone.png).
#  3. A cut of both pasted into Downloads, "Apply to all", Merge: both folders and Photos/Sub merge without more
#     questions but the file a.txt's, answered Skip.  The items move; Desktop/Photos keeps a.txt, its emptied Sub
#     is removed, Desktop/Notes (emptied) is removed (moved.png).
#  4. Ctrl+Z: the three moved items go back into their folders, the removed folders made again (move-undone.png).
#  5. Replace with the trash on another file system (XDG_DATA_HOME on the UFS root, the home on /tmp's tmpfs):
#     the replaced file is copied into that trash and copied back by Ctrl+Z (xfs-replaced.png, xfs-undone.png).
#
#   plan/tools/files/files-guest.sh start     (the guest must be up)
#   plan/ws035/tests/zdesktop-p115.sh [OUTDIR] [SHOTS PREFIX]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws035-p115}
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

# Waits for the Nth question about a taken name and prints "NAME MERGE" (MERGE is 1 for two folders).
question() {
	expect_log /tmp/f.log 'ZFILES DIALOG collision' "$1" >/dev/null
	guest "grep 'ZFILES DIALOG collision' /tmp/f.log | sed -n '$1p'" | sed -n 's/.* name=\(.*\) left=[0-9]* merge=\([01]\).*/\1 \2/p'
}

# The cards are centred in the 1000x640 body: a folder's 520x200 at (240, 220) with Skip, Keep Both, Replace and
# Merge; a file's 460x200 at (270, 220) with Skip, Keep Both and Replace.  The buttons' middles and the check box.
merge_x=$((240 + 444)); folder_replace_x=$((240 + 330)); folder_all_x=$((240 + 40))
file_replace_x=$((270 + 384)); file_skip_x=$((270 + 156))
buttons_y=$((220 + 160)); all_y=$((220 + 118))

home=/tmp/fhome
guest "$stop_all" >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0 /tmp/files.clipboard; rm -rf /tmp/fhome; sh /usr/share/files-tests/make-home.sh /tmp/fhome >/dev/null
cd /tmp/fhome && rm -rf Desktop/* && mkdir -p Desktop/Photos/Sub Downloads/Photos/Sub Desktop/Notes Downloads/Notes
printf new > Desktop/Photos/a.txt; printf b > Desktop/Photos/b.txt; printf deep > Desktop/Photos/Sub/deep.txt
printf old > Downloads/Photos/a.txt; printf c > Downloads/Photos/c.txt; printf keep > Downloads/Photos/Sub/keep.txt
printf n1 > Desktop/Notes/n1.txt; printf n2 > Downloads/Notes/n2.txt
picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=900 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & for w in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30; do grep -q ZWL.READY /tmp/zdesktop.log 2>/dev/null && break; sleep 0.5; done; sleep 1
HOME=/tmp/fhome /bin/files --token=f1 --timeout-s=800 --width=1000 --height=640 /tmp/fhome/Desktop > /tmp/f.log 2>&1 </dev/null & sleep 5; echo started' >/dev/null
zwl_app_clients
set -- $(guest "grep 'ZWL MAP client=$zc1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* surface=\([0-9]*\) x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2 \3/p')
surface=${1:-0}; wx=${2:-0}; wy=${3:-0}
echo "files: surface $surface at $wx,$wy"
expect_log /tmp/f.log 'ZFILES LOCATION kind=folder path=/tmp/fhome/Desktop items=2 error=0'

# Answers the questions of one paste, by name, until N questions have been asked in all.
answer_copy() {
	n=$1
	while :; do
		set -- $(question "$n")
		name=${1:-}; merge=${2:-}
		echo "question $n: $name merge=$merge"
		case "$name:$merge" in
		Photos:1)
			shot folder.png
			click $merge_x $buttons_y 1200 ;;
		Sub:1|Notes:1)
			keys '\n'; sleep 1.2 ;;
		a.txt:0)
			shot file.png
			click $file_replace_x $buttons_y 1200 ;;
		*)
			echo "unexpected question: $name merge=$merge"; status=1; return ;;
		esac
		n=$((n + 1))
		[ "$n" -gt 4 ] && return
	done
}

# 1. Copy Photos and Notes into Downloads, merging.
click 600 400
keys '<ctrl-a>'
sleep 0.5
keys '<ctrl-c>'
sleep 0.5
expect_log /tmp/f.log 'ZFILES CLIPBOARD mode=1 items=2'
click 100 155
expect_log /tmp/f.log 'ZFILES LOCATION kind=folder path=/tmp/fhome/Downloads'
keys '<ctrl-v>'
sleep 1
answer_copy 1
expect_log /tmp/f.log 'ZFILES COLLISION answer=merge index=[0-9]+ all=0' 3
expect_log /tmp/f.log 'ZFILES COLLISION answer=replace index=[0-9]+ all=0'
expect_log /tmp/f.log 'ZFILES TASK done id=1 kind=copy state=done files=[0-9]+ bytes=[0-9]+ errors=0'
d=$home/Downloads
expect_guest "grep -q new $d/Photos/a.txt && [ -f $d/Photos/b.txt ] && [ -f $d/Photos/c.txt ]" 'Photos merged: a replaced, b added, c kept'
expect_guest "[ -f $d/Photos/Sub/deep.txt ] && [ -f $d/Photos/Sub/keep.txt ] && [ -f $d/Notes/n1.txt ] && [ -f $d/Notes/n2.txt ]" 'Sub and Notes merged'
expect_guest "[ ! -e '$d/Photos 2' ] && [ ! -e '$d/Notes 2' ] && [ ! -e '$d/Photos/Sub 2' ] && [ ! -e '$d/Photos/a 2.txt' ]" 'no "2" names'
expect_guest "[ -f $home/Desktop/Photos/Sub/deep.txt ]" 'the copy left the sources'
shot merged.png

# 2. Undo: the copies go, the replaced a.txt comes back.
keys '<ctrl-z>'
sleep 3
expect_log /tmp/f.log 'ZFILES UNDO redo=0 kind=1 items=4'
expect_log /tmp/f.log 'ZFILES UNDO replaced redo=0 items=1'
expect_guest "grep -q old $d/Photos/a.txt && [ ! -e $d/Photos/b.txt ] && [ -f $d/Photos/c.txt ]" 'undo: Photos as it was'
expect_guest "[ ! -e $d/Photos/Sub/deep.txt ] && [ -f $d/Photos/Sub/keep.txt ] && [ ! -e $d/Notes/n1.txt ] && [ -f $d/Notes/n2.txt ]" 'undo: Sub and Notes as they were'
shot undone.png

# 3. Cut both, paste into Downloads, apply to all, Merge; the file inside is still asked: Skip.
click 100 95
expect_log /tmp/f.log 'ZFILES LOCATION kind=folder path=/tmp/fhome/Desktop items=2'
click 600 400
keys '<ctrl-a>'
sleep 0.5
keys '<ctrl-x>'
sleep 0.5
expect_log /tmp/f.log 'ZFILES CLIPBOARD mode=2 items=2'
click 100 155
keys '<ctrl-v>'
sleep 1
set -- $(question 5)
echo "question 5: ${1:-} merge=${2:-} (apply to all, Merge)"
click $folder_all_x $all_y
expect_log /tmp/f.log 'ZFILES COLLISION all=1'
click $merge_x $buttons_y 1500
expect_log /tmp/f.log 'ZFILES COLLISION merge index=[0-9]+ items=[0-9]+ error=0 all=1' 2
set -- $(question 6)
echo "question 6: ${1:-} merge=${2:-} (Skip)"
[ "${1:-}" = a.txt ] || { echo "question 6 is not a.txt"; status=1; }
click $file_skip_x $buttons_y 2000
expect_log /tmp/f.log 'ZFILES TASK done id=[0-9]+ kind=move state=done files=[0-9]+ bytes=[0-9]+ errors=0'
expect_guest "[ -f $d/Photos/b.txt ] && [ -f $d/Photos/Sub/deep.txt ] && [ -f $d/Notes/n1.txt ] && grep -q old $d/Photos/a.txt" 'the items moved, a.txt skipped'
expect_guest "[ -f $home/Desktop/Photos/a.txt ] && [ ! -e $home/Desktop/Photos/Sub ] && [ ! -e $home/Desktop/Notes ]" 'the emptied folders are removed, Photos with a.txt stays'
expect_guest "[ ! -e /tmp/files.clipboard ]" 'the clipboard is emptied'
shot moved.png

# 4. Undo of the move: every item back in its folder, the removed ones made again.
keys '<ctrl-z>'
sleep 3
expect_log /tmp/f.log 'ZFILES UNDO redo=0 kind=0 items=3'
expect_guest "[ -f $home/Desktop/Photos/b.txt ] && [ -f $home/Desktop/Photos/Sub/deep.txt ] && [ -f $home/Desktop/Notes/n1.txt ]" 'undo: the items are back'
expect_guest "[ ! -e $d/Photos/b.txt ] && [ ! -e $d/Photos/Sub/deep.txt ] && [ ! -e $d/Notes/n1.txt ] && [ -f $d/Photos/Sub/keep.txt ] && [ -f $d/Notes/n2.txt ]" 'undo: Downloads as it was'
shot move-undone.png

# 5. Replace with the trash on another file system: files again with XDG_DATA_HOME on the root (UFS) while the
#    home is on /tmp (tmpfs).  Budget.csv of Documents replaces an older one in Downloads; the older one is copied
#    into that trash; Ctrl+Z copies it back (xfs-replaced.png, xfs-undone.png).
guest 'for p in $(ps -A -o pid,args | grep -E "[/]bin/files" | awk "{print \$1}"); do kill $p; done; sleep 1
rm -rf /root/p115-data; mkdir -p /root/p115-data; printf older > /tmp/fhome/Downloads/Budget.csv
export XDG_RUNTIME_DIR=/tmp WAYLAND_DISPLAY=wayland-0
HOME=/tmp/fhome XDG_DATA_HOME=/root/p115-data /bin/files --token=f2 --timeout-s=600 --width=1000 --height=640 /tmp/fhome/Documents > /tmp/f2.log 2>&1 </dev/null & sleep 5; echo started' >/dev/null
set -- $(guest "grep 'ZWL MAP client=' /tmp/zdesktop.log | tail -1" | sed -n 's/.* surface=\([0-9]*\) x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2 \3/p')
wx=${2:-$wx}; wy=${3:-$wy}
expect_log /tmp/f2.log 'ZFILES LOCATION kind=folder path=/tmp/fhome/Documents items=6 error=0'
pointer move $((wx + 560)) $((wy + 400)) sleep 200 down sleep 60 up sleep 500
keys '<ctrl-a>'
sleep 0.5
keys '<ctrl-c>'
sleep 0.5
click 100 155
keys '<ctrl-v>'
sleep 1
expect_log /tmp/f2.log 'ZFILES DIALOG collision index=[0-9]+ name=Budget.csv left=1 merge=0'
click $file_replace_x $buttons_y 2500
expect_log /tmp/f2.log 'ZFILES TASK done id=[0-9]+ kind=copy state=done files=[0-9]+ bytes=[0-9]+ errors=0'
expect_guest "grep -q older /root/p115-data/Trash/files/Budget.csv && [ -f /root/p115-data/Trash/info/Budget.csv.trashinfo ] && ! grep -q older $d/Budget.csv" 'the replaced file is copied into the trash on the other file system'
shot xfs-replaced.png
keys '<ctrl-z>'
sleep 3
expect_log /tmp/f2.log 'ZFILES UNDO replaced redo=0 items=1'
expect_log /tmp/f2.log 'ZFILES TASK done id=[0-9]+ kind=restore state=done files=[0-9]+ bytes=[0-9]+ errors=0'
expect_guest "grep -q older $d/Budget.csv && [ ! -e /root/p115-data/Trash/files/Budget.csv ] && [ ! -e /root/p115-data/Trash/info/Budget.csv.trashinfo ]" 'undo copies it back and clears it from that trash'
shot xfs-undone.png
guest 'cat /tmp/f2.log' > "$out/f2.log"
guest 'rm -rf /root/p115-data' >/dev/null

errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
guest 'cat /tmp/f.log' > "$out/f.log"
guest "$stop_all" >/dev/null
[ $status = 0 ] && echo "zdesktop-p115: PASS" || echo "zdesktop-p115: FAIL"
exit $status
