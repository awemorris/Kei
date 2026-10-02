<!-- awesome-plan project=zedbsd record=ws033-p003 -->
# ws033-p003: WS033 の source の全文規約の確認

Status: planned
Disposition: normal
Parent: [WS033](../ws.md)
Focused goal: fg019（ベータ1）
Queue: none（未承認）
目安: 2h

## 範囲

WS033 は ws.md と conformance の Phase を持たずに実行された。WS033 が書いた／変えた source（`userland/base/networkd/managed-lan.c`・`managed-lan.h`、
`userland/base/networkd/main.c` の有線の管理の部分、`userland/base/net/main.c` の `lan_command`・`startup_command`、init の setting の許可の表、
`userland/base/init/services/networking`）を git の履歴で特定し、`plan/coding-style.md` の全文と照合する。`plan/tools/style-check.py`、
`make managed-lan-host-test`、build（warning 0）、最後に `plan/tools/boot-test.sh`。

## 受け入れ

違反 0（またはguardrail の承認つきの例外）、試験の PASS、適用した規則・command・結果・未実施を記録。コードの意味を変えない修正に限る（意味が変わる修正が要るなら p001 に回す）。

## 所有 path

上の source、`plan/ws033/`。

## 依存

p001 の修正がある場合はその後（同じ file を触るので同時に走らせない）。

## 未決の判断

なし。
