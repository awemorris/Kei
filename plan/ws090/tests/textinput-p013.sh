#!/bin/sh
# ws090-p013: Text Editor takes the text of zdesktop's text input (libkeiui's kui_window, text-input-unstable-v3) on the
# Venus guest of the text input image (config-amd64-textinput.mk: WS095's input method image with Text Editor).
# zdesktop --glass at 1280x800 starts /usr/libexec/keiland-ime; Text Editor opens /root/ti.txt.  Checks:
#  1. Direct input: "ab" typed as keys (Text Editor's own typing); no text input commit.
#  2. Japanese (Alt+Space): "kanji" is a preedit shown at the caret (TEXT input preedit=かんじ, preedit.png); Space
#     converts it (漢字), Enter commits it: Text Editor inserts it (TEXT commit ... text=漢字).
#  3. "kana" and Enter without converting commits the kana かな.
#  4. Alt+Space back to direct input, "c" typed, Ctrl+S saves: the file is "ab漢字かなc" (read over SSH).
#  5. The input method's activation follows Text Editor (ZWL IME activate client=).
#   plan/ws090/tests/textinput-p013.sh [IMAGE] [OUTDIR]   (IMAGE default: build it into build/amd64 and copy it)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
. plan/tools/guest/jobs.sh  # ZEDBSD_JOBS, the parallel jobs (default 16)
image=${1:-}
out=${2:-build/ws090-shots/p013}
mkdir -p "$out"
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws090/ti-run}"
export GUEST_RUNTIME
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1 </dev/null; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; sleep 0.7; }
shot() { python3 plan/ws035/tests/zdesktop-check.py "$out/$1" --runtime "$GUEST_RUNTIME" >/dev/null 2>&1; echo "shot: $out/$1"; }
status=0

# The image: built with the text input config when none is given.
if [ -z "$image" ]; then
	extra=$(python3 plan/tools/guest/guest.py extra-files | sed -n "s/^ZEDBSD_TEST_EXTRA_FILES='\(.*\)'$/\1/p")
	timeout 3000 make -j"$ZEDBSD_JOBS" ZEDBSD_CONFIG=plan/ws090/tests/config-amd64-textinput.mk BUILD=build/amd64 \
	    "ZEDBSD_TEST_EXTRA_FILES=$extra" disk-image > "$out/image.log" 2>&1 || { echo "image: FAIL"; exit 1; }
	mkdir -p build/ws090
	cp --reflink=auto build/amd64/hdd-image.img build/ws090/textinput.img
	image=build/ws090/textinput.img
fi

# Counts the lines of a guest file that match a pattern (matched on the host, whose grep knows UTF-8).
count() {
	guest "cat $1" > "$out/.count" 2>/dev/null
	n=$(LC_ALL=C.UTF-8 grep -cE "$2" "$out/.count")
	case "$n" in ''|*[!0-9]*) n=0;; esac
	echo "$n"
}

# Fails the run unless a log has a line matching a pattern (within a few seconds).
expect_log() {
	tries=0
	found=0
	while [ $tries -lt 10 ]; do
		found=$(count "$1" "$2")
		[ "$found" -ge 1 ] && break
		tries=$((tries + 1))
		sleep 1
	done
	if [ "$found" -ge 1 ]; then
		echo "log: $2 ok"
	else
		echo "log: $2 MISSING"
		status=1
	fi
}

# 0. The guest, zdesktop with the input method, and Text Editor on an empty file.
sh plan/ws035/tests/zdesktop-guest.sh stop >/dev/null 2>&1
timeout 180 sh plan/ws035/tests/zdesktop-guest.sh start "$image" >/dev/null 2>&1
sleep 25
guest 'service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[k]eiland-ime" | awk "{print \$1}"); do kill $p; done; sleep 1; echo stopped' >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp HOME=/root; rm -f /tmp/wayland-0 /root/.config/kei/ime/ja-user.dict; picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=900 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4; echo started' >/dev/null
expect_log /tmp/zdesktop.log 'KEI-IME READY'
guest 'export XDG_RUNTIME_DIR=/tmp HOME=/root; : > /root/ti.txt; /bin/textedit /root/ti.txt > /tmp/te.log 2>&1 </dev/null & sleep 5; echo started' >/dev/null
expect_log /tmp/te.log 'TEXTEDIT READY'
expect_log /tmp/zdesktop.log 'ZWL IME activate client='

# 1. Direct input.
keys 'ab'
sleep 1
n=$(count /tmp/te.log 'TEXT commit')
[ "$n" = 0 ] && echo "direct: keys typed, no text input commit ok" || { echo "direct: text input commit FAIL"; status=1; }

# 2. Japanese: a preedit at the caret, a conversion, a commit.
keys '<alt-spc>'
expect_log /tmp/zdesktop.log 'ZWL IME language=ja'
keys 'kanji'
expect_log /tmp/te.log 'TEXT input preedit=かんじ'
sleep 1
shot preedit.png
keys ' '
expect_log /tmp/te.log 'TEXT input preedit=漢字'
shot converted.png
keys '\n'
expect_log /tmp/te.log 'TEXT commit bytes=6 text=漢字'

# 3. Kana, committed without converting.
keys 'kana' '\n'
expect_log /tmp/te.log 'TEXT commit bytes=6 text=かな'
sleep 1
shot committed.png

# 4. Direct input again, then save.
keys '<alt-spc>'
expect_log /tmp/zdesktop.log 'ZWL IME language=direct'
keys 'c' '<ctrl-s>'
sleep 2
guest 'cat /root/ti.txt' | tail -1 > "$out/ti.txt"
text=$(cat "$out/ti.txt")
echo "file: $text"
[ "$text" = "ab漢字かなc" ] && echo "file: ab漢字かなc ok" || { echo "file: ab漢字かなc FAIL"; status=1; }
guest 'grep -E "TEXT" /tmp/te.log' > "$out/te-text.log"
guest 'grep -E "ZWL (IME|ERROR)|KEI-IME" /tmp/zdesktop.log' > "$out/zdesktop-ime.log"
errors=$(count /tmp/zdesktop.log 'ZWL ERROR')
[ "$errors" = 0 ] && echo "no ZWL ERROR ok" || { echo "ZWL ERROR FAIL"; status=1; }
sh plan/ws035/tests/zdesktop-guest.sh stop >/dev/null 2>&1
[ $status -eq 0 ] && echo "textinput-p013: PASS" || echo "textinput-p013: FAIL"
exit $status
