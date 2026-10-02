#!/bin/sh
# ws095-p005: the candidate window, the language indicator in the system bar and the input method's key repeat, on
# the Venus guest (the lean image with the editor, plan/ws095/tests/build-ime-textedit-image.sh).  zdesktop --glass at
# 1280x800 starts /usr/libexec/keiland-ime; textedit opens a file.  Judged by zdesktop's log (the input method writes
# its KEI-IME lines there), the editor's log read on the host, and the screens (never the guest's console):
#  1. The input method's candidate window is made (KEI-IME POPUP ready=1).  bar-direct.png: the indicator "A" left of
#     the volume in the system bar.
#  2. Alt+Space: bar-ja.png, the indicator "あ" (ZWL IME language=ja).
#  3. "kanji" and Space three times (the window opens at the third candidate, design §7.2): the candidate window
#     below the caret (KEI-IME POPUP shown), candidates.png; "3" chooses the third candidate (chosen.png) and Enter
#     commits it (TEXTEDIT TEXT input commit=…), the window goes (KEI-IME POPUP hidden), committed.png.
#  4. "a" held 1.6 s: the input method repeats it into the preedit (a preedit of more than five あ), repeat.png;
#     Escape drops it.
#  5. A click on the indicator chooses the next language (ZWL IME indicator next, ZWL IME language=direct),
#     bar-clicked.png.
#
#   plan/ws095/tests/ime-guest.sh start IMAGE     (the guest must be up; GUEST_RUNTIME names its runtime)
#   plan/ws095/tests/ime-p005.sh [OUTDIR]          (default build/ws095-shots/p005)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws095-run}"
export GUEST_RUNTIME
out=${1:-build/ws095-shots/p005}
mkdir -p "$out"
status=0
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1; }
check() { python3 plan/ws035/tests/zdesktop-check.py "$@" --runtime "$GUEST_RUNTIME"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; sleep 1.2; }
input() { python3 plan/tools/files/qmp-input.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
shot() { input move 640 790 sleep 600; check "$out/$1" >/dev/null; echo "shot: $out/$1"; }
stop_all='service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[t]extedit|[k]eiland-ime" | awk "{print \$1}"); do kill -CONT $p 2>/dev/null; kill $p; done; i=0; while ps -A -o args | grep -qE "[w]ayland( |$)|[t]extedit|[k]eiland-ime" && [ $i -lt 50 ]; do sleep 0.2; i=$((i+1)); done'

# Waits up to 10 s for an ASCII log line matching a pattern (the guest's grep takes no UTF-8 pattern).
expect_log() {
	i=0
	while [ $i -lt 20 ]; do
		found=$(guest "grep -cE '$2' $1" | tail -1)
		if [ "${found:-0}" -ge "${3:-1}" ] 2>/dev/null; then
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
printf "Hello\n" > /tmp/p005.txt
picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=900 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & echo started' >/dev/null
expect_log /tmp/zdesktop.log 'KEI-IME READY'
expect_log /tmp/zdesktop.log 'KEI-IME POPUP ready=1'
guest 'export XDG_RUNTIME_DIR=/tmp HOME=/root; /bin/textedit --timeout-s=800 --width=900 --height=520 /tmp/p005.txt > /tmp/te.log 2>&1 </dev/null & echo started' >/dev/null
expect_log /tmp/te.log 'TEXTEDIT READY'
keys '<ctrl-end>'

# 1. Direct input: "A" in the bar.
shot bar-direct.png

# 2. Japanese: "あ" in the bar.
keys '<alt-spc>'
expect_log /tmp/zdesktop.log 'ZWL IME language=ja'
shot bar-ja.png

# 3. The candidate window, a choice by its digit.
keys 'kanji' ' ' ' ' ' '
expect_log /tmp/zdesktop.log 'KEI-IME POPUP shown'
shot candidates.png
keys '3'
shot chosen.png
keys '\n'
expect_log /tmp/zdesktop.log 'KEI-IME POPUP hidden'
shot committed.png

# The commit writes the user dictionary (fsync, ja-user.c); in QEMU it may take longer than zdesktop waits (500 ms),
# and zdesktop passes keys by the input method until it answers again: the held key waits for that.
sleep 2

# 4. A held key repeats into the preedit.
input hold a sleep 1600 free a sleep 800
shot repeat.png
keys '<esc>'

# 5. A click on the indicator: the next language.
x=$(guest "grep -o 'ZWL IME indicator x=[0-9]*' /tmp/zdesktop.log | tail -1" | sed -n 's/.*x=\([0-9]*\).*/\1/p' | tail -1)
input move $(( ${x:-700} + 13 )) 17 sleep 300 down sleep 60 up sleep 1200
expect_log /tmp/zdesktop.log 'ZWL IME indicator next'
expect_log /tmp/zdesktop.log 'ZWL IME language=direct' 2
shot bar-clicked.png

# The editor's log, read on the host: the third candidate committed, and a preedit of more than five あ.
timeout 60 python3 plan/tools/guest/guest.py get /tmp/te.log "$out/te.log" >/dev/null 2>&1
if grep -q 'TEXT input commit=' "$out/te.log"; then
	echo "te: commit $(grep 'TEXT input commit=' "$out/te.log" | head -1) ok"
else
	echo "te: commit MISSING"
	status=1
fi
if grep -qE 'TEXT input preedit=[^ ]*ああああああ' "$out/te.log"; then
	echo "te: repeat $(grep -oE 'preedit=[^ ]*' "$out/te.log" | grep 'ああああああ' | tail -1) ok"
else
	echo "te: repeat MISSING"
	status=1
fi

guest "grep -E 'ZWL IME|KEI-IME|ZWL ERROR' /tmp/zdesktop.log" > "$out/zdesktop-ime.txt"
errors=$(guest "grep -c ERROR /tmp/zdesktop.log" | tail -1)
[ "${errors:-1}" = 0 ] && echo "zdesktop: no ERROR" || { echo "zdesktop: ERROR lines"; status=1; }
[ $status = 0 ] && echo "ime-p005: PASS" || echo "ime-p005: FAIL"
exit $status
