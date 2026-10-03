#!/bin/sh
# ws005-p024: checks sessiond's notices of the Wi-Fi stores on the Venus desktop guest (no radio).
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
#
# The image is plan/ws005/phase024/config-venus.mk (the real networkd, sessiond with the greeter, kei logged in
# by itself at boot).  networkd's status line ("wifi ... owner= store= sessions=", read as root over SSH) must show
# kei's session after the automatic login, none after the session ends, and kei's again after a login at the
# greeter; sessiond's log must have its notices.  Screens: OUT/*.png (VNC of the Venus output).
#   sh plan/ws005/phase024/venus-session-check.sh IMAGE [OUT]
set -u
cd "$(dirname -- "$0")/../../.."
. plan/tools/guest/venus-hostmem.sh
image=${1:?image}
out=${2:-build/p1-venus-shots}
mkdir -p "$out"
GUEST_RUNTIME="$(pwd)/build/p1-venus-run"
export GUEST_RUNTIME
export LIBGL_ALWAYS_SOFTWARE=1 MESA_LOADER_DRIVER_OVERRIDE=zink
export VK_DRIVER_FILES="${VK_DRIVER_FILES:-/usr/share/vulkan/icd.d/lvp_icd.json}"
renderer=${VENUS_RENDERER:-/home/awe/zedBSD-claude1/build/ws035-sq-venus/install}
export RENDER_SERVER_EXEC_PATH="$renderer/libexec/virgl_render_server"
export LD_LIBRARY_PATH="$renderer/lib/x86_64-linux-gnu${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
guest() { timeout 90 python3 plan/ws005/phase024/guest-ports.py run "$1" 2>&1 </dev/null; }
shot() { timeout 60 python3 plan/ws035/tests/zdesktop-shot.py "$out/$1.png" --runtime "$GUEST_RUNTIME" >/dev/null 2>&1; }
keys() { python3 plan/ws035/tests/qmp-keys.py "$GUEST_RUNTIME/qmp.sock" "$@"; }
state() { echo "== $1"; guest 'net wifi list | grep "^wifi "; grep NETWORK /var/log/sessiond.log'; }

timeout 300 python3 plan/ws005/phase024/guest-ports.py start "$image" \
	--qemu-extra "-object memory-backend-memfd,id=mem,size=8G,share=on -machine memory-backend=mem -device virtio-gpu-gl-pci,id=venus,venus=on,blob=on,hostmem=$VENUS_HOSTMEM,max_outputs=1 -display egl-headless,rendernode=/dev/dri/renderD128 -vnc unix:$GUEST_RUNTIME/vnc.sock,display=venus -device usb-tablet,bus=xhci.0,port=4" \
	|| { echo "venus-session-check: the guest did not start"; exit 1; }
trap 'timeout 60 python3 plan/ws005/phase024/guest-ports.py stop >/dev/null 2>&1' EXIT
timeout 400 python3 plan/ws005/phase024/guest-ports.py wait || { echo "venus-session-check: no SSH"; exit 1; }
sleep 30
shot 1-autologin
state "after the automatic login of kei"

# Log Out from App Home (the Kei button, then the Log Out tile at 1280x800); sessiond notices after its sweep.
python3 plan/ws035/tests/qmp-pointer.py --width 1280 --height 800 "$GUEST_RUNTIME/qmp.sock" \
	move 22 16 sleep 100 down up sleep 2000 move 280 518 sleep 150 down up sleep 3000
sleep 25
shot 2-greeter
state "after Log Out"

# kei logs in at the greeter (password kei).
keys 'kei\n'
sleep 20
shot 3-login
state "after kei logged in at the greeter"
