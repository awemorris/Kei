<!-- awesome-plan project=zedbsd record=ws095-p005 -->

# ws095-p005: 候補の窓と indicator

Status: uncleared（2026-09-29、ユーザーの指示で中断。2026-10-02 新 attempt を Queue 投入可）
Disposition: normal
Parent: [WS095](../ws.md)
Queue: main が割り当て（2026-09-29、worktree `.claude/worktrees/ws095-ime`、branch `wt/ws095`）。Queue の ID は main が記録する

## 範囲

[design.md](../design.md) §4.3・§8・§13 の p005:
- 候補の窓（IME の input popup surface）を zdesktop が合成する。plain と glass の両方、全画面を含む。
- IME の program が候補の窓を描く。
- top bar の言語の indicator（A／あ）と、その click。
- IME の中の key の repeat。
- 画面で確かめる。

## 中断の理由

ユーザーの指示（2026-09-29 夜、main 経由）:「IMEはとりあえず変換できるようになったら、ブラッシュアップは後回しにして、Keilandを優先しましょう。」

p004 で変換（kanji → 漢字、確定）まで guest で確かめてあるので、ここで一区切りにした。

## どこまで進んだか

- **調べて決めたこと**（design にはまだ書いていない。再開の時に §4.3・§8・§15 に写す）
  - **候補の窓の描画**: `compose.c` の `compose_record()` で、カーソルの前（`compose_cursor` の直前）に `zwl_ime_popup_draw()` を 1 行足せば、plain と glass の両方の経路に効く。xdg_popup と同じく、`zwl_compose_surface_image()` と `zwl_compose_surface_quad()` を alpha（`ZWL_DRAW_ALPHA`）で使い、glass では `glass_shape`（`MODE_SHADOW`）で影を付ける。
  - **surface の commit**: IME の popup の surface は role を持たない surface として commit され（`protocol.c` の `surface_commit()` の `role == NULL` の分岐、`zwl_surface_queue`）、カーソルと同じく image になる。その分岐の後に `zwl_ime_surface_commit()` を 1 行足して `server->dirty = 1` にする。
  - **全画面**: `display.c` の direct scanout の条件（398〜414 行）で、`zwl_ime_popup_visible()` の時は `overlay` と同じに扱う（2 行）。
  - **indicator**
    - `shell.c` の `bar_layout()` で network の左に幅を取る（`zwl_ime_indicator_width()`）。
    - `draw_status()` で `zwl_ime_indicator_draw()` を呼ぶ。
    - button の処理で network の前に `zwl_ime_indicator_button()` を呼ぶ。
    - 日本語の「あ」は glass の文字の fallback の font（`server->fallback_font_path`）で描ける。
  - **status の `languages`**: 足さない。indicator に要るのは今の言語の label だけで、`language(id, label)` で届く。
  - **IME の側の描画**
    - libkeiland の `kl_text_*`・`kl_paint_*`（`paint.c`・`paint-text.c`、`kl_text_open(Inter, DroidSansFallback)` で日本語を描ける）は libkeiland の外へ export されていない。
    - IME の program にこの 2 つの file を source として一緒に compile し、`libtruetype.so` を link する（`platform/amd64/vmunix.mk` の `keiland-ime` の規則に `-l:libtruetype.so` を足す）。
  - **key の repeat**: IME の main loop を `wl_display_dispatch` から「poll に時間の上限を付ける」形（ime-probe と同じ）に変え、grab の `repeat_info` の rate と delay で、消費した key の press を合成する。
- **書いたが build していないもの**: `plan/ws095/p005-wip.patch`（`userland/desktop/wayland/ime.h` と `input-method.c` の差分、459 行）。
  - popup の surface を名前で持つこと
  - `zwl_ime_surface_commit`・`zwl_ime_popup_visible`・`zwl_ime_popup_draw`・`ime_popup_place`（cursor の矩形の下、はみ出す時は上、左右は画面の中）
  - indicator の幅・描画・click
  - label を覚えること
  - activate・deactivate・IME の死で `dirty` にすること
  - source の tree には入れていない。tree は p004 の build・試験済みの状態のまま。
- **未着手**
  - `compose.c`・`protocol.c`・`display.c`・`shell.c` への差し込み（上の各 1〜3 行）
  - IME の側の `popup.c`（候補の窓を wl_shm に描く、9 個ずつの頁、番号、選んだ行の強調）
  - key の repeat
  - guest の試験と画面での確認

## 未実施

- build、guest の試験、画面での確認は、どれも行っていない。

## Resume point

1. `git apply plan/ws095/p005-wip.patch` で zdesktop の側の書きかけを戻す。
2. 上の「未着手」の差し込みと、IME の `popup.c` を書く。
3. image を作り直し（`plan/ws095/tests/build-ime-image.sh`）、guest で候補の窓と indicator を撮り、目で確かめる（画面は `build/ws095-shots/p005/`）。
4. 表示の文字は英語か日本語だけにする（Keiland の名は UI に出さない）。

## 次の attempt（2026-10-02 計画担当、Queue 承認ではない）

- 前提の照合（済み）: 09-29 以降 IME の機能の変更は無く、`p005-wip.patch` は現行 source に当たる（offset 2 行）。旧 worktree は壊れているので P 担当の新しい worktree で `git apply plan/ws095/p005-wip.patch` から始める。
- 範囲: 上の Resume point の 1〜4 ＋ `plan/ws095/tests/ime-p004.sh` の kill を起動時の pid に直す（ws.md の「再開のときに直すこと」）。IME の program を Linux/FreeBSD の Makefile（ba46edf89・1ed1a4b59）でも build が壊れないこと（`popup.c` と `paint.c`・`paint-text.c` の追加を Makefile.linux/.freebsd にも反映）。
- 受け入れ: ws.md の I-B1（Alt+Space で切替え、候補の窓が cursor の下に出て選択・確定、indicator の追従と click、全画面の窓の上の候補、IME の中の key の repeat）を zedBSD QEMU の PNG で確かめ、ユーザーに見せる。直接入力の遅延が増えないこと。zedBSD target build warning 0、Linux の keiland の build（`make keiland-linux` の IME と compositor）が通る、`boot-test.sh` の PNG。変更 C の全文規約。
- 所有 path: `userland/desktop/wayland/{ime.h,input-method.c,compose.c,protocol.c,display.c,shell.c}`（各 1〜3 行の差し込みは設計どおり）、`userland/desktop/ime/`、`platform/amd64/vmunix.mk` の `keiland-ime` の規則（`-l:libtruetype.so`、platform の共有 file なので main に確認）、`plan/ws095/`。
- 衝突: compositor の compose.c・protocol.c・display.c・shell.c を他 WS の Queue（WS114 p007 の修正、WS117 p003、WS099、WS094、WS113）と同時に変えない。main が順を決める。
- 目安 4h（timebox 4h）。
- 未決の判断: なし（表示の文字は英語か日本語だけ）。

2026-10-02 / ws095-beta1-plan-20261002: 次 attempt の範囲・受け入れ・所有 path・衝突を記録。Status は uncleared のまま。
