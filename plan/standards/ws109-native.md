# WS109 native FreeBSD scope / approved rules

ユーザー「drm-kmodを利用OKです。FreeBSDにも例外を適用します。FreeBSD実機は用意しておくので、作業を進めておいてください。」（2026-10-02 JST、このchat）。D1:既存FreeBSD drm-kmod利用可、GPL-free systemstack条件をこの範囲で置換。Keiland sourceの寛容license/外部実装を取り込まない境界は維持。D2:WS109専用FreeBSD QEMU guestのloopback SSH/QMP PNG検証を承認。D3:実機はユーザーが準備、入手前にnative build/backend実装を進める。実GPU/WiFi結果は将来の実機関門に残し、mock/QEMUbuildで代替しない。

## Scope and authority

This adds a WS109 FreeBSD guest boot verification exception and replaces the
GPL-free system graphics constraint with permission to use existing drm-kmod.
It does not change C style or the Keiland source license. No external driver
implementation is imported into this tree. Keep /opt/keiland, native libc and
system Vulkan behind our WSI; keep Linux rendering shared and OS code in modules.

## Verification and ownership

- FreeBSD15.1-RELEASE amd64 pinned official image, own overlay/seed/key,8GiB.
- SSH via127.0.0.1 forwarding, QMP screendump PNG inspected and shown.
- serial null; no console/serial-log based acceptance. Cleanup own processes only.
- Native compile/DESTDIR/header/ELF/loader and services verified in FreeBSD guest.
- Physical tests are waived by the later user decision below. Actual FreeBSD QEMU
  Venus usage is the replacement graphics gate; no lavapipe/other-OS substitution.
- Native QEMU audio/wired/seat checks remain real evidence; absent physical WiFi
  scan/association is waived, with native ABI/refusal/WPA wire results classified.
- Full C standard and near-final all-source review still apply. No WS106/107
  relocation-style exception is extended. Toolchain and unrelated builds preserved.

Expiry: WS109 end. FreeBSD QEMU Venus gate remains until verified evidence or a later explicit user decision.

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
