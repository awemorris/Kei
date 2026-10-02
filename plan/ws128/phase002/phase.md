<!-- awesome-plan project=zedbsd record=ws128-p002 -->

# ws128-p002: Notes の Open と Save As

Status: planned（2026-10-02。Queue なし）
Disposition: normal
Parent: [WS128](../ws.md)
Queue: なし
依存: なし（p001 と並列可）。libkeiui の `kui_file_chooser`（WS090 p006・p008 で PDF Viewer・Image Viewer が使う形）
目安: 2h（1 Queue）。実行者の目安: phase-runner-mid
所有 path: `userland/desktop/notes/`（`main.c` の `NOTES_ACTION_OPEN`・menu・window の chooser の結線）、`plan/ws128/tests/`

## 範囲

1. Notes の File > Open...（Ctrl+O）で `kui_file_chooser` の sheet を開き、Notes の PDF（自前の metadata あり）は編集を再開、他の PDF は WS079 p014 の「他の PDF に書き込む」の経路で開く。開く前に今の文書を保存する（自動保存の規則に合わせる）。
2. File > Save As...（Ctrl+Shift+S）で別の名前に保存し、以後はその file を編集する。
3. 「Opening from Notes is not available yet」の文言と `NOTES OPEN chooser unsupported` の log を消す。
範囲の外: libkeiui の変更（要るなら Q1 に報告して止める）、新しい file の形式。

## 受け入れ

- guest の新しい手順: Notes で書いて保存 → Open で別の PDF（自前と他の 2 種）を開き書き込み保存 → Save As で別名 → 再び開いて線が残る（log と画面）。Esc・Cancel で何も変わらない。
- 既存の `demo-s8-s9.sh` と Notes の host 試験 PASS、style-check の違反 0、build の warning 0、boot test PASS。

## 検証の方法と範囲

QEMU の Venus（pen の image の注入）。console・serial の log では判定しない。 やっていない確認は「未実施」と書く。

## 未決の判断

なし。

## Event

2026-10-02 / ws128-beta1-plan: fg019 の計画で新設。
