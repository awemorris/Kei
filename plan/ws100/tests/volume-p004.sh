#!/bin/sh
# ws100-p004: the system bar's volume (A1..A6) on the Venus guest of the volume image (build-volume-image.sh), kei's
# session at boot, QEMU's HD Audio recorded to a WAV (volume-guest.sh).  The guest is started by this script:
#  1. A1: the icon is drawn (ZWL VOLUME icon, reachable=1 device=1); icon.png.
#  2. A2: a click on the icon opens the popup; a press on the slider at a quarter, a drag to three quarters and the
#     release set the volume (ZWL VOLUME set ... final=1 at the end), and audiod reports it (audiod-feedback get);
#     the mute row switches mute on and off; a click outside closes it (via=outside); again, Esc closes it (via=key).
#  3. A3: the wheel over the icon: three notches down, two up -> five percent a notch, audiod follows.
#  4. A4: the WAV (the guest stopped) has a feedback sound for the changes, the wheel's in falling then rising loudness,
#     none while muted, and none louder than one sound.
#  5. A5 (BUG-161, ws100-p012): while the session changes the volume, kei's desktop.conf does not change (no write
#     for a change, no ZWL VOLUME kept line); at Log Out the volume is written once (ZWL VOLUME kept ... why=logout
#     write=1, sound.volume in desktop.conf); after audiod set to 100 (as a new boot leaves it) and a login, the
#     session sets audiod back to the kept volume (ZWL VOLUME preferences).
#  6. A6: a guest without HD Audio: the icon says no sound (device=0), the popup says "No sound output"; with audiod
#     stopped, reachable=0 and the popup says so; zdesktop keeps running without ZWL ERROR.
#   plan/ws100/tests/volume-p004.sh IMAGE [OUTDIR]
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
image=${1:?usage: volume-p004.sh IMAGE [OUTDIR]}
out=${2:-build/ws100-shots/p004}
mkdir -p "$out"
GUEST_RUNTIME=$(pwd)/build/ws100-run
export GUEST_RUNTIME
log=/run/user/1000/session.log
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1 </dev/null; }
pointer() { python3 plan/ws035/tests/qmp-pointer.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; sleep 0.8; }
shot() { python3 plan/ws035/tests/zdesktop-check.py "$out/$1" --runtime "$GUEST_RUNTIME" >/dev/null 2>&1; }
status=0

# Records a verdict.
verdict() {
	if [ "$1" = ok ]; then
		echo "$2 ok"
	else
		echo "$2 FAIL"
		status=1
	fi
}

# The number of lines of a guest file matching a pattern.
count() {
	guest "grep -cE '$2' $1" | tail -1
}

# Waits until a guest file has more than N lines matching a pattern (within some seconds); fails the run otherwise.
expect_more() {
	tries=0
	found=0
	while [ $tries -lt "$4" ]; do
		found=$(count "$1" "$2")
		[ "${found:-0}" -gt "$3" ] 2>/dev/null && break
		tries=$((tries + 1))
		sleep 1
	done
	if [ "${found:-0}" -gt "$3" ] 2>/dev/null; then
		echo "log: $2 ok"
		return 0
	fi
	echo "log: $2 MISSING"
	status=1
	return 1
}

# audiod's volume as "left muted".
audiod_volume() {
	guest 'audiod-feedback get' | sed -n 's/.*volume left=\([0-9]*\) right=[0-9]* muted=\([0-9]\).*/\1 \2/p' | tail -1
}

# audiod's mute, waited for (up to about 4 s) until it is $1: the set is sent at once, but the guest may be slow to
# answer (T2-006: zdesktop's "set ... muted=0 via=mute final=1" was logged and the one reading after 0.8 s still said 1).
audiod_muted_wait() {
	tries=0
	while :; do
		set -- "$1" $(audiod_volume)
		[ "${3:-x}" = "$1" ] && break
		tries=$((tries + 1))
		[ $tries -ge 4 ] && break
		sleep 1
	done
	echo "${3:-?}"
}

# The last line of the session log matching a pattern.
last() {
	guest "grep -E '$1' $log | tail -1"
}

# 0. The guest with HD Audio recording, and kei's session.
sh plan/ws100/tests/volume-guest.sh stop >/dev/null 2>&1
VOLUME_AUDIO=duplex timeout 180 sh plan/ws100/tests/volume-guest.sh start "$image" >/dev/null 2>&1
sleep 35
expect_more $log 'ZWL HANDOFF go=1' 0 60

# What kei's desktop.conf holds of the volume as the session begins (A5 compares it later).
conf_start=$(guest 'grep -E "^sound\.(volume|muted)=" /home/kei/.config/keiland/desktop.conf' | tr '\n' ' ')

# 1. A1: the icon, and audiod reached with its device.
expect_more $log 'ZWL VOLUME icon x=' 0 10
expect_more $log 'ZWL VOLUME reachable=1 device=1' 0 10
set -- $(last 'ZWL VOLUME icon x=' | sed -n 's/.* x=\([0-9]*\) y=\([0-9]*\) width=\([0-9]*\) height=\([0-9]*\).*/\1 \2 \3 \4/p')
ix=$((${1:-900} + ${3:-30} / 2)); iy=$((${2:-3} + ${4:-28} / 2))
echo "icon at $ix,$iy"
pointer move 640 600 sleep 500
shot icon.png

# 2. A2: the popup, the slider, mute, closing.
opens=$(count $log 'ZWL VOLUME popup open')
pointer move $((ix - 2)) $iy sleep 200 move $ix $iy sleep 300 down sleep 60 up sleep 800
expect_more $log 'ZWL VOLUME popup open' "$opens" 5
set -- $(last 'ZWL VOLUME popup open' | sed -n 's/.* x=\([0-9]*\) y=\([0-9]*\) width=\([0-9]*\) height=\([0-9]*\) slider=\([0-9]*\) mute=\([0-9]*\).*/\1 \2 \3 \4 \5 \6/p')
px=${1:-900} pw=${3:-260} slider=${5:-100} mute=${6:-134}
# The knob's centre runs from px + 14 + 9 to px + pw - 14 - 9.
track_left=$((px + 23)) track_width=$((pw - 46))
sy=$((slider + 17)) my=$((mute + 17))
shot popup.png
q1=$((track_left + track_width / 4)) q3=$((track_left + track_width * 3 / 4))
pointer move $q1 $sy sleep 300 down sleep 200 move $((q1 + track_width / 6)) $sy sleep 150 move $((q1 + track_width / 3)) $sy sleep 150 move $q3 $sy sleep 300 up sleep 800
expect_more $log 'ZWL VOLUME set value=7[4-6] muted=0 via=slider final=1' 0 5
shot slider.png
sleep 1
set -- $(audiod_volume)
[ "${1:-0}" -ge 74 ] && [ "${1:-0}" -le 76 ] && [ "${2:-1}" = 0 ] && verdict ok "slider: audiod at ${1:-?}" || verdict no "slider: audiod at ${1:-?} muted ${2:-?}"
pointer move $((px + pw - 40)) $my sleep 300 down sleep 60 up sleep 800
muted=$(audiod_muted_wait 1)
[ "$muted" = 1 ] && verdict ok "mute on: audiod muted" || verdict no "mute on: audiod muted ($muted)"
shot muted.png
pointer move $((px + pw - 40)) $my sleep 300 down sleep 60 up sleep 800
muted=$(audiod_muted_wait 0)
[ "$muted" = 0 ] && verdict ok "mute off: audiod unmuted" || verdict no "mute off: audiod unmuted ($muted)"
guest "grep -E 'ZWL VOLUME (set .*via=mute|send errno)' $log" > "$out/mute.log"
closes=$(count $log 'ZWL VOLUME popup close via=outside')
pointer move 400 600 sleep 300 down sleep 60 up sleep 800
expect_more $log 'ZWL VOLUME popup close via=outside' "$closes" 5
pointer move $ix $iy sleep 300 down sleep 60 up sleep 800
keys '<esc>'
expect_more $log 'ZWL VOLUME popup close via=key' 0 5

# 3. A3: the wheel over the icon, three notches down and two up.
before=$(audiod_volume | cut -d' ' -f1)
pointer move $ix $iy sleep 400 wheel-down sleep 700 wheel-down sleep 700 wheel-down sleep 700 wheel-up sleep 700 wheel-up sleep 1000
after=$(audiod_volume | cut -d' ' -f1)
echo "wheel: $before -> $after"
[ "${after:-0}" -eq $((${before:-0} - 5)) ] 2>/dev/null && verdict ok "wheel: five percent a notch (-15 +10)" || verdict no "wheel: five percent a notch ($before -> $after)"
shot wheel.png

# The first session's volume lines, before Log Out replaces its log.
guest "grep -E 'ZWL (VOLUME|ERROR)' $log" > "$out/session-first.log"
grep -q 'ZWL ERROR' "$out/session-first.log" && { echo "ZWL ERROR in the first session"; status=1; }
n=$(grep -c 'via=wheel final=1' "$out/session-first.log")
[ "$n" -eq 5 ] && verdict ok "wheel: five changes logged" || verdict no "wheel: five changes logged ($n)"

# 5. A5: nothing written during the session; kept once at Log Out, then a new session takes it back.
sleep 2
during=$(guest 'grep -E "^sound\.(volume|muted)=" /home/kei/.config/keiland/desktop.conf' | tr '\n' ' ')
echo "during the session: $during (the session began with: $conf_start)"
[ "$during" = "$conf_start" ] && verdict ok "no write while the volume changes" || verdict no "no write while the volume changes"
grep -q 'ZWL VOLUME kept' "$out/session-first.log" && verdict no "no kept line during the session" || verdict ok "no kept line during the session"
pointer move 23 17 sleep 300 down sleep 60 up sleep 1500
set -- $(last 'ZWL HOME icon name="Log Out"' | sed -n 's/.* x=\([0-9]*\) y=\([0-9]*\).*/\1 \2/p')
pointer move "${1:-0}" "${2:-0}" sleep 400 down sleep 60 up
expect_more /var/log/sessiond.log 'SESSIOND GREETER adopt pid=' 0 20
sleep 2
kept=$(guest 'grep -E "^sound\.(volume|muted)=" /home/kei/.config/keiland/desktop.conf' | tr '\n' ' ')
echo "kept at Log Out: $kept"
echo "$kept" | grep -q "sound.volume=${after:-x}" && verdict ok "kept in desktop.conf at Log Out" || verdict no "kept in desktop.conf at Log Out"
guest 'audiod-feedback volume 100' >/dev/null
greeter_volume=$(audiod_volume | cut -d' ' -f1)
keys 'kei\n'
expect_more /var/log/sessiond.log 'SESSIOND HANDOFF go written=3' 1 30
sleep 6
restored=$(audiod_volume | cut -d' ' -f1)
echo "a new session: audiod $greeter_volume at the greeter -> $restored"
[ "${restored:-0}" = "${after:-x}" ] && verdict ok "a new session applies the kept volume" || verdict no "a new session applies the kept volume"

# 4. A4: the WAV, once the guest is stopped.
guest "grep -E 'ZWL (VOLUME|ERROR)' $log" > "$out/session-volume.log"
grep -q 'ZWL ERROR' "$out/session-volume.log" && { echo "ZWL ERROR in the session"; status=1; }
sh plan/ws100/tests/volume-guest.sh stop >/dev/null 2>&1
sleep 2
cp "$GUEST_RUNTIME/out.wav" "$out/p004.wav" 2>/dev/null
python3 plan/ws035/tests/hda-wav-check.py windows "$out/p004.wav" > "$out/windows.txt" 2>&1
sounds=$(python3 - "$out/windows.txt" <<'EOF'
import re, sys
line = [l for l in open(sys.argv[1]).read().splitlines() if l.startswith("windows:")]
values = [int(v) for v in re.findall(r"-?\d+", line[0])] if line else []
runs, current = [], None
for index, value in enumerate(values):
	if value > 0:
		if current is None:
			current = [index, value]
			runs.append(current)
		else:
			current[1] = max(current[1], value)
	else:
		current = None
print(" ".join("%d:%d" % (r[0], r[1]) for r in runs))
EOF
)
echo "sounds (quarter second: peak): $sounds"
n=$(echo "$sounds" | wc -w)
[ "$n" -ge 6 ] && verdict ok "feedback sounds recorded ($n)" || verdict no "feedback sounds recorded ($n)"
most=$(echo "$sounds" | tr ' ' '\n' | cut -d: -f2 | sort -n | tail -1)
[ "${most:-0}" -le 8124 ] && verdict ok "no sound louder than one feedback sound ($most)" || verdict no "a sound louder than one feedback sound ($most)"

# 6. A6: no HD Audio, then audiod stopped.
VOLUME_AUDIO=none timeout 180 sh plan/ws100/tests/volume-guest.sh start "$image" >/dev/null 2>&1
sleep 35
expect_more $log 'ZWL VOLUME reachable=1 device=0' 0 30
pointer move 640 600 sleep 500
shot no-device-icon.png
set -- $(last 'ZWL VOLUME icon x=' | sed -n 's/.* x=\([0-9]*\) y=\([0-9]*\) width=\([0-9]*\) height=\([0-9]*\).*/\1 \2 \3 \4/p')
ix=$((${1:-900} + ${3:-30} / 2)); iy=$((${2:-3} + ${4:-28} / 2))
pointer move $ix $iy sleep 300 down sleep 60 up sleep 800
expect_more $log 'ZWL VOLUME popup open .* sound=0' 0 5
shot no-device-popup.png
keys '<esc>'
guest 'service stop audiod >/dev/null 2>&1; echo stopped' >/dev/null
expect_more $log 'ZWL VOLUME reachable=0' 0 10
pointer move $ix $iy sleep 300 down sleep 60 up sleep 800
shot no-audiod-popup.png
keys '<esc>'
alive=$(guest "ps -A -o args | grep -c '[w]ayland'" | tail -1)
[ "${alive:-0}" -ge 1 ] && verdict ok "zdesktop runs without audiod" || verdict no "zdesktop runs without audiod"
errors=$(count $log 'ZWL ERROR')
[ "${errors:-1}" = 0 ] && verdict ok "no ZWL ERROR" || verdict no "ZWL ERROR ($errors)"
guest "grep -E 'ZWL VOLUME' $log" > "$out/no-device-volume.log"
sh plan/ws100/tests/volume-guest.sh stop >/dev/null 2>&1
[ $status -eq 0 ] && echo "volume-p004: PASS" || echo "volume-p004: FAIL"
exit $status
