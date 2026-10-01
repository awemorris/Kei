#!/bin/bash
set -euo pipefail
W=ws105-p011
O=$PWD/build/$W
python3 - <<'PY'
from pathlib import Path
s=Path('build/ws105-p011/criteria/results.txt').read_text().splitlines()
assert len(s)==13,s
fails=[(l.split()[0],l.split()[1]) for l in s if ' FAIL ' in l]
assert fails==[('C2','c2-geometry'),('C9','p076')],fails
print('criteria: 11/13 PASS; original C2/p076 resize FAIL retained, BUG-125 user non-blocking decision applied')
PY
sh plan/tools/gpu-boundary/build-forge-image.sh "build/$W-forge" > "$O/forge-build.log" 2>&1
export GUEST_RUNTIME="$PWD/build/$W-forge-run"
trap 'sh plan/ws035/tests/zdesktop-guest.sh stop > "$O/forge-stop.log" 2>&1 || true' EXIT
sh plan/ws035/tests/zdesktop-guest.sh start "build/$W-forge/hdd-image.img" > "$O/forge-start.log" 2>&1
sh plan/ws035/tests/zdesktop-guest.sh wait --timeout 240 > "$O/forge-wait.log" 2>&1
sh plan/tools/gpu-boundary/forge-guest.sh "$O/forge" > "$O/forge-test.log" 2>&1
sh plan/tools/gpu-boundary/fence-guest.sh "$O/fence" > "$O/fence-test.log" 2>&1
sh plan/ws035/tests/zdesktop-guest.sh stop > "$O/forge-stop.log" 2>&1
trap - EXIT
bash build/ws105-control/p011-target-extra.sh
