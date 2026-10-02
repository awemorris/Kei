<!-- awesome-plan project=zedbsd record=ws115-p008 -->

# ws115-p008: gdk-pixbuf・libjpeg-turbo・libtiff・graphene・libepoxy・libxkbcommon・xkeyboard-config

Parent: [WS115](../ws.md)
Status: in-progress（q610-i01、P3。作業と受け入れの証拠は揃った。clearance の確定は Q1）
Disposition: normal
Primary Milestone: MG002（WSから継承）
Queue / attempts: q610-i01（P3、継続 dispatch、時限 4h、base main 9f408ea0c）
Purpose / goal: GTK4 の残りの必須依存を移植する。
Prerequisites: p005（p007 と並行可）
Investigation bound: timebox 4h。収まらなければ libxkbcommon/xkeyboard-config を別 Queue に分ける
Origin: WS034 p028 の後半（移管を main に依頼）

## 範囲

- `userland/packages/libs/{gdk-pixbuf,libjpeg-turbo,libtiff,graphene,libepoxy,libxkbcommon}` と xkeyboard-config の data。gdk-pixbuf の loader は builtin、introspection 無し。libepoxy は EGL/GLES を zedBSD の libegl/libglesv2 で解決するか、GL 無しで build するか（p001 の方針）。libxkbcommon は Wayland の keymap の解析に要る（compositor が送る keymap と合うかを確認）。

## 受け入れ

全 package が build・install、license audit。guest で gdk-pixbuf の PNG/JPEG 読み込み、libxkbcommon で compositor の keymap の解析の小さな試験。

## 所有 path

`userland/packages/libs/` の上記、`plan/ws115/`

## Standards / evidence / resume

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[scoped方針](../../standards/ws114-gtk-qt-learning.md)、[automation](../../standards/automation.md)、[license audit](../../tools/packages/audit-licenses.sh) を実装前に確認する。外部 package は公式 tarball を取得・検証して patch し、source tree へ取り込まない（AGENTS.md）。新コードは全文規約。toolchain（`toolchain/`・`lang/clang`・`devel/libcxx`・共有 `build/llvm` 等）は変更・build しない。共有 `build/` は読取専用、自分の worktree の `build/` を使う。全体の `make check` は禁止。具体コマンド/版/結果/commit/環境/成果物/skip/制限は未実施（計画のみ）。各PhaseはQueueの個別scope承認後に実行する。前提未達・判断待ちならattemptをunclearedとして証拠/再開条件を残す。

Event ws115-beta1-plan-20261002: 2026-10-02 計画担当が依存 package の移植 Phase として作成。版・option は p001 の移植契約で確定する。planning/Queue none。GitHub publication pending。

## q610-i01 の結果（2026-10-02、P3）

承認: Q1 の継続 dispatch（q610、時限 4h）。base は main 9f408ea0c（前回統合済み 03f00921d）。vars は `build/p3-q600/vars`。

### commit

- 8fe02c02b: 7 package の Makefile と patch（libepoxy 0001・0002、libxkbcommon 0001〜0003）、xkeyboard-config の `files/data.list`。p007 を cleared、p008 を in-progress にした。
- 本 commit: `tests/image-probe/`・`image-probe.sh`・`config-amd64-image.mk`、`evidence/q610/boot.png`、この記録、ws.md、port-contract §11。

### package

| package | 版 | build | 実物（SONAME → file） | NEEDED | patch | 警告（すべて upstream か §11 の libc の差） |
| --- | --- | --- | --- | --- | --- | --- |
| libjpeg-turbo | 3.2.0 | CMake（SIMD は nasm で、実行時に AVX2/SSE2 を選ぶ。TurboJPEG・道具なし。`HAVE_VERSION_SCRIPT=FALSE`） | libjpeg.so.62 → `.62.4.0` | libc.so | なし | 0 |
| libtiff | 4.7.2 | CMake（zlib・JPEG。C++ の tiffxx・道具・libdeflate・jbig・lerc・lzma・zstd・webp なし。version script なし） | libtiff.so.6 → `.6.3.0` | libz.so.1・libjpeg.so.62・libc.so | なし | `-Wformat` 135（PRId64、§11） |
| gdk-pixbuf | 2.44.8 | meson（loader は全部 builtin。png・jpeg・tiff・gif。glycin・thumbnailer・gio_sniffing なし） | libgdk_pixbuf-2.0.so.0 → `.0.4400.8` | glib・gobject・gmodule・gio・png16・jpeg・tiff・c | なし | cast-align など upstream |
| graphene | 1.10.8 | meson（gobject 型あり、SSE2・GCC vector） | libgraphene-1.0.so.0 → `.0.1000.8` | gobject・glib・c | なし | `-ffast-math` と INFINITY（upstream） |
| libepoxy | 1.5.10 | meson（EGL あり、GLX・X11 なし） | libepoxy.so.0 → `.0.0.0` | libc.so | 0001・0002 | 0 |
| xkeyboard-config | 2.48 | meson（data。compat rules あり、nls なし） | data 263 file（`/usr/share/xkeyboard-config-2`）と `/usr/share/X11/xkb` の link | — | なし | — |
| libxkbcommon | 1.13.2 | meson（library だけ。x11・registry・tools・wayland の道具なし。data root は `/usr/share/xkeyboard-config-2`） | libxkbcommon.so.0 → `.0.13.2` | libc.so | 0001〜0003 | `-Wformat` 32（PRId64、§11） |

- **libjpeg の方針**: port-contract §2 のとおり libjpeg-turbo（libjpeg 6b の ABI、SONAME libjpeg.so.62）。
- **libepoxy patch 0001**（Q1 の指示の `__ZEDBSD__` の patch）: dlopen の名前を zedBSD の `libEGL.so`・`libGLESv2.so`・`libGL.so` にした。zedBSD の `/lib/libGL.so` は desktop GL と GLX を export している（`glClear`・`glBegin`・`glXCreateContext` を `llvm-nm` で確かめた）。GLES 1 は無く、名前だけ置いた。
- **libepoxy patch 0002**: `RTLD_NOLOAD` が無ければ、「読み込まずに調べる」問い合わせは「読み込まれていない」と答える。epoxy 自身が開いた library は handle に残るので見つかる。
- **libxkbcommon patch 0001**: zedBSD では version script を使わない。
- **libxkbcommon patch 0002**: `<assert.h>` に static_assert が無ければ、config.h で `_Static_assert` を定義する。
- **libxkbcommon patch 0003**: cross build（`meson.can_run_host_binaries()` が偽）では試験・fuzz・bench を作らない。bench が `CLOCK_PROCESS_CPUTIME_ID` で止まっていた。
- **xkeyboard-config の配り方**: image は make の開始時に分かる file の一覧で作られる。stage はまだ無いので、data の一覧を `files/data.list` に置いた。stage と一覧が違えば `.zedbsd-checked` が失敗する。geometry（30 file）は libxkbcommon が読まないので入れない。
- `-Db_lundef=false` は 7 つとも要らなかった（TLS なし）。

### 試験（QEMU）

- `sh plan/ws115/tests/image-probe.sh build/amd64/packages/toolchain build/packages build/p3-q610/probe`: exit 0。probe の NEEDED は gdk_pixbuf・gobject・glib・xkbcommon・epoxy・c で、検査を通った。
  - 試験の画像は package の source から取った（libpng の pngtest.png 91×69、libjpeg-turbo の testorig.jpg 227×149、libtiff の rgb-3c-8b.tiff 157×151）。git には入れない。
  - compositor の keymap は `userland/desktop/wayland/keymap.c` の `keymap_text` の文字列を script で取り出した（190 行）。
- test image: `config-amd64-image.mk`（glib の image ＋ 7 package と libegl）、`BUILD=build/p3-q610/img`、`ZEDBSD_TEST_IMAGE_TAG=imageprobe`。exit 0。
- guest（`amd64-serial.sh`、serial）で `image-probe /usr/share/image-probe /usr/share/image-probe/compositor.xkb`:
  - gdk-pixbuf: pngtest.png 91x69、testorig.jpg 227x149、rgb-3c-8b.tiff 157x151。
  - libxkbcommon: 既定の名前（evdev/pc105/us）の keymap を xkeyboard-config の data から作った。Keiland の compositor の keymap の文字列も解析した。どちらも key A が `a`、Shift で `A`。
  - epoxy: `eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS)` が zedBSD の libEGL.so を開いた。返った値は `EGL_EXT_client_extensions EGL_EXT_platform_base EGL_KHR_platform_wayland EGL_EXT_platform_wayland EGL_MESA_platform_surfaceless`。
  - 出力は `image-probe: PASS gdk-pixbuf 2.44.8 png jpeg tiff, xkbcommon names and compositor, epoxy egl`、status=0。
  - `ls /usr/share/X11/xkb/` で link が `compat keycodes rules symbols types` を指すことを確かめた。記録は `build/p3-q610/serial.txt`。
- `plan/tools/boot-test.sh build/p3-q610/img/hdd-image.img`: PASS。[evidence/q610/boot.png](../evidence/q610/boot.png) に login prompt が出ていることを目で確かめた。
- graphene は build と NEEDED の検査だけ。guest では動かしていない（受け入れに無い。GTK の build で使う）。
- 実機: 未実施。

### license（p005〜p007 と同じ方法）

- GPL の文言は graphene・libepoxy・libjpeg-turbo・xkeyboard-config で 0 件。
- libxkbcommon: 1 件（`test/evdev-scancodes.h`。patch 0003 で compile されない）。
- libtiff: 10 件（autotools）。
- gdk-pixbuf: 27 件。thumbnailer（無効）、tests（無効）、COPYING（LGPL の本文）。
- compile_commands で、gdk-pixbuf の thumbnailer・tests、libxkbcommon の test が compile されていないことを確かめた（gdk-pixbuf 24、graphene 19、libepoxy 4、libxkbcommon 37 の file）。
- 配る license:
  - libjpeg-turbo: `LICENSE.md`・`README.ijg`
  - libtiff: `LICENSE.md`
  - gdk-pixbuf・libepoxy: `COPYING`
  - graphene: `LICENSE.txt`
  - libxkbcommon: `LICENSE`
  - xkeyboard-config: `COPYING`
  - いずれも `/usr/share/licenses/<名前>/` に入る。
- 新しい file（image-probe、data.list）は Zlib（data.list は upstream の file 名の一覧）。

### 受け入れの判定（P3 の評価。確定は Q1）

- 全 package が build・install され、license の確認を通った: 満たす（手での分類）。
- guest で gdk-pixbuf の PNG/JPEG の読み込みが通った: 満たす（TIFF も。QEMU）。
- libxkbcommon で compositor の keymap の解析が通った: 満たす（QEMU）。
- boot-test: 満たす（QEMU）。
- 実機: 未実施。

### 残り・申し送り

- libc の候補（§11、記録だけ）:
  - `<inttypes.h>` の PRI*64 の長さ
  - `static_assert`
  - `RTLD_NOLOAD`
  - `CLOCK_PROCESS_CPUTIME_ID`
- GL の renderer の実際の描画（EGL の display・context、GLES 3.1 の過不足）は p010 で測る。
- worktree の build の `-o`（harfbuzz を含む build。p008 の 7 つは harfbuzz に依存しないので要らなかった）。
