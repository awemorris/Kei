<!-- awesome-plan project=zedbsd record=ws127-p005 -->

# ws127-p005: 日本語の UI と、名前の変更の IME（F-041 の残り）

Status: planning（2026-10-02 ベータ1の計画。要る判断・成果は「未決の判断」と「依存」）
Disposition: normal
Parent: [WS127](../ws.md)
Queue: なし
依存: p001、WS095（IME の現状）、WS089 p016 と仕組みを共有（先に作った側に合わせる）
目安: 4h（1 Queue）。実行者の目安: phase-runner-mid
所有 path: `files/` の文言の所・`ui-field.c`、共有の仕組みの置き場所は Q1 が決める（新しい library なら所有の承認）

## 範囲

(1) UI の文言を表（message の catalog）に出し、`LANG=ja_JP.UTF-8` で日本語にする最小の仕組み（Settings と共有の小さな library か header）。(2) 名前の変更の欄（`ui-field.c`）で text-input-v3 の IME（WS095）の組み立てと確定。

## 受け入れ

`LANG=ja_JP.UTF-8` で menu・sidebar・dialog の文言が日本語（画面）、英語の既定が変わらない（既存の回帰 PASS）、IME で「漢字.txt」に名前を変えられる（guest）。
変えた source の style-check の違反 0、build の warning 0、`files-regress.sh` の 14 本と boot test PASS。

## 未決の判断

ユーザー: 日本語の UI をベータ1 に入れるか。仕組みの置き場所（libkeiui か新しい小さな library か）

## 検証の方法と範囲

QEMU の Venus（`plan/tools/files/files-guest.sh`、`files-regress.sh`）と host の試験。console・serial の log では判定しない。各 Phase の最後に `files-regress.sh` の 14 本と boot test。
Settings と共有の `canvas.c`・`text.c`・`icons.c` を変えたら `sh plan/ws089/tests/host-build.sh` と Settings の host 試験も流す。

## Event

2026-10-02 / ws127-beta1-plan: fg019 の計画で新設（planning）。p001 の結果とユーザーの選択で範囲を確定し planned にする。
