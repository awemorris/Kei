#!/bin/sh
# ws127-p002: starts the Files Venus guest (as plan/tools/files/files-guest.sh start does) with its forwarded ports in
# P1's range 10100-10199 (plan/ws005/phase024/guest-ports.py) and the runtime build/p1-files-run.
#   sh plan/ws127/tests/files-guest-p1.sh start IMAGE | stop
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../.."
. plan/tools/guest/venus-hostmem.sh
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/p1-files-run}"
export GUEST_RUNTIME LIBGL_ALWAYS_SOFTWARE=1 MESA_LOADER_DRIVER_OVERRIDE=zink
export VK_DRIVER_FILES="${VK_DRIVER_FILES:-/usr/share/vulkan/icd.d/lvp_icd.json}"
renderer=${VENUS_RENDERER:-$(pwd)/build/ws035-sq-venus/install}
export RENDER_SERVER_EXEC_PATH="$renderer/libexec/virgl_render_server"
export LD_LIBRARY_PATH="$renderer/lib/x86_64-linux-gnu${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
if [ "${1:-start}" = stop ]; then
	exec python3 plan/ws005/phase024/guest-ports.py stop
fi
python3 plan/ws005/phase024/guest-ports.py stop >/dev/null 2>&1 || true
python3 plan/ws005/phase024/guest-ports.py start "${2:?image}" \
	--qemu-extra "-object memory-backend-memfd,id=mem,size=8G,share=on -machine memory-backend=mem -device virtio-gpu-gl-pci,id=venus,venus=on,blob=on,hostmem=$VENUS_HOSTMEM,max_outputs=1 -display egl-headless,rendernode=/dev/dri/renderD128 -vnc unix:$GUEST_RUNTIME/vnc.sock,display=venus -device usb-tablet,bus=xhci.0,port=4"
exec python3 plan/ws005/phase024/guest-ports.py wait
