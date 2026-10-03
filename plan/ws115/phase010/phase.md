<!-- awesome-plan project=zedbsd record=ws115-p010 -->

# ws115-p010: zedBSD QEMU で GTK4 demo app の起動と代表操作

Parent: [WS115](../ws.md)
Status: in-progress（q615-i01、P3。2026-10-03 ユーザーの判断（N=2、GTK4 の移植を後回し）による通常のラップアップで中断。未完了）
Disposition: normal
Primary Milestone: MG002（WSから継承）
Queue / attempts: q615-i01（P3、継続 dispatch、時限 4h、base main c2f7a8ec4）
Purpose / goal: zedBSD 上の Keiland で upstream GTK4 の demo app を動かし、代表操作と renderer を確かめる。
Prerequisites: p002（GTK4 本体）、WS117 p003（compositor の改良）と WS114 p007（CSD）
Investigation bound: timebox 4h。起動しない場合は原因の切り分けと残件を返す
Origin: 2026-10-02 p002 から分割

## 範囲

- p001 で決めた demo app を zedBSD amd64 QEMU の Keiland で起動（`GDK_BACKEND=wayland`、最初は `GSK_RENDERER=cairo`）。window・CSD・click・key・menu・dialog・resize・clipboard を操作し、QMP PNG で確認。
- 余力があれば GL（zedBSD の libegl/libglesv2）と Vulkan（Venus）の renderer を各 1 回試し、成功と fallback を分ける。
- Linux の標準 GTK4（WS114）との差を表にする。

## 受け入れ

demo app が zedBSD QEMU で window を表示し、p001 の代表操作が通る PNG と操作の証拠。renderer の実際の種類を記録。`boot-test.sh` の起動確認。実機は未実施と書く（実機の確認はユーザーに依頼）。

## 所有 path

`userland/packages/desktop/gtk4/`（修正が要る場合の patch）、`plan/ws115/`。compositor の修正が要る場合は WS117/WS114 の Phase か main に依頼する

## Standards / evidence / resume

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[scoped方針](../../standards/ws114-gtk-qt-learning.md)、[automation](../../standards/automation.md)、[license audit](../../tools/packages/audit-licenses.sh) を実装前に確認する。外部 package は公式 tarball を取得・検証して patch し、source tree へ取り込まない（AGENTS.md）。新コードは全文規約。toolchain（`toolchain/`・`lang/clang`・`devel/libcxx`・共有 `build/llvm` 等）は変更・build しない。共有 `build/` は読取専用、自分の worktree の `build/` を使う。全体の `make check` は禁止。具体コマンド/版/結果/commit/環境/成果物/skip/制限は未実施（計画のみ）。各PhaseはQueueの個別scope承認後に実行する。前提未達・判断待ちならattemptをunclearedとして証拠/再開条件を残す。

Event ws115-beta1-plan-20261002: 2026-10-02 計画担当が依存 package の移植 Phase として作成。版・option は p001 の移植契約で確定する。planning/Queue none。GitHub publication pending。


## 2026-10-02 user の追加

「GtkApplication が、session bus が無いと警告を出す件は、該当コードを無効化するパッチをお願いします。」→ この Phase の範囲に含める: GtkApplication（と下の GApplication の session bus の接続）が zedBSD で session bus を探さず警告を出さないよう、該当コードを `__ZEDBSD__` で無効化する patch を `userland/packages/desktop/gtk4/`（または glib 側なら glib の package）に置く。一意性（single instance）は無効のまま、app は通常どおり起動・終了する。gtk4-demo・widget-factory の起動で警告が出ないことを確かめる。

## q615-i01 の途中の記録（2026-10-03、P3、ラップアップで中断）

承認: Q1 の継続 dispatch（q615、時限 4h）。base は main c2f7a8ec4（前回統合済み 34843813c）。

### 済んだ物

- `plan/ws115/tests/build-desktop-image.sh` に CJK の fallback（`/usr/share/fonts/keiland-fallback.ttf` = `userland/desktop/fonts/DroidSansFallbackFull.ttf`）を足した。
- image: `sh plan/ws115/tests/build-desktop-image.sh build/p3-q615/desktop '' ZEDBSD_LLVM_SOURCE=/home/awe/zedBSD-claude1/build/llvm-source ZEDBSD_CONFIG=plan/ws115/tests/config-amd64-gtk.mk -o <libcxx の stage の stamp>`、exit 0。
- guest（Venus の desktop、`GUEST_RUNTIME=build/p3-q615/run`、`VENUS_RENDERER=/home/awe/zedBSD-claude1/build/ws035-sq-venus/install`）で compositor（`/bin/wayland`）を起動した（ZWL READY）。
- runtime の前提（guest で確かめた）:
  - `fc-list` は Droid Sans Fallback・Inter・JetBrains Mono・Noto Color Emoji を列挙した。
  - `/usr/share/glib-2.0/schemas/gschemas.compiled` がある。
  - `/usr/share/X11/xkb`（compat・keycodes・rules・symbols・types）がある。
  - `/var/cache/fontconfig` がある。
- **見つかった阻害（移植できない所）**: `GDK_BACKEND=wayland GSK_RENDERER=cairo GTK_A11Y=none gtk4-widget-factory` が `ld.so: too many dependencies` で起動しない。
  - `src/rtld/rtld.h` の `RTLD_NEEDED_MAX` 16 を、`libgtk-4.so.1` の DT_NEEDED 24 が超える（rtld.c:2846）。
  - 全 object の `RTLD_OBJECT_MAX` 32 も、GTK の依存の木（35 file ＋ libc・ld.so）が超える（rtld.c:2556）。
  - 記録は `build/p3-q615/guest/wf-run1.txt`。画面の PNG（`wf-cairo.png`）は起動前の物で、証拠ではない。
- Q1 の判断（2026-10-03）: 定数を `RTLD_NEEDED_MAX` 64・`RTLD_OBJECT_MAX` 128 に上げる変更を P3 に許可した（所有 path に `src/rtld/rtld.h` と、上限の値を使う所）。上限を越えたときの error は残す。bss の増分は記録する。確認する物は、ld.so・libc・base の build（warning 0）、desktop の app（Terminal・Files・Settings・Text Editor）、dlopen の回帰（libvulkan・libEGL の driver の読み込み）、boot-test、GTK の起動。動的に伸ばす設計は Future Work（WS066 に関係づける）。
- **rtld は未変更**（ラップアップの指示が先に来た）。調べて分かった、変更に要る物:
  - `rtld.h` の 2 つの定数。
  - `lookup_handle_graph`（rtld.c:5033）と呼び出し元 `rtld_dlsym_common`（rtld.c:4907）の `uint32_t visited` の bitmask。object の index（0〜127）で shift するので、`uint32_t visited[(RTLD_OBJECT_MAX + 31U) / 32U]` のような配列にする（32 を超える index で `1U << index` が未定義になる）。
  - ほかの上限の使い方（objects[]・initialization_order[]・tls_modules[]・dtv の数・unload の dependencies[]）は配列の大きさだけで、定数に従う。
  - bss の見積もり: object ごとに needed_offset・needed が 12 B×48 増える（約 +576 B）。それに objects[] の数の 4 倍、各 thread の dtv が 129 項目（約 1 KB）。

### 再開点

1. `src/rtld/rtld.h` の 2 つの定数と、rtld.c の visited の bitmask の配列化（Q1 の許可の範囲、上限の error は残す）。ld.so・libc・base の build（warning 0）。ld.so の bss の増分を `llvm-size` などで記録する。
2. image（上の build-desktop-image.sh の command）、boot-test、desktop の app と dlopen の回帰（wltest の Vulkan、zgears・egltest の EGL、Terminal・Files・Settings・Text Editor）。
3. guest で `gtk4-widget-factory` と `gtk4-demo` を `GSK_RENDERER=cairo` で起動し、PNG で window・文字（日本語の fallback を含む）・CSD を確かめる。CSD の見た目と、二重の装飾やタイトルバーの欠けを記録する（Keiland は `org_kde_kwin_server_decoration` を出さない。ユーザーと SSD の既定を検討中）。次に GL（`GSK_RENDERER=gl`、epoxy → libEGL）。
4. 代表操作（click・key・menu・dialog・resize・clipboard）と、session bus の警告が出ないこと（gtk4 の patch 0005、GApplication は警告を出さない。p005 で確かめ済み）。
5. Linux の WS114 との差の表、port-contract の更新。
