# q568 / real Intel GPU passthrough and native synchronization blocker

Item uncleared /whole p003 uncleared; WS incomplete. User selected awe@10.0.10.25 native i915
passthrough instead of waiting for missing Venus. No false Venus/whole graphics clearance.

## Actual environment / successful subset

Host chaos/Linux6.19.13/QEMU10.0.11, IrisXe8086:46a8/0000:00:02.0 alreadyvfio-pci, exclusive
IOMMUgroup0, no other VM/DRMconsumer. No host unbinding/reboot/kernel/package/driver changes.
Stopped verified local FreeBSD fixture converted to independently backed compressed20GiBqcow2,
2.92GiBactual, transferred with exactSHA256 match. Owned remote /home/awe/ws109-freebsd-fixture.
Q35/KVM4CPU4GiB, Intel GPU at00:02.0, emulatedVGA separate, guest loopback47969 reached via
SSHProxyJump named authorized host/local key. No private key copied or serial log interpretation.
Initial DMAmap warning: actualIOMMU39bits vs defaultCPUhost highBAR; corrected host-phys-bits-limit39
and consoleVGAat0:1 before loading driver, owned clean shutdown/restart. Subsequent QMP running/
emulator stderr empty. Native SSH kern.osrelease15.1-RELEASE-p4 and pciconf actual46a8 confirmed.
QMP PNGs inspected/shown; initial console did not initialize; afterdriver shows kernel output,
not claimed as visible final login or physical screen. NativeSSH provides runningOS evidence.

Only own guest packages: drm-66-kmod6.6.25.1501000_8 (FreeBSD15.1 build), Intel alderlake firmware
20260519.1500068, vulkan-tools1.4.356 and drm_info. Existing drm-kmod permission covers driver;
GPLv2/BSD2/MIT systemlicenses separate from KeilandZlib. No kernel/driver implementation imported.
Native kldload i915kms succeeds; actual card0/renderD128 node, hw.dri.busid/name/modesetting.
IntelICD26.1.3 selects real IrisXe, driverIDIntelMesa/typeINTEGRATED_GPU; no lavapipe substitution.
Keiland installed private libvulkan +absolute systemloader +Intel ICD actual1MiBfill/copy/fence
PASS. Actual vkdemo texturedGPUoffscreenreadback/digests PASS and resulting image inspected/shown.
Real unprivileged nobody+video compositor +nativeVT-boundseatd starts Intel1920x1080@48.009Hz,
shmclient3frames completes, compositor5..7realdisplayframes and cleanup_failed0/exit0. Actual
input devices leased, no synthetic collaborators. No injected-input/realpause-resume yet.

## Failed native Wayland DMA-BUF path

Initial fixture incorrectly used vkdemo as a Wayland client; it is a direct-display application
and cannot take the alreadyowned primary. Replaced with public WSI probe. Original registered
probe lacked xdg role; new owning wsi-window-freebsd.c binds/acknowledges realxdg-toplevel,
retains cleanup/error/ownership, records physicaldevice and rejectsCPU. Stillfails:
realDMA_BUF imports3images, create-swapchain/get-images succeed; firstvkAcquireNextImageKHR
returnsSURFACE_LOST. Realtruss shows EXPORT_SYNC_FILE on livefd returnsEBADF9 beforecommit.
Tracewrapper itself exits0 even when childfails; old fixture printed a falsePASS. This is explicitly
rejected as evidence. Final fixture additionally requires client PASSmarker; allfailedreceipts
retained, no Phase cleared from wrapperstatus. Driver/renderer not repaired in this attempt.

Independent actual Vulkan64x64buffer export: F_GETFD1/CLOEXEC, F_GETFL=-1 with errno0, fstat
returns0 (driver stat fields unfinished/nottrusted), nativeproductionEXPORT gives-1/EBADF9,
output999unchanged. Native driver [fixedsource](https://github.com/freebsd/drm-kmod/blob/drm_v6.6.25_13/drivers/dma-buf/dma-buf.c)
creates the file with zeroaccessflags, ignoringexportflags. This explains FreeBSD syscall-level
ioctl refusal and unusual successful flags query. Actual installedpackage also reproduces this
fingerprint. No arbitrary EBADF fallback or driver patch applied. [BUG-130](../../../bugs/BUG-130.md)
tracks reproduced upstream defect, not a verifiedrepair/nonblocking transfer of failedF3.

## Source review / next bounded work

New probes reuse real existing exporter/WSI contracts, no fake provider/productiontestswitch.
Full C rules/manual declarations/static order/paragraphs/errors/fd/rolecleanup reviewed; formatter
Clang19.1.7 ColumnLimit0 +canonicaldefinitions; style-check0, native-Wall-Wextra-Werrorcompile0.
Owning fixture bytecompile/manual process/socket/temp/prefix cleanup review; near-finalp005
must include these addedtests. No production source changed/q566 regressions remain unchanged.

Nextp003 finiteQueue: native-only recognize precisely livezeroaccess descriptors (F_GETFD>=0,
F_GETFL=-1 with errno0) on failedEBADF and classify unavailableioctl as ENOTTY, reusing existing
verified CPUcompletion/release fallback. Closed/ordinary/descriptors and allothererrors remain
unchanged; outputfd untouched, never counterfeit payload/success. No weaker sharedCPUwait or
kernel port/newpublicAPI. Verify realGPU window3colors/framecallbacks/borrowedfd/error tests,
then actualinput/VTlifecycle as scope permits. If this safe existing fallback cannot meet ownership
contract, expose humanchoice instead of skipping it. WholeF3/F5 remain unmet; no hardware waiver
reinstated, no claim that Venus ran. Remote own VM remainsrunning for next authorizedQueue;
local fixture stopped, GPU binding baselinevfio unchanged. WIP/no push/publication; outbox pending.
