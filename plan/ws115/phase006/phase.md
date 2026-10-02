<!-- awesome-plan project=zedbsd record=ws115-p006 -->

# ws115-p006: libpng・freetype・harfbuzz・fontconfig

Parent: [WS115](../ws.md)
Status: in-progress（q605-i01、P3。作業と受け入れの証拠は揃った。clearance の確定は Q1）
Disposition: normal
Primary Milestone: MG002（WSから継承）
Queue / attempts: q605-i01（P3、継続 dispatch、時限 4h、base main 55ff880b4）
Purpose / goal: 文字の描画の基盤の library を移植する。
Prerequisites: p005（harfbuzz は glib/gobject 付き）
Investigation bound: timebox 4h
Origin: WS034 p027（移管を main に依頼）

## 範囲

- `userland/packages/libs/{libpng,freetype,harfbuzz,fontconfig}`。freetype を harfbuzz 無しで作り harfbuzz を作る（循環の扱いは inventory §2.5）。fontconfig は host の gperf が要る（p004）。harfbuzz は C++（libc++）。
- fontconfig の設定（`/usr/share/fonts` と zedBSD の既存の font の場所、cache の生成）。
- 共通の依存として WS116（Qt6）でも使う前提で option を決める。

## 受け入れ

4 package が build・install、license audit。guest で `fc-list` が zedBSD の font を列挙し、harfbuzz の shape の小さな試験が通る。

## 所有 path

`userland/packages/libs/{libpng,freetype,harfbuzz,fontconfig}/`、`plan/ws115/`

## Standards / evidence / resume

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[scoped方針](../../standards/ws114-gtk-qt-learning.md)、[automation](../../standards/automation.md)、[license audit](../../tools/packages/audit-licenses.sh) を実装前に確認する。外部 package は公式 tarball を取得・検証して patch し、source tree へ取り込まない（AGENTS.md）。新コードは全文規約。toolchain（`toolchain/`・`lang/clang`・`devel/libcxx`・共有 `build/llvm` 等）は変更・build しない。共有 `build/` は読取専用、自分の worktree の `build/` を使う。全体の `make check` は禁止。具体コマンド/版/結果/commit/環境/成果物/skip/制限は未実施（計画のみ）。各PhaseはQueueの個別scope承認後に実行する。前提未達・判断待ちならattemptをunclearedとして証拠/再開条件を残す。

Event ws115-beta1-plan-20261002: 2026-10-02 計画担当が依存 package の移植 Phase として作成。版・option は p001 の移植契約で確定する。planning/Queue none。GitHub publication pending。

## q605-i01 の結果（2026-10-02、P3）

承認: Q1 の継続 dispatch（q605、時限 4h）。base は main 55ff880b4（前回統合済み 958b6c060）。vars は `build/p3-q600/vars`（`ZEDBSD_CONFIG=build/p3-q592/config.mk BUILD=build/p3-q592/zedbsd ZEDBSD_LLVM_SOURCE=/home/awe/zedBSD-claude1/build/llvm-source`）。

### commit

- 19de99f8e: libpng・freetype・harfbuzz の Makefile。p005 を cleared、p006 を in-progress にした。
- f2156d9fc: fontconfig の Makefile。
- 94c9ba05b: fontconfig の `files/55-zedbsd-families.conf`、`tests/text-probe/`、`tests/text-probe.sh`、`tests/config-amd64-text.mk`。
- 本 commit: この記録、ws.md、port-contract §11。

### package（4 つとも patch なし）

| package | 版 | build | 実物（SONAME → file） | NEEDED | 警告 |
| --- | --- | --- | --- | --- | --- |
| libpng | 1.6.58 | CMake（pcre2 と同じ形。zlib は stage を直接指定） | libpng16.so.16 → `.16.58.0` | libz.so.1・libc.so | 0 |
| freetype | 2.14.3 | meson（harfbuzz なし、png・zlib あり、bzip2・brotli なし） | libfreetype.so.6 → `.6.20.6` | libz.so.1・libpng16.so.16・libc.so | 1（upstream: zlib の zconf.h と ftconfig.h の HAVE_UNISTD_H の再定義） |
| harfbuzz | 14.5.0 | meson（glib・gobject・freetype・subset あり。cairo・icu・raster・vector・gpu・png・zlib・utilities なし） | libharfbuzz.so.0・-gobject.so.0・-subset.so.0 → `.61450.0` | harfbuzz: freetype・glib・c、gobject: harfbuzz・glib・gobject、subset: harfbuzz・c | driver の `-pthread` の unused だけ |
| fontconfig | 2.17.1 | meson（gperf 3.3、expat、tools あり、cache-build・iconv・nls なし） | libfontconfig.so.1 → `.1.16.0`。fc-cache・fc-list・fc-match・fc-query・fc-cat | libfreetype.so.6・libexpat.so.1・libc.so（道具は libfontconfig.so.1・libc.so） | 2（upstream: fcfreetype.c の unused、fc-cat.c の set-but-unused） |

- 各 Makefile は check-dynamic-elf の契約（SONAME・NEEDED）で stage を確かめる。fontconfig は conf.d の有効な file の一覧も照合する（22 個）。
- freetype の URL: savannah の転送先の mirror がつながらなかったので、FreeType の SourceForge の配布に変えた。size と SHA-256 の照合は同じ。
- `-Db_lundef=false` は p006 の 4 つでは要らなかった（TLS を使う library が無い）。共通の規則にするかは、要る package が出たときに決める。

### harfbuzz と libc++

- harfbuzz は upstream の既定（`with_libstdcxx=false`）で C の linker で link する。3 つの library の NEEDED に libc++ が無く、`llvm-nm -D -u` に `_Z*`・`__cxa*`・`_Unwind*` の未定義も無い。**image に libc++.so.1 は要らない**。
- compile には libc++ の header（`<new>`・`<atomic>`・`<type_traits>`・`<functional>` など）が要る。`ZEDBSD_EXT_harfbuzz_DEPENDS` に `libcxx` を入れ、view の `usr/include/c++/v1` を `-nostdinc++ -isystem` で渡す（lang/clang と同じ形）。
- libcxx は build も変更もしていない。worktree の `build/packages/libcxx/stage` は、main の `build/packages/libcxx/stage`（LLVM 23.1.0-zedbsd8、libcxx zedbsd1。現在の版と同じ）の `cp -a` の複写。file は読み取り専用にし、directory は view の `cp -as` のために書けるままにした。
- make には `-o build/packages/libcxx/stage/.zedbsd-staged` を渡して libcxx を作り直させない。`-o` が無いと make は llvm-source の stamp が無くて止まり、何も build しない（`make -n` で確かめた）。main の checkout では libcxx の stage が本物なので、`-o` は要らないはず（未確認）。

### fontconfig の設定

- `/etc/fonts/fonts.conf`: dir は `/usr/share/fonts`・XDG・`~/.fonts`。cachedir は `/var/cache/fontconfig`・XDG・`~/.fontconfig`。
- conf.d の 22 個は、image が file の一覧で作られるので、link ではなく通常の file で入れる。
- cache の directory は配らない。root の `fc-cache` が `/var/cache/fontconfig` を作る（guest で確かめた）。ほかの user は自分の cache を使う。
- **`files/55-zedbsd-families.conf`**（zedBSD の file）: upstream の family の一覧に Inter が無く、sans-serif が JetBrains Mono になっていた。そこで sans-serif・system-ui → Inter（次に Droid Sans Fallback）、monospace → JetBrains Mono、emoji → Noto Color Emoji にした。fontconfig の fonts.dtd で xmllint の検査を通した。

### 試験（QEMU）

- `sh plan/ws115/tests/text-probe.sh build/amd64/packages/toolchain build/packages build/p3-q605/probe`: exit 0。probe の NEEDED は fontconfig・freetype・harfbuzz・c で、検査を通った。
- test image: `ZEDBSD_CONFIG=plan/ws115/tests/config-amd64-text.mk BUILD=build/p3-q605/img ZEDBSD_TEST_IMAGE_TAG=textprobe ZEDBSD_TEST_EXTRA_FILES=<probe の出力> -o <libcxx の stamp>` で disk-image を作り、exit 0。
  - font: Inter・JetBrains Mono・Droid Sans Fallback（`userland/desktop/fonts` から）、Noto Color Emoji（package）。
- guest（`plan/tools/guest/amd64-serial.sh`、serial。判定は道具の出力で、console の log は読んでいない）:
  - `fc-list : family file | sort` は 4 つの font（Inter、JetBrains Mono、Droid Sans Fallback、Noto Color Emoji）を `/usr/share/fonts` に列挙した。status=0。
  - `fc-cache -f` は status=0 で、`/var/cache/fontconfig` に 2 file ができた。
  - `fc-match`: sans-serif → Inter、monospace → JetBrains Mono、emoji → Noto Color Emoji、`:lang=ja` → Droid Sans Fallback。
  - `text-probe /usr/share/fonts /usr/share/fonts/keiland-emoji.ttf` の結果:
    - fontconfig の sans-serif は Inter。
    - harfbuzz（hb-ft）は「zedBSD」を 6 glyph に shape した（glyph 0 なし、advance 3841/64 px）。
    - FreeType は U+1F600 の PNG を libpng で 136 px の BGRA に decode した。
    - 出力は `text-probe: PASS harfbuzz 14.5.0 fontconfig 21701 freetype 2.14.3`、status=0。
  - 記録は `build/p3-q605/serial2.txt`。1 回目（55 の conf の前）は sans-serif が JetBrains Mono で、ほかは同じく PASS だった（`serial.txt`）。
- `plan/tools/boot-test.sh build/p3-q605/img/hdd-image.img`（`BOOT_TEST_WORK=/tmp/p3bt`）: PASS。`build/p3-q605/boot-test/login.png` に login prompt が出ていることを目で確かめた。
- 実機: 未実施。

### license（p005 と同じ方法。audit-licenses.sh の拡張は ws129-p002）

- 4 つの tarball に `GNU General Public License|GNU Lesser General Public|SPDX-License-Identifier: *L?GPL` の検索を当てた。
  - harfbuzz: 0 件。
  - fontconfig: 22 件。autotools と gettext の m4、ABOUT-NLS で、すべて build の道具。
  - freetype: 8 件。autotools と、二重 license の片方の本文 `docs/GPLv2.TXT`。zedBSD は FTL を選び、`docs/FTL.TXT` を配る。
  - libpng: 26 件。autotools と `contrib/gregbook`（GPL の例の program）。
- build の対象: meson の compile_commands（freetype 41、harfbuzz 83、fontconfig 64）にも、libpng の ninja の target にも、contrib の source は無い。配る物に GPL の source は無い。
- 配る license の本文:
  - libpng: `LICENSE`（Libpng-2.0）
  - freetype: `docs/FTL.TXT`
  - harfbuzz: `COPYING`（MIT）
  - fontconfig: `COPYING`
  - いずれも `/usr/share/licenses/<名前>/` に入る（image で確かめた）。
- zedBSD の新しい file（`55-zedbsd-families.conf`、text-probe）は Zlib。

### 受け入れの判定（P3 の評価。確定は Q1）

- 4 package が build・install され、license の確認を通った: 満たす（手での分類。p005 と同じ扱い）。
- guest で `fc-list` が zedBSD の font を列挙した: 満たす（QEMU）。
- harfbuzz の shape の小さな試験が通った: 満たす（QEMU。text-probe）。
- boot-test: 満たす（QEMU）。
- 実機: 未実施。

### 範囲の点

- 範囲の「共通の依存として WS116（Qt6）でも使う前提で option を決める」について:
  - harfbuzz の subset・gobject・freetype の統合は残した。
  - cairo（hb-cairo）は cairo の移植（p007）の後で作り直すかを決める。
  - freetype を harfbuzz 付きで作り直すか（auto-hinter の script の範囲）は、inventory §2.5 のとおり保留。
  - Qt6 が何を要るかは WS116 で照合する。

### 残り・申し送り

- worktree で harfbuzz を build するには、libcxx の stage の複写と `-o` が要る。main の checkout での build（本物の libcxx の stage）では要らないはず。merge 後の main の build で確かめてほしい。
- hb-cairo（p007 の後）。freetype を harfbuzz 付きで作り直すか。
