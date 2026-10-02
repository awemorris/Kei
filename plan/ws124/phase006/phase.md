<!-- awesome-plan project=zedbsd record=ws124-p006 -->

# ws124-p006: 全文規約と回帰

Parent: [WS124](../ws.md)
Status: planning
Disposition: normal
Queue / attempts: none
Goal: WS の全変更（Makefile・patch・試験・文書）を Guardrail と全文規約で見直し、最後の回帰と制限を整理する。
Prerequisites: p004 cleared（p005 を行うならそれも）。
Investigation bound: 2 時間。

## 範囲

- [coding-style](../../coding-style.md)（新しい C があれば全文）、Makefile と patch の近傍の形式、license の表示と provenance の完全さ。
- E1〜E6 の再実行、boot test。制限（GUI 無し、無効にした機能、未実施の実機）を ws.md に書く。

## 受け入れ

違反 0、または承認済みの例外。E1〜E6 PASS。WS の完了の形に ws.md を書き直せる状態。

## 検証

style の確認、guest の試験、`plan/tools/boot-test.sh`。

## 所有 path

`userland/packages/editors/emacs/`、`plan/ws124/`。

## 依存・未決の判断

p004（p005）。
