<!-- awesome-plan project=zedbsd record=ws109p003 -->

# ws109p003: 共有描画と FreeBSD の device/session/input 境界

Status: uncleared
Disposition: normal
Parent: [WS109](/home/awe/zedBSD-claude1/plan/ws109/ws.md)
Queue / Attempt: q569 / q569-i01

## 目的・範囲

描画本体を再利用し、確定した FreeBSD fd/sync・VT/seat/input の OS module を実装。protocol/標準 Vulkan の契約を守る。

## 完了条件

F3。Linux source を丸ごと複製した renderer を作らない。device release/fd lifetime も確認。

## 前提・未決・実行手順

依存: p002。WS の scope と acceptance、設計の未決を確認する。
技術的な細部は委任範囲で決める。対象/受け入れ/外部契約を変える結果は実装前に計画と承認範囲へ反映する。
調査→変更表と手順の確定→有限 Queue の承認→実装→指定検証→結果・WS・Master・Queue の照合。
最初の p001 は調査の案（1 session / 最大60分、満たせない点と再開条件を残す）。後続の timebox/command は設計後に選定する。

## 適用規則・影響する部品・検証

[WS の制約/部品/受け入れ](/home/awe/zedBSD-claude1/plan/ws109/ws.md)、[Guardrail](/home/awe/zedBSD-claude1/plan/guardrail.md)、
[C 規約全文](/home/awe/zedBSD-claude1/plan/coding-style.md)、[自動化](/home/awe/zedBSD-claude1/plan/standards/automation.md)を適用。
新規/変更 C は全文該当節を読み、clang-format-19 と style-check の限界を補う。
最終 conformance は全 WS の source を全文で review。build warning 0、必要な契約検証、diff-check を記録する。
具体的な build/config/tool version と script は p001 の結果で固定する。`make check` は禁止。
zedBSD の起動は boot-test.sh の PNG。Linux の既存検証は WS105 の手順と許可範囲、FreeBSD は検証環境の確定が必要。

## 証拠・結果・再開条件

未実行。command/result/commit/artifact/skipped checks はまだ無い。計画の作成を clearance としない。
再開: prerequisite の実 output と変更の所有、scope snapshot、Queue 承認を確認する。

## イベント

2026-10-01 / review-20261001-planning: 新設した Phase 案。親 WS の目標への寄与と依存を記録。実装の選定は未実施。
GitHub の Phase 作成/comment/Project の projection は公開保留、local outbox に保持する。

## 2026-10-02 / ws109-user-decisions-20261002 / このPhaseへの反映

ユーザー「drm-kmodを利用OKです。FreeBSDにも例外を適用します。FreeBSD実機は用意しておくので、作業を進めておいてください。」（2026-10-02 JST、このchat）。D1:既存FreeBSD drm-kmod利用可、GPL-free systemstack条件をこの範囲で置換。Keiland sourceの寛容license/外部実装を取り込まない境界は維持。D2:WS109専用FreeBSD QEMU guestのloopback SSH/QMP PNG検証を承認。D3:実機はユーザーが準備、入手前にnative build/backend実装を進める。実GPU/WiFi結果は将来の実機関門に残し、mock/QEMUbuildで代替しない。

shared graphics +FreeBSD OS/seat/input/backend実装を先行可。QEMUにはDRM実表示の証拠無し、realGPU/fences/device releaseのcriteriaは実機到着後にverify。partial実装Queueと全F3のclearanceを分ける。 [origin](/home/awe/zedBSD-claude1/plan/ws109/phase001/phase.md)・[WS summary](/home/awe/zedBSD-claude1/plan/ws109/ws.md)・[scope](/home/awe/zedBSD-claude1/plan/standards/ws109-native.md)。foreign Phase own検証/再開条件を更新、remote comment pending。

## q551 design reconciliation / ws109-q551-native-environment

Prerequisite is verified p002 L1 library/header output, not full F2 integration. Implement shared renderer/OS sync and native seat/evdev modules; seatd-only permissive path avoids installed libseat LGPLbasu dependency. GPU/fence/seat actual acceptance retained for physical machine. Reason: actual native dependencies and backend/link ordering inspected in p001. [Origin](/home/awe/zedBSD-claude1/plan/ws109/phase001/phase.md), [WS](/home/awe/zedBSD-claude1/plan/ws109/ws.md), [native design](/home/awe/zedBSD-claude1/plan/history/ws109/q551/environment.md). Own revised verification/resume condition saved; remote structural comment pending.

## q557 shared dma-buf/evdev/session mechanism / exact partial scope

Verified q553L1 and q556actual native service/UI/PDF output; wholeF2/F4 physical criteria
are not claimed. Maximum60min/onePhase. Reuse Linux-defined standard dma-buf protocol/Vulkan
import/render machinery by moving wayland/linux/gpu-linux.c to dmabuf/gpu-dmabuf.c;
one reservation-export operation goes behind private dmabuf/sync.h with nativeLinux/FreeBSD
sync modules using previously verified q551 fixedDRMUAPI and q553 exporter ownership convention.
No duplicate renderer. Keep identical implicit fence/ENOTTY CPU-wait fallback/CLOEXEC/errors.
This fd reservation query preserves WS105's approved dma-buf synchronization contract;
zedBSD GPUUAPI/directGPU/display ioctls remain excluded, mandatory display uses libvulkan.
Move portable Linux handoff lifecycle to session/handoff-session.c. Move Linux evdev scan,
capability/read/clock/probe mechanism to evdev/input-evdev.c, isolate seat device authority
behind small private evdev/seat.h and Linux adapters to existing direct/logind ownership.
Add the approved tiny FreeBSD native input-header selector in zwl-evdev.h; no broadOSmacros.
Native seat implementation and fullcompositor linkage are subsequent partialp003 output;
this attempt has no successful stub or physical/device authority claim.

Files: three moved mechanismC/privateheaders, wayland/{linux,freebsd}/sync OS modules,
Linux seat adapter, Makefile.linux, tiny zwl-evdev.h selector and WS109 rejected-sync
native/Linux probes/compile recipes/evidence. FullC rules for moved/changed code, no style
relocation exception. Review actual moved source against full rules and runclang-format19/
stylecheck; fix in-scope findings within timebox. Linux actual fullcompositor -j16warning0
and affected contract regression; native commonmechanism object build with real headers,
new syscall-export rejection/borrowedfd/output/errno ownership; physicalpositiveDMAfence
and actualFreeBSD seat/input/display remain F3/p005. Unsupportednativecompile prerequisites
end attempt uncleared with evidence/revised plan. No hosttoolchain/network/buildcleanup,
no push/publication, WIPcommit. FullWSconformance and nativeapps/3OS/physical gates retained.

q557 boundary tooling clarification: existing check.sh must recognize freebsd OS directories and the
shared evdev mechanism's five existing native metadata/clock request families selected by the
approved tiny input-header bridge. Scope includes required path/membership/guard-test reference
updates; no new device operation, public contract, renderer algorithm or hardware acceptance.

## Result / q557-i01 / 2026-10-01T20:40:05.930503+00:00

Queue item cleared /whole Phase uncleared。Linux共有dma-buf/evdev/sessionを再利用、FreeBSDsync/nativeinput header境界を実装。native4objects/exportownershipとLinuxfullcompositor/境界契約PASS。[result](/home/awe/zedBSD-claude1/plan/history/ws109/q557/result.md)。FreeBSDseat/fullcompositor/actualGPU・positiveDMA/全F3は保持。

Event ws109-q557-cleared: local evidence/outcome saved; remote comment (no Phase close) pending.

## q558 verified permissive seat client / exact partial scope

Use p001's existing design alternative: verified seatd-only permissive client rather than the
installed multi-backend libseat dependency on LGPLbasu. User's drm-kmod permission is not expanded.
Maximum60min/onePhase. Select official seatd0.9.3 archive (FreeBSDports authoritative SHA256
302564d54d8e28191fadfd734f2675ecb0c9e0615a58011b89ef15dfa4dbaa96,42086bytes), MITlicense.
External source stays in ignored distfile/build directories, not imported into own base/desktop
source; include original license in private install. Build only client with Meson logind/builtin/
server/examples/manpages disabled, seatd enabled, native defaultpath=/var/run/seatd.sock, /opt/keiland.
Native external build tooling Meson/Ninja may be installed only in the owned guest, host/target
compiler untouched. Produce native libseat.so.1 with ELF/native dependencies verified free of
basu/logind; configurable independent native make/package rules include dependency/private stage.

Verify real installed seatd daemon in owned guest on an exclusively owned socket and real native
evdev device; child has no supplementary groups/gid/uid65534. Direct kernel access must be denied
but daemon-authorized actual fd/capability query/close ownership must work. Headless native service
configuration (if needed) is a normal daemon configuration, never physicalVT/display evidence;
record exact config and limitations. Bound peer/client/process lifetime and restore only owned
socket/processes. No mock for this actual native IPC/permission contract; no GPU/WiFi claim.
Test/provenance/recipe and newC probe use fullstandard/formatter/style/manual. Native maincompositor
OS/seat callbacks and physicalVT/display lifecycle are nextscope; no successfulstub. F3/F2/F5 retained.
Files: independent native externalclient package/build/install rules, native makefile selection,
provenance/sourcehash/recipe/probe/evidence. Unexpected nativebuild/service prerequisite failure
ends this attempt uncleared with evidence and explicit resumecondition. WIPcommit/no push/publication.

## Result / q558-i01 / 2026-10-01T22:56:18.384491+00:00

Queue item cleared /whole Phase uncleared。MIT seatd-only libseat nativebuild/privateinstall・実seatdで非特権evdev fd/capability/release PASS。[result](/home/awe/zedBSD-claude1/plan/history/ws109/q558/result.md)。native compositor callbacks・VT/display/actualGPU/F3は保持。

Event ws109-q558-cleared: local evidence/outcome saved; remote comment (no Phase close) pending.

## q559 native seatd OS/backend / exact partial scope

Maximum60min/onePhase, verified q558 actual permissive privateclient/nativeevdev authorization,
q557 sharedinput/dma-buf/session mechanism and q556 native service/UI output. Implement native
FreeBSD seatd authority and zwl_os interface, single-owner private leases with independently closed
kernel fd/deviceID; forced supported LIBSEAT_BACKEND=seatd rejects noop/otherclient providers.
Normal FreeBSD seat service owns VT behavior; do not copy Linux console ioctls or DRMrenderer.
Explicit KEILAND_SEAT accepts seatd only (default), exact absolute KEILAND_DRM_DEVICE or native
/dev/dri/card0 selected before Vulkan's inquiry. All primary/display opens are real seatd requests,
missingDRM/seat refuses startup. No direct-root fallback/stub; Vulkan AcquireDrmDisplay/ReleaseDisplay
continues standard displayownership. Retain all successfully acquired lease owners until cleanup.
Disable notification: publishpause, quiesce frame/closeoutput before withdrawinginputs/primary,
then libseat_disable_seat ACK. Enable reacquires primary before unpause/dirty and nextinputrescan;
never reuses revoked descriptors. Real dispatch/disconnectedauthority stopsservice via normalcleanup.

Files: wayland/freebsd/seat-freebsd.{c,h}, os-freebsd.c; native compile/probe recipes and tests/evidence.
No common compositor algorithms/publicABI/otherOS changed. Native compile -j16warning0, fullC/manual/
clang-format19/stylecheck. Actual service/nativeIPC tests check nonblocking/CLOEXEC ownership,
permissiondenial/primaryabsent/partialstartup/idempotentcleanup/socketdeath/pause/reopen. Where
render/input notification collaborators are represented by bounded testrecorders, label their
limits; actual kernel/service operations remain real, not GPU/display acceptance. Production draw
functions/threeOS/fullcompositor integration and physicalVT/input/display/fences remain later gates.
An unexpected missing prerequisite ends attemptuncleared; revise separately. WIPcommit/no push/
publication/host changes. WholeF3 and p005 acceptance retained.

## Result / q559-i01 / 2026-10-01T23:11:23.567679+00:00

Queue item cleared /whole Phase uncleared。FreeBSD seatd/OS backend実装、nativeobject warning0、実VT通知/input fd停止・再取得・daemon切断/partialcleanup PASS。[result](/home/awe/zedBSD-claude1/plan/history/ws109/q559/result.md)。描画/通知collaboratorは順序観測のみ、fullnativecompositor/実GPU/physicalVT/F3は保持。

Event ws109-q559-cleared: local evidence/outcome saved; remote comment (no Phase close) pending.

## 2026-10-02 / ws109-20261002-qemu-venus-acceptance

Current user, this chat async reply: 「実機検証は不要です。qemuでVenusが使えればclearとします。」
This explicitly replaces the earlier user-provided-machine gate. Physical GPU/display/WiFi
acceptance is waived for WS109; do not request hardware or reintroduce those gates. Required
replacement evidence is actual FreeBSD QEMU Venus usage; mere host support, headless lavapipe or
Linux/zedBSD Venus does not establish that evidence. Native backend/build/standards and affected
regression obligations remain. p004 hardware radio operations become waived, not falsely tested;
native audio/wired and native radio ABI/refusal/WPA wire evidence remains classified accurately.
p003/F3 and p005/F5 replace physical display/main-app checks with owned FreeBSD QEMU Venus-backed
checks. If the native guest stack lacks a required Venus driver, investigate a bounded actual
capability chain and expose the remaining platform/scope choice; kernel/driver port is still outside
WS109's agreed scope. Current q566 standards/regression/docs subset stays authorized; Venus
configuration/implementation is selected separately after q566. No automatic WS/Phase clearance.
Origin user decision reconciled to WS/all changed Phase own criteria, Guardrail/scoped standard,
Queue supplement and docs; remote decision/structural events pending publication.

## q567 / actual native QEMU Venus capability chain

User decision supersedes physical-machine gates. Maximum45min/onePhase, consume verified
p002 native build/header/ELF output and p003 shared/native seat modules. Inspect primary current
QEMU/Mesa/FreeBSD ports/drm-kmod sources and actual installed versions/options. Configure only
owned guest to virtio-vga-gl blob/hostmem/venus and existing host rendernode where supported;
preserve original config/overlay, stop/restart owned QEMU only. Verify native guest kernel DRM
nodes/ICD and actual Vulkan Venus device commands if prerequisites exist. Native existing
packages/compatible official drivers may be used under prior drm-kmod permission. No host stack
replacement or new FreeBSD kernel/driver port, Linuxulator or lavapipe substitution. Fixture-only
config/probes/evidence; no production semantic changes planned. Unknown kernel/driver prerequisite
ends affected attempt uncleared with concrete evidence/scope options; no endless unsupported retries.
Whole F3 requires actual native Venus path and existing lifecycle/renderer obligations. p005 consumes
verified Venus output after its final conformance subset q566, not host-only capabilities. This is
internal p003 procedure refinement, unchanged p005 output/dependency commitment. WS event recorded.

## Result / q567-i01 / 2026-10-02T00:26:10.543066+00:00

Queue item uncleared /whole Phase uncleared。Actual Venus-configured FreeBSD QEMU boots, but native DRM/Venus ICD absent and latest upstream virtio driver lacks HOST_VISIBLE. [result](/home/awe/zedBSD-claude1/plan/history/ws109/q567/result.md). Kernel/driver port outside WS109; concrete acceptance/scope decision requested, physical tests waived.

Event ws109-q567-uncleared: local evidence/outcome saved; remote comment (no Phase close) pending.

## 2026-10-02 / ws109-20261002-user-i915-passthrough

Current user chat reply: 「awe@10.0.10.25 でi915をPCIパススルーして、FreeBSDゲストを実行してみましょう。」
This authorizes SSH to that specified host and an owned FreeBSD QEMU guest with existing
IrisXe0000:00:02.0 passthrough. The prior loopback-only rule has a scoped host-control exception;
guest SSH remains via remote127.0.0.1 forward, QMP PNG and real native observations, no serial
logs. Physical WiFi/user-supplied-machine request remains waived. Try this explicit native i915
GPU route to address q567 Venus prerequisite; boot alone does not clear F3/F5 or claim Venus.
Remote survey: chaos/Linux6.19.13/QEMU10.0.11/7.4GiBmemory; GPU8086:46a8 alreadyvfio-pci,
IOMMUgroup0 GPUalone, no other QEMU. Use4GiB guest, owned disk copy/overlay/endpoint. No host
GPU unbinding/reboot/kernel/library replacement or unrelated VM/process changes. Existing
native i915/drm-kmod may be installed in own guest under prior permission. New driver port
still excluded. Actual native Vulkan/DRM/provider/render/lease checks required where possible.

q568 exact partial scope: maximum45min, inspect existing remoteVFIO setup, prepare owned image
from stopped verified native-final fixture, transfer/rebase own copy, launch Q35/KVM4GiB4CPU
with exclusive vfio GPU assignment and emulated framebuffer for QMP boot observations. Install
compatible official FreeBSD drm-kmod/i915 firmware and Vulkan probe tools in own guest; verify
kernel node/physical GPU Vulkan properties and command/render chain if usable. Existing shared
WSI and seat lifecycle unchanged. Ordinary fixture configuration discretion delegated. Missing
planned prerequisites or new kernel port need end uncleared with evidence; no endless resets.

## Result / q568-i01 / 2026-10-02T01:05:38.622984+00:00

Queue item uncleared /whole Phase uncleared。Native i915/Intel Vulkan1MiB/offscreen and realunpriv compositor-shm PASS; liveDMA_BUF zeroaccessflags block ioctl/Vulkanwindow. [result](/home/awe/zedBSD-claude1/plan/history/ws109/q568/result.md) / [BUG-130](/home/awe/zedBSD-claude1/plan/bugs/BUG-130.md). Nextbounded native capability adaptation; no driverpatch/falseclear. Own remoteVMrunning, baselineVFIOretained.

Event ws109-q568-uncleared: local evidence/outcome saved; remote comment (no Phase close) pending.

## q569 native zeroaccess descriptor capability adaptation

q568 actual nativeIrisXe/IntelICD/compositor-shm prerequisites verified. Reproduced BUG-130:
realGPU-exported DMA_BUF live F_GETFD1 but F_GETFL=-1/errno0, kernel f_flags0 means any ioctl
returnsEBADF before nativebuffer driver's handlers. Kernelport excluded. Maximum40min/onePhase.
Within existing approved safe CPUcompletion/release contract, recognize exactly that unavailable
native ioctl transport in native sync wrappers: only failedEBADF, liveborrowedbufferF_GETFD and
successfulminus-oneF_GETFL/errno0 =>ENOTTY. Allothererrors/closed/readable descriptors preserved;
import also requires liveborrowed sync fd before classifying. No successful fakefence/descriptor
publication or metadata/flag mutation, no globallymaskingEBADF. SharedLinux/CPUcompletion rules
unchanged. One small privateFreeBSD-only header can share classification, never installed.
This selects no new synchronization algorithm, architecture/publicAPI or weaker wait; it supplies
native capability recognition to alreadyapproved Linux-contract fallback. Verify actualfixed
wrapped ioctl/nativepipe/closedfd/output/borrowedfd tests, realGPU mappedVulkanwindow/bytecolors/
frame callbacks/protocolcleanup. Add realinput and VT/GPUleasepause/resume if eligible in bounds.
Full Cstandard/Clang19/manual source/probe review, nativeaffected library/compositor rebuild;
sharedLinux/zedBSD source unaffected by nativeheader selection. Unknown remaining prerequisites
end uncleared and revise. p005 must include new nativeheader/wrappers/tests in finalconformance.

## Result / q569-i01 / 2026-10-02T01:21:16.708590+00:00

Queue item cleared /whole Phase uncleared。Native zeroaccess capability adaptation and actual mappedVulkan3frames/ownership PASS; realGPU VT notification/lease test failed, remains F3. [result](/home/awe/zedBSD-claude1/plan/history/ws109/q569/result.md). Next finite nativeVT integration investigation, no falseclear/kernelrepair.

Event ws109-q569-cleared: local evidence/outcome saved; remote comment (no Phase close) pending.
