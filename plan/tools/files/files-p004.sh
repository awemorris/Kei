#!/bin/sh
# ws071-p004: the file operations of files on the Venus guest (the lean image,
# build-files-image.sh).  zdesktop --glass at 1280x800; files at 1000x640 on the
# sample home (/tmp/fhome), opened on Documents.  Each step checks the file manager's log and
# the files in the guest; the screens are kept for reading:
#  1. rename.png: F2 on Budget.csv, "Costs" typed; Enter renames it (RENAME, Costs.csv exists).
#  2. Delete moves it to the trash (TASK done kind=trash), in ~/.local/share/Trash with its record.
#  3. trash.png: the Trash; Put Back returns it (TASK done kind=restore, Costs.csv back).
#  4. Ctrl+Z undoes the put back (trash again), Ctrl+Z again undoes the trash (back again).
#  5. Ctrl+C on Report.pdf, Downloads, Ctrl+V copies it (TASK done kind=copy, the file there).
#  6. dialog.png: Shift+Delete asks; Enter deletes it for good (TASK done kind=delete).
#  7. folder.png: Ctrl+Shift+N makes "untitled folder" and edits its name; "Drafts" Enter.
#  8. Ctrl+Z undoes the rename of the new folder (untitled folder again).
#
#   plan/tools/files/files-guest.sh start     (the guest must be up)
#   plan/tools/files/files-p004.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws071-run}"
export GUEST_RUNTIME
out=${1:-build/ws071-p004}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
. plan/tools/guest/zwl-clients.sh
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[f]iles" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[f]iles" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
status=0

# Fails the run unless a log has a line matching a pattern.
expect_log() {
	found=$(guest "grep -cE '$2' $1" | tail -1)
	if [ "${found:-0}" -gt 0 ] 2>/dev/null; then
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
}

guest "$stop_all" >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0 /tmp/files.clipboard; rm -rf /tmp/fhome; sh /usr/share/files-tests/make-home.sh /tmp/fhome >/dev/null
picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=900 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4
HOME=/tmp/fhome /bin/files --token=f1 --timeout-s=800 --width=1000 --height=640 /tmp/fhome/Documents > /tmp/f.log 2>&1 </dev/null & sleep 5; echo started' >/dev/null
zwl_app_clients
set -- $(guest "grep 'ZWL MAP client=$zc1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* surface=\([0-9]*\) x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2 \3/p')
surface=${1:-0}; wx=${2:-0}; wy=${3:-0}
echo "files: surface $surface at $wx,$wy"
expect_log /tmp/f.log 'ZFILES LOCATION kind=folder path=/tmp/fhome/Documents items=6 error=0'
trash=/tmp/fhome/.local/share/Trash

# 1. Rename.
click 443 120
keys '<f2>'
sleep 0.5
keys 'Costs'
sleep 0.8
shot rename.png
keys '<ret>'
sleep 1
expect_log /tmp/f.log 'ZFILES RENAME from=/tmp/fhome/Documents/Budget.csv to=/tmp/fhome/Documents/Costs.csv'
expect_guest '[ -f /tmp/fhome/Documents/Costs.csv ]' 'Costs.csv exists'

# 2. To the trash.
keys '<delete>'
sleep 1.5
expect_log /tmp/f.log 'ZFILES TASK done id=1 kind=trash state=done files=1 '
expect_guest "[ -f $trash/files/Costs.csv ] && grep -q 'Path=/tmp/fhome/Documents/Costs.csv' $trash/info/Costs.csv.trashinfo" 'in the trash with its record'

# 3. The Trash, and Put Back.
click 100 343
expect_log /tmp/f.log 'ZFILES LOCATION kind=trash path= items=1 error=0'
click 331 120
shot trash.png
click 803 44 1200
expect_log /tmp/f.log 'ZFILES TASK done id=2 kind=restore state=done files=1 '
expect_guest '[ -f /tmp/fhome/Documents/Costs.csv ]' 'put back'

# 4. Undo the put back, then the trash.
click 100 125
keys '<ctrl-z>'
sleep 1.5
expect_log /tmp/f.log 'ZFILES TASK done id=3 kind=trash state=done files=1 '
expect_guest '[ ! -e /tmp/fhome/Documents/Costs.csv ]' 'undo put back: in the trash again'
keys '<ctrl-z>'
sleep 1.5
expect_log /tmp/f.log 'ZFILES TASK done id=4 kind=restore state=done files=1 '
expect_guest '[ -f /tmp/fhome/Documents/Costs.csv ]' 'undo trash: back again'

# 5. Copy and paste into Downloads.
click 779 120
keys '<ctrl-c>'
sleep 0.5
expect_log /tmp/f.log 'ZFILES CLIPBOARD mode=1 items=1'
click 100 155
keys '<ctrl-v>'
sleep 1.5
expect_log /tmp/f.log 'ZFILES TASK done id=5 kind=copy state=done files=1 '
expect_guest '[ -f /tmp/fhome/Downloads/Report.pdf ]' 'pasted'

# 6. Delete for good, asked.
keys '<shift-delete>'
sleep 1
shot dialog.png
expect_log /tmp/f.log 'ZFILES DIALOG ask=1 items=1'
keys '<ret>'
sleep 1.5
expect_log /tmp/f.log 'ZFILES TASK done id=6 kind=delete state=done files=1 '
expect_guest '[ ! -e /tmp/fhome/Downloads/Report.pdf ]' 'deleted for good'

# 7. A new folder, named.
keys '<ctrl-shift-n>'
sleep 1
expect_log /tmp/f.log 'ZFILES NEWFOLDER path=/tmp/fhome/Downloads/untitled folder'
keys 'Drafts'
sleep 0.8
shot folder.png
keys '<ret>'
sleep 1
expect_guest '[ -d /tmp/fhome/Downloads/Drafts ]' 'new folder named Drafts'

# 8. Undo the rename.
keys '<ctrl-z>'
sleep 1
expect_guest '[ -d "/tmp/fhome/Downloads/untitled folder" ]' 'undo rename'

errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
guest "$stop_all" >/dev/null
guest 'cat /tmp/f.log' > "$out/f.log"
[ $status = 0 ] && echo "files-p004: PASS" || echo "files-p004: FAIL"
exit $status
