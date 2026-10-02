<!-- awesome-plan project=zedbsd record=ws095-p011 -->

# ws095-p011: 全体の規約の適合と guest の回帰

Status: planning
Disposition: normal
Parent: [WS095](../ws.md)
Queue: none
Prerequisites: p005〜p008・p012（p009/p010 は行った時だけ）
Investigation bound: timebox 2〜3h

## 範囲

WS095 の全変更の C 全文規約、formatter/style-check、zedBSD target build（warning 0）、host の engine の試験（`plan/ws095/tests/host-engine.sh`）、guest の IME の試験（`ime-guest.sh`・`ime-p004.sh`）と各 app の日本語の入力の回帰、`boot-test.sh`。実機（5330）の切替えの key と内蔵 keyboard の確認はユーザーに依頼し別に記録。

## 受け入れ

全て通り、版/結果/例外/skip/制限を記録。実機は未実施ならそう書く。

## 所有 path

`plan/ws095/`（修正が要る場合は各 Phase の path）

## 未決の判断

なし

## Standards / evidence

[Guardrail](../../guardrail.md)、[C全文規約](../../coding-style.md)、[design.md](../design.md) を変更前に読む。新しい C は全文規約。zedBSD の起動は `plan/tools/boot-test.sh` だけ、guest の操作は serial/SSH、画面は PNG で目視しユーザーに見せる。serial/console log を判定に使わない。共有 `build/` と toolchain は読取専用。全体の `make check` は禁止。QEMU の証拠と実機の証拠を分け、やっていない確認は未実施と書く。具体コマンド/結果は実行時に記録する（今は計画のみ）。

2026-10-02 / ws095-beta1-plan-20261002: 計画担当が作成。
