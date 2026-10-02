#!/bin/sh
# ws095-p013 (BUG-139): Japanese typed into the Text Editor through the input method, on the Venus guest (the lean
# image with the editor, plan/ws095/tests/build-ime-textedit-image.sh).  zdesktop --glass at 1280x800 starts
# /usr/libexec/keiland-ime; textedit opens a file of two lines.  Judged by the editor's and zdesktop's logs and the
# screens (never the guest's console):
#  1. before.png: the file, the cursor after "Hello, " on the first line.
#  2. Alt+Space chooses Japanese; "kanji" typed: a preedit (TEXTEDIT TEXT input preedit=かんじ begin=9 end=9), preedit.png.
#  3. Space converts (preedit=漢字 with a segment), converted.png; Space twice more opens the candidates,
#     candidates.png.
#  4. Enter commits (TEXTEDIT TEXT input commit=…), committed.png.
#  5. "watasihanihongowohanasimasu" and Space: a conversion of several segments, segments.png; Right moves to the
#     next segment, segment-next.png; Enter commits.
#
#   plan/ws095/tests/ime-guest.sh start IMAGE     (the guest must be up; GUEST_RUNTIME names its runtime)
#   plan/ws095/tests/ime-p013.sh [OUTDIR]          (default build/ws095-shots/p013)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws095-run}"
export GUEST_RUNTIME
out=${1:-build/ws095-shots/p013}
mkdir -p "$out"
status=0
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; sleep 1.2; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
shot() { pointer move 1270 790 sleep 600; check "$out/$1" >/dev/null; echo "shot: $out/$1"; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[t]extedit|[k]eiland-ime" | awk "{print \$1}"); do kill -CONT $p 2>/dev/null; kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[t]extedit|[k]eiland-ime" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'

# Waits up to 10 s for a log line matching a pattern; fails the run without one.
expect_log() {
	i=0
	while [ $i -lt 20 ]; do
		found=$(guest "grep -cE '$2' $1" | tail -1)
		if [ "${found:-0}" -gt 0 ] 2>/dev/null; then
			echo "log: $2 ok"
			return 0
		fi
		sleep 0.5
		i=$((i+1))
	done
	echo "log: $2 MISSING"
	status=1
}

guest "$stop_all" >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp HOME=/root; rm -f /tmp/wayland-0 /root/.config/kei/ime/ja-user.dict
printf "Hello, world\nThe second line of the file.\n" > /tmp/p013.txt
picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=900 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & echo started' >/dev/null
expect_log /tmp/zdesktop.log 'KEI-IME READY'
guest 'export XDG_RUNTIME_DIR=/tmp HOME=/root; /bin/textedit --timeout-s=800 --width=900 --height=520 /tmp/p013.txt > /tmp/te.log 2>&1 </dev/null & echo started' >/dev/null
expect_log /tmp/te.log 'TEXTEDIT READY'

# 1. The cursor after "Hello, " (Ctrl+Home, then seven times Right).
keys '<ctrl-home>'
keys '<right>' '<right>' '<right>' '<right>' '<right>' '<right>' '<right>'
shot before.png

# 2. Japanese, a preedit.
keys '<alt-spc>'
expect_log /tmp/zdesktop.log 'ZWL IME language=ja'
keys 'kanji'
shot preedit.png

# 3. Converted, then the candidates.
keys ' '
shot converted.png
keys ' ' ' '
shot candidates.png

# 4. Committed.
keys '\n'
shot committed.png

# 5. Several segments, the next one focused, committed.
keys 'watasihanihongowohanasimasu'
keys ' '
sleep 3
shot segments.png
keys '<right>'
sleep 1
shot segment-next.png
keys '\n'
sleep 2

# The editor's log, read on the host (the guest's grep takes no UTF-8 pattern): each step's line, in order.
timeout 60 python3 plan/tools/guest/guest.py get /tmp/te.log "$out/te.log" >/dev/null 2>&1
for line in 'TEXT input preedit=かんじ begin=9 end=9' 'TEXT input preedit=漢字 begin=0 end=6' 'TEXT input commit=' \
    'TEXT input preedit=わたしはにほんごをはなします begin=42 end=42' 'TEXT input preedit=私は日本語を話します begin=0 end=6' \
    'TEXT input preedit=私は日本語を話します begin=6 end=18' 'TEXT input commit=私は日本語を話します'; do
	if grep -qF "$line" "$out/te.log"; then
		echo "te: $line ok"
	else
		echo "te: $line MISSING"
		status=1
	fi
done
errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
[ $status = 0 ] && echo "ime-p013: PASS" || echo "ime-p013: FAIL"
exit $status
