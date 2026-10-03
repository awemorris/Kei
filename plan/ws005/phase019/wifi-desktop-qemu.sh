#!/bin/bash
# ws005-p019: the desktop image on the 5330 with its AX211 passed through, on the 5330.
#
# 2026-10-03 (user, Guardrail): the iGPU and the AX211 are never given to one QEMU (they had never been passed
# through together before ws005-p019, and the third such run hung the host), and a Wi-Fi test does not use the
# iGPU at all.  The guest has the standard VGA only: the greeter finds no GPU and the console login comes up on
# it; the desktop's Wi-Fi UI is checked on the Venus QEMU instead.
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
#
# The conditions of plan/ws075/tests/hdmi/h4-qemu.sh (q35, KVM, 4 GiB, 4 vCPUs, OVMF,
# NVMe, two QMP sockets, a USB tablet and keyboard, so h4-ctl.py takes shots and drives the pointer), plus the
# AX211 (0000:00:14.3, moved from iwlwifi to vfio-pci for the run and back to iwlwifi when QEMU ends, as
# plan/ws004/tests/run-intel-ax211-vfio-qemu.sh does) and a USB CDC ECM interface on QEMU's user network as the
# wired connection.  Only 0000:00:14.3 is moved; the host's USB LAN, the iGPU mode and other guests are not
# touched.  Copied to ~/bigbang/h4/ by plan/ws005/phase019/wifi-desktop-hw.sh.  The restore's outcome is in
# restore.log.
#
#   wifi-desktop-qemu.sh IMAGE
set -u
image=$1
dir=/home/awe/bigbang/h4
bdf=0000:00:14.3
device=/sys/bus/pci/devices/$bdf
safe=enx6c1ff71a08b6
cd "$dir" || exit 1
rm -f qmp.sock qmp-load.sock run.log serial.log qemu.log restore.log
driver() {
	if [ -L "$device/driver" ]; then basename "$(readlink -f "$device/driver")"; else echo none; fi
}
route_dev() {
	ip route get 10.0.10.1 | awk '{ for (i = 1; i <= NF; i++) if ($i == "dev") { print $(i + 1); exit } }'
}
restore() {
	[ "$(driver)" = vfio-pci ] && printf '%s' "$bdf" | sudo -n tee /sys/bus/pci/drivers/vfio-pci/unbind > /dev/null
	printf '\n' | sudo -n tee "$device/driver_override" > /dev/null
	sudo -n modprobe iwlwifi
	[ "$(driver)" = iwlwifi ] || printf '%s' "$bdf" | sudo -n tee /sys/bus/pci/drivers_probe > /dev/null
	for i in $(seq 1 10); do [ "$(driver)" = iwlwifi ] && break; sleep 1; done
	echo "driver=$(driver) override=$(cat "$device/driver_override") route=$(route_dev)" > restore.log
	if [ "$(driver)" = iwlwifi ] && [ "$(route_dev)" = "$safe" ]; then
		echo "restored" >> restore.log
	else
		echo "NOT RESTORED" >> restore.log
	fi
}

# Refuses anything but the exact AX211 owned by iwlwifi, with the SSH route on the USB LAN.
[ "$(cat "$device/vendor")" = 0x8086 ] && [ "$(cat "$device/device")" = 0x51f0 ] || { echo "not the AX211" > restore.log; exit 1; }
[ "$(driver)" = iwlwifi ] || { echo "AX211 not on iwlwifi" > restore.log; exit 1; }
[ "$(route_dev)" = "$safe" ] || { echo "route not on $safe" > restore.log; exit 1; }
trap restore EXIT
sudo -n modprobe vfio-pci
printf '%s' vfio-pci | sudo -n tee "$device/driver_override" > /dev/null
printf '%s' "$bdf" | sudo -n tee /sys/bus/pci/drivers/iwlwifi/unbind > /dev/null
printf '%s' "$bdf" | sudo -n tee /sys/bus/pci/drivers_probe > /dev/null
[ "$(driver)" = vfio-pci ] || { echo "cannot bind vfio-pci" >> qemu.log; exit 1; }

cp -f /usr/share/OVMF/OVMF_VARS_4M.fd vars.fd
args=(
	-machine q35,accel=kvm,memory-backend=mem
	-cpu host,host-phys-bits-limit=39
	-m 4096
	-smp 4
	-object memory-backend-memfd,id=mem,size=4G,share=on
	-device vfio-pci,host=$bdf,id=ax211
	-drive if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE_4M.fd
	-drive if=pflash,format=raw,file=$dir/vars.fd
	-drive file=$image,format=raw,if=none,id=zd0 -device nvme,drive=zd0,serial=zedbsd0
	-vga std -display none -monitor none -serial file:$dir/serial.log
	-debugcon file:$dir/run.log -no-reboot
	-qmp unix:$dir/qmp.sock,server=on,wait=off
	-qmp unix:$dir/qmp-load.sock,server=on,wait=off
	-device qemu-xhci,id=xhci -device usb-tablet,bus=xhci.0 -device usb-kbd,bus=xhci.0
	-netdev user,id=n0 -device usb-net,bus=xhci.0,netdev=n0,id=lan0
)
sudo -n timeout $(( ${H4_MINUTES:-60} * 60 )) qemu-system-x86_64 "${args[@]}" > qemu.log 2>&1
echo "wifi-desktop-qemu: QEMU ended (status $?)" >> qemu.log
