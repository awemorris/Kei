#!/bin/bash
set -euo pipefail
export GUEST_DIR="$PWD/build/ws105-p011/guest" GUEST_RUN="$PWD/build/ws105-p011/run" SSH_PORT=2228
G=plan/tools/keiland-linux/guest.sh
O=$PWD/build/ws105-p011/final-guest
mkdir -p "$O"
sh "$G" ssh 'systemctl stop gdm; pkill -TERM -x wayland || true; chvt 1'
sh plan/tools/keiland-linux/install-guest.sh
sh "$G" ssh '/opt/keiland/bin/seat-fd' > "$O/seat-fd.log"
for mode in acquire direct; do
 option=; if test "$mode" = acquire; then option=--acquire; fi
 sh "$G" ssh "chvt 1; rm -f /tmp/display.out; openvt -c 9 -s -- sh -c 'timeout 60 /opt/keiland/bin/display-probe $option > /tmp/display.out 2>&1'"
 for color in ff0000 00ff00 0000ff; do
  sh "$G" ssh "for i in \$(seq 70); do grep -q 'color=$color' /tmp/display.out && exit 0; sleep .1; done; cat /tmp/display.out; exit 1"
  sh "$G" screenshot "$O/$mode-$color.png"
  python3 plan/tools/keiland-linux/png-probe.py "$O/$mode-$color.png" 10 10 640 400 1270 790 10 790 > "$O/$mode-$color-pixels.log"
 done
 sh "$G" ssh 'for i in $(seq 70); do grep -q "display-probe: PASS" /tmp/display.out && exit 0; sleep .1; done; cat /tmp/display.out; exit 1'
 sh "$G" get /tmp/display.out "$O/display-$mode.log"
 sh "$G" ssh 'chvt 1'
 sh "$G" screenshot "$O/$mode-console.png"
done
sh "$G" ssh 'openvt -c 9 -s -- sh -c "timeout 30 /opt/keiland/bin/vkdemo --time-ms=1000 --hold=5 > /tmp/p011-vkdemo.log 2>&1"'
sh "$G" ssh 'sleep 3'
sh "$G" screenshot "$O/vkdemo.png"
sh "$G" ssh 'sleep 5; chvt 1'
sh "$G" get /tmp/p011-vkdemo.log "$O/vkdemo.log"
sh "$G" ssh 'openvt -c 10 -s -- sh -c "KEILAND_SEAT=direct /opt/keiland/bin/wayland --session --glass --socket=/run/keiland-0 --wallpaper=/opt/keiland/share/keiland/wallpaper.ppm > /tmp/wayland.log 2>&1"'
sh "$G" ssh 'for i in $(seq 100); do grep -q "ZWL READY" /tmp/wayland.log && { sleep 1; exit 0; }; sleep .1; done; exit 1'
sh "$G" screenshot "$O/root-desktop.png"
sh "$G" ssh 'XDG_RUNTIME_DIR=/run WAYLAND_DISPLAY=keiland-0 nohup /opt/keiland/bin/wlshm --size=480x320 --frames=6000 > /tmp/wlshm.log 2>&1 &'
sh "$G" ssh 'for i in $(seq 100); do grep -q "WLSHM FRAME" /tmp/wlshm.log && { sleep 1; exit 0; }; sleep .1; done; exit 1'
sh "$G" screenshot "$O/wlshm.png"
sh "$G" move 200 200
sh "$G" screenshot "$O/pointer1.png"
sh "$G" move 900 600
sh "$G" screenshot "$O/pointer2.png"
sh "$G" ssh 'pkill -TERM -x wlshm; sleep 1; XDG_RUNTIME_DIR=/run WAYLAND_DISPLAY=keiland-0 timeout 180 /opt/keiland/bin/wltest --frames=600 > /tmp/p011-wltest.log 2>&1' > "$O/wltest-command.log"
sh "$G" get /tmp/p011-wltest.log "$O/wltest.log"
sh "$G" ssh 'XDG_RUNTIME_DIR=/run WAYLAND_DISPLAY=keiland-0 KEILAND_DRM_DEVICE=none timeout 40 /opt/keiland/bin/dmabuf-forge > /tmp/p011-forge.log 2>&1'
sh "$G" get /tmp/p011-forge.log "$O/linux-forge.log"
sh "$G" ssh 'cat /tmp/wayland.log' > "$O/wayland-before-apps.log"
python3 - "$O" <<'PY'
from pathlib import Path
import json,sys
r=Path(sys.argv[1])
for mode in ['acquire','direct']:
 for c in ['ff0000','00ff00','0000ff']:
  data=(r/f'{mode}-{c}-pixels.log').read_text().splitlines()
  assert len(data)==4 and all(line.split()[2]=='#'+c for line in data),data
assert 'dmabuf-forge: PASS' in (r/'linux-forge.log').read_text()
print('guest-basic: PASS seat-fd / KMS 6x4 pixels / oldSwapchain / wlshm / pointer / wltest600 / forged-bounds')
PY
