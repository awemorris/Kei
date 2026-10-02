<!-- awesome-plan project=zedbsd record=ws095-p013 -->
# ws095-p013: 変換中の文字（preedit）を本文と同じ大きさで inline に出す

Status: planned
Disposition: normal
Parent: [WS095](../ws.md)
Bug: [BUG-139](../../bugs/BUG-139.md)

## 範囲（2026-10-02 ユーザー）

「IMEでキーボード入力したとき、テキストエディットアプリでは、入力文字のサイズが小さくておかしかったです。アプリ側でインラインで表示しているのか、システムが表示しているのかわかりませんが、インラインであるなら、もっとなんとかならないでしょうか。テキスト本文と同じ文字の大きさにしたいです。ここはUI/UXの完成度の要だと思いました。」

1. 今の preedit の描画の経路（app の inline: `libkeiui/text-input.c`・`textedit/draw.c`、または compositor/IME の窓）を特定し、小さく見える原因を時刻つきの PNG と code で示す。
2. preedit を本文と同じ font・大きさ・行の高さで、caret の位置に inline に描く。変換中の範囲の下線、選択中の文節の強調、caret の位置を text-input-v3 の preedit の cursor に合わせる。
3. libkeiui の text-input を使う他の app（Notes、Settings の検索、Files の field）も同じ見た目になるか確かめ、共通の部品で直す。Terminal は p006。
4. QEMU の Venus で日本語の入力の PNG（変換前・変換中・候補・確定）で確認。build warning 0、C 全文規約、host の試験（libkeiui）、boot-test。実機の確認はユーザー。

## 所有 path

`userland/desktop/libkeiui/`（text-input と描画）、`userland/desktop/textedit/`、必要なら `userland/desktop/ime/`、`plan/ws095/`。
