<!-- awesome-plan project=zedbsd record=ws131-p015 -->

# ws131-p015: app の骨組みの API（kl_app）

Status: planning（p002 第 2 版はユーザーのレビュー済み（2026-10-03、D7 は確認中）。開始はユーザーの承認と P2 の終了の後に Q1 が指示）
Disposition: normal
Parent: [WS131](../ws.md)、計画の正本 [design.md](../design.md)
Queue: none
依存: p014・p010 cleared
目安: 4〜5h（1 Queue）。実行者: Q1 が割り当てる（high）
所有 path: `userland/desktop/libkeiland/`（新しい `app/`、`ui/window.c` の app への分け、宣言的な層）、`keiland-ui.h`、exports.map、`userland/tests/kuidemo/`、`plan/ws131/tests/`、`plan/ws131/`

## 目的と結果

design.md §6 の `kl_app`（一つの registry と一度の roundtrip、pull 型の event loop、fd の監視、一つの event の queue、`kl_app_system()`）と、宣言的な menu・titlebar・glass と action の queue を足す。既存の `kl_window_open()` は暗黙の app を作る形で残す。

## 範囲

1. `kl_app_*`、`kl_app_window_create`、`kl_window_set_menu`・`set_controls`・`set_action_state`・`popup_menu`・`set_glass`、`kl_window_vulkan_surface`（`VK_VERSION_1_0` の条件、review 20）。
2. registry の一本化（titlebar・menu・glass・edit・inset・text-input・data device・primary）。
3. host の試験（表 → model、差分だけ送る、bind の回数）。kuidemo を新 API の見本に。

## 受け入れ

- 3 OS の build（zedBSD の amd64 は exit 0・自前の warning 0、`make keiland-linux` の gcc と clang、FreeBSD の native build と `native-build-audit.py`）と `plan/tools/keiland-os-boundary/check.sh` PASS。
- 新しい host の試験 PASS、kuidemo が新 API だけで動く（zedBSD と Linux の PNG）。Text Editor の試験一式（互換のまま）PASS、boot-test。BUG-111・BUG-112・IME の順・touch の時刻の補正の退行が無い。

## 検証の方法

[zedbsd-commands.md](../../tools/keiland-linux/zedbsd-commands.md)、[keiland-linux/README.md](../../tools/keiland-linux/README.md)、[keiland-freebsd/README.md](../../tools/keiland-freebsd/README.md)。QEMU の console・serial の log で判定しない（不具合は gdbstub・monitor・QMP）。build は自分の `BUILD=build/ws131-pNNN/`、共有の `build/` を消さない。FreeBSD の起動の確認は判断 D11 の範囲（passthrough なし）。QEMU と実機の証拠を分ける。

## 衝突・危険・rollback

- 衝突: — 順は Q1 が決める（design.md §7.2）。
- rollback: 前へ直すのを基本にし、後ろに同じ file の変更が無い時だけ統合の commit を revert する（design.md §7.4）。

## Resume

依存の Phase の cleared と main への統合、関係する判断の決定の後に、Q1 が Queue を作る。
