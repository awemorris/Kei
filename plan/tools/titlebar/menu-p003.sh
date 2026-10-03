#!/bin/sh
# ws070 (p002-p004): the System Menu on the Venus guest (the lean image, plan/tools/titlebar/build-menu-image.sh).
# zdesktop --glass runs at 1280x800 with the wallpaper, and terminal (t1) gives it its menus
# (Shell, Edit, View, Session, Help) through libkeiland.  Every step checks zdesktop's and the
# terminal's log lines, and the screens are kept for reading:
#  1. floating.png: the menus in the floating title bar after the title.
#  2. edit.png: Edit opened by the pointer: Copy and Paste pale (nothing selected, nothing copied).
#  3. selected.png: Edit > Select All chosen by the pointer: the screen selected (the selection's colour).
#  4. Ctrl+Shift+C (the shortcut) copies: zdesktop activates Copy (via=shortcut), the terminal copies.
#  5. keyboard.png: F10, Right x3, Down x2 selects Session > Clear Screen; Enter chooses it.
#  6. The line typed is copied (Ctrl+Shift+A, Ctrl+Shift+C), Session > Send Interrupt drops it,
#     Edit > Paste (enabled now) types it again and Enter runs it: paste.png shows PASTE-OK.
#  7. submenu.png: View > Text Size opens its submenu (Medium checked); Large is chosen (ZTERM ZOOM 20);
#     view-large.png: View again with Large checked and Zoom In still enabled; Esc closes it.
#  8. Ctrl+- and Ctrl+0 zoom out and back (18, 16).
#  9. A press outside the open Help menu closes it and reaches nobody.
# 10. docked-edit.png: docked (a double click on the title), the menus are in the system bar; Edit opens
#     under the bar; Esc closes it.
# 11. F11 makes the window fullscreen and back (Fullscreen checked, then not).
# 12. about.png: Help > About Terminal writes its line.
# 13. two.png: Ctrl+Shift+N starts a second terminal (t1-new) on top: its own menus, and the system bar
#     without the docked window's; its Shell > Close Window ends it (reason=menu-close).
# 14. Ctrl+Shift+Q ends t1 (reason=menu-close); no ERROR line in zdesktop's log.
#
#   plan/tools/titlebar/menu-guest.sh start     (the guest must be up)
#   plan/tools/titlebar/menu-p003.sh [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws070-run}"
export GUEST_RUNTIME
out=${1:-build/ws070-p003}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
stop_all='for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[t]erminal" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[t]erminal" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
status=0

# Fails the run unless a log has a line matching a pattern (within a few seconds: a loaded host
# delays the line of an action the guest has already carried out).
expect_log() {
	tries=0
	found=0
	while [ $tries -lt 5 ]; do
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

# The centre (x) of a top-level item of a client's bar: floating from the window's x, docked from 0.
item_x() {
	guest "grep 'MENU bar client=$1 .* where=$2 item=$3 ' /tmp/zdesktop.log | tail -1" |
	    sed -n 's/.* offset=\([-0-9]*\) top=[-0-9]* width=\([0-9]*\).*/\1 \2/p' | { read offset width; echo $(( ${4:-0} + ${offset:-0} + ${width:-0} / 2 )); }
}

# The middle (y) of the latest popup row of an item, and the latest popup's left edge.
row_y() {
	guest "grep 'MENU row item=$1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* y=\([0-9]*\) height=\([0-9]*\).*/\1 \2/p' | { read y h; echo $(( ${y:-0} + ${h:-0} / 2 )); }
}
popup_x() {
	guest "grep 'MENU open ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* x=\([0-9]*\) y=.*/\1/p'
}

# Clicks a point and moves the pointer out of the way.
click() {
	pointer move $(($1 - 2)) "$2" sleep 150 move "$1" "$2" sleep 300 down sleep 60 up sleep "${3:-700}"
}

guest "$stop_all" >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0; picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=900 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4
/bin/terminal --token=t1 --timeout-s=800 > /tmp/t.log 2>&1 </dev/null & sleep 6; echo started' >/dev/null
set -- $(guest "grep 'ZWL MAP client=1 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* surface=\([0-9]*\) x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2 \3/p')
surface=${1:-0}; wx=${2:-0}; wy=${3:-0}; bar=$((wy - 30))
echo "terminal: surface $surface at $wx,$wy"

# 1. The menus in the floating title bar.
expect_log /tmp/t.log 'ZTERM MENU ready items=31'
expect_log /tmp/zdesktop.log 'MENU commit client=1 menu=[0-9]+ serial=1 items=31'
expect_log /tmp/zdesktop.log "MENU bar client=1 surface=$surface where=floating item=5 "
pointer move 1250 780 sleep 400
check "$out/floating.png" --expect $((wx + 400)),$((wy + 300)),1d2230 || status=1

# 2. Edit by the pointer.
click "$(item_x 1 floating 2 $wx)" $bar
check "$out/edit.png" >/dev/null
expect_log /tmp/zdesktop.log "MENU open client=1 surface=$surface item=2 depth=1"
expect_log /tmp/t.log 'ZTERM MENU state selection=0 clipboard=0 pixels=16 fullscreen=0'

# 3. Select All.
click $(( $(popup_x) + 60 )) "$(row_y 23)" 900
pointer move 1250 780 sleep 400
check "$out/selected.png" --expect $((wx + 400)),$((wy + 300)),3a5a98 || status=1
expect_log /tmp/zdesktop.log 'MENU activate client=1 place=[0-9]+ item=23 action=5 .*via=pointer'
expect_log /tmp/t.log 'ZTERM MENU state selection=1 clipboard=0'

# 4. The Copy shortcut.
keys '<ctrl-shift-c>'
sleep 1
expect_log /tmp/zdesktop.log 'MENU activate client=1 place=[0-9]+ item=20 action=3 .*via=shortcut'
expect_log /tmp/t.log 'ZTERM COPY bytes=[1-9]'
expect_log /tmp/t.log 'ZTERM MENU state selection=1 clipboard=1'

# 5. The keyboard: F10 opens Shell, Right three times goes to Session, Down twice to Clear Screen.
keys '<f10>' '<right>' '<right>' '<right>' '<down>' '<down>'
sleep 1
check "$out/keyboard.png" >/dev/null
keys '<ret>'
sleep 1
expect_log /tmp/zdesktop.log 'MENU activate client=1 place=[0-9]+ item=53 action=16 .*via=key'

# 6. Copy what is typed, drop the line, paste it and run it.
keys 'echo PASTE-OK'
sleep 0.5
keys '<ctrl-shift-a>' '<ctrl-shift-c>'
sleep 1
expect_log /tmp/t.log 'ZTERM COPY bytes=13'
click "$(item_x 1 floating 4 $wx)" $bar
click $(( $(popup_x) + 60 )) "$(row_y 50)" 900
expect_log /tmp/zdesktop.log 'MENU activate client=1 place=[0-9]+ item=50 action=14'
click "$(item_x 1 floating 2 $wx)" $bar
click $(( $(popup_x) + 60 )) "$(row_y 21)" 900
keys '\n'
sleep 1.5
pointer move 1250 780 sleep 400
check "$out/paste.png" >/dev/null
expect_log /tmp/t.log 'ZTERM PASTE bytes=13'

# 7. View > Text Size > Large, and View again.
click "$(item_x 1 floating 3 $wx)" $bar
px=$(popup_x)
pointer move $((px + 40)) "$(row_y 33)" sleep 500 move $((px + 120)) "$(row_y 33)" sleep 600
check "$out/submenu.png" >/dev/null
expect_log /tmp/zdesktop.log "MENU open client=1 surface=$surface item=33 depth=2"
click $(( $(popup_x) + 60 )) "$(row_y 42)" 1200
expect_log /tmp/t.log 'ZTERM ZOOM run=t1 pixels=20 '
expect_log /tmp/t.log 'ZTERM MENU state selection=[01] clipboard=1 pixels=20'
click "$(item_x 1 floating 3 $wx)" $bar
px=$(popup_x)
pointer move $((px + 40)) "$(row_y 33)" sleep 500 move $((px + 120)) "$(row_y 33)" sleep 600
check "$out/view-large.png" >/dev/null
keys '<esc>' '<esc>'
sleep 0.5
expect_log /tmp/zdesktop.log "MENU close client=1 surface=$surface item=3 depth=0"

# 8. Zoom out and back by the shortcuts.
keys '<ctrl-minus>'
sleep 1
expect_log /tmp/t.log 'ZTERM ZOOM run=t1 pixels=18 '
keys '<ctrl-0>'
sleep 1
expect_log /tmp/t.log 'ZTERM ZOOM run=t1 pixels=16 '

# 9. A press outside an open menu closes it.
click "$(item_x 1 floating 5 $wx)" $bar
closes=$(guest "grep -c 'MENU close client=1 surface=$surface item=5 ' /tmp/zdesktop.log" | tail -1)
click 1200 600
now=$(guest "grep -c 'MENU close client=1 surface=$surface item=5 ' /tmp/zdesktop.log" | tail -1)
if [ "${now:-0}" -gt "${closes:-0}" ] 2>/dev/null; then echo "outside press closes: ok"; else echo "outside press closes: MISSING"; status=1; fi

# 10. Docked: the menus in the system bar.
pointer move $((wx + 80)) $bar sleep 400 down sleep 60 up sleep 60 down sleep 60 up sleep 1500
expect_log /tmp/zdesktop.log "GLASS dock surface=$surface via=double-click"
expect_log /tmp/zdesktop.log "MENU bar client=1 surface=$surface where=docked item=2 "
click "$(item_x 1 docked 2 0)" 17
check "$out/docked-edit.png" >/dev/null
expect_log /tmp/zdesktop.log "MENU open client=1 surface=$surface item=2 depth=1 x=[0-9]+ y=40 "
keys '<esc>'
sleep 0.5

# 11. Fullscreen by F11, and back.
keys '<f11>'
sleep 3
expect_log /tmp/t.log 'ZTERM MENU state .*fullscreen=1'
keys '<f11>'
sleep 3
expect_log /tmp/t.log 'ZTERM MENU state .*fullscreen=0'

# 12. Help > About.
click "$(item_x 1 docked 5 0)" 17
click $(( $(popup_x) + 60 )) "$(row_y 60)" 1000
pointer move 1250 780 sleep 400
check "$out/about.png" >/dev/null
expect_log /tmp/t.log 'ZTERM ACTION run=t1 action=18'

# 13. A new window by Ctrl+Shift+N; its own menu closes it.
keys '<ctrl-shift-n>'
sleep 6
set -- $(guest "grep 'ZWL MAP client=2 ' /tmp/zdesktop.log | tail -1" | sed -n 's/.* surface=\([0-9]*\) x=\([-0-9]*\) y=\([-0-9]*\).*/\1 \2 \3/p')
nx=${2:-0}; ny=${3:-0}
expect_log /tmp/t.log 'ZTERM START run=t1-new'
expect_log /tmp/zdesktop.log 'MENU bar client=2 surface=[0-9]+ where=floating item=5 '
pointer move 1250 780 sleep 400
check "$out/two.png" >/dev/null
click "$(item_x 2 floating 1 $nx)" $((ny - 30))
click $(( $(popup_x) + 60 )) "$(row_y 12)" 2000
expect_log /tmp/t.log 'ZTERM DONE run=t1-new reason=menu-close'

# 14. Ctrl+Shift+Q ends t1.
keys '<ctrl-shift-q>'
sleep 2
expect_log /tmp/t.log 'ZTERM DONE run=t1 reason=menu-close'
guest 'grep -E "FAILED|ERROR" /tmp/t.log /tmp/zdesktop.log' | tee "$out/errors.txt"
[ -s "$out/errors.txt" ] && status=1
guest 'cat /tmp/t.log' > "$out/t.log"
guest 'grep MENU /tmp/zdesktop.log' > "$out/zdesktop-menu.log"
guest "$stop_all" >/dev/null
[ $status -eq 0 ] && echo "menu-p003: PASS (and judge the screens)" || echo "menu-p003: FAIL"
exit $status
