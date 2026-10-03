<!-- awesome-plan project=zedbsd record=ws128-p011 -->
# ws128-p011: Terminal で IME の日本語を入力する（BUG-155）

Status: cleared（Q1 判定 2026-10-03: T1-012（QEMU）terminal-p011-ime PASS、変換中・変換後・確定の画面を確認。実機（5330）の確認は S2）。元の記載: in-progress（実装済み・T1 の試験待ち）
Disposition: normal
Parent: [WS128](../ws.md)
Queue: q638（P1 generation11、2026-10-03。承認: user「次のセッションはP1とT1を起動、実機がなくても修正できるバグをP1で修正、T1で順次テスト、のパイプラインを実行してください。」「実行してください。」）
Bug: [BUG-155](../../bugs/BUG-155.md)

## 範囲

S1 の実機で Terminal に日本語を打てない（Text Editor では打てる）。原因: Terminal は libkeiui の窓を使うが、text-input-v3 を
求めていなかった（`kui_window_text_input` を呼ばず、`KUI_WINDOW_TEXT_*` を捨てていた）。zdesktop は text input が served でないと
key を IME の grab に渡さないので、日本語の入力が始まらない。user「IME非対応なんじゃない？これは要修正。」

## 実装（`userland/desktop/terminal/`）

- `window.c`: 窓を開いたら `kui_window_text_input(kui, 1)`（窓の全体が shell の入力の場所）。`KUI_WINDOW_TEXT_COMMIT` の text を
  打った key と同じく shell への入力に足す（入りきらない時は文字を切らずに丸ごと捨てて log）。`KUI_WINDOW_TEXT_PREEDIT` を窓に保持して
  再描画を求める。`KUI_WINDOW_TEXT_DELETE`（screen keyboard の濁点の key が最後の kana を置き換える）は、最後の commit の末尾の bytes の
  範囲なら文字数ぶんの Backspace（DEL）を打つ。それより前・key を挟んだ後は無視（shell の行は端末の物ではない）。log `ZTERM IME commit/preedit/delete`。
- `render.c`: `terminal_renderer_draw` に窓を渡し、組み立て中の文字を cursor の cell から右へ描く（全角は 2 cell、背景は選択の色、
  変換中の文節は反転、grid の右端で切る、vertex buffer の容量を確かめる）。UTF-8 の decode は `render_utf8_next`。
- `main.c`: preedit が変わったら再描画。描画の後に cursor の cell の矩形を `kui_window_text_cursor` で伝える（候補の窓の位置）。
- `terminal.h`: 窓に `preedit`・`preedit_begin/end`・`preedit_changed`・`last_commit`。

## 検証

- build: `make -j8 ZEDBSD_CONFIG=plan/ws089/tests/config-amd64-settings.mk BUILD=build/p1-q638 build/p1-q638/bin/terminal` → exit 0、warning 0。
- host 試験: 無し（変更は Wayland の窓と Vulkan の描画で、host で動かす枠が無い）。実装を読みで確かめた。
- QEMU（T1 に依頼）: `plan/ws128/tests/terminal-p011-ime.sh`（新規。直接入力・かんじ→漢字の変換・かなの確定・shell が file に
  `a漢字かな` を書く、PNG 3 枚）。未実施（結果待ち）。
- screen keyboard の濁点の置き換え（DELETE）は QEMU の試験に入れていない（未実施）。実機（5330）は未実施。
