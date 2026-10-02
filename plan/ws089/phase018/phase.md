<!-- awesome-plan project=zedbsd record=ws089-p018 -->

# ws089-p018: 全文規約と回帰（WS の最後）

Status: planning（最後）
Disposition: normal
Parent: [WS089](../ws.md)
Queue: なし
依存: 選んだ実装の Phase の全て
目安: 2h（1 Queue）。実行者の目安: phase-runner-mid
所有 path: WS089 で変えた source（p010 以後）、`plan/ws089/`

## 範囲

p010 以後に WS089 で変えた source を coding-style.md の全文で見直し、style-check・build（warning 0）・`settings-regress.sh`・`volume-p005.sh`・host・boot test。完了の処理（試験の `plan/tools/settings/` への移し、[guide.md](../guide.md) §3.3 の順の制約: WS104 p001・p003 の適用の後）は Q1。

## 受け入れ

範囲の中の違反 0、全て PASS（S-B1・S-B6）。WS の完了の判定を Q1 に依頼。

## 検証の方法と範囲

QEMU の Venus。 やっていない確認は「未実施」と書く。

## 未決の判断

なし。

## Event

2026-10-02 / ws089-beta1-plan: fg019 の計画で新設。
