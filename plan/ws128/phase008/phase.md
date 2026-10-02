<!-- awesome-plan project=zedbsd record=ws128-p008 -->

# ws128-p008: 全文規約と回帰（WS の最後）

Status: planning
Disposition: normal
Parent: [WS128](../ws.md)
Queue: なし
依存: 実装の Phase の全て
目安: 2h（1 Queue）。実行者の目安: phase-runner-mid
所有 path: WS128 で変えた source、`plan/ws128/`

## 範囲

WS128 で変えた全ての source を coding-style.md の全文で見直し、style-check・build（warning 0）・A1 の回帰の全て・boot test。

## 受け入れ

範囲の中の違反 0、全て PASS（A1・A6）。WS の完了の判定を Q1 に依頼（残す試験は `plan/tools/` へ）。

## 検証の方法と範囲

QEMU の Venus と host。 やっていない確認は「未実施」と書く。

## 未決の判断

なし。

## Event

2026-10-02 / ws128-beta1-plan: fg019 の計画で新設。
