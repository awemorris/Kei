# GTK4 / Linux Keiland 互換性レビュー表（暫定、2026-10-02）

この表は **ソース調査の結果** である。標準ビルドの GTK4 はこの時点で Keiland 上で未実行なので、「実装あり」は動作合格を意味しない。各行の対応方針は提案であり、右端のユーザー判断が入るまで受け入れ範囲は確定しない。WS114 p001 で Debian 13 の標準 GTK4 を実際に動かし、観測結果・GTK版・環境・ログを追加する。GNOME の全機能を目標にしない。

凡例: **基礎**＝通常の GTK4 window のために優先検証、**用途依存**＝対象アプリ・sandbox 次第、**保留案**＝まず不採用を提案。すべて「提案」であり、実装承認ではない。

| ID | GTK4の機能・経路 | Keilandのソース調査 / 既知の不足 | 標準GTK4実行 | 対応提案 | ユーザー判断 |
| --- | --- | --- | --- | --- | --- |
| G01 | GDK Wayland接続、socket環境 | Linux版compositorにWayland serverあり。GTK4本体の guest への導入と接続試験なし | 未検証 | 基礎: `GDK_BACKEND=wayland` と `XDG_RUNTIME_DIR` / `WAYLAND_DISPLAY` を記録して起動 | 未決 |
| G02 | `wl_compositor`、`xdg_wm_base` の生成・configure/ack・ping | [protocol.c](../../userland/desktop/wayland/protocol.c) に global、[toplevel.c](../../userland/desktop/wayland/toplevel.c) に処理あり。GTK4相手の順序未検証 | 未検証 | 基礎: 初回表示/close/再表示を試験し不足を直す | 未決 |
| G03 | `xdg_positioner` / `xdg_popup`、popup grab・reposition | [popup.c](../../userland/desktop/wayland/popup.c) に処理あり。menu/popoverの座標・grab・再配置未検証 | 未検証 | 基礎: menu・popover・tooltipを試験 | 未決 |
| G04 | toplevelのmove/resize/maximize/fullscreen・最小最大サイズ | move/resize/maximize等は[toplevel.c](../../userland/desktop/wayland/toplevel.c)、fullscreenは[protocol.c](../../userland/desktop/wayland/protocol.c)に処理あり。`show_window_menu` は検証のみで表示処理なし | 未検証 | 基礎: resize/最大化を試験。window menuは用途依存で判断 | 未決 |
| G05 | 親子関係、modal dialog、装飾交渉 | parent と [decoration.c](../../userland/desktop/wayland/decoration.c) のglobalあり。GTK4 dialog/CSD/SSDの結果未検証 | 未検証 | 基礎: dialog と CSD。SSDは用途依存 | 未決 |
| G06 | `wl_shm` と frame callback / `wp_viewporter` | [protocol.c](../../userland/desktop/wayland/protocol.c) で宣言。GTK4の実buffer表示未検証 | 未検証 | 基礎: 画面更新/損傷/resizeを画像で確認 | 未決 |
| G07 | GTK4 の GL/Vulkan/ソフトウェア renderer と dmabuf | Linux版にはdmabufコードがある。GTK4の既定renderer・EGL/Vulkan選択・importは未観測 | 未検証 | 基礎: 既定rendererを記録。software fallbackだけでGPU対応を合格としない | 未決 |
| G08 | pointer/keyboard/touch/cursor shape | `wl_seat` global、cursor shape globalあり。GTK4入力widgetで未検証 | 未検証 | 基礎: click/key/scroll/focus/カーソルを試験 | 未決 |
| G09 | clipboard と drag & drop | `wl_data_device_manager` と primary selection global、[data.c](../../userland/desktop/wayland/data.c) あり。GTK4の型/交渉未検証 | 未検証 | 基礎: text copy/paste。D&D/PRIMARYは用途依存 | 未決 |
| G10 | text-input-v3、IME | `zwp_text_input_manager_v3` global と [text-input.c](../../userland/desktop/wayland/text-input.c) あり。GTK4 IME未検証 | 未検証 | 基礎: ASCII入力。日本語IMEは用途依存で検証 | 未決 |
| G11 | output、整数scale、fractional scale | `wl_output` v4あり。fractional scale globalは現状の固定一覧にない | 未検証 | 基礎: 既定scale。fractional scaleは用途依存 | 未決 |
| G12 | xdg-activation (起動時focus/通知) | 固定global一覧に見当たらない | 未検証 | 用途依存: app間起動/focusを試して判断 | 未決 |
| G13 | portal FileChooser / FileDialog | Keiland固有のportal backend・session設定は見つからない。GTK4の通常dialog経路とportal経路は別に試験が必要 | 未検証 | 用途依存: sandbox/FileDialogを対象にするなら既存backend再利用も含めて決定 | 未決 |
| G14 | portal Settings (dark mode等) | Keilandのportal backend/選択設定は未確認 | 未検証 | 用途依存: 対象アプリのtheme要求を調査 | 未決 |
| G15 | portal OpenURI、通知、印刷、inhibit | Keilandのportal backend/選択設定は未確認 | 未検証 | 用途依存: 個別に対象アプリの要求で決定 | 未決 |
| G16 | portal screenshot / screencast、RemoteDesktop | Keilandのportal backend/選択設定は未確認 | 未検証 | 保留案: 今回の標準GTK4基本起動の受け入れには入れない | 未決 |
| G17 | accessibility AT-SPI、theme/fonts、GSettings | compositor以外のdistro/session統合とGTK4依存が関係。導入状態未調査 | 未検証 | 用途依存: 対象アプリと配布要件で分ける | 未決 |
| G18 | GNOME固有の全サービス/拡張 | GTK4の通常Wayland windowと別のdesktop機能 | 未検証 | 保留案: 網羅目標にしない | 未決 |
| G19 | Keiland native menubarへのGTK4連携 | [F-045](../future-work.md)に後日案。upstream GTK4の必須Wayland機能ではない | 未検証 | 用途依存: 通常windowの互換性とは別に後で判断 | 未決 |

## 検証の順番と判断の材料

1. Debian 13 guest の標準 GTK4 と依存を distro package で導入し、版と `gtk4-demo` 等の対象アプリを固定。`GDK_BACKEND=wayland` で起動し、global bind、stderr、QMP画面、操作結果を取る。使えるrendererを GSK/GTK の診断手段で観測し、通常起動と切替診断を混同しない。
2. G01–G11を主なwindow経路として再現し、未対応とGTK4側/環境側の問題を分ける。portalはsession D-Busとbackend選択/実アプリ呼出しを別に調べる。未使用のoptional protocolを「必須」と推定しない。
3. 各行についてユーザーが採用・限定採用・保留を決めた後、WS114 p003/p004 の具体scope/受け入れを定める。結果はWS115のzedBSD移植と後の独自実装に渡す。

参照: [GTK4 Wayland](https://docs.gtk.org/gtk4/wayland)、[GTK4 runtime options](https://docs.gtk.org/gtk4/running.html)、[Gtk.FileChooserNative](https://docs.gtk.org/gtk4/class.FileChooserNative.html)、[xdg-desktop-portal system integration](https://flatpak.github.io/xdg-desktop-portal/docs/system-integration.html)、[portal backend](https://flatpak.github.io/xdg-desktop-portal/docs/writing-a-new-backend.html)。FileChooserは通常dialog経路があり、portalが利用可能ならportal経路を使うため、portal全体をGTK4の一律の起動必須条件とはしない。
