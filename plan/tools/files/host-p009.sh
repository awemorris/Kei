#!/bin/sh
# ws071-p009: the context menus of files on the host (files-render, host-render.c).
# Builds nothing: run host-build.sh first.  Each case runs files-render on a fresh sample home,
# started in Documents (the Trash for 3), and checks the lines it prints:
#  1. A right press on an item: the item is selected, the window asks for a context menu of items
#     (request 5, where 0) with Open, Open With, Cut, Copy, Rename, Tags, Get Info, Move to Trash;
#     Paste is disabled while the clipboard is empty.
#  2. A right press on the empty part of the content: nothing stays selected; New Folder, View (as Icons
#     checked), Sort By (Name checked), Show Hidden Files.
#  3. In the trash, a right press on an item: Put Back, Delete Immediately, Empty Trash.
#  4. A right press on a place of the sidebar (Downloads): Open in New Tab, Remove from Sidebar; the
#     context menu's own action opens it in a new tab.
#  5. (ws127-p002) Move To: the items' menu has the submenu with the sidebar's folders but not the folder shown
#     (Documents); its Downloads row moves the item there.
#
#   plan/tools/files/host-p009.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
out=${1:-build/ws071-p009-host}
mkdir -p "$out"
home=$(pwd)/build/ws071-host/home
docs=$home/Documents
status=0

# Runs files-render on a fresh home with the actions given; the output goes to $out/NAME.txt.
run() {
	name=$1
	shift
	sh plan/tools/files/host-run.sh --fresh "$@" > "$out/$name.txt" 2>&1
}

# Fails the run unless the output of a case has a line matching a pattern.
expect() {
	if grep -qE "$2" "$out/$1.txt"; then
		echo "$1: $2 ok"
	else
		echo "$1: $2 MISSING"
		status=1
	fi
}

# 1. Items (Budget.csv, the second icon).
run items "--start=$docs" right=446,150 state context draw="$out/items.ppm"
expect items "^state selection=1 "
expect items "^context request=5 where=0 place=-1 x=446 y=150 count="
expect items "^row id=1003 parent=0 kind=0 enabled=1 checked=0 action=3 label=Open\$"
expect items "^row id=100 parent=0 kind=4 enabled=1 .* label=Open With\$"
expect items "^row id=1011 parent=0 kind=0 enabled=0 checked=0 action=11 label=Paste\$"
expect items "^row id=1014 parent=0 kind=0 enabled=1 checked=0 action=14 label=Rename\$"
expect items "^row id=1300 parent=101 kind=2 enabled=1 checked=0 action=300 label=Work\$"
expect items "^row id=1004 .* label=Get Info\$"
expect items "^row id=1005 .* label=Move to Trash\$"

# 2. The empty part of the content.
run empty "--start=$docs" click=446,150 right=700,560 state context
expect empty "^state selection=0 "
expect empty "^context request=5 where=1 "
expect empty "^row id=1002 parent=0 kind=0 enabled=1 checked=0 action=2 label=New Folder\$"
expect empty "^row id=1015 parent=102 kind=2 enabled=1 checked=1 action=15 label=as Icons\$"
expect empty "^row id=1017 parent=103 kind=2 enabled=1 checked=1 action=17 label=Name\$"
expect empty "^row id=1023 parent=0 kind=2 enabled=1 checked=0 action=23 label=Show Hidden Files\$"

# 3. The trash: an item trashed first (Delete moves it to the trash), then the trash's own rows.
run trash "--start=$docs" click=446,150 key=111 action=33 wait=400 right=334,150 context
expect trash "^context request=5 where=0 "
expect trash "^row id=1046 parent=0 kind=0 enabled=1 checked=0 action=46 label=Put Back\$"
expect trash "^row id=1047 .* label=Delete Immediately\$"
expect trash "^row id=1048 .* label=Empty Trash\$"

# 4. A place of the sidebar, and its Open in New Tab.
run place "--start=$docs" right=100,155 context action=49 tabs
expect place "^context request=5 where=2 place=3 "
expect place "^row id=1049 parent=0 kind=0 enabled=1 checked=0 action=49 label=Open in New Tab\$"
expect place "^row id=1050 .* label=Remove from Sidebar\$"
expect place "^tabs count=2 shown=1 0=$docs 1=$home/Downloads\$"

# 5. Move To (ws127-p002): Budget.csv to Downloads.
run moveto "--start=$docs" right=446,150 context action=503 wait=1500
expect moveto "^row id=105 parent=0 kind=4 enabled=1 checked=0 action=0 label=Move To\$"
expect moveto "^row id=1503 parent=105 kind=0 enabled=1 checked=0 action=503 label=Downloads\$"
if grep -qE "^row id=15[0-9][0-9] parent=105 .* label=Documents\$" "$out/moveto.txt"; then
	echo "moveto: the folder shown is offered MISSING"
	status=1
fi
expect moveto "CONTEXT move-to place=3 path=$home/Downloads count=1"
if [ -f "$home/Downloads/Budget.csv" ] && [ ! -e "$docs/Budget.csv" ]; then
	echo "moveto: Budget.csv is in Downloads ok"
else
	echo "moveto: Budget.csv is in Downloads MISSING"
	status=1
fi

exit $status
