<!-- awesome-plan project=zedbsd record=ws126-p006 -->

# ws126-p006: 全文規約と回帰

Parent: [WS126](../ws.md)
Status: planning
Disposition: normal
Queue / attempts: none
Goal: WS の全変更（Makefile・patch・依存の package・試験）を Guardrail と全文規約で見直し、最後の回帰と制限を整理する。
Prerequisites: p005 cleared。
Investigation bound: 2 時間。

## 範囲

- Makefile と patch の近傍の形式、試験の Python の review、新しい C があれば [coding-style](../../coding-style.md) の全文。license の表示と provenance（同梱の第三者部分を含む）。
- P1〜P8 の再実行、boot test。制限（入れない module、`python3 -m test` の失敗の扱い、未実施の実機）を ws.md に書く。

## 受け入れ

違反 0 または承認済みの例外。P1〜P8 PASS。

## 検証

style の確認、guest の試験、`plan/tools/boot-test.sh`。

## 所有 path

`userland/packages/lang/python3/`、p004 で作った package、`plan/ws126/`。

## 依存・未決の判断

p005。
