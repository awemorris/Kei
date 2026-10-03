#!/bin/sh
# ws128-p011 (BUG-155): Terminal takes the input method's text (text-input-v3 through libkeiui's window) on the Venus
# guest of the Settings image with the input method (plan/ws089/tests/config-amd64-settings-ime.mk, which has the
# terminal, keiland-ime and its Japanese dictionary).  zdesktop --glass at 1280x800 starts /usr/libexec/keiland-ime;
# the terminal runs the shell.  Checks:
#  1. The input method follows the terminal (ZWL IME activate client=).  Direct input: "echo a" typed as keys; no
#     ZTERM IME commit.
#  2. Japanese (Alt+Space): "kanji" is a preedit drawn at the cursor (ZTERM IME preedit=かんじ, preedit.png); Space
#     converts it (漢字, converted.png), Enter commits it: the terminal sends it to the shell (ZTERM IME commit
#     bytes=6 text=漢字).
#  3. "kana" and Enter without converting commits かな (committed.png: the line reads "echo a漢字かな").
#  4. Alt+Space back to direct input, " > /root/ime.txt" and Enter: the shell wrote "a漢字かな" (read over SSH).
#  No ZWL ERROR in zdesktop's log.
#
#   SETTINGS_CONFIG=plan/ws089/tests/config-amd64-settings-ime.mk plan/ws089/tests/build-settings-image.sh BUILD
#   plan/ws128/tests/terminal-p011-ime.sh BUILD/hdd-image.img [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
image=${1:?usage: terminal-p011-ime.sh IMAGE [OUTDIR]}
out=${2:-build/ws128-shots/p011}
mkdir -p "$out"
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws128/ime-run}"
export GUEST_RUNTIME
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1 </dev/null; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; sleep 0.7; }
shot() { python3 plan/ws035/tests/zdesktop-check.py "$out/$1" --runtime "$GUEST_RUNTIME" >/dev/null 2>&1; echo "shot: $out/$1"; }
status=0

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

# 0. The guest, zdesktop with the input method, and a terminal.
mkdir -p "$(dirname "$GUEST_RUNTIME")"
sh plan/ws035/tests/zdesktop-guest.sh stop >/dev/null 2>&1
timeout 180 sh plan/ws035/tests/zdesktop-guest.sh start "$image" >/dev/null 2>&1
sleep 25
guest 'service stop greeter >/dev/null 2>&1; for p in $(ps -A -o pid,args | grep -E "[w]ayland( |$)|[k]eiland-ime|[t]erminal" | awk "{print \$1}"); do kill $p; done; sleep 1; echo stopped' >/dev/null
guest 'export XDG_RUNTIME_DIR=/tmp HOME=/root; rm -f /tmp/wayland-0 /root/ime.txt /root/.config/kei/ime/ja-user.dict; picture=; [ -f /usr/share/keiland/wallpaper.ppm ] && picture=--wallpaper=/usr/share/keiland/wallpaper.ppm
/bin/wayland --timeout=900 --width=1280 --height=800 --glass $picture > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4; echo started' >/dev/null
expect_log /tmp/zdesktop.log 'KEI-IME READY'
guest 'export XDG_RUNTIME_DIR=/tmp HOME=/root; cd /root; /bin/terminal --token=ime --timeout-s=600 > /tmp/t.log 2>&1 </dev/null & sleep 6; echo started' >/dev/null
expect_log /tmp/t.log 'ZTERM START run=ime'
expect_log /tmp/zdesktop.log 'ZWL IME activate client='

# 1. Direct input: keys, no commit.
keys 'echo a'
sleep 1
n=$(count /tmp/t.log 'ZTERM IME commit')
[ "$n" = 0 ] && echo "direct: keys typed, no text input commit ok" || { echo "direct: text input commit FAIL"; status=1; }

# 2. Japanese: a preedit at the cursor, a conversion, a commit.
keys '<alt-spc>'
expect_log /tmp/zdesktop.log 'ZWL IME language=ja'
keys 'kanji'
expect_log /tmp/t.log 'ZTERM IME preedit=かんじ'
sleep 1
shot preedit.png
keys ' '
expect_log /tmp/t.log 'ZTERM IME preedit=漢字'
sleep 1
shot converted.png
keys '\n'
expect_log /tmp/t.log 'ZTERM IME commit bytes=6 text=漢字'

# 3. Kana, committed without converting.
keys 'kana' '\n'
expect_log /tmp/t.log 'ZTERM IME commit bytes=6 text=かな'
sleep 1
shot committed.png

# 4. Direct input again; the shell writes the line to a file.
keys '<alt-spc>'
expect_log /tmp/zdesktop.log 'ZWL IME language=direct'
keys ' > /root/ime.txt' '\n'
sleep 2
guest 'cat /root/ime.txt' | tail -1 > "$out/ime.txt"
text=$(cat "$out/ime.txt")
echo "file: $text"
[ "$text" = "a漢字かな" ] && echo "file: a漢字かな ok" || { echo "file: a漢字かな FAIL"; status=1; }
shot done.png
guest 'grep -E "ZTERM (IME|START)" /tmp/t.log' > "$out/t-ime.log"
guest 'grep -E "ZWL (IME|ERROR)|KEI-IME" /tmp/zdesktop.log' > "$out/zdesktop-ime.log"
errors=$(count /tmp/zdesktop.log 'ZWL ERROR')
[ "$errors" = 0 ] && echo "no ZWL ERROR ok" || { echo "ZWL ERROR FAIL"; status=1; }
sh plan/ws035/tests/zdesktop-guest.sh stop >/dev/null 2>&1
[ $status -eq 0 ] && echo "terminal-p011-ime: PASS" || echo "terminal-p011-ime: FAIL"
exit $status
