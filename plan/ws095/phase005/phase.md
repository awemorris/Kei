<!-- awesome-plan project=zedbsd record=ws095-p005 -->

# ws095-p005: 候補の窓と indicator

Status: cleared（q604-i01、2026-10-02、P4。QEMU の Venus で確認、実機の目視はユーザー。判定と記録は Q1）。旧: uncleared（2026-09-29、ユーザーの指示で中断）
Disposition: normal
Parent: [WS095](../ws.md)
Queue: main が割り当て（2026-09-29、worktree `.claude/worktrees/ws095-ime`、branch `wt/ws095`）。Queue の ID は main が記録する

2026-10-02 user:「IMEのステータスを画面右上の通知領域に追加してください。日本語入力はできることを確認しましたので、ステータスがあればいいと思いました。」→ indicator を system bar の右上の通知領域に置くことをこの Phase の必須にする。

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

## 結果（q604-i01、2026-10-02、P4、worktree `/home/awe/zedBSD-worktrees/p4`、main eab8635d8 に合わせ `p005-wip.patch` を当ててから）

承認: user「IMEのステータスを画面右上の通知領域に追加してください。」＋継続 dispatch（時限 4h）。

### 変えた file

- compositor（`userland/desktop/wayland/`、最小の差し込み）:
  - `input-method.c`・`ime.h`: WIP の patch を全文規約に書き直し（複合条件を 1 つずつ、候補の窓の 1 枚の描画を `ime_popup_draw_one()` に分けた）。`zwl_ime_popup_visible()` は使い道（direct scanout）が ws103-p002 で無くなったので消した。indicator の位置が変わった時に `ZWL IME indicator x=… label=…` を出す（試験が click の位置を読む）。
  - `compose.c`: `compose_record()` の cursor の直前に `zwl_ime_popup_draw()`（plain・glass 共通）と `#include "ime.h"`。
  - `protocol.c`: role の無い surface の commit の後に `zwl_ime_surface_commit()`（3 行）。
  - `shell.c`: `struct shell_bar` に `ime_x`、`bar_layout()` で volume の左に indicator の幅、`draw_status()` で描画、button の処理で volume の前に click（system bar の右上の status の並び）。`#include "ime.h"`。
  - `display.c` は変えていない（direct scanout が無いので全画面も同じ合成の経路）。
- IME（`userland/desktop/ime/`）: 新しい `popup.c`（input popup surface、wl_shm、libkeiui の canvas と text で 9 個の頁・番号・選択の強調・頁の行。変更ごとに窓の大きさの wl_buffer を作るので影は窓の大きさ）、`program.h`（popup・repeat の状態）、`main.c`（wl_compositor・wl_shm の bind、`main_serve()` の poll の loop に repeat の時刻）、`method.c`（`method_press()` に key の処理を分け、repeat の rate・delay、`program_repeat_*`、送るたびに候補の窓を更新、活性の変化で窓を消す）、Makefile・Makefile.linux・Makefile.freebsd（`popup.c`、libkeiui 等）。
- `platform/amd64/vmunix.mk` の `keiland-ime` の link の規則（**platform の共有 file**）: libkeiui・libkeiland・libtruetype・libvulkan・libpng-compat・libz-compat を足した（textedit と同じ組）。
- 試験: `plan/ws095/tests/ime-p005.sh`（新）、`ime-p004.sh` の kill を password の probe の起動時の pid だけに（`$!` を `/tmp/pw.log.pid` に）。

### 確認（QEMU の Venus、console・serial の log は使っていない）

- image `build/p4-ime-p005.img`（`build-ime-textedit-image.sh`、SHA-256 `69d2a8f24d5d44a5f06ce0593044d72807e6b6a3968ae189331c4c4923a6b75d`）、build の warning 0（zedBSD target、`-Werror`）。
- `GUEST_RUNTIME=build/p4-run sh plan/ws095/tests/ime-p005.sh build/p4-p005d` → **PASS**: 候補の窓が作られ（`KEI-IME POPUP ready=1`）、indicator「A」→ Alt+Space で「あ」、kanji と Space 3 回で caret の下に候補の窓（6 候補、3 番が選択、p013 の `te_app_caret_rect()` の文節の位置）、Enter で確定し窓が消える、
  `a` を 1.6 秒押し続けると IME が repeat して preedit が「あああああああ」、indicator の click で次の言語（`ZWL IME indicator next`、`language=direct`）。zdesktop の ERROR なし。
  PNG: `plan/ws095/tests/evidence-p005/`（summary.png: bar の A・あ・click 後の A と候補の窓、candidates・bar-ja・repeat）。
- `ime-p004.sh`（kill の修正の後）→ status=0。
- host: `host-engine.sh` 203 passed。
- Linux: `make -f userland/desktop/keiland-linux.mk KEILAND_LINUX_BUILD=build/p4-linux build/p4-linux/libexec/keiland-ime build/p4-linux/bin/wayland` → 通る（warning 0）。FreeBSD の build は未実施（Makefile.freebsd は Linux と同じ形に直した）。
- `plan/tools/boot-test.sh build/p4-ime-p005.img` → PASS（`build/p4-boot5/login.png`）。

### 気づき・未実施

- **確定の時に zdesktop の 500 ms の待ちを越える**: Enter の確定の直後に `ZWL IME bypass after_ms=500` が毎回出る（その間の key は IME を通らずに app へ行く）。確定ごとの利用者の辞書の書き込み（`ja-user.c` の fsync と rename）が QEMU の UFS で遅い見込み。直接入力の遅延ではないが、確定の直後の速い入力が IME を素通りしうる。p005 の範囲の外なので直していない（試験は確定の後 2 秒待つ）。Bug Board の候補として報告。
- 全画面の窓の上の候補: text-input を使う全画面の client が無く、未実施（direct scanout が無いので合成の経路は同じ）。
- 候補の窓の 1〜9 は選ぶだけで確定しない（engine の今の動き）。実機の目視（5330）はユーザー。
