<!-- awesome-plan project=zedbsd record=ws119-p005 -->
# ws119-p005: インストーラの全文規約の確認と回帰

Status: planning（p004 を待つ）
Disposition: normal
Parent: [WS119](../ws.md)
Focused goal: fg019（ベータ1）
Queue: none
目安: 2h

## 範囲

WS119 が書いた／変えた全ての source を `plan/coding-style.md` の全文と照合し、`plan/tools/style-check.py`、build（warning 0）、p002〜p004 の試験の再実行、`plan/tools/boot-test.sh`。

## 受け入れ

違反 0（またはguardrail の承認つきの例外）、試験の PASS、適用した規則・command・結果・未実施の記録。

## 所有 path

WS119 の source、`plan/ws119/`。

## 依存

p004。

## 未決の判断

なし。
