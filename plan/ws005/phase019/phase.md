<!-- awesome-plan project=zedbsd record=ws005-p019 -->
# ws005-p019: ベータ1 の WiFi の流れの実装

Status: planning（p018 の案とユーザーの判断を待つ）
Disposition: normal
Parent: [WS005](../ws.md)
Focused goal: fg019（ベータ1）
Queue: none
目安: 3〜4h

## 範囲

p018 の flow.md で決めた流れを最小の変更で実装する。予定の対象（p018 で確定）: `userland/base/networkd/`（所有者と store の扱い）、`userland/base/net/`、
`userland/desktop/libkeiland/zedbsd/network-zedbsd.c`（要求の順）、login 時の enable を選んだときの `userland/desktop/sessiond/`。
Settings（WS089）・system bar（WS035）の画面の source を変える必要があれば、その差分は main に依頼する（このPhaseの所有の外）。
networkd の protocol（ZNV2）の変更は p002 の固定した契約の範囲で行い、外れるなら止めて main に返す。

## 受け入れ

- 本物の networkd と偽の `wifi` の子（既存の P012 の story の仕組み）で: 所有者が root の状態から利用者の鍵つき join が通る、再起動相当（networkd の再起動と
  login）の後に p018 で決めた方法で再接続する、鍵の誤り・AP の不在・off で理由が返り daemon が止まらない。
- 既存の 30 の story（p012）と WS033 の `make managed-lan-host-test` が通る。build の warning 0。新しい code は `plan/coding-style.md` の全文。
- 最後に `plan/tools/boot-test.sh` で login prompt（PNG をユーザーに見せる）。

## 検証

変更した領域の host の試験（WS005 の story、managed-lan、libkeiland の network の試験があればそれ）と boot test。集約の `make check` は走らせない。

## 所有 path

上の範囲の source、`plan/ws005/phase019/`、`plan/ws005/tests/`。

## 依存

p018、ユーザーの判断（p018 の未決の判断）。WS033 p001 と `userland/base/networkd/`・`userland/base/net/` が重なるので同時に走らせない。

## 未決の判断

p018 の結果による。
