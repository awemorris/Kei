<!-- awesome-plan project=zedbsd record=ws131-p009 -->

# ws131-p009: backend の GPU の buffer の領域と境界の確定

Status: planning（p002 第 2 版はユーザーのレビュー済み（2026-10-03、D7 は確認中）。開始はユーザーの承認と P2 の終了の後に Q1 が指示）
Disposition: normal
Parent: [WS131](../ws.md)、計画の正本 [design.md](../design.md)
Queue: none
依存: p008 cleared。判断 D14（決定済み）
目安: 4〜5h（1 Queue）。実行者: Q1 が割り当てる（high）
所有 path: `userland/desktop/wayland/`（`zedbsd/gpu-*`、`dmabuf/`、`linux/sync-linux.c`・`freebsd/sync-freebsd.c`、`zwl-gpu.h`、`protocol.c`・`objects.c`・`import.c`・`compose.c` の結線、OS の data の `data/` への移動）、`userland/desktop/libkeiland-backend*/`、`plan/ws131/`。（Q1 の委任が要る: Q1 が Phase の前に委任を記録する）: `plan/tools/gpu-boundary/`・`plan/tools/keiland-os-boundary/`・`plan/tools/keiland-freebsd/dmabuf-export-rejected.c`

## 目的と結果

GPU の buffer の protocol（zedBSD の `keiland_gpu_buffer_v1`、Linux・FreeBSD の `zwp_linux_dmabuf_v1`）を backend へ移し（2026-10-03 user の D2）、compositor は wl_buffer の寿命と OS に依らない画像の型だけを持つ。逆向きの依存は compositor が渡す protocol の host の interface で解く（design.md §3.5）。終わると compositor の tree に OS の directory が無く、配置の Guardrail と checker を確定する。

## 範囲

1. 最初の 30 分で `protocol.c:188`・`:632`・`:1560`、`objects.c:571`、`import.c` の import の呼び出しを読み直し、design.md §3.5 の protocol の host の 11 操作に足りない物を足して phase.md に書く。
2. compositor: `struct kl_backend_protocol_host` の実装（wire の object を不透明な resource として渡す）、global の表の 3 番を backend に問う形に。
3. `git mv`: `zedbsd/gpu-buffer-zedbsd.c`・`gpu-zedbsd.*` → backend-zedbsd、`dmabuf/*` → `libkeiland-backend/dmabuf/`（Linux・FreeBSD の system の Vulkan の header で compile）、`linux/sync-linux.c`・`freebsd/sync-freebsd.c` → 各 backend。`zwl_buffer_layout` は backend-zedbsd の中だけ。
4. OS ごとの install の data（`linux/apps.conf.in`・`keiland.desktop`・`freebsd/apps.conf.in`）を `wayland/data/` へ。
5. X server（D14、2026-10-03 user）: `userland/desktop/xserver/keymap.c` の `<uapi/input.h>` の include を除き、Wayland の規格（evdev）の key code の定数を X server の自分の header に持たせる（値は OS に依らない）。所有 path に `userland/desktop/xserver/keymap.c` と新しい header を加える（Q1 の委任が要る）。
6. checker の確定（design.md §3.8、D14 の許可の表）と `v1-check.sh` の改訂。故意の違反で FAIL・exit 1、戻して PASS を確かめる。配置の Guardrail の本文を Q1 に渡す（§3.7）。

## 受け入れ

- 3 OS の build（zedBSD の amd64 は exit 0・自前の warning 0、`make keiland-linux` の gcc と clang、FreeBSD の native build と `native-build-audit.py`）と `plan/tools/keiland-os-boundary/check.sh` PASS。
- `find userland/desktop/wayland -type d` に `zedbsd`・`linux`・`freebsd`・`dmabuf`・`evdev`・`session` が無い。compositor の 3 つの Makefile の source の一覧が一致。`v1-check.sh` PASS。
- zedBSD: boot-test、C1・C2・C9、GPU の境界の試験（zedbsd-commands.md §6、forge・fence の guest）。Linux: `dmabuf-probe`・`dmabuf-forge`・`wsi-check.sh`（90 frame ×4）・acquire-fence。FreeBSD: `dmabuf-export-rejected.c` の native の実行、起動は D11。

## 検証の方法

[zedbsd-commands.md](../../tools/keiland-linux/zedbsd-commands.md)、[keiland-linux/README.md](../../tools/keiland-linux/README.md)、[keiland-freebsd/README.md](../../tools/keiland-freebsd/README.md)。QEMU の console・serial の log で判定しない（不具合は gdbstub・monitor・QMP）。build は自分の `BUILD=build/ws131-pNNN/`、共有の `build/` を消さない。FreeBSD の起動の確認は判断 D11 の範囲（passthrough なし）。QEMU と実機の証拠を分ける。

## 衝突・危険・rollback

- 衝突: WS113 p004 と同時に流さない。WS103 の道具の path を変える。 順は Q1 が決める（design.md §7.2）。
- rollback: 前へ直すのを基本にし、後ろに同じ file の変更が無い時だけ統合の commit を revert する（design.md §7.4）。

## Resume

依存の Phase の cleared と main への統合、関係する判断の決定の後に、Q1 が Queue を作る。
