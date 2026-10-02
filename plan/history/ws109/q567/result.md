# q567 / native QEMU Venus prerequisite missing

Attempt uncleared, whole p003 uncleared; WS109 incomplete. User explicitly waived physical
GPU/display/WiFi tests; those gates are not reinstated. Required replacement remains actual
native FreeBSD QEMU Venus usage, not merely host support or guest lavapipe.

## Actual configuration and observations

Host QEMU10.0.11/virglrenderer1.1.0/Mesa25.0.7, Linux6.12.101; renderD128 is vgem (not a
hardware Vulkan GPU), installed host Vulkan driver llvmpipe. Existing Mesa documentation allows
lavapipe as a Venus HOST backend; this does not waive the required Venus GUEST transport.
Owned guest restarted only after q566 completion: virtio-vga-gl hostmem4G/blob=true/venus=true,
8GiB shared memory-backend-memfd, egl-headless renderD128, VK_DRIVER_FILES pointing to host lvp.
QMP query-status running true, loopback SSH native kern.osrelease15.1-RELEASE-p4 succeeded,
actual pciconf shows Virtio1.0GPU1af4:1050. QMP PNG login inspected/shown. Emulator stderr empty.
This establishes configured QEMU/guest boot, not negotiated Venus context or command execution.
Guest /dev/dri absent; installed Mesa26.1.3 contains intel/lvp/radeon ICDs and no virtio/Venus ICD.
No native Venus physical device/command or GUI/fence result obtained.

## Primary source capability chain / retrieved 2026-10-02

[Mesa Venus](https://docs.mesa3d.org/drivers/venus.html) requires guest virtio_gpu 3D,
CAPSET_QUERY_FIX,RESOURCE_BLOB,HOST_VISIBLE,CONTEXT_INIT. vtest bypasses guest kernel, so it is
not silently substituted for actual QEMU virtio transport.
[QEMU](https://www.qemu.org/docs/master/system/devices/virtio/virtio-gpu.html) documents blob/
hostmem/venus; version/property support observed and actual configured boot succeeds.
[Official FreeBSD Mesa port](https://github.com/freebsd/freebsd-ports/blob/main/graphics/mesa-dri/Makefile)
currently selects anv/radv/swrast_vk, no virtio option, consistent with actual installed package.
Current pkg repository has drm515/61/66/latest and virtio-gpu-qemu-kmod0.2. The latter's
[official description](https://github.com/freebsd/freebsd-ports/blob/main/graphics/virtio-gpu-qemu-kmod/pkg-descr)
is framebuffer mmap/scfb support, not DRM Venus. Installing it cannot supply missing DRM ABI.
Latest [drm-kmod proposal517](https://github.com/freebsd/drm-kmod/pull/517), open, createdOct1,
head8fc258c653c0695ebe32b484ba93ded4f5db4afa, explicitly lacks host-visible memory; blob and
context-init untested, test platform16-CURRENT. Source getparam returns has_host_visible and
VRAM maps refuse absent capability. Existing upstream proposal therefore cannot satisfy the
required Venus capability without further driver work; FreeBSD15 compatibility also unverified.
Older119 is obsolete/basic5.5 compilation, not a usable Venus alternative. No unsupported kernel
modules were installed, no full external implementation imported, no host stack replacement.

## Scope decision and resume

WS109 excludes FreeBSD kernel/driver port. User was asked in chat: plan virtio-gpu/Mesa Venus
work separately keeping WS109 incomplete; explicitly replace acceptance with real native
lavapipe execution; or wait upstream. No option assumed approved. If Venus retained, resume when
native FreeBSD15 virtio DRM HOST_VISIBLE/blob/context +Mesa Venus ICD are supplied/authorized,
then real device/command/render/ownership acceptance and final p005 revalidation. Further host
compatibility or Vulkan display extensions could still require verification; boot alone proves none.
Independent p004 native OSS/wired/radioABI/WPA closure can proceed under explicit physical waiver.
Owned QEMU stopped after evidence capture, original plain fixture config restored; Venus config
archived. WIP/no push/publication; local Phase/WS decision events/outbox pending remote delivery.
