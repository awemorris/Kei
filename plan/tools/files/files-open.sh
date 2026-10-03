#!/bin/sh
# ws093-p002: Files opens a file with the app its kind has (the built-in defaults of files/apps.c) on the Venus guest.
# The running guest gets this worktree's files, imageview, textedit, compositor and libraries (BIN), and a folder
# /tmp/demo of a PNG, a JPEG and a GIF (plan/tools/imageview/make-images.py), a text, an HTML page and a PDF.
# zdesktop --glass at 1280x800; files at 1000x640 on /tmp/demo in the list view (HOME=/tmp/fhome, no user list).
#   mouse   each row double-clicked (the PNG, the JPEG, the GIF, the text, the page, the PDF), and the text again by
#           Enter: Files logs OPEN with the app and LAUNCH with its command on the path, the app runs (ps), its
#           window maps; a picture of each
#   always  the text's context menu Always Open With > Terminal (less) (its default, opened with less, and a double
#           click opens it so), then File > Always Open With > Use System Default (Text Editor again)
#   info    the information card (Ctrl+I) of the PNG, the text and the page: their ways, the new default first
#   touch   (needs the pen image, /bin/touchinject) the PNG and the text double-tapped with the injected touch screen
# The app's own output goes to /dev/null (files starts it apart), so the steps read Files' log, zdesktop's log and ps
# through SSH, and the pictures; nothing reads the console.
#   GUEST_RUNTIME=$PWD/build/ws093-run BIN=build/ws093-amd64 plan/tools/files/files-open.sh OUTDIR mouse|always|info|touch
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
export GUEST_RUNTIME="${GUEST_RUNTIME:-$PWD/build/ws093-run}"
bin=${BIN:-build/ws093-amd64}
out=${1:-build/ws093-shots}
mode=${2:-mouse}
mkdir -p "$out"
status=0
guest() { timeout 120 python3 plan/tools/guest/guest.py run "$1" 2>&1 </dev/null; }
. plan/tools/guest/zwl-clients.sh
put() { timeout 120 python3 plan/tools/guest/guest.py put "$1" "$2" >/dev/null 2>&1 </dev/null || { echo "put $1: FAILED"; status=1; }; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@" >/dev/null; sleep 0.7; }
shot() {
	python3 plan/ws035/tests/zdesktop-check.py "$out/$1" --runtime "$GUEST_RUNTIME" >/dev/null
	echo "shot $1"
}
apps='[i]mageview|[t]extedit|[b]rowser|[p]dfviewer'
stop_apps="for p in \$(ps -A -o pid,args | grep -E '$apps' | awk '{print \$1}'); do kill \$p; done; sleep 1"
stop_all="service stop greeter >/dev/null 2>&1; for p in \$(ps -A -o pid,args | grep -E '[w]ayland( |\$)|[f]iles|$apps|[t]ouchinject' | awk '{print \$1}'); do kill \$p; done; sleep 1"

# Fails the run unless a log has a line matching a pattern (within a few seconds).
expect_log() {
	tries=0
	found=0
	while [ $tries -lt 10 ]; do
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

# Fails the run unless a program runs (zedBSD's ps shows no arguments; the path is in Files' LAUNCH line).
expect_running() {
	running=$(guest "ps -A -o args | grep -cE '$1'" | tail -1)
	if [ "${running:-0}" -ge 1 ] 2>/dev/null; then
		echo "running: $1 ok"
	else
		echo "running: $1 MISSING"
		status=1
	fi
}

# The row n of the list view (1 the first), as in plan/tools/files/files-p012.sh.
row_y() { echo $((86 + 28 * $1)); }

# Double-clicks a row of the window.
double() {
	y=$(row_y "$1")
	pointer move $((wx + 400)) $((wy + y)) sleep 300 down sleep 50 up sleep 80 down sleep 50 up sleep "${2:-900}"
}

# The middle (y) of the latest popup row of a menu item (zdesktop's log), and the latest popup's left edge.
menu_row_y() {
	guest "grep 'MENU row item=$1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* y=\([0-9]*\) height=\([0-9]*\).*/\1 \2/p' | { read y h; echo $(( ${y:-0} + ${h:-0} / 2 )); }
}
menu_x() {
	guest "grep 'MENU open ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([0-9]*\) y=.*/\1/p'
}

# Clicks the row of a menu item in the latest popup.
menu_click() {
	x=$(( $(menu_x) + 60 ))
	y=$(menu_row_y "$1")
	pointer move $((x - 2)) "$y" sleep 150 move "$x" "$y" sleep 300 down sleep 60 up sleep 900
}

# Opens a top-level menu (its item) from the titlebar's "...", as plan/tools/files/files-p008.sh does.
menu_top() {
	zwl_app_clients
	set -- "$1" $(guest "grep 'ZWL TITLEBAR control client=$zc1 .* where=floating id=0 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([-0-9]*\) y=\([-0-9]*\) width=\([0-9]*\) height=\([0-9]*\).*/\1 \2 \3 \4/p')
	x=$((${2:-0} + ${4:-0} / 2)); y=$((${3:-0} + ${5:-0} / 2))
	pointer move $((x - 2)) "$y" sleep 150 move "$x" "$y" sleep 300 down sleep 60 up sleep 900
	menu_click "$1"
}

# Checks one opening: Files' OPEN line, the app on the path, a new window; a picture, and the app stops.
opened() {
	expect_log /tmp/f.log "ZFILES OPEN path=/tmp/demo/$1 app=$2 error=0"
	sleep "${5:-5}"
	expect_log /tmp/f.log "ZFILES LAUNCH name=$2 command=/bin/$3 ./tmp/demo/$1.$"
	expect_running "[${3%"${3#?}"}]${3#?}"
	maps=$(guest "grep -c 'ZWL MAP client=' /tmp/zdesktop.log" | tail -1)
	[ "${maps:-0}" -gt "$maps_before" ] 2>/dev/null && echo "window: $2 mapped" || { echo "window: $2 MISSING"; status=1; }
	maps_before=${maps:-0}
	shot "$4"
	guest "$stop_apps" >/dev/null
	sleep 1
}

# The programs under test, the folder, the compositor and files.
python3 plan/tools/imageview/make-images.py build/ws093-images >/dev/null
python3 plan/ws081/tests/make-touch-pdf.py build/ws093-images/doc.pdf >/dev/null
printf 'Meeting notes\n\n- Kei demo on Friday\n- Open pictures and text from Files\n' > build/ws093-images/notes.txt
printf '<!doctype html>\n<html><head><title>Kei page</title></head><body><h1>Hello from Files</h1><p>This page was opened by a double click.</p></body></html>\n' > build/ws093-images/page.html
guest "$stop_all" >/dev/null
for program in files imageview textedit wayland; do
	put "$bin/bin/$program" "/bin/$program"
done
for library in libkeiland libvulkan libwayland-client libtruetype libpng-compat libjpeg-compat libgif-compat libz-compat; do
	put "$bin/dynamic/$library.so" "/lib/$library.so"
done
guest 'chmod 755 /bin/files /bin/imageview /bin/textedit /bin/wayland; rm -rf /tmp/demo /tmp/fhome; mkdir -p /tmp/demo /tmp/fhome' >/dev/null
put build/ws093-images/01-splash.png /tmp/demo/1-photo.png
put build/ws093-images/02-landscape.jpg /tmp/demo/2-photo.jpg
put build/ws093-images/05-anim.gif /tmp/demo/3-anim.gif
put build/ws093-images/notes.txt /tmp/demo/4-notes.txt
put build/ws093-images/page.html /tmp/demo/5-page.html
put build/ws093-images/doc.pdf /tmp/demo/6-doc.pdf
guest 'ls /bin/browser /bin/pdfviewer /bin/imageview /bin/textedit; ls /tmp/demo' > "$out/programs-$mode.txt"
guest 'export XDG_RUNTIME_DIR=/tmp
picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
rm -f /tmp/wayland-0; /bin/wayland --timeout=1200 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & sleep 5
HOME=/tmp/fhome /bin/files --token=f1 --timeout-s=1100 --width=1000 --height=640 /tmp/demo > /tmp/f.log 2>&1 </dev/null & sleep 6; echo started' >/dev/null
zwl_app_clients
set -- $(guest "grep 'ZWL MAP client=$zc1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* surface=\([0-9]*\) x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2 \3/p')
wx=${2:-0}; wy=${3:-0}
echo "files at $wx,$wy"
expect_log /tmp/f.log 'ZFILES LOCATION kind=folder path=/tmp/demo items=6 error=0'
keys '<ctrl-2>'
shot "files-$mode.png"
maps_before=$(guest "grep -c 'ZWL MAP client=' /tmp/zdesktop.log" | tail -1)

case "$mode" in
mouse)
	double 1
	opened 1-photo.png 'Image Viewer' imageview open-png.png
	double 2
	opened 2-photo.jpg 'Image Viewer' imageview open-jpeg.png
	double 3
	opened 3-anim.gif 'Image Viewer' imageview open-gif.png
	double 4
	opened 4-notes.txt 'Text Editor' textedit open-text.png
	double 5
	opened 5-page.html Browser browser open-html.png 10
	double 6
	opened 6-doc.pdf 'PDF Viewer' pdfviewer open-pdf.png
	# Enter opens the selected text as a double click does.
	y=$(row_y 4)
	pointer move $((wx + 400)) $((wy + y)) sleep 300 down sleep 60 up sleep 800
	guest 'echo > /dev/null' >/dev/null
	lines=$(guest "grep -c 'ZFILES OPEN path=/tmp/demo/4-notes.txt' /tmp/f.log" | tail -1)
	keys '<ret>'
	sleep 2
	again=$(guest "grep -c 'ZFILES OPEN path=/tmp/demo/4-notes.txt app=Text Editor' /tmp/f.log" | tail -1)
	[ "${again:-0}" -gt "${lines:-0}" ] 2>/dev/null && echo "enter: ok" || { echo "enter: MISSING"; status=1; }
	opened 4-notes.txt 'Text Editor' textedit open-enter.png
	;;
always)
	# The text's context menu: Always Open With > Terminal (less) makes less its default and opens it with less.
	y=$(row_y 4)
	pointer move $((wx + 400)) $((wy + y)) sleep 300 down sleep 60 up sleep 800
	pointer move $((wx + 402)) $((wy + y)) sleep 200 right-down sleep 60 right-up sleep 900
	expect_log /tmp/zdesktop.log 'MENU row item=104 '
	shot always-context.png
	menu_click 104
	expect_log /tmp/zdesktop.log 'MENU row item=1401 '
	shot always-submenu.png
	menu_click 1401
	expect_log /tmp/f.log 'ZFILES DEFAULT set type=text/plain app=Terminal \(less\) error=0'
	expect_log /tmp/f.log 'ZFILES LAUNCH name=Terminal \(less\) command=@terminal less ./tmp/demo/4-notes.txt.$'
	sleep 3
	shot always-less.png
	guest "for p in \$(ps -A -o pid,args | grep -E '[t]erminal|[l]ess ' | awk '{print \$1}'); do kill \$p; done; sleep 1" >/dev/null
	guest 'cat /tmp/fhome/.config/keiland/open-with' > "$out/always-list.txt"
	# A double click now opens the text with less.
	before=$(guest "grep -c 'ZFILES OPEN path=/tmp/demo/4-notes.txt app=Terminal (less)' /tmp/f.log" | tail -1)
	double 4 3000
	after=$(guest "grep -c 'ZFILES OPEN path=/tmp/demo/4-notes.txt app=Terminal (less)' /tmp/f.log" | tail -1)
	[ "${after:-0}" -gt "${before:-0}" ] 2>/dev/null && echo "double click: less ok" || { echo "double click: less MISSING"; status=1; }
	guest "for p in \$(ps -A -o pid,args | grep -E '[t]erminal|[l]ess ' | awk '{print \$1}'); do kill \$p; done; sleep 1" >/dev/null
	# File > Always Open With shows the choice; Use System Default gives the text back to Text Editor.
	menu_top 1
	menu_click 14
	expect_log /tmp/zdesktop.log 'MENU row item=1450 '
	shot always-file-menu.png
	menu_click 1450
	expect_log /tmp/f.log 'ZFILES DEFAULT clear type=text/plain error=0'
	shot always-system.png
	double 4 1500
	opened 4-notes.txt 'Text Editor' textedit always-back.png
	guest 'cat /tmp/fhome/.config/keiland/open-with' > "$out/always-list-after.txt"
	;;
info)
	# The information card of the PNG and of the text lists their ways, the new default first.
	for step in "1 1-photo.png info-png.png" "4 4-notes.txt info-text.png" "5 5-page.html info-html.png"; do
		set -- $step
		y=$(row_y "$1")
		pointer move $((wx + 400)) $((wy + y)) sleep 300 down sleep 60 up sleep 800
		keys '<ctrl-i>'
		sleep 1
		expect_log /tmp/f.log "ZFILES INFO path=/tmp/demo/$2 "
		shot "$3"
		keys '<esc>'
	done
	guest "grep 'ZFILES INFO path' /tmp/f.log" > "$out/info-lines.txt"
	;;
touch)
	# A double tap (two taps 120 ms apart) on the PNG's row, then on the text's.
	for step in "1 1-photo.png Image_Viewer imageview touch-png.png" "4 4-notes.txt Text_Editor textedit touch-text.png"; do
		set -- $step
		x=$((wx + 400)); y=$((wy + $(row_y "$1")))
		printf 'size 1279 799 2\nwait 2600\ndown 1 %d %d\nwait 60\nup 1\nwait 120\ndown 1 %d %d\nwait 60\nup 1\nhold 1200\n' "$x" "$y" "$x" "$y" > "$out/tap-$1.script"
		put "$out/tap-$1.script" "/tmp/tap-$1.script"
		result=$(guest "/bin/touchinject /tmp/tap-$1.script 2>&1; echo replay=\$?")
		printf '%s\n' "$result" | grep -q '^replay=0$' || { echo "touchinject $1: FAILED"; status=1; }
		opened "$2" "$(echo "$3" | tr _ ' ')" "$4" "$5"
	done
	;;
esac

errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
guest 'cat /tmp/f.log' > "$out/f-$mode.log"
guest "$stop_all" >/dev/null
[ $status = 0 ] && echo "open-guest $mode: PASS" || echo "open-guest $mode: FAIL"
exit $status
