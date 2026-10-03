#!/bin/sh
# ws089-p007: zdesktop follows the desktop's preferences (~/.config/keiland/desktop.conf, root's home) on the Venus
# guest (the lean image, build-settings-image.sh).  zdesktop --glass at 1280x800, started without --wallpaper
# (the landscape drawn by zdesktop), with Settings' About page open to see the windows' opacity.
#  1. No file: ZWL PREFERENCES open, the landscape (start.png).
#  2. wallpaper=/usr/share/keiland/wallpaper.ppm written (a new file renamed over, as libkeiland does): within a few
#     seconds ZWL PREFERENCES key=wallpaper applied and ZWL GLASS wallpaper path=... ms=N (wallpaper.png).
#  3. window.opacity=85: key=window.opacity applied value=85 (opacity.png).
#  4. pointer.speed=200, pointer.natural=1, keyboard.repeat.rate=40, keyboard.repeat.delay=250: each applied.
#  5. The file removed: the landscape again (ZWL GLASS wallpaper path=-) and the opacity back to 100 (removed.png).
#  6. zdesktop started again with a wallpaper in the file: it is applied before the look is made (key=wallpaper
#     applied, no ZWL GLASS wallpaper line) (restart.png).
#  7. No ERROR line in zdesktop's log.
# The pointer's speed on a relative mouse and the repeat seen by a client are checked with Settings' pages (p005).
#
#   plan/ws089/tests/settings-guest.sh start     (the guest must be up)
#   plan/ws089/tests/settings-p007.sh [OUTDIR]   (default build/ws089-shots/p007)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws089-run}"
export GUEST_RUNTIME
out=${1:-build/ws089-shots/p007}
mkdir -p "$out"
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1 </dev/null; }
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[s]ettings" | awk "{print \$1}"); do kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[s]ettings" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'
start_desktop='export XDG_RUNTIME_DIR=/tmp HOME=/root; rm -f /tmp/wayland-0
/bin/wayland --timeout=900 --width=1280 --height=800 --glass > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4; echo started'
start_settings='export XDG_RUNTIME_DIR=/tmp HOME=/root; /bin/settings --timeout-s=800 about > /tmp/s.log 2>&1 </dev/null & sleep 5; echo started'
conf=/root/.config/keiland/desktop.conf
status=0

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

# Fails the run when a log has a line matching a pattern.
refuse_log() {
	found=$(guest "grep -cE '$2' $1" | tail -1)
	if [ "${found:-1}" = 0 ]; then
		echo "log: no $2 ok"
	else
		echo "log: $2 UNEXPECTED"
		status=1
	fi
}

# Writes the preferences as a new file renamed over the old (the writer's way), one key=value an argument.
write_conf() {
	lines=""
	for line in "$@"; do lines="$lines$line\n"; done
	guest "mkdir -p /root/.config/keiland; printf '$lines' > $conf.new; mv $conf.new $conf; echo written" >/dev/null
}

# A picture of the screen with the pointer out of the way.
shot() {
	pointer move 1270 790 sleep 600
	check "$out/$1" >/dev/null
	echo "shot: $out/$1"
}

# 1. No file.
guest "$stop_all" >/dev/null
guest "rm -f $conf" >/dev/null
guest "$start_desktop" >/dev/null
expect_log /tmp/zdesktop.log 'ZWL PREFERENCES open'
guest "$start_settings" >/dev/null
shot start.png

# 2. The wallpaper.
write_conf 'wallpaper=/usr/share/keiland/wallpaper.ppm'
expect_log /tmp/zdesktop.log 'ZWL PREFERENCES key=wallpaper applied'
expect_log /tmp/zdesktop.log 'ZWL GLASS wallpaper path=/usr/share/keiland/wallpaper.ppm ms=[0-9]+'
guest "grep 'ZWL GLASS wallpaper' /tmp/zdesktop.log"
shot wallpaper.png

# 3. The windows' opacity.
write_conf 'wallpaper=/usr/share/keiland/wallpaper.ppm' 'window.opacity=85'
expect_log /tmp/zdesktop.log 'ZWL PREFERENCES key=window.opacity applied value=85'
shot opacity.png

# 4. The pointer and the keyboards.
write_conf 'wallpaper=/usr/share/keiland/wallpaper.ppm' 'window.opacity=85' 'pointer.speed=200' 'pointer.natural=1' 'keyboard.repeat.rate=40' 'keyboard.repeat.delay=250'
expect_log /tmp/zdesktop.log 'ZWL PREFERENCES key=pointer.speed applied value=200'
expect_log /tmp/zdesktop.log 'ZWL PREFERENCES key=pointer.natural applied value=1'
expect_log /tmp/zdesktop.log 'ZWL PREFERENCES key=keyboard.repeat.rate applied value=40'
expect_log /tmp/zdesktop.log 'ZWL PREFERENCES key=keyboard.repeat.delay applied value=250'

# 5. The file removed: back to the command line's.
guest "rm -f $conf" >/dev/null
expect_log /tmp/zdesktop.log 'ZWL GLASS wallpaper path=- ms='
expect_log /tmp/zdesktop.log 'ZWL PREFERENCES key=window.opacity applied value=100'
expect_log /tmp/zdesktop.log 'ZWL PREFERENCES key=pointer.speed applied value=100'
shot removed.png

# 6. A wallpaper in the file when zdesktop starts.
guest "$stop_all" >/dev/null
write_conf 'wallpaper=/usr/share/keiland/wallpaper.ppm'
guest "$start_desktop" >/dev/null
expect_log /tmp/zdesktop.log 'ZWL PREFERENCES key=wallpaper applied'
expect_log /tmp/zdesktop.log 'ZWL PREFERENCES open'
refuse_log /tmp/zdesktop.log 'ZWL GLASS wallpaper path='
refuse_log /tmp/zdesktop.log 'ZWL GLASS no wallpaper'
shot restart.png

# 7. zdesktop saw no error.
errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; guest "grep ERROR /tmp/zdesktop.log | head -5"; status=1; }
guest 'grep -E "PREFERENCES|GLASS wallpaper|STARTUP step=wallpaper" /tmp/zdesktop.log' > "$out/preferences.log"
guest "$stop_all" >/dev/null
guest "rm -f $conf" >/dev/null
[ $status = 0 ] && echo "settings-p007: PASS" || echo "settings-p007: FAIL"
exit $status
