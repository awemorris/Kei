# WS109 native contract probes

Run from the source root on FreeBSD15 with the selected native dependency packages.
`gmake -j16 -f userland/desktop/keiland-freebsd.mk libraries` builds the L1 foundation;
`gmake -f userland/desktop/keiland-freebsd.mk install DESTDIR=/tmp/keiland-stage` stages it.
Real libkeiland/libkeiui/libpdf are also built; compositor/apps need later backend integration.

- public-libraries.c: compile with `-I/tmp/keiland-stage/opt/keiland/include -I/usr/local/include`,
  link staged libwayland-client.so/libtruetype.so/libvulkan.so.1. Use `-fPIE -pie` for native clients.
- ancillary.c: compile `-I. -Iuserland/desktop/keiland -pthread`, link staged libwayland-client.so.
  Uses public calls and private FIFO inspection to verify actual native kernel rights and ownership.
- sync-rejected.c: compile `-I.` with the selected `libvulkan-compat/freebsd/sync-freebsd.c`
  (Linux uses linux/sync-linux.c). Checks real rejected ioctls, not positive hardware fences.

All probes: `-std=gnu17 -Wall -Wextra -Werror`; run with LD_LIBRARY_PATH set to the staged lib directory.
The existing plan/tools/keiland-linux/vk-chain-test.c runs natively with system Vulkan headers,
staged own library and VK_DRIVER_FILES=/usr/local/share/vulkan/icd.d/lvp_icd.x86_64.json,
DISPLAY/WAYLAND_DISPLAY unset and KEILAND_DRM_DEVICE=none. It checks a real headless1MiB workload.
Never label that result physical GPU/display acceptance. Exact environment/results: q553 history.

## Native OSS probes

Inside an owned FreeBSD fixture only: compile audio-freebsd.c probe with the production
libkeiland/freebsd/audio-freebsd.c and `-Iuserland/desktop/keiland -lmixer`.
It changes actual mixer values, checks independent readback and restores its original settings.
Compile audio-retry-freebsd.c with the same production module; run as root through
`python3.11 audio-retry-freebsd.py /absolute/probe` only in the owned guest.
The controller restores original /dev/mixer* permissions on all exits; child drops all privilege,
verifies denial, then reconnects the same subscription through normal production retry logic.
This test proves permission arrival on the QEMU HDA mixer, not physical device removal or WiFi.

## Native network and WPA wire probes

Compile network-native.c/network-carrier.c/network-radio-freebsd.c/network-wpa-contract.c with
actual libkeiland/freebsd/network-link-freebsd.c and wpa/network-{wpa,config-wpa}.c,
`-I. -Iuserland/desktop/keiland -std=gnu17 -Wall -Wextra -Werror`.
network-native selected-interface absent reads real metadata; selected-interface denied must run
already unprivileged (no supplementary groups/gid/uid65534). network-radio-freebsd runs in the
owned guest only, requests already-UP vtnet0 state and compares both flag halves independently.
network-carrier must start before an owned QMP net0 down/up; eight-second native sampling requires
actual loss then recovery. Restore the virtual link even on controller error.
network-wpa-contract.py requires root in the owned guest and an existing empty native control
directory. Run with the absolute C probe path. It owns one socket, observes exact production
commands, removes its own endpoint/config, and bounds client/server lifetime. All mock evidence
is wire-only; actual WiFi scan/connect/disconnect/storage is retained for the user's real machine.
network-address.c links the selected OS link module alone; actual private datagram round trip
checks native address extent on Linux and FreeBSD without touching host supplicant directories.

## Installed UI/service/PDF libraries

ui-libraries.c uses installed public headers only, not private source headers. Compile with
`-fPIE -pie -std=gnu17 -Wall -Wextra -Werror -I$stage/opt/keiland/include
-L$stage/opt/keiland/lib -Wl,-rpath-link,$stage/opt/keiland/lib -lkeiland -lkeiui -lpdf`.
Run LD_LIBRARY_PATH=$stage/opt/keiland/lib in the owned native fixture with accessible OSS and wired
vtnet0. It checks real service observations, all64 CPU pixels and an owned PDF write/read/digest
round trip; it does not prove display, a window or WiFi. Exact q556 evidence and runtime limits retained.

## Shared dma-buf exporter

dmabuf-export-rejected.c links the selected wayland/{linux,freebsd}/sync module;
`cc -std=gnu17 -Wall -Wextra -Werror -I.`. Actual invalid fd and real pipe ioctls must return
EBADF/ENOTTY without publishing an output or consuming a borrowed descriptor. No positiveDMA
claim. q557 compiles sharedgpu/evdev/session as actual nativeobjects; native seat/backend and
fullcompositor are still needed before runtime/physical display or input acceptance.
