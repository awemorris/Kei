#!/bin/bash
set -euo pipefail
W=ws105-p011
O=$PWD/build/$W
export GUEST_RUNTIME="$PWD/build/$W-glass-run"
trap 'sh plan/ws035/tests/zdesktop-guest.sh stop > "$O/extra-stop.log" 2>&1 || true' EXIT
sh plan/ws035/tests/zdesktop-guest.sh start "build/$W-criteria/hdd-image.img" > "$O/glass-start.log" 2>&1
sh plan/ws035/tests/zdesktop-guest.sh wait --timeout 240 > "$O/glass-wait.log" 2>&1
sh plan/ws035/tests/zdesktop-p059.sh "$O/p059" > "$O/glass.log" 2>&1
sh plan/ws035/tests/zdesktop-guest.sh stop > "$O/glass-stop.log" 2>&1
trap - EXIT
sh plan/ws089/tests/host-build.sh > "$O/settings-host.log" 2>&1
sh plan/ws089/tests/build-settings-image.sh "build/$W-settings" > "$O/settings-build.log" 2>&1
export GUEST_RUNTIME="$PWD/build/$W-settings-run"
trap 'sh plan/ws089/tests/settings-guest.sh stop > "$O/settings-stop.log" 2>&1 || true' EXIT
sh plan/ws089/tests/settings-guest.sh start "build/$W-settings/hdd-image.img" > "$O/settings-start.log" 2>&1
sh plan/ws089/tests/settings-regress.sh "$O/settings-regress" > "$O/settings-regress.log" 2>&1
sh plan/ws089/tests/settings-guest.sh stop > "$O/settings-stop.log" 2>&1
trap - EXIT
sh plan/ws100/tests/host-audio.sh > "$O/audio-host.log" 2>&1
test -f build/ws100-tests/audiod-feedback
sh plan/ws100/tests/build-volume-image.sh "build/$W-volume" > "$O/volume-build.log" 2>&1
sh plan/ws100/tests/volume-p004.sh "build/$W-volume/hdd-image.img" "$O/volume-p004" > "$O/volume-p004.log" 2>&1
sh plan/ws100/tests/volume-p005.sh "build/$W-volume/hdd-image.img" "$O/volume-p005" > "$O/volume-p005.log" 2>&1
python3 - "$O" <<'PY'
from pathlib import Path
import sys
r=Path(sys.argv[1])
for log,expected in [('glass','p059: PASS'),('settings-regress','settings-regress: PASS'),('volume-p004','volume-p004: PASS'),('volume-p005','volume-p005: PASS')]:assert expected in (r/(log+'.log')).read_text(),log
print('target-extra: PASS glass / Settings / host audio / volume p004+p005')
PY
