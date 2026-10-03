#!/bin/sh
# ws100-p004: starts the Venus guest as plan/ws035/tests/zdesktop-guest.sh does, with QEMU's HD Audio recording to a WAV
# (VOLUME_AUDIO=duplex, the default: intel-hda + hda-duplex, the codec's volume applied; duplex-off: mixer=off; none: no
# HD Audio, as the other tests' guests).  The WAV is $GUEST_RUNTIME/out.wav, complete once the guest is stopped.
# The shared script is not changed (it has no way to add devices).
#   VOLUME_AUDIO=duplex plan/ws100/tests/volume-guest.sh start IMAGE
#   plan/ws100/tests/volume-guest.sh stop
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -eu
cd "$(dirname -- "$0")/../../.."
. plan/tools/guest/venus-hostmem.sh
export GUEST_RUNTIME="${GUEST_RUNTIME:-$PWD/build/ws100-run}"
export LIBGL_ALWAYS_SOFTWARE=1
export MESA_LOADER_DRIVER_OVERRIDE=zink
export VK_DRIVER_FILES="${VK_DRIVER_FILES:-/usr/share/vulkan/icd.d/lvp_icd.json}"
VENUS_RENDERER=${VENUS_RENDERER:-$PWD/build/ws035-sq-venus/install}
export RENDER_SERVER_EXEC_PATH="${RENDER_SERVER_EXEC_PATH:-$VENUS_RENDERER/libexec/virgl_render_server}"
export LD_LIBRARY_PATH="$VENUS_RENDERER/lib/x86_64-linux-gnu${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
command=${1:-start}
case "$command" in
start)
	image=${2:?usage: volume-guest.sh start IMAGE}
	python3 plan/tools/guest/guest.py stop >/dev/null
	mkdir -p "$GUEST_RUNTIME"
	rm -f "$GUEST_RUNTIME/out.wav"
	wav="-audiodev wav,id=w,path=$GUEST_RUNTIME/out.wav,out.frequency=48000,out.channels=2,out.format=s16"
	case "${VOLUME_AUDIO:-duplex}" in
	duplex) audio="-device intel-hda,id=hda -device hda-duplex,bus=hda.0,audiodev=w $wav" ;;
	duplex-off) audio="-device intel-hda,id=hda -device hda-duplex,bus=hda.0,audiodev=w,mixer=off $wav" ;;
	*) audio= ;;
	esac
	exec python3 plan/tools/guest/guest.py start "$image" \
	    --qemu-extra "-object memory-backend-memfd,id=mem,size=8G,share=on -machine memory-backend=mem -device virtio-gpu-gl-pci,id=venus,venus=on,blob=on,hostmem=$VENUS_HOSTMEM,max_outputs=1 -display egl-headless,rendernode=/dev/dri/renderD128 -vnc unix:$GUEST_RUNTIME/vnc.sock,display=venus -device usb-tablet,bus=xhci.0,port=4 $audio"
	;;
*)
	exec python3 plan/tools/guest/guest.py "$@"
	;;
esac
