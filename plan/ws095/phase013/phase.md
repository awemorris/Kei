<!-- awesome-plan project=zedbsd record=ws095-p013 -->
# ws095-p013: 変換中の文字（preedit）を本文と同じ大きさで inline に出す

Status: cleared（q603-i01、2026-10-02、P4。QEMU の Venus で確認、実機の確認はユーザー。判定と記録は Q1）
Disposition: normal
Parent: [WS095](../ws.md)
Bug: [BUG-139](../../bugs/BUG-139.md)
Queue: q603 / q603-i01（承認: 2026-10-02 user「テキスト本文と同じ文字の大きさにしたいです。ここはUI/UXの完成度の要だと思いました。」。時限 4h）

## 範囲（2026-10-02 ユーザー）

「IMEでキーボード入力したとき、テキストエディットアプリでは、入力文字のサイズが小さくておかしかったです。アプリ側でインラインで表示しているのか、システムが表示しているのかわかりませんが、インラインであるなら、もっとなんとかならないでしょうか。テキスト本文と同じ文字の大きさにしたいです。ここはUI/UXの完成度の要だと思いました。」

1. 今の preedit の描画の経路（app の inline: `libkeiui/text-input.c`・`textedit/draw.c`、または compositor/IME の窓）を特定し、小さく見える原因を時刻つきの PNG と code で示す。
2. preedit を本文と同じ font・大きさ・行の高さで、caret の位置に inline に描く。変換中の範囲の下線、選択中の文節の強調、caret の位置を text-input-v3 の preedit の cursor に合わせる。
3. libkeiui の text-input を使う他の app（Notes、Settings の検索、Files の field）も同じ見た目になるか確かめ、共通の部品で直す。Terminal は p006。
4. QEMU の Venus で日本語の入力の PNG（変換前・変換中・候補・確定）で確認。build warning 0、C 全文規約、host の試験（libkeiui）、boot-test。実機の確認はユーザー。

## 所有 path

`userland/desktop/libkeiui/`（text-input と描画）、`userland/desktop/textedit/`、必要なら `userland/desktop/ime/`、`plan/ws095/`。

## 結果（q603-i01、2026-10-02、P4、worktree `/home/awe/zedBSD-worktrees/p4`、main fd65810dd に合わせて実行）

### 原因（範囲 1）

Text Editor の preedit は 2 つの経路があった。`textedit/main.c` の `main_preedit_draw()` が **UI の font・UI の大きさ（`TE_UI_PIXELS`）** で caret の上に白い角丸を重ねて描き（本文の文字を隠す）、
`draw.c` の `draw_cursor()` には本文の font で描く経路があったが、それが読む `app->preedit` は誰も入れていなかった。text-input の preedit は `main_preedit` にだけ入っていた。
なので変換中の文字は本文（等幅の本文の大きさ）より小さく、後ろの文字（例 world）の上に重なった（before の PNG: `tests/evidence-p013/before-converted.png`）。compositor・IME の窓は関わらない（Text Editor の inline の描画）。

### 直し（範囲 2、`userland/desktop/textedit/` だけ）

- `main.c`: overlay の `main_preedit_draw()` と `main_preedit` を消し、preedit の文字列と cursor（begin・end の byte）を `te_app` の `preedit`・`preedit_begin`・`preedit_end` に入れる（log に begin・end を足した）。
- `textedit.h`: `preedit` を `KUI_WINDOW_TEXT_MAX`（256）に、`preedit_begin`・`preedit_end` を足した。`te_app_preedit_cells()`。
- `draw.c`: cursor の行で、preedit を **本文と同じ font・大きさ・cell**（全角は 2 cell の中央）で cursor の位置に描き、cursor の後ろの文字を preedit の cell の分だけ後ろへずらす（`draw_glyphs()`・`draw_preedit()`・`draw_glyph()`）。
  変換中の文節（preedit の cursor の begin〜end）は選択の色の地と 2 px の線、他は細い線。caret は preedit の cursor に置き、文節の変換中は出さない（`draw_cursor()`）。
- `app.c`: `te_app_caret_rect()` は変換中、注目の文節（か preedit の caret）の位置を返す（IME の候補の窓の置き場所、`kui_window_text_cursor`）。`te_app_preedit_cells()`。

### 他の app（範囲 3）

libkeiui の text-input（`kui_window_text_input`）を使う app は今は **Text Editor だけ**（Notes・Settings の検索・Files の field は text-input を使っておらず、IME の入力自体がまだ無い）。
共通の部品にする対象が無いので、今回は Text Editor の中で直した。proportional の field の preedit の描き方は、それらが text-input を使う時（ws095-p007・p008）に libkeiui の field に入れるのがよい。

### 確認（範囲 4、QEMU の Venus。実機は未実施）

- image: `sh plan/ws095/tests/build-ime-textedit-image.sh build/p4-ime-img`（新しい `config-amd64-ime-textedit.mk` = IME の image ＋ textedit、font 入り）。
  直す前 `build/p4-ime-before.img`、直した後 `build/p4-ime-after.img`（SHA-256 `c4df415a8c5810bf3ea2048bcf9b47524acd19d28ace1d051e3c6ea91de356f1`）。直した後の build の warning 0。
- `GUEST_RUNTIME=build/p4-run sh plan/ws095/tests/ime-p013.sh build/p4-p013/after2` → **ime-p013: PASS**（Alt+Space で日本語、kanji → preedit `かんじ begin=9 end=9`、Space で `漢字 begin=0 end=6`、
  候補の切替、Enter で commit、watasihanihongowohanasimasu → `私は日本語を話します` の文節 `begin=0 end=6`、→ で `begin=6 end=18`、commit。Text Editor の log を host で照合。zdesktop の ERROR なし）。
  PNG: `tests/evidence-p013/after-*.png`（preedit・変換・確定・文節・次の文節）と `before-converted.png`。変換中の文字は本文と同じ大きさで inline に出て、後ろの文字がずれる。
- host: `sh plan/tools/textedit/host-core.sh` → 34/34。
- `plan/tools/boot-test.sh build/p4-ime-after.img` → PASS（`build/p4-boot/login.png`、login prompt）。
- 気づき: 候補の窓はまだ出ない（ws095-p005 の範囲）。長い文（27 字）の変換は QEMU で 1.8 秒以上かかって画面に出た（Space の後 1.8 秒の画面はまだかなのまま、3 秒後は変換済み）。IME の engine の guest での速さは別に測る価値がある。

### 未実施・残り

実機（5330）での目視（ユーザー）。Notes・Settings・Files は IME の入力自体が無い（p007・p008）。selection・検索の一致の色の帯は preedit の後ろでずれうる（変換中は通常 selection が無い）。
折り返しの行で preedit が行の右端を越える分は切れる（次の行へは回らない）。
