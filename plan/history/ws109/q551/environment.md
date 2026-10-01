# q551 / FreeBSD native environment and port procedure

2026-10-02 JST. Image pinned by q550 SHA256. Native root SSH at loopback forwarded port47969; QEMU10.0.11 KVM,8GiB/4CPU, own20GiB overlay. Native kernel/userland both15.1-RELEASE-p4 amd64, baseClang19.1.7. [Login PNG](boot.png) inspected/shown, [SSH](native-boot.txt). serialnull; no console/serial logs used.

## Actual environment

[Native inventory](native-survey.txt), [ABI/mixer/header hashes](native-abi.txt), [dependency headers](native-deps.txt), [DRM/rtld](native-driver-abi.txt), [ports versions/licenses](native-packages.txt), [actual loader dependencies](native-link-libraries.txt). These commands query running native interfaces, not just upstream definitions. HDA mixer0 lists vol/pcm/line/rec, evdev event0..5 present, vtnet0 active/MTU1500/nativeIPv4/IPv6. No /dev/dri: this VM supplies neither hardware Vulkan display nor dma-buf fence proof. Native extattr, libutil/PTY, libmixer, AF_LINK headers exist; syscall contracts are verified in implementing Phases.

Guest-only `pkg update -f; pkg install -y gmake python311 vulkan-loader vulkan-headers mesa-dri libdrm seatd pkgconf` exit0 ([full package log](pkg-install.txt)). No host/target/shared toolchain mutation, kernel/image update or distribution publishing. GNUmake4.4.1; Vulkanloader1.4.356/header1.4.356_1; Mesa26.1.3; nativeDRM2.4.133; seatd0.9.3_1; Python3.11.16 (Mesa pullsPython3.12); pkg2.6.2. Ports/base licenses are recorded separately. GPLv3 gmake is a build tool, not shipped Keiland code. DriverGPLv2 permission is scoped to existing drm-kmod. Native libseat links LGPL21+basu; do not silently extend that permission. Use seatd's MIT daemon/client wire or a verified seatd-only permissive client, not this multi-backend libseat linkage. p003 owns native seat implementation/proof.

## ABI and implementation decisions

- Native `RTLD_DEEPBIND=0x04000`; absolute `/usr/local/lib/libvulkan.so.1` behind own WSI. Actual symbol/self-recursion and buffer-copy workloads in p002.
- Native libdrm headers are `/usr/local/include/libdrm/{drm.h,drm_mode.h}` with native `_IOWR`; build-local `drm` include alias preserves common source spelling. Never use Linux header/ioctl constants on FreeBSD.
- Ports do not install dma-buf.h. Own tiny OS synchronization module encodes the fixed drm-kmod UAPI record `{uint32_t flags,int32_t fd}`, WRITE2/READ1, native EXPORT `_IOWR('b',2,record)` /IMPORT `_IOW('b',3,record)`. Reference fixedtag/commit in [q550](../q550/survey.md); preserve Linux fallback/errno/fd ownership. Runtime actual buffers/fences stay physical gate.
- Native OSS/libmixer for enumeration/volume/mute (no PCM scope addition); periodic refresh if no pollable event fd. Preserve original guest mixer settings after any probe.
- AF_LINK/if_data/native flags/counters + shared WPA wire; native path `/var/run/wpa_supplicant`. DHCP/IP belongs to system service. Native wired state can be tested now; actual scan/join/persistence later on user's WiFi.
- Native evdev/VT/seatd client with enable/disable acknowledgement, device closure and VT restoration. Common compositor renderer reused. Native OSS/vtnet/evdev is hardware emulation, distinguished from physical GPU/WiFi.
- PTY through native libutil and Files user extattr through OS adapters; namespace/list encodings/link/fd/copy semantics cannot be replaced by successful stubs.

## Finite execution and dependencies

p002 L1: independent GNUmake `keiland-freebsd.mk` plus package Makefile.freebsd for codecs/digests/privateWayland/TrueType/ownVulkan WSI; native standard headers/ELF/DESTDIR/symbol chain/headless copy. p002 L2: real libkeiland/compositor/apps integrated after p003/p004 backend output. No stub providers. p003/p004 depend on verified L1 headers/libraries rather than all of p002, avoiding an integration cycle. p002 L2 requires actual p003/p004 source outputs; p005 requires all3 completed implementation outputs plus real hardware gates and full standards/affectedLinux+zedBSD regression. Partial Queue outcomes must not claim fullPhase/WS acceptance.

Commands: native `gmake -j16 -f userland/desktop/keiland-freebsd.mk libraries CC=cc`; DESTDIR install and system-inclusive header dependencies; `readelf -d`, `ldd`, own standard header client, reused vk-chain-test default production path with own installed libvulkan +systemloader+Mesa. Linux affected native build and Vulkan chain tests, zedBSD affected builds/boot-test final, fullmanual Cstandard review p005. No make check.

## Attempts and limits

First boot90s SSH wait failed while own disk writes continued; later QMP disconnected and first emulator exited0. Cause is unproven (no console logs analyzed), preserved process metadata/empty emulator stderr. Second boot allowed reset, native SSH/login succeeded. No claim that a boot bug was fixed. Package preparation and native inventory succeeded; p001 software/environment/design criteria now clear. Physical device model/driver/sync/display/seat/WiFi acceptance remains WS F1/F3/F4/F5. q550 uncleared history retained. Production port sources remain unmodified by q551. GitHub event/close/projection publication deferred; local outbox pending.
