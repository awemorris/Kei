#!/bin/sh
# ws081-p019 (BUG-156): the PS/2 mouse's wheel on a QEMU guest whose only mouse is the PS/2 one (the i8042 of q35:
# plan/tools/guest/guest.py start without usb-tablet, so QEMU routes QMP's relative motion, buttons and wheel to it).
# QEMU's PS/2 mouse answers the IntelliMouse knocks (ID 3, then the Explorer's ID 4), as a laptop touchpad's PS/2
# emulation does for its scroll.  The run:
#  1. ps2wheel (ps2wheel.c, built here and put into the guest over SSH) opens the PS/2 mouse's input device, which
#     starts and identifies it: the kernel log says "i8042: mouse id=3" or "id=4" with packet=4.
#  2. Through QMP: three notches up, two down, a relative motion of +40 x / +20 y, a left click, a side-button click.
#  3. ps2wheel's totals: wheel_up=3 wheel_down=2, x>0 and y>0, left=1, and with id=4 side=1.
# The image is any Kei image (no desktop is needed; a running zdesktop also reads the mouse and does not matter).
#   plan/ws081/tests/ps2-wheel-qemu.sh IMAGE [BUILD] [OUTDIR]
#     BUILD: the build whose sysroot and libc.so link ps2wheel (default build/amd64; build/amd64/sysroot must exist).
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib
set -u
cd "$(dirname -- "$0")/../../.."
image=${1:?usage: ps2-wheel-qemu.sh IMAGE [BUILD] [OUTDIR]}
build=${2:-build/amd64}
out=${3:-build/ws081-shots/ps2wheel}
mkdir -p "$out"
GUEST_RUNTIME="${GUEST_RUNTIME:-$(pwd)/build/ws081-ps2-run}"
export GUEST_RUNTIME
guest() { timeout 90 python3 plan/tools/guest/guest.py run "$1" 2>&1 </dev/null; }
status=0

# The probe, for Kei.
sysroot=$(pwd)/build/amd64/sysroot
[ -d "$build/sysroot" ] && sysroot=$(pwd)/$build/sysroot
build/llvm/bin/clang --target=x86_64-unknown-zedbsd --sysroot="$sysroot" -nostdinc -I. -Iinclude -isystem "$sysroot/usr/include" \
    -DHAL_ARCH_AMD64 -DKERN_USER_ABI_LP64 -DKERN_DYNAMIC_LIBC -m64 -march=x86-64 -mno-red-zone -O2 -ffreestanding -fPIC \
    -fno-builtin -fno-stack-protector -Wall -Wextra -Werror -c plan/ws081/tests/ps2wheel.c -o "$out/ps2wheel.o" || { echo "ps2-wheel-qemu: FAIL (build)"; exit 1; }
build/llvm/bin/clang --target=x86_64-unknown-zedbsd --sysroot="$sysroot" -m64 -nostdlib -pie -Wl,--no-relax \
    -Wl,--hash-style=sysv,-z,now,-z,relro -Wl,--allow-shlib-undefined -Wl,--dynamic-linker=/lib/ld.so \
    "$sysroot/usr/lib/crt1.o" "$out/ps2wheel.o" -L"$build/dynamic" -Wl,-rpath-link,"$build/dynamic" \
    -l:libc.so -o "$out/ps2wheel" || { echo "ps2-wheel-qemu: FAIL (link)"; exit 1; }

# The guest: q35's i8042, no usb-tablet.
python3 plan/tools/guest/guest.py stop >/dev/null 2>&1
timeout 120 python3 plan/tools/guest/guest.py start "$image" >/dev/null 2>&1
timeout 260 python3 plan/tools/guest/guest.py wait --timeout 240 >/dev/null 2>&1 || { echo "ps2-wheel-qemu: FAIL (no SSH)"; exit 1; }
timeout 60 python3 plan/tools/guest/guest.py put "$out/ps2wheel" /tmp/ps2wheel >/dev/null 2>&1
guest 'chmod 755 /tmp/ps2wheel; (/tmp/ps2wheel --seconds=12 > /tmp/ps2wheel.log 2>&1 &); sleep 2; cat /tmp/ps2wheel.log'

# The mouse's identification, from the kernel's log.
guest 'dmesg 2>/dev/null | grep "i8042: mouse" | tail -2' > "$out/dmesg.txt"
cat "$out/dmesg.txt"
grep -qE 'i8042: mouse id=(3|4) packet=4' "$out/dmesg.txt" && echo "identified: ok" || { echo "identified: FAIL"; status=1; }
explorer=0
grep -q 'i8042: mouse id=4' "$out/dmesg.txt" && explorer=1

# The wheel, a motion and two buttons through QMP.
python3 - "$GUEST_RUNTIME/qmp.sock" <<'PYEOF'
import json, socket, sys, time
s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
s.connect(sys.argv[1])
f = s.makefile("rw")
f.readline()
def send(command, arguments=None):
	f.write(json.dumps({"execute": command, "arguments": arguments or {}}) + "\n")
	f.flush()
	while True:
		reply = json.loads(f.readline())
		if "return" in reply or "error" in reply:
			return reply
def button(name):
	for down in (True, False):
		send("input-send-event", {"events": [{"type": "btn", "data": {"down": down, "button": name}}]})
		time.sleep(0.15)
send("qmp_capabilities")
for name in ("wheel-up", "wheel-up", "wheel-up", "wheel-down", "wheel-down"):
	button(name)
	time.sleep(0.3)
send("input-send-event", {"events": [{"type": "rel", "data": {"axis": "x", "value": 40}}, {"type": "rel", "data": {"axis": "y", "value": 20}}]})
time.sleep(0.3)
button("left")
time.sleep(0.3)
button("side")
PYEOF
sleep 12
guest 'cat /tmp/ps2wheel.log' > "$out/ps2wheel.log"
cat "$out/ps2wheel.log"
line=$(grep '^PS2WHEEL device=' "$out/ps2wheel.log" | tail -1)
value() { echo "$line" | sed -n "s/.* $1=\([-0-9]*\).*/\1/p"; }
[ "$(value wheel_up)" = 3 ] && echo "wheel up 3: ok" || { echo "wheel up 3: FAIL"; status=1; }
[ "$(value wheel_down)" = 2 ] && echo "wheel down 2: ok" || { echo "wheel down 2: FAIL"; status=1; }
x=$(value x); y=$(value y)
[ "${x:-0}" -gt 0 ] 2>/dev/null && [ "${y:-0}" -gt 0 ] 2>/dev/null && echo "motion x=$x y=$y: ok" || { echo "motion x=$x y=$y: FAIL"; status=1; }
[ "$(value left)" = 1 ] && echo "left button: ok" || { echo "left button: FAIL"; status=1; }
if [ $explorer = 1 ]; then
	[ "$(value side)" = 1 ] && echo "side button: ok" || { echo "side button: FAIL"; status=1; }
fi
python3 plan/tools/guest/guest.py stop >/dev/null 2>&1
[ $status = 0 ] && echo "ps2-wheel-qemu: PASS" || echo "ps2-wheel-qemu: FAIL"
exit $status
