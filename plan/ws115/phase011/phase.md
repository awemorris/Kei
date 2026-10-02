<!-- awesome-plan project=zedbsd record=ws115-p011 -->
# ws115-p011: GTK4 のファイルダイアログを Keiland ネイティブの file chooser で出す

Status: planning（p010 の後）
Disposition: normal
Parent: [WS115](../ws.md)

## 範囲（2026-10-02 user「Keilandネイティブのfile chooserを呼び出すlibkeilandの関数呼び出しを、GTK4に追加してもOKです。」）

1. GTK 4.18.6 の `GtkFileChooserNative`／`GtkFileDialog` に、Windows（`gtkfilechoosernativewin32.c`）・macOS（`gtkfilechoosernativequartz.c`）と同じ形の zedBSD の backend（例 `gtkfilechoosernativekeiland.c`）を patch で足し、libkeiland（libkeiui）の共有の file chooser を呼ぶ。portal と D-Bus は使わない（2026-10-02 user）。
2. 開く・保存・folder の選択・filter・複数選択・初期の folder と名前・親の窓（sheet の位置）を対応させ、未対応の項目は GTK 組み込みの dialog に戻す。
3. patch は `userland/packages/desktop/gtk4/` の個別の patch として置き、license（GTK は LGPL-2.1+、libkeiland は Zlib）と provenance を記録する。libkeiland の公開 API の追加が要るなら KEILAND_VERSION の変更として記録する。
4. QEMU の Venus で gtk4-demo の file の開く・保存の PNG と結果の file で確認。build warning 0、C 全文規約（zedBSD 側の新しい code）。
