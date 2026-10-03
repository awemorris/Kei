<!-- awesome-plan project=zedbsd record=ws131-p008 -->

# ws131-p008: backend の表示の領域

Status: planning（p002 第 2 版はユーザーのレビュー済み（2026-10-03、D7 は確認中）。開始はユーザーの承認と P2 の終了の後に Q1 が指示）
Disposition: normal
Parent: [WS131](../ws.md)、計画の正本 [design.md](../design.md)
Queue: none
依存: p007 cleared
目安: 3〜4h（1 Queue）。実行者: Q1 が割り当てる（high）
所有 path: `userland/desktop/wayland/`（`zedbsd/os-zedbsd.c`、`linux/os-linux.c`・`freebsd/os-freebsd.c` の残り、`zwl-os.h`、`compose.c`・`display.c` の結線）、`userland/desktop/libkeiland-backend*/`、`plan/ws131/`

## 目的と結果

表示の node（Linux・FreeBSD の primary node）と Vulkan の display の取得・解放を backend へ移す。backend は compositor が渡す Vulkan の instance と proc addr だけを使う。

## 範囲

1. interface に display（`kl_backend_display_node`・`display_acquire`・`display_release`、`struct kl_backend_vulkan`）。
2. `git mv`: `zedbsd/os-zedbsd.c` と `os-linux.c`・`os-freebsd.c` の残り → 各 backend。`zwl-os.h` を除く。

## 受け入れ

- 3 OS の build（zedBSD の amd64 は exit 0・自前の warning 0、`make keiland-linux` の gcc と clang、FreeBSD の native build と `native-build-audit.py`）と `plan/tools/keiland-os-boundary/check.sh` PASS。
- zedBSD: boot-test、C1・C2・C9。Linux: KMS の確認（`display-probe.c`）と compositor の PNG。

## 検証の方法

[zedbsd-commands.md](../../tools/keiland-linux/zedbsd-commands.md)、[keiland-linux/README.md](../../tools/keiland-linux/README.md)、[keiland-freebsd/README.md](../../tools/keiland-freebsd/README.md)。QEMU の console・serial の log で判定しない（不具合は gdbstub・monitor・QMP）。build は自分の `BUILD=build/ws131-pNNN/`、共有の `build/` を消さない。FreeBSD の起動の確認は判断 D11 の範囲（passthrough なし）。QEMU と実機の証拠を分ける。

## 衝突・危険・rollback

- 衝突: WS113 p004（compositor の出力）と同時に流さない。 順は Q1 が決める（design.md §7.2）。
- rollback: 前へ直すのを基本にし、後ろに同じ file の変更が無い時だけ統合の commit を revert する（design.md §7.4）。

## Resume

依存の Phase の cleared と main への統合、関係する判断の決定の後に、Q1 が Queue を作る。
