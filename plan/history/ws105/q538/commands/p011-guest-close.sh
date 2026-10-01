#!/bin/bash
set -euo pipefail
export GUEST_DIR="$PWD/build/ws105-p011/guest" GUEST_RUN="$PWD/build/ws105-p011/run" SSH_PORT=2228
G=plan/tools/keiland-linux/guest.sh
O=$PWD/build/ws105-p011/final-guest
sh "$G" ssh 'pid=$(pgrep -u kei -x wayland); tr "\000" "\n" < /proc/$pid/environ | grep -E "^(XDG_SESSION_ID|XDG_RUNTIME_DIR|XDG_SESSION_TYPE)="; ps -o user,pid,args -p "$pid"' > "$O/gdm-environment.log"
sh "$G" click 23 17
sh "$G" ssh 'sleep .5'
sh "$G" click 712 426
sh "$G" ssh 'sleep 5; test -z "$(pgrep -u kei -x wayland || true)"; journalctl -b --no-pager -t /usr/libexec/gdm-wayland-session | grep -E "ZWL EXIT|KEILAND SESSION logout"; find /tmp /run /run/user/1000 -maxdepth 1 -name "keiland-wpa-*" -o -name "keiland-network-*"' > "$O/gdm-logout.log"
sh "$G" screenshot "$O/gdm-logout.png"
sh "$G" stop > "$O/guest-stop.log"
test ! -e "$GUEST_RUN/overlay.qcow2"
stat -c '%s %Y %n' "$GUEST_DIR/guest.img" > "$O/base-after.txt"
python3 - "$O" <<'PY'
from pathlib import Path
import sys
r=Path(sys.argv[1]);s=(r/'gdm-environment.log').read_text()
assert 'XDG_SESSION_TYPE=wayland' in s and 'XDG_RUNTIME_DIR=/run/user/1000' in s
s=(r/'gdm-logout.log').read_text()
assert 'error=0 cleanup_failed=0 pid=5733' in s,s
assert 'keiland-wpa-' not in s and 'keiland-network-' not in s
print('final guest cleanup: PASS user kei / manual session chooser / 10 pause+resume / Log Out / paths0 / stopped')
PY
