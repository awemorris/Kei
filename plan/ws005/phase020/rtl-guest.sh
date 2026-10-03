#!/bin/sh
# ws005-p020: the Venus desktop guest (as plan/ws035/tests/zdesktop-guest.sh starts it) with the development host's
# TP-Link 802.11ac NIC (2357:0138, RTL8822BU) passed through by usb-host on a controller of its own.  The device's
# node is made writable for the run (QEMU runs as the user) and given back its mode by stop.  Nothing else of the
# host's USB or network is touched.  No key is passed here: keys go to the guest on standard input only.
#   GUEST_RUNTIME=... sh plan/ws005/phase020/rtl-guest.sh start IMAGE | stop
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../.."
. plan/tools/guest/venus-hostmem.sh
export GUEST_RUNTIME="${GUEST_RUNTIME:-$PWD/build/p1-rtl-run}"
export LIBGL_ALWAYS_SOFTWARE=1 MESA_LOADER_DRIVER_OVERRIDE=zink
export VK_DRIVER_FILES="${VK_DRIVER_FILES:-/usr/share/vulkan/icd.d/lvp_icd.json}"
renderer=${VENUS_RENDERER:-$PWD/build/ws035-sq-venus/install}
export RENDER_SERVER_EXEC_PATH="$renderer/libexec/virgl_render_server"
export LD_LIBRARY_PATH="$renderer/lib/x86_64-linux-gnu${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
# The exact device: its bus and address now (they change when it is plugged again).
device=""
for path in /sys/bus/usb/devices/*; do
	[ -f "$path/idVendor" ] || continue
	if [ "$(cat "$path/idVendor"):$(cat "$path/idProduct")" = "2357:0138" ]; then
		device=$path
		break
	fi
done
[ -n "$device" ] || { echo "rtl-guest: no 2357:0138 on the host"; exit 1; }
bus=$(cat "$device/busnum")
address=$(cat "$device/devnum")
node=$(printf '/dev/bus/usb/%03d/%03d' "$bus" "$address")
case "${1:-start}" in
stop)
	python3 plan/tools/guest/guest.py stop || true
	sudo -n chmod 0664 "$node"
	echo "rtl-guest: stopped, $node $(stat -c %a "$node")"
	;;
start)
	sudo -n chmod 0666 "$node"
	exec python3 plan/tools/guest/guest.py start ${GUEST_CPUS:+--cpus "$GUEST_CPUS"} "${2:?image}" \
	    --qemu-extra "-object memory-backend-memfd,id=mem,size=8G,share=on -machine memory-backend=mem -device virtio-gpu-gl-pci,id=venus,venus=on,blob=on,hostmem=$VENUS_HOSTMEM,max_outputs=1 -display egl-headless,rendernode=/dev/dri/renderD128 -vnc unix:$GUEST_RUNTIME/vnc.sock,display=venus -device usb-tablet,bus=xhci.0,port=4 -device qemu-xhci,id=wlanxhci -device usb-host,hostbus=$bus,hostaddr=$address,bus=wlanxhci.0"
	;;
esac
