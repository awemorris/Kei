<!-- awesome-plan project=zedbsd record=ws128-p003 -->

# ws128-p003: Text Editor の Find & Replace と Open Recent

Status: cleared（q621-i01、P1 generation4、2026-10-03。QEMU の Venus と host。結果は末尾）
Disposition: normal
Parent: [WS128](../ws.md)
Queue: q621 / q621-i01（Q1 の dispatch、2026-10-03。承認: 自走の指示（desktop の機能、planned の Phase）。時限 4 時間、p002 と同じ Queue）
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

## q621-i01 の結果（P1 generation4、2026-10-03 05:45〜06:35、base main `52c1d37ef`）

### 実装したこと（`userland/desktop/textedit/`）

- **Edit > Replace...（Ctrl+H）**: 窓の card の上に「Replace」の panel（libkeiui の `kui_field` 2 つ「Find」「Replace with」と `kui_button` の Replace（既定）・
  Replace All・Done）。開いた時は、選択（1 行の短い物）か最後の検索語が Find に、前の置換語が Replace with に入り、Find が空なら Find に、
  そうでなければ Replace with に keyboard。Tab で欄と button を移る。Replace with の Enter = Replace、Find の Enter = 次を検索、Esc・Done で閉じる。
  - Replace: 選択が検索語（既存の Find と同じく ASCII の大文字小文字を区別しない）なら置換語にして（1 つの undo の段）次を選ぶ。選択が一致で
    なければ次の一致を選ぶだけ（次の Replace で置換する。gedit などと同じ）。
  - Replace All: 先頭から重ならない一致を全て集め、後ろから置換し、全体を 1 つの undo group に（Undo 1 回で全て戻る）。置換語が検索語を含んでも
    再検索しない。件数を message（「Replaced 3 places」）に。
  - panel を開いている間は他の action（menu・key）を止める（他の dialog と同じ）。zdesktop が menu の shortcut として取る Ctrl+A（Select All）は、
    panel を開いている間は欄の全選択に回す。
  - 正規表現・複数 file・大文字小文字の切り替えは範囲の外（既存の Find に切り替えが無いので合わせた）。
- **File > Open Recent**: File の submenu。libkeiland の recent の一覧（`keiland_recent_list`）から Text Editor（`textedit`）が使った file を新しい順に
  最大 10 件、file の名前で。無くなった file は灰色（押すと「… is no longer there.」）。一つも無ければ灰色の「No Recent Files」。file を開いた・
  保存した時と起動時に作り直す（`te_menu_recent`、項目の id 70〜79）。選ぶと未保存の変更を先に聞き（File > Open と同じ）、その file を開く。
- menu が無い compositor 向けに Ctrl+H を editor の key にも（`edit.c`）。

### 試験

- host（`plan/tools/textedit/host-core.sh`）: 置換の 15 check と Open Recent の 4 check を足して **53/53 PASS**（Replace の選択と置換、undo で 1 つ戻る、
  Replace All の日本語の置換語・1 回の undo・1 回の redo、空の置換、日本語の検索語、検索語を含む置換語、一致無しで変化と undo の段が無い、panel の
  Replace All・空の検索語・panel 中の他の action の停止・閉じる、recent の無い file・開く・未保存の確認・Don't Save で続けて開く）。
  `host-core.sh` は ws127-p002 で libkeiui の `scroll.c` が `scroll-bar.c` を使うようになってから link に失敗していた（`kui_scroll_bar_shape` の未定義）ので、
  `scroll-bar.c` を足した（直す前 link 失敗、直した後 34/34、試験を足して 53/53）。
- guest（新しい [textedit-p003.sh](../tests/textedit-p003.sh)、Settings の試験の image（textedit 入り）、QEMU の Venus）→ **PASS**: Ctrl+H、`cat` Tab `fox`、
  Enter×4（置換 3 回）→ Esc → Ctrl+S → SSH で読んだ file が「fox fox dog fox」。Ctrl+H、Find を `fox`・置換を `ox`、Tab Tab で Replace All、Enter →
  `REPLACE all count=3` → 保存 →「ox ox dog ox」、Ctrl+Z 1 回 → 保存 →「fox fox dog fox」。終了して file 無しで起動 → `MENU recent count=1`、
  F10 → File → Open Recent → `t.txt` → `RECENT open index=0 path=/root/t.txt`・`OPEN path=/root/t.txt`。画面 `build/ws128-p003/guest/replace.png`・
  `replace-all.png`・`recent-menu.png`・`recent-opened.png`（worktree）。
- style-check: textedit の違反は直す前と同じ 4（既存の app.c 1・draw.c 3、新しい違反 0）。`host-core.c` も直す前と同じ 36（既存の試験の形）。
- build: Settings の試験の image（`build/p1-settings`）exit 0、warning 0（`-Werror`）。boot test PASS（`build/ws128-p003/boot-test/login.png`）。

### 未実施・注意

- Open Recent の灰色（無くなった file）は host の試験だけ（guest の画面では未確認）。panel の欄の見出しは placeholder だけ（入力すると消える）。
- Linux・FreeBSD の Keiland の build（textedit の `Makefile.linux`・`Makefile.freebsd`、新しい file は無い）は未実施。実機は未実施。
