<!-- awesome-plan project=zedbsd record=ws131-p016 -->

# ws131-p016: Text Editor を新しい API へ（最初の移行）

Status: planning（p002 第 2 版はユーザーのレビュー済み（2026-10-03、D7 は確認中）。開始はユーザーの承認と P2 の終了の後に Q1 が指示）
Disposition: normal
Parent: [WS131](../ws.md)、計画の正本 [design.md](../design.md)
Queue: none
依存: p015 cleared。D8 の単独走行で番号の順
目安: 3〜4h（1 Queue）。実行者: Q1 が割り当てる（high）
所有 path: `userland/desktop/textedit/`、API の不足の補い（`libkeiland/app/`・`ui/`）、`plan/ws131/`

## 目的と結果

Text Editor（部品・IME・chooser・inset・編集の操作を全て使う）を最初に `kl_app` と宣言的な部品へ移して API を確かめる。

## 範囲

1. `<keiland.h>` と新名に。`menu.c`（540 行の結線）・`titlebar.c`（313）・`glass.c`（112）・main loop を宣言的な表と `kl_app_dispatch`・`take` へ。
2. 見つかった API の不足は libkeiland に足して記録。

## 受け入れ

- 3 OS の build（zedBSD の amd64 は exit 0・自前の warning 0、`make keiland-linux` の gcc と clang、FreeBSD の native build と `native-build-audit.py`）と `plan/tools/keiland-os-boundary/check.sh` PASS。
- 対象の app に旧名（`kui_`・`KUI_`・`keiland_`・`KEILAND_`）と自前の menu・titlebar・glass の結線・main loop が無い（grep）。結線の行数の減りを記録。
- zedBSD: `plan/tools/textedit/host-core.sh`、`textinput-p013.sh`、`plan/ws090/tests/sheet-guest.sh`、`plan/ws102/tests/edit-guest.sh`・`inset-guest.sh`、`plan/ws128/tests/textedit-p003.sh`、boot-test。Linux: Text Editor の起動と入力の PNG。

## 検証の方法

[zedbsd-commands.md](../../tools/keiland-linux/zedbsd-commands.md)、[keiland-linux/README.md](../../tools/keiland-linux/README.md)、[keiland-freebsd/README.md](../../tools/keiland-freebsd/README.md)。QEMU の console・serial の log で判定しない（不具合は gdbstub・monitor・QMP）。build は自分の `BUILD=build/ws131-pNNN/`、共有の `build/` を消さない。FreeBSD の起動の確認は判断 D11 の範囲（passthrough なし）。QEMU と実機の証拠を分ける。

## 衝突・危険・rollback

- 衝突: WS128 と同じ file。 順は Q1 が決める（design.md §7.2）。
- rollback: 前へ直すのを基本にし、後ろに同じ file の変更が無い時だけ統合の commit を revert する（design.md §7.4）。

## Resume

依存の Phase の cleared と main への統合、関係する判断の決定の後に、Q1 が Queue を作る。
