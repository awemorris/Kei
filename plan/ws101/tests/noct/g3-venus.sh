#!/bin/sh
# ws101-p011: G3 (Noct's automatic parallelization on the GPU) on the Venus guest (the image of
# plan/ws101/tests/gles/build-image.sh, which has libEGL and libGLESv2 but not noct).  Boots the guest as
# plan/ws101/tests/gles/venus.sh does, copies an accelerator-enabled /bin/noct (NOCT: a build's bin/noct made with
# ZEDBSD_NOCT_ACCEL := y, by default the main checkout's demonstration build) and mix.nct into /tmp, then runs:
#  1. gpu-list: noct --gpu-list (the devices Noct's backends see);
#  2. cpu: noct -O2 -j mix.nct N ROUNDS (the CPU, with the JIT);
#  3. gpu: KEI_GLES_COMPUTE_TRACE=1 noct -O2 -j --gpu mix.nct N ROUNDS (the GPU; libGLESv2 writes a line per dispatch).
# It passes when both runs check 1001 elements with none wrong, their checksums are equal, and the GPU run's
# dispatches (one a call) are in its log.  Timing on Venus (the host's lavapipe) is not the 5330's.
#
#   plan/ws101/tests/noct/g3-venus.sh [OUTDIR]     N (default 4000000), ROUNDS (default 8), NOCT, IMAGE, SYMBOLS
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../../.."
out=${1:-build/ws101-p011-venus}
n=${N:-4000000}
rounds=${ROUNDS:-8}
# The GC's tenured region: 1 MiB by default on zedBSD (Noct's NOCT_MEMORY_SMALL), too small for two 16 MB arrays.
tenure=${TENURE:-100000000}
noct=${NOCT:-/home/awe/zedBSD-rpi4/build/demo-lcd9/bin/noct}
image=${IMAGE:-build/ws101-p009-img/hdd-image.img}
symbols=${SYMBOLS:-build/ws101-p009-img/vmunix}
mkdir -p "$out"
export GUEST_RUNTIME="${GUEST_RUNTIME:-$PWD/build/ws101-p011-run}"
export LIBGL_ALWAYS_SOFTWARE=1
export MESA_LOADER_DRIVER_OVERRIDE=zink
export VK_DRIVER_FILES="${VK_DRIVER_FILES:-/usr/share/vulkan/icd.d/lvp_icd.json}"
VENUS_RENDERER=${VENUS_RENDERER:-$PWD/build/ws035-sq-venus/install}
export RENDER_SERVER_EXEC_PATH="${RENDER_SERVER_EXEC_PATH:-$VENUS_RENDERER/libexec/virgl_render_server}"
export LD_LIBRARY_PATH="$VENUS_RENDERER/lib/x86_64-linux-gnu${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
guest() { timeout "${2:-300}" python3 plan/tools/guest/guest.py run "$1" 2>&1; }
status=0

# The guest, and the programs in /tmp.
python3 plan/tools/guest/guest.py stop >/dev/null 2>&1
timeout 300 python3 plan/tools/guest/guest.py start "$image" --symbols "$symbols" \
    --qemu-extra "-object memory-backend-memfd,id=mem,size=8G,share=on -machine memory-backend=mem -device virtio-gpu-gl-pci,id=venus,venus=on,blob=on,hostmem=256M,max_outputs=1 -display egl-headless,rendernode=/dev/dri/renderD128 -vnc unix:$GUEST_RUNTIME/vnc.sock,display=venus -device usb-tablet,bus=xhci.0,port=4" \
    > "$out/start.txt" 2>&1 || { cat "$out/start.txt"; echo "g3-venus: FAIL (the guest did not start)"; exit 1; }
timeout 400 python3 plan/tools/guest/guest.py wait --timeout 360 >> "$out/start.txt" 2>&1 || { echo "g3-venus: FAIL (no SSH)"; exit 1; }
timeout 120 python3 plan/tools/guest/guest.py put "$noct" /tmp/noct > /dev/null || status=1
timeout 60 python3 plan/tools/guest/guest.py put userland/tests/gpudemo/mix.nct /tmp/mix.nct > /dev/null || status=1
guest 'chmod 755 /tmp/noct' > /dev/null

# The runs.
guest '/tmp/noct --gpu-list; echo exit=$?' > "$out/gpu-list.txt"
guest "/tmp/noct -O2 -j --gc-tenure-size=$tenure /tmp/mix.nct $n $rounds > /tmp/g3-cpu.log 2>&1; echo exit=\$? >> /tmp/g3-cpu.log; cat /tmp/g3-cpu.log" 1800 > "$out/cpu.txt"
guest "KEI_GLES_COMPUTE_TRACE=1 /tmp/noct -O2 -j --gc-tenure-size=$tenure --gpu /tmp/mix.nct $n $rounds > /tmp/g3-gpu.log 2>&1; echo exit=\$? >> /tmp/g3-gpu.log; cat /tmp/g3-gpu.log" 1800 > "$out/gpu.txt"
python3 plan/tools/guest/guest.py stop >/dev/null 2>&1
for f in gpu-list cpu gpu; do echo "== $f"; grep -v '^gles: compute dispatch' "$out/$f.txt"; done

# The verdict: both checks clean, equal checksums, the GPU's dispatches.
cpu_sum=$(grep '^MIX checksum' "$out/cpu.txt")
gpu_sum=$(grep '^MIX checksum' "$out/gpu.txt")
dispatches=$(grep -c '^gles: compute dispatch' "$out/gpu.txt")
echo "gpu dispatches: $dispatches"
grep -q '^MIX check=1001 wrong=0' "$out/cpu.txt" || { echo "g3-venus: the CPU run's check failed"; status=1; }
grep -q '^MIX check=1001 wrong=0' "$out/gpu.txt" || { echo "g3-venus: the GPU run's check failed"; status=1; }
[ -n "$cpu_sum" ] && [ "$cpu_sum" = "$gpu_sum" ] || { echo "g3-venus: the checksums differ"; status=1; }
[ "$dispatches" -ge "$rounds" ] || { echo "g3-venus: fewer dispatches than calls (the GPU was not used)"; status=1; }
grep -q '^exit=0' "$out/gpu.txt" || status=1
[ $status -eq 0 ] && echo "g3-venus: PASS" || echo "g3-venus: FAIL"
exit $status
