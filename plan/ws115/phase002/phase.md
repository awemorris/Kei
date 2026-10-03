<!-- awesome-plan project=zedbsd record=ws115-p002 -->

# ws115-p002: upstream GTK4 package移植・実行

Parent: [WS115](../ws.md)
Status: cleared（2026-10-03 Q1。q614-i01、34843813c まで main に統合）
Disposition: normal
Primary Milestone: MG002（WSから継承）
Queue / attempts: q614-i01（P3、継続 dispatch、時限 4h、base main b73021f5b）
Purpose / goal: upstream GTK4 本体の package（`userland/packages/desktop/gtk4`）の build/install（2026-10-02 改訂: zedBSD 上の実行は p010 へ）
Prerequisites: p001契約、p007・p008（描画系）、p009（libwayland 互換）
Investigation bound: Queue選定時に対象・時間・検証上限を決める。

## Procedure / affected components

external.mkと公式tarball/patchでuserland/packages/desktop/gtk4を追加。zedBSD build/install/Wayland起動/操作を検証し、Linuxとの差を記録する。

## Clearance / verification

決めたapp/操作がtargetで通り、失敗とrenderer/fallbackを区別する。

## Standards / evidence / resume

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[scoped方針](../../standards/ws114-gtk-qt-learning.md)、[automation](../../standards/automation.md)を実装前に確認する。新コードは全文規約、外部tarballはprovenance・license・patch分離。具体コマンド/版/結果/commit/環境/成果物/skip/制限は未実施（計画のみ）。各PhaseはQueueの個別scope承認後に実行する。前提未達・判断待ちならattemptをunclearedとして証拠/再開条件を残す。

Event ws114-gtk-qt-port-plan-20261002: 2026-10-02 user指示から作成。planning/Queue none。GitHub publication pending。

2026-10-02 / ws115-beta1-plan-20261002（redesign）: 依存 package を p004〜p009 に分け、本 Phase は GTK4 本体の `external.mk` 定義・公式 tarball・patch・meson 構成（p001 の任意機能の無効化）・build・sysroot/image への install・license audit・ELF の NEEDED/SONAME の確認に限る。zedBSD 上の起動と操作は新しい [p010](../phase010/phase.md) へ移した。受け入れ: target 向けの `libgtk-4.so` と demo app が warning を記録した上で build され、image に入り、NEEDED が全て image 内で解決する。目安 4h。所有 path `userland/packages/desktop/gtk4/`・`plan/ws115/`。Status は planning のまま。[WS summary](../ws.md)。

## q614-i01 の結果（2026-10-03、P3）

承認: Q1 の継続 dispatch（q614、時限 4h）。user「まずは素のGTK4を移植してください。移植できないところがないか、ノウハウを蓄積します。」。base は main b73021f5b（前回統合済み ad993eb6a）。所有 path に `userland/desktop/keiland/wayland/wayland-util.h` を Q1 が加えた（追加だけ）。

### commit

- b5abef788:
  - `userland/packages/desktop/gtk4`（Makefile と patch 5 つ）
  - cairo（zlib を有効にして PDF/PS と script interpreter、alloca の patch を削除）
  - pango（patch を警告の部分だけに）
  - `wayland-util.h`
  - p009 を cleared、p002 を in-progress にした。
- 本 commit: `tests/config-amd64-gtk.mk`・`check-needed.py`・`build-desktop-image.sh`（ZEDBSD_CONFIG を後から渡せる形のまま使った）、`evidence/q614/boot.png`、この記録、ws.md、port-contract §11。

### GTK 4.18.6 の build

- meson の option（ユーザーの決定どおり）:
  - 有効: Wayland の backend だけ、demos。
  - 無効: x11・broadway・win32・macos・android、vulkan（Cairo → GL）、gstreamer・cups・cpdb・cloudproviders・sysprof・tracker・colord・accesskit、introspection、文書、試験、例。
- 依存: glib・cairo・pango・harfbuzz・fribidi・fontconfig・freetype・gdk-pixbuf・libpng・libtiff・libjpeg-turbo・libepoxy・libxkbcommon・graphene・wayland-client（p009 の stage、1.23.1）・wayland-protocols（1.49）。
- 構成の要約: Display backends wayland、Print backends file、Vulkan false、Introspection false、Demos true。
- **wayland-scanner 1.23.1 での生成**:
  - gdk/wayland の protocol の code は全部生成された。
  - color-management-v1.xml だけ `frozen` 属性で `XML failed validation against built-in DTD` の警告が出た。GTK は `--strict` を使わないので、生成は通る。
- 最初の `ninja -k 0` で失敗は 9 件。中身は、下の patch 4 つで直る物、wayland-util.h の不足、cairo-pdf.h（demo の path_fill.c）。

| patch | 理由 | 外せる条件 |
| --- | --- | --- |
| 0001-round-without-fenv-rounding-modes | `FE_UPWARD` などが無い（zedBSD は最近接の丸めだけ）。ceil・floor・trunc・nearbyint で同じ結果にする | libc に丸めの mode が入ったとき（ws034-p058） |
| 0002-use-guint-for-the-bsd-uint-type | `uint` が無い。guint に | libc に uint が入ったとき（ws034-p058） |
| 0003-use-setjmp-with-libpng-jmpbuf-on-zedbsd | zedBSD の `sigjmp_buf` は `jmp_buf` と別の型。libpng の jmp_buf には setjmp が対 | libc が 2 つを同じ型にしたとき |
| 0004-leave-out-malloc-h-on-zedbsd | `<malloc.h>` が無い（標準外。roaring 自身が「要らないはず」と書く） | libc に malloc.h が入ったとき |
| 0005-report-missing-session-bus-as-a11y-debug | session bus が無いことの g_warning を a11y の debug に（ユーザーの決定。動作は同じ） | D-Bus を使うと決めたとき |

- wayland-util.h（Q1 の許可、追加だけ）: `wl_container_of`・`wl_list_for_each`・`_safe`・`_reverse`・`_reverse_safe`・`wl_array_for_each`、`<math.h>`・`<inttypes.h>` の include。同じ名前の macro を定義する zedBSD の client は無い（grep）。
- cairo: `-Dzlib=enabled` で PDF・PS の surface（gtk4-demo の path_fill.c と print-to-file が使う）と libcairo-script-interpreter.so.2（GTK が link する）を作り、package に加えた。NEEDED の契約も更新した。
- **libc の取り込みで外した patch**（Q1 の指示で確かめた）:
  - cairo の alloca の patch: 外した。meson が `alloca.h` を見つけ、build と契約が通った。
  - pango の getc_unlocked の部分: 外した。meson の検査が libc の物を使う。clang の警告の部分は残した。
- 結果:
  - `make gtk4`: exit 0。`libgtk-4.so.1`（`.1800.6`）は契約の NEEDED 24 個（glib 系・pango 系・harfbuzz・harfbuzz-subset・cairo 系・fribidi・gdk-pixbuf・epoxy・graphene・fontconfig・png・tiff・jpeg・xkbcommon・wayland-client・wayland-egl・cairo-script-interpreter・libc）。TLS は使わない。
  - gtk4-demo・gtk4-demo-application・gtk4-widget-factory も NEEDED の契約を通った。
  - schema は host の glib-compile-schemas 2.84.4 `--strict` で `gschemas.compiled` にした。
- 警告: GTK の build の警告は upstream の物だけ。`-Wunused-but-set-global`（G_DEFINE_TYPE の parent_class）188、`-Wdeprecated-declarations` 14、`-Walloc-size` 6、driver の `-pthread`。patch を当てた行からの警告は無い。

### image と NEEDED の解決

- `plan/ws115/tests/config-amd64-gtk.mk`（criteria の desktop の image ＋ GTK とその依存の全 package）を `build-desktop-image.sh build/p3-q614/desktop '' ZEDBSD_CONFIG=... -o <libcxx の stamp>` で作り、exit 0。
  - 新しい BUILD なので、全 client（3581 の compile）が新しい wayland-util.h で作り直された。zedBSD の code の警告は無い（残りは openssl などの外部 package の upstream の物）。
- rootfs に `/usr/bin/gtk4-demo`・`gtk4-demo-application`・`gtk4-widget-factory`、`/usr/lib/libgtk-4.so.1`、`/usr/share/glib-2.0/schemas/gschemas.compiled`、`/usr/share/licenses/gtk4` が入った。
- `python3 plan/ws115/tests/check-needed.py <rootfs> llvm-readelf /usr/bin/gtk4-demo /usr/bin/gtk4-widget-factory /usr/bin/gtk4-demo-application /usr/lib/libgtk-4.so.1`: **35 file を辿って、解決できない NEEDED は 0**（/lib と /usr/lib の中で全部見つかる）。
- `plan/tools/boot-test.sh build/p3-q614/desktop/hdd-image.img`: PASS。[evidence/q614/boot.png](../evidence/q614/boot.png) に login prompt が出ていることを目で確かめた。
- zedBSD 上での GTK の起動・描画は p010 の範囲で、未実施。

### license

- GTK 4.18.6: LGPL-2.1-or-later（COPYING を `/usr/share/licenses/gtk4/` に）。
- tarball の GPL の文言は 83 件で、po の翻訳・tests・demo の外の file。compile された file の中では 3 件で、どれも GPL ではない:
  - gdkvulkancontext-wayland.c・gskoffload.c: LGPL の本文の「received a copy of the GNU General Public License」の行。gskoffload.c の SPDX は LGPL-2.1-or-later。
  - gtkaboutdialog.c: license の名前の一覧。

### 受け入れの判定（P3 の評価。確定は Q1）

- target 向けの `libgtk-4.so` と demo app が、warning を記録した上で build された: 満たす。
- image に入り、NEEDED が全て image 内で解決する: 満たす（check-needed 0 unresolved、boot-test PASS）。
- 実機・zedBSD 上の実行: p010（未実施）。

### 残り・申し送り（p010）

- runtime の前提:
  - fonts（config-amd64-gtk.mk の image には Inter・JetBrains Mono・emoji。CJK の fallback は build-desktop-image.sh に足していない）
  - fontconfig の cache
  - `GSK_RENDERER=cairo` から始める
  - EGL の GL renderer（epoxy の patch は p008 で確かめ済み）
- FileDialog の gsettings-desktop-schemas（p011）。
- libc の候補（ws034-p058）が入ったら、GTK の patch 0001・0002 を外す。
