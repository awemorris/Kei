<!-- awesome-plan project=zedbsd record=ws125-p004 -->

# ws125-p004: 全文規約と回帰

Parent: [WS125](../ws.md)
Status: planning
Disposition: normal
Queue / attempts: none
Goal: WS の全変更（Makefile・patch・p002 の共通の仕組み・試験）を Guardrail と全文規約で見直し、最後の回帰と制限を整理する。
Prerequisites: p003 cleared。
Investigation bound: 2 時間。

## 範囲

- p002 で触れた Python の build tool と Makefile は近傍の形式と全文の review。新しい C があれば [coding-style](../../coding-style.md) の全文。
- V1〜V6 の再実行、boot test。制限を ws.md に書く。

## 受け入れ

違反 0 または承認済みの例外。V1〜V6 PASS。

## 検証

style の確認、guest の試験、`plan/tools/boot-test.sh`。

## 所有 path

`userland/packages/editors/vim/`、p002 の所有 path、`plan/ws125/`。

## 依存・未決の判断

p003。
