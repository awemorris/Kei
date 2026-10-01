# q557 / shared native device mechanisms partial result

Mechanism/source/compile subset cleared, whole p003/F3 remains uncleared. Actual FreeBSD seat/device
service and fullcompositor link/display/positiveDMA synchronization are subsequent gates, no stub.
Moved existing Linux Vulkan/dma-buf protocol import to wayland/dmabuf/gpu-dmabuf.c, sharing one renderer
for both native OSs. Standard zwp_linux_dmabuf_v1 name/revision, layout bounds, modifier/import queries,
fd/Vulkanimage/memory/error/client retirement remain. Sole implicit reservation export behind private
sync.h and separate Linux/nativeFreeBSD modules; fixeddrm_v6.6.25_13 native eight-byte request/READ1/
_IOWR encoding, no driver implementation imported. Export output published only on native success;
shared commit keeps ENOTTY CPU-completed presentation fallback, real other error/clienttermination,
CLOEXEC and common acquire ownership. fdreservation query preserves approved WS105 synchronization;
no zedBSD GPUUAPI/display ioctl is introduced, display uses libvulkan.

Portable native session lifecycle moved to session/handoff-session.c. Existing evdev discovery/
capability/read/monotonicclock/probe moved to evdev/input-evdev.c with private seat authority interface.
Linux adapters retain existing direct/logind ownership/revocation ordering. FreeBSD selector in the
one approved tiny zwl-evdev.h supplies actual dev/evdev/input.h; no broadOSmacros or duplicate input
algorithm. Native seat implementation still required; objects are not a working native compositor.

Native FreeBSD15.1-p4/baseClang19.1.7 allfour sharedgpu/input/session/nativesync production objects
compile `-O2 -g -std=gnu17 -Wall -Wextra -Werror -fPIC -I. -Iuserland/desktop/keiland -I/usr/local/include`,
warning0. [Actual rejected sync probe](shared-native-build.txt) links native exporter and real kernel
pipe/invaliddescriptor: EBADF/ENOTTY, output unchanged, borrowedbuffer fd retained. [Linux same](shared-linux-export.txt)
PASS. Positive DMA export/import/fences/actualdevice ownership remains physicalgate.

Linux [full actual compositor build](shared-linux-final-build.txt) -j16/configuredGCC14.2 warning0,
links actual shared mechanisms+Linux adapters. Package makefile-sync PASS with explicit native-only
mechanism ownership rules. [Boundary C1..C5/L1..L5 PASS](shared-boundary.txt): nativeFreeBSD roots recognized,
only the five existing shared native evdev metadata/clock request families allowed in exactly the
sharedinput source; unknown/otherdevice ioctls still rejected. Existing top-level make -pn emits two
pre-existing suffix-rule warnings (Makefile:769), not a warning0 claim for that metadata command.
No hostdisplay/input/network changes or new nativecompositor runtime claim. Source algorithms preserved;
modifier lookup classification rewritten as break→guard/refusal→finalsuccess, equivalent token result.

Full applicable C rules reviewed for allthree moved sources, bothsync OS modules, Linux seat adapter
and newprobe; no relocation exception. declarations/static/public/semantic paragraphs/nativeheader/
request layout/call/error/fd/image/memory ownership inspected. clang-format19 output reviewed with
mandatory argument/three-clause/non-counter loop layouts retained; style-check allseven C0, diff-check
PASS. Native objects and export probe repeated after formatting; Linuxcompositor/export rebuilt after
finalsource changes. FullallWS conformance/threeOS/runtime/physical finalgates remain p005.
Local records/outbox pending, no make check/hosttoolchain/sharedcleanup/push/remotepublication.
