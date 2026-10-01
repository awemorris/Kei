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
- Real GPU display, dma-buf/fences/seat lifecycle and actual WiFi require the
  user-provided machine; prepare implementations first and retain these gates.
- Physical device model/driver versions are recorded at that gate. A VM/headless
  Vulkan result does not supply the physical acceptance. No false clearance.
- Full C standard and near-final all-source review still apply. No WS106/107
  relocation-style exception is extended. Toolchain and unrelated builds preserved.

Expiry: WS109 end. Hardware gates remain until actual evidence, not a timeout.
