<!-- awesome-plan project=zedbsd record=ws005-p022 -->
# ws005-p022: ベータ1 の network の変更の全文規約の確認と回帰

Status: planning（p019〜p021 の source の変更が出揃うのを待つ）
Disposition: normal
Parent: [WS005](../ws.md)
Focused goal: fg019（ベータ1）
Queue: none
目安: 2h

## 範囲

p019〜p021 で WS005 が変えた source（networkd・net・libkeiland の network・sessiond）の全体を `plan/coding-style.md` の全文と照合し、`plan/tools/style-check.py`
とその領域の host の試験（WS005 の story、`make managed-lan-host-test`）、build（warning 0）、`plan/tools/boot-test.sh` を通す。p017 の役割（統合と最終規約）を引き継ぐ。

## 受け入れ

規約の違反 0（例外は guardrail の承認つきだけ）、試験の PASS、適用した規則・command・結果・未実施を記録。

## 所有 path

WS005 が変えた source、`plan/ws005/phase022/`。

## 依存

p019（p020・p021 で修正があればそれも）。

## 未決の判断

なし。
