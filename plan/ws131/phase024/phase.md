<!-- awesome-plan project=zedbsd record=ws131-p024 -->

# ws131-p024: 全文規約と 3 OS の回帰、WS131 の完了

Status: planning（p002 第 2 版はユーザーのレビュー済み（2026-10-03、D7 は確認中）。開始はユーザーの承認と P2 の終了の後に Q1 が指示）
Disposition: normal
Parent: [WS131](../ws.md)、計画の正本 [design.md](../design.md)
Queue: none
依存: p003〜p023 cleared
目安: 4〜6h（1 Queue）。実行者: Q1 が割り当てる（high）
所有 path: WS131 で変えた全ての source（規約の直しだけ）、`plan/ws131/`。（Q1 の委任が要る: Q1 が Phase の前に委任を記録する）: WS131 の道具を `plan/tools/` へ移す file（Master の Tools 節の登録は Q1）

## 目的と結果

code を作る WS の必須の最終の照合。WS131 の全ての変更を [coding-style.md](../../coding-style.md) の全文と照合し（D10 の移動の例外の範囲を除く）、3 OS の build・回帰・全 checker を最終の source で流して WS131 の受け入れを判定する。

## 範囲

1. 規約: 全 commit の diff の新しい code と変えた関数を全文で照合、例外の対象と理由を記録。内部の `keiui_` の名前を揃えるかを決める。
2. checker: `check.sh`（C1〜C5・L1〜L5・B1〜B5）、`v1-check.sh`、`makefile-sync.sh`、`elf-check.sh`、`header-check.sh`、`native-build-audit.py`。故意の違反で FAIL を一度確かめる。
3. 回帰: zedBSD（build・boot-test・C1・C2・C9・GPU の境界・Settings と音・全 app）、Linux（build・install・全 app の PNG・WiFi と ALSA の probe）、FreeBSD（native build・audit・D11 の起動）。
4. ws.md を完了の形に、Phase の dir を消し、今後も使う試験と `tools/rename-map.py` の要否を決めて `plan/tools/` へ（AGENTS.md）。

## 受け入れ

- 規約の違反 0（例外は記録つき）、全ての checker PASS、回帰 PASS（未実施は理由つき）、QEMU と実機の証拠を分ける（WS131 は実機を条件にしない）。

## 検証の方法

[zedbsd-commands.md](../../tools/keiland-linux/zedbsd-commands.md)、[keiland-linux/README.md](../../tools/keiland-linux/README.md)、[keiland-freebsd/README.md](../../tools/keiland-freebsd/README.md)。QEMU の console・serial の log で判定しない（不具合は gdbstub・monitor・QMP）。build は自分の `BUILD=build/ws131-pNNN/`、共有の `build/` を消さない。FreeBSD の起動の確認は判断 D11 の範囲（passthrough なし）。QEMU と実機の証拠を分ける。

## 衝突・危険・rollback

- 衝突: — 順は Q1 が決める（design.md §7.2）。
- rollback: 前へ直すのを基本にし、後ろに同じ file の変更が無い時だけ統合の commit を revert する（design.md §7.4）。

## Resume

依存の Phase の cleared と main への統合、関係する判断の決定の後に、Q1 が Queue を作る。
