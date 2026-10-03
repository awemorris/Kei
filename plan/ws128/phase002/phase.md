<!-- awesome-plan project=zedbsd record=ws128-p002 -->

# ws128-p002: Notes の Open と Save As

Status: cleared（q621-i01、P1 generation4、2026-10-03。QEMU の Venus と host。結果は末尾）
Disposition: normal
Parent: [WS128](../ws.md)
Queue: q621 / q621-i01（Q1 の dispatch、2026-10-03。承認: 自走の指示（desktop の機能、planned の Phase）。時限 4 時間、p003 と同じ Queue）
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

## q621-i01 の結果（P1 generation4、2026-10-03 06:10〜06:35、base main `52c1d37ef`）

### 実装したこと（`userland/desktop/notes/`）

- **File > Open...（Ctrl+O）**: libkeiui の `kui_file_chooser`（Open、notebook の folder、filter「PDF documents」「All files」、app_id `notes`）。
  選んだ file は main loop で開く: 描いている線を終え、変更があれば先に保存（`NOTES SAVE reason=open`、保存に失敗したら開かない）、今の notebook
  （journal・document・背景の絵）を片付け、起動時と同じ `app_start_document` で開く（その file の journal があれば復元、Notes の PDF は編集を再開、
  他の PDF は WS079 p014 の「書き込む」経路）。開けない PDF は従来どおり新しい notebook と理由の status。最初の頁を窓に合わせ、title と recent を更新
  （`NOTES OPENED ...`）。同じ file は「That notebook is open」。Cancel・Esc は何もしない（`NOTES CHOOSER cancelled`）。
- **File > Save As...（Ctrl+Shift+S、menu の item と Notes の key）**: chooser の Save（notebook の名前から）。新しい path に保存し、以後はその file を
  編集する（古い path の journal は discard、新しい path の journal を作る。前の file は保存した時のまま）。他の PDF に書いている notebook も、
  元の PDF の bytes と revision を新しい file に書く（`save_update` は文書の中の base から書く）。
- 「Opening from Notes is not available yet」の文言と `NOTES OPEN chooser unsupported` の log は無くなった。冒頭の注釈も直した。

### 試験

- guest（新しい [notes-p002.sh](../tests/notes-p002.sh)、[make-foreign-pdf.py](../tests/make-foreign-pdf.py) で作る他のプログラムの PDF、Settings の試験の
  image（notes・libpdf 入り）、QEMU の Venus）→ **PASS**: 新しい notebook に線を描いて保存 → Ctrl+O で chooser（`mode=open folder=/root/Documents/Notes`）、
  `f` Enter で foreign.pdf（`kind=foreign`）→ 線を描いて保存 → Ctrl+O `n` Enter で最初の notebook（`strokes=1 kind=notes`）→ Ctrl+Shift+S で chooser
  （`mode=save`）、`copy` Enter で `SAVE reason=save-as path=.../copy.pdf` → 2 本目を描いて保存（`strokes=2`）→ Ctrl+O Esc で `CHOOSER cancelled`（開かない）
  → Notes を copy.pdf で起動し直すと `strokes=2 kind=notes`、最初の notebook は `strokes=1`、foreign.pdf は `strokes=1 kind=annotated`。log に
  「not available」が無い、zdesktop の ERROR 0。画面 `build/ws128-p002/guest/open-chooser.png`・`foreign.png`・`save-chooser.png`・`copy.png`（worktree）。
- 既存の試験: `plan/ws079/tests/run-notes-host.sh` → `run-notes-host: ok`。`demo-s8-s9.sh`（demo の image `config-amd64-demo.mk` を `build/p1-demo-notes` に
  作った）→ **PASS**（`RESULT scroll_turn_ms=140 page_frame_ms=127 page_turn_ms=450 limit=200`）。ただし script の guest の起動の後の固定の `sleep 20` では
  この host で guest が間に合わず 1 回目は全て MISSING だったので、`sleep 20` を `guest.py wait --timeout 180` に替えた一時の複写
  （`build/ws128-p002/demo-s8-s9-wait.sh`、commit していない。WS079 の file は所有の外）で流した。F-062（固定の待ち）と同じ種類。
- style-check: notes の違反 0。build: exit 0、warning 0。boot test PASS（`build/ws128-p003/boot-test/login.png`、同じ image）。

### 未実施・注意

- 範囲の「開く前に今の文書を保存する（自動保存の規則に合わせる）」は「変更があれば保存（保存の失敗で中止）」とした。
- Linux・FreeBSD の Keiland の build（notes の `Makefile.linux`・`Makefile.freebsd`、新しい file は無い）は未実施。実機（5330）は未実施（p007）。
