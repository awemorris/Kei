<!-- awesome-plan project=zedbsd record=ws128-p003 -->

# ws128-p003: Text Editor の Find & Replace と Open Recent

Status: planned（2026-10-02。Queue なし）
Disposition: normal
Parent: [WS128](../ws.md)
Queue: なし
依存: なし（p001 と並列可）
目安: 2h（1 Queue）。実行者の目安: phase-runner-mid
所有 path: `userland/desktop/textedit/`、`plan/tools/textedit/host-core.c`（試験を足す）、`plan/ws128/tests/`

## 範囲

1. Edit > Replace...（Ctrl+H）: 既存の Find の欄に置換の欄を足し、Replace（次の一致を置換して進む）・Replace All（1 回の undo で戻る）。大文字小文字の区別の切り替え（既存の Find に合わせる）。
2. File > Open Recent: libkeiland の recent の API（Files が使う物）で最近の 10 件、無い file は灰色。
範囲の外: 正規表現、複数 file の検索、encoding。

## 受け入れ

- host（`host-core.sh`）に置換の試験（Replace・Replace All・undo で元に戻る・日本語の文字列・空の置換）を足して PASS。
- guest: Ctrl+H で置換して保存した file の中身が期待どおり（SSH で読む）、Open Recent に直前の file が出て開ける。
- style-check の違反 0、build の warning 0、boot test PASS。

## 検証の方法と範囲

host と QEMU の Venus。 やっていない確認は「未実施」と書く。

## 未決の判断

なし。

## Event

2026-10-02 / ws128-beta1-plan: fg019 の計画で新設。
