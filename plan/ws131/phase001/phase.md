<!-- awesome-plan project=zedbsd record=ws131-p001 -->
# ws131-p001: libkeiland を GUI toolkit 兼 desktop 抽象化層にする実現性の検討と設計

Status: cleared（2026-10-03、study.md、判断はユーザー待ち）
Disposition: normal
Parent: [WS131](../ws.md)

範囲は ws.md の 1。source は変えない。成果は `plan/ws131/study.md`（棚卸しの表、案と比較、推奨、移行の Phase の案、ユーザーの判断が要る点）。

## 結果（2026-10-03、読み取りの調査）

成果: [study.md](../study.md)（棚卸しの表、責務の整理、案 A/B/C/C2 の比較、Phase の案、判断の点）。source・build・QEMU は無し、main `bf3abb3a5` を読んだ。

- **結論: 実現できる。** 窓の土台は libkeiui の `kui_window` を Text Editor・Image Viewer・PDF Viewer・Notes・Terminal が既に使う。残る重複は、7 app の titlebar・glass・menu の結線（約 6,600 行、全て「静的な表 → model → 差分 → action の queue」の同じ形）、touch の結線（約 4,650 行）、CPU の canvas・text の写し（約 4,500 行）、Files・Settings・browser shell の自前の窓と present（約 6,400 行、Files・Settings の present は libkeiui の写しで差 140 行ほど）、Notes・Terminal の自前の registry、libkeiland の各 object が別々に作る registry と roundtrip。
- **推奨: 案 B（層を分ける）**。libkeiland = app の骨組み（`keiland_app`・`keiland_window`: 接続と registry を一つに、pull 型の event loop、窓（toplevel・desktop surface・sheet、Vulkan/shm/無し）、clipboard・primary・DnD・IME、宣言的な menu・titlebar・glass、desktop の設定）。libkeiui = widget と描画。依存の向き libkeiui → libkeiland は保つ。`kui_window_*` は移行の間 wrapper として残し、既存 app を無変更で動かす。ABI は追加のみ（KEILAND_VERSION 22〜）。Linux/FreeBSD は窓の層が OS に依存しないので共通の source に置き、libkeiland の link に libvulkan が加わる。
- 吸収（案 A）は zdesktop・xserver・probe に font・widget の依存を負わせ WS090 J1 を逆にするので推奨しない。案 C（骨組みを libkeiui に置く）は技術的に同等だが名前の意図と逆。
- Phase の案: p002 設計 → p003 libkeiland に app/window（libkeiui の窓の実装を移す）→ p004 宣言的な menu/titlebar/glass → p005 Text Editor（最初の移行）→ p006 PDF/Image Viewer → p007 Terminal・Notes（DnD・primary・tablet）→ p008 Settings → p009 Files → p010 browser shell（任意）→ p011 wrapper の除去 → p012 規約の全文と回帰。
- ユーザーの判断: 案の選択、ベータ1 の標準 app の作業（WS127・WS089・WS128・WS120）との時期、WS090 p007/p010 の窓の移行を WS131 へ移すか、Wayland を隠す度合い、desktop の設定の反映を範囲に入れるか、WS097/096 の backend を前提に API を設計するか。
- 範囲外の気付き: keiland-ime が `keiland_ime_status_v1` を libkeiland を通さず bind している（keiland.h の方針から外れる）。
