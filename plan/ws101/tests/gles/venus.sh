#!/bin/sh
# ws101-p009: OpenGL ES 3.1's compute on the Venus guest (the image of plan/ws101/tests/gles/build-image.sh).
# Boots the guest with a Venus GPU as plan/ws035/tests/zdesktop-guest.sh does (in its own runtime directory) and runs
# /bin/glescompute over SSH (plan/tools/guest/guest.py):
#  1. auto: Noct's order of platforms (surfaceless first), the default steps (GLESCOMPUTE DONE failures=0).
#  2. default: the default display's pbuffer (the platform Noct falls back to).
#  3. repeat: 1000 rounds of new buffer storage, a dispatch and a read back.
# A compositor (/bin/wayland) runs throughout, and it must still be alive after each run: a pbuffer context does not
# take the screen.  The guest is stopped at the end.
#
#   plan/ws101/tests/gles/venus.sh [OUTDIR]     (IMAGE, SYMBOLS: the image and its vmunix)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../../.."
. plan/tools/guest/venus-hostmem.sh
out=${1:-build/ws101-p009-venus}
image=${IMAGE:-build/ws101-p009-img/hdd-image.img}
symbols=${SYMBOLS:-build/ws101-p009-img/vmunix}
mkdir -p "$out"
export GUEST_RUNTIME="${GUEST_RUNTIME:-$PWD/build/ws101-p009-run}"
export LIBGL_ALWAYS_SOFTWARE=1
export MESA_LOADER_DRIVER_OVERRIDE=zink
export VK_DRIVER_FILES="${VK_DRIVER_FILES:-/usr/share/vulkan/icd.d/lvp_icd.json}"
VENUS_RENDERER=${VENUS_RENDERER:-$PWD/build/ws035-sq-venus/install}
export RENDER_SERVER_EXEC_PATH="${RENDER_SERVER_EXEC_PATH:-$VENUS_RENDERER/libexec/virgl_render_server}"
export LD_LIBRARY_PATH="$VENUS_RENDERER/lib/x86_64-linux-gnu${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
guest() { timeout "${2:-300}" python3 plan/tools/guest/guest.py run "$1" 2>&1; }
status=0

# The guest.
python3 plan/tools/guest/guest.py stop >/dev/null 2>&1
timeout 300 python3 plan/tools/guest/guest.py start "$image" --symbols "$symbols" \
    --qemu-extra "-object memory-backend-memfd,id=mem,size=8G,share=on -machine memory-backend=mem -device virtio-gpu-gl-pci,id=venus,venus=on,blob=on,hostmem=$VENUS_HOSTMEM,max_outputs=1 -display egl-headless,rendernode=/dev/dri/renderD128 -vnc unix:$GUEST_RUNTIME/vnc.sock,display=venus -device usb-tablet,bus=xhci.0,port=4" \
    > "$out/start.txt" 2>&1 || { cat "$out/start.txt"; echo "venus: FAIL (the guest did not start)"; exit 1; }
timeout 400 python3 plan/tools/guest/guest.py wait --timeout 360 >> "$out/start.txt" 2>&1 || { cat "$out/start.txt"; echo "venus: FAIL (no SSH)"; exit 1; }

# A compositor running through the runs (as plan/ws068/tests/egl-p030.sh starts it), which none may take the screen from.
guest 'export XDG_RUNTIME_DIR=/tmp; rm -f /tmp/wayland-0; /bin/wayland --timeout=3600 --width=1280 --height=800 --glass > /tmp/zdesktop.log 2>&1 </dev/null & sleep 4; echo started' > /dev/null
guest 'ps -A -o pid,args' > "$out/ps-before.txt"

# Runs glescompute with arguments into a log, and fails the run unless it ends with no failure.
run() {
	name=$1; shift
	guest "/bin/glescompute $* > /tmp/gc-$name.log 2>&1; echo exit=\$?; cat /tmp/gc-$name.log" "${TIMEOUT:-600}" > "$out/$name.txt"
	cat "$out/$name.txt"
	if grep -q 'GLESCOMPUTE DONE failures=0' "$out/$name.txt" && grep -q '^exit=0' "$out/$name.txt"; then
		echo "venus $name: PASS"
	else
		echo "venus $name: FAIL"
		status=1
	fi
}
run auto
guest 'ps -A -o pid,args' > "$out/ps-auto.txt"
run default --platform=default
guest 'ps -A -o pid,args' > "$out/ps-default.txt"
TIMEOUT=1800 run repeat --repeat=1000
guest 'ps -A -o pid,args' > "$out/ps-after.txt"

# The compositor alive before and after each run (one that died with a run, or lost its screen, would be missing).
for f in before auto default after; do
	count=$(grep -cE '/bin/[w]ayland' "$out/ps-$f.txt")
	echo "compositor $f: $count"
	[ "$count" -ge 1 ] || status=1
done
python3 plan/tools/guest/guest.py stop >/dev/null 2>&1
[ $status -eq 0 ] && echo "ws101 venus: PASS" || echo "ws101 venus: FAIL"
exit $status
