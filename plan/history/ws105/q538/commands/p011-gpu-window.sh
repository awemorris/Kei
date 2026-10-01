#!/bin/bash
set -euo pipefail
export GUEST_DIR="$PWD/build/ws105-p011/guest" GUEST_RUN="$PWD/build/ws105-p011/run" SSH_PORT=2228
G=plan/tools/keiland-linux/guest.sh
O=$PWD/build/ws105-p011/final-gpu-window
mkdir -p "$O"
trap 'sh "$G" stop > "$O/emergency-stop.log" 2>&1 || true' EXIT
sh "$G" start > "$O/start.log"
sh "$G" ssh 'systemctl stop gdm; chvt 1'
sh plan/tools/keiland-linux/install-guest.sh
sh "$G" ssh 'openvt -c 8 -s -- sh -c "KEILAND_SEAT=direct /opt/keiland/bin/wayland --session --glass --log-frames --socket=/run/keiland-0 --wallpaper=/opt/keiland/share/keiland/wallpaper.ppm > /tmp/wayland.log 2>&1"'
sh "$G" ssh 'for i in $(seq 100); do grep -q "ZWL READY" /tmp/wayland.log && exit 0; sleep .1; done; exit 1'
sh "$G" ssh 'XDG_RUNTIME_DIR=/run WAYLAND_DISPLAY=keiland-0 nohup /opt/keiland/bin/wltest --windowed --size=640x420 --frames=600 --delay-ms=5 > /tmp/wltest.log 2>&1 &'
sh "$G" ssh 'for i in $(seq 60); do grep -q "WLTEST FRAME" /tmp/wltest.log && { sleep 1; exit 0; }; sleep .1; done; exit 1'
sh "$G" screenshot "$O/wltest.png"
sh "$G" ssh 'for i in $(seq 150); do grep -q "WLTEST DONE.*frames=600" /tmp/wltest.log && exit 0; sleep 1; done; exit 1'
sh "$G" get /tmp/wltest.log "$O/wltest.log"
sh "$G" ssh 'pid=$(pgrep -x wayland); sleep 1; ls /proc/$pid/fd | wc -l' > "$O/fd-before.log"
sh "$G" ssh 'set -e; for i in 1 2 3 4 5; do XDG_RUNTIME_DIR=/run WAYLAND_DISPLAY=keiland-0 timeout 30 /opt/keiland/bin/wltest --windowed --size=640x420 --frames=20; done; sleep 1; pid=$(pgrep -x wayland); ls /proc/$pid/fd | wc -l' > "$O/fd-cycles.log"
sh "$G" ssh 'pid=$(pgrep -x wayland); ls /proc/$pid/fd | wc -l' > "$O/fd-after.log"
sh "$G" ssh 'XDG_RUNTIME_DIR=/run WAYLAND_DISPLAY=keiland-0 KEILAND_DRM_DEVICE=none timeout 40 /opt/keiland/bin/dmabuf-forge; pgrep -x wayland' > "$O/forge.log"
sh "$G" ssh 'pkill -TERM -x wayland; sleep 2; cat /tmp/wayland.log' > "$O/wayland.log"
sh "$G" screenshot "$O/console.png"
sh "$G" stop > "$O/stop.log"
trap - EXIT
python3 - "$O" <<'PY'
from pathlib import Path
import re,sys
r=Path(sys.argv[1]);s=(r/'wayland.log').read_text();frames=re.findall(r'^ZWL ACQUIRE_FENCE client=([0-9]+) .*$',s,re.M);imports=re.findall(r'^ZWL IMPORT client=([0-9]+) ',s,re.M)
frames=[x for x in frames if int(x)>=3];imports=[x for x in imports if int(x)>=3]
assert len(frames)==700 and len(imports)==18,(len(frames),len(imports))
assert 'frames=600' in (r/'wltest.log').read_text()
a=int((r/'fd-before.log').read_text());b=int((r/'fd-after.log').read_text());assert abs(a-b)<=2,(a,b)
assert 'dmabuf-forge: PASS' in (r/'forge.log').read_text()
assert 'error=0 cleanup_failed=0' in s
print('final GPU: PASS wltest600 +5x20 / imports18 / acquire-fences700 / fd%d->%d / forge refusal / SIGTERM0 / stopped'%(a,b))
PY
