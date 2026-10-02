<!-- awesome-plan project=zedbsd record=ws095-p006 -->

# ws095-p006: Terminal の text-input と CJK の font

Status: planned（p005 cleared の後）
Disposition: normal
Parent: [WS095](../ws.md)
Queue: none
Prerequisites: p005（候補の窓）。D14 は本 Phase の中で小さく足す案（design §14）
Investigation bound: timebox 3〜4h

## 範囲

- Terminal（`userland/desktop/terminal/`、libkeiui の window）で `kui_window_text_input`（WS090 p013）を使い、preedit を cursor の位置に表示、commit を pty へ書く。cursor の矩形を text-input に送る（候補の窓の位置）。
- password の入力の検出（tty の ECHO が off の時は text input を無効にし、content purpose を password にする。design §4）。
- D14: Terminal の CJK の fallback の font（DroidSansFallback 等、既に image にある font を使う）を足す。Terminal の担当の WS（WS128 標準アプリ全般）と file が重なる場合は main が調整。

## 受け入れ

zedBSD QEMU の Terminal で日本語の入力・確定・表示（`echo 日本語` の結果）、`passwd` 等の ECHO off の間は IME が切替わらない、PNG。Terminal の既存の試験（host の試験があればそれ）に回帰無し、`boot-test.sh`。

## 所有 path

`userland/desktop/terminal/`、必要なら `userland/desktop/libkeiui/text-input.c`（共有 library、main に確認）、`plan/ws095/`

## 未決の判断

D14 の font を WS095 で足すか Terminal の WS に任せるか（design §14 の既定は WS095 で小さく足す）

## Standards / evidence

[Guardrail](../../guardrail.md)、[C全文規約](../../coding-style.md)、[design.md](../design.md) を変更前に読む。新しい C は全文規約。zedBSD の起動は `plan/tools/boot-test.sh` だけ、guest の操作は serial/SSH、画面は PNG で目視しユーザーに見せる。serial/console log を判定に使わない。共有 `build/` と toolchain は読取専用。全体の `make check` は禁止。QEMU の証拠と実機の証拠を分け、やっていない確認は未実施と書く。具体コマンド/結果は実行時に記録する（今は計画のみ）。

2026-10-02 / ws095-beta1-plan-20261002: 計画担当が作成。
