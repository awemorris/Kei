<!-- awesome-plan project=zedbsd record=ws131-p019 -->

# ws131-p019: Settings の窓を新しい API へ

Status: planning（p002 第 2 版はユーザーのレビュー済み（2026-10-03、D7 は確認中）。開始はユーザーの承認と P2 の終了の後に Q1 が指示）
Disposition: normal
Parent: [WS131](../ws.md)、計画の正本 [design.md](../design.md)
Queue: none
依存: p011・p016 cleared。旧 ws090-p007 の窓の部分はこの Phase（D9 の決定）。WS090 の他の残りは WS131 の完了の後
目安: 4h（1 Queue）。実行者: Q1 が割り当てる（high）
所有 path: `userland/desktop/settings/`（`window.c`・`present.c`・`menu.c`・`titlebar.c`・`glass.c`・`main.c`、Makefile 3 本）、`plan/ws131/`

## 目的と結果

Settings の自前の窓（`window.c` 1,350 行）と present（`present.c` 1,218 行）を除き、`kl_app` と宣言的な部品へ。Files の canvas・text・icons の source の共有の置き換えは描画が byte で一致する時だけ。

## 範囲

1. 窓・present・入力を libkeiland へ、menu（371）・titlebar（358）・glass（162）を宣言的に、`kl_app_system()` を使う。

## 受け入れ

- 3 OS の build（zedBSD の amd64 は exit 0・自前の warning 0、`make keiland-linux` の gcc と clang、FreeBSD の native build と `native-build-audit.py`）と `plan/tools/keiland-os-boundary/check.sh` PASS。
- 対象の app に旧名（`kui_`・`KUI_`・`keiland_`・`KEILAND_`）と自前の menu・titlebar・glass の結線・main loop が無い（grep）。結線の行数の減りを記録。
- zedBSD: `plan/ws089/tests/host-build.sh`・`settings-regress.sh`・`settings-p007.sh`、`volume-p005.sh`、boot-test。Linux: Settings の全頁の PNG。

## 検証の方法

[zedbsd-commands.md](../../tools/keiland-linux/zedbsd-commands.md)、[keiland-linux/README.md](../../tools/keiland-linux/README.md)、[keiland-freebsd/README.md](../../tools/keiland-freebsd/README.md)。QEMU の console・serial の log で判定しない（不具合は gdbstub・monitor・QMP）。build は自分の `BUILD=build/ws131-pNNN/`、共有の `build/` を消さない。FreeBSD の起動の確認は判断 D11 の範囲（passthrough なし）。QEMU と実機の証拠を分ける。

## 衝突・危険・rollback

- 衝突: WS089・WS113 p006 と同じ file。 順は Q1 が決める（design.md §7.2）。
- rollback: 前へ直すのを基本にし、後ろに同じ file の変更が無い時だけ統合の commit を revert する（design.md §7.4）。

## Resume

依存の Phase の cleared と main への統合、関係する判断の決定の後に、Q1 が Queue を作る。
