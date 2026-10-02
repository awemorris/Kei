<!-- awesome-plan project=zedbsd record=ws115-p007 -->

# ws115-p007: pixman・cairo・fribidi・pango

Parent: [WS115](../ws.md)
Status: in-progress（q608-i01、P3。作業と受け入れの証拠は揃った。clearance の確定は Q1）
Disposition: normal
Primary Milestone: MG002（WSから継承）
Queue / attempts: q608-i01（P3、継続 dispatch、時限 4h、base main e99a3c589）
Purpose / goal: GTK4 の text と Cairo renderer の基盤を移植する。
Prerequisites: p006
Investigation bound: timebox 4h
Origin: WS034 p028 の前半（移管を main に依頼）

## 範囲

- `userland/packages/libs/{pixman,cairo,fribidi,pango}`。cairo は image surface と PNG だけ（X11・GL は無効）。pango は fontconfig/freetype/harfbuzz の backend。
- pixman の SIMD の option は target の CPU に合わせる。

## 受け入れ

4 package が build・install、license audit。guest で cairo の image surface に pango で日本語を含む文字列を描いた PNG を作り、host で目視確認できる（PNG はユーザーに見せる）。

## 所有 path

`userland/packages/libs/{pixman,cairo,fribidi,pango}/`、`plan/ws115/`

## Standards / evidence / resume

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[scoped方針](../../standards/ws114-gtk-qt-learning.md)、[automation](../../standards/automation.md)、[license audit](../../tools/packages/audit-licenses.sh) を実装前に確認する。外部 package は公式 tarball を取得・検証して patch し、source tree へ取り込まない（AGENTS.md）。新コードは全文規約。toolchain（`toolchain/`・`lang/clang`・`devel/libcxx`・共有 `build/llvm` 等）は変更・build しない。共有 `build/` は読取専用、自分の worktree の `build/` を使う。全体の `make check` は禁止。具体コマンド/版/結果/commit/環境/成果物/skip/制限は未実施（計画のみ）。各PhaseはQueueの個別scope承認後に実行する。前提未達・判断待ちならattemptをunclearedとして証拠/再開条件を残す。

Event ws115-beta1-plan-20261002: 2026-10-02 計画担当が依存 package の移植 Phase として作成。版・option は p001 の移植契約で確定する。planning/Queue none。GitHub publication pending。

## q608-i01 の結果（2026-10-02、P3）

承認: Q1 の継続 dispatch（q608、時限 4h）。base は main e99a3c589（前回統合済み cbbc1607f）。vars は `build/p3-q600/vars`。harfbuzz を依存に含む build では、make に `-o build/packages/libcxx/stage/.zedbsd-staged` を渡した（p006 と同じ）。

### commit

- 53b5dbe6b: 4 package の Makefile、cairo と pango の patch、`tests/pango-probe/`・`pango-probe.sh`・`config-amd64-pango.mk`、`evidence/q608/`。p006 を cleared、p007 を in-progress にした。
- 本 commit: この記録、ws.md、port-contract §11。

### package

| package | 版 | 実物（SONAME → file） | NEEDED | patch | 警告（すべて upstream） |
| --- | --- | --- | --- | --- | --- |
| pixman | 0.46.4 | libpixman-1.so.0 → `.0.46.4` | libc.so | なし | pixman-sse2.c の unused 1、driver の `-pthread` |
| fribidi | 1.0.17 | libfribidi.so.0 → `.0.4.0` | libc.so | なし | fribidi-bidi.c の enum の cast 5 |
| cairo | 1.18.6 | libcairo.so.2・libcairo-gobject.so.2 → `.11806.6` | cairo: png16・fontconfig・freetype・pixman・c、gobject: cairo・glib・gobject | 0001 | unterminated-string 6、switch-enum 1、colr の return-type 1 |
| pango | 1.56.4 | libpango-1.0・libpangoft2-1.0・libpangocairo-1.0 `.so.0` → `.5600.4` | Makefile の契約のとおり（glib・gobject・gio・fribidi・harfbuzz・fontconfig・freetype・cairo・c） | 0001 | parent_class の unused-but-set-global 11（warning に戻した物）、pangofc-fontmap.c の cast-align 1 |

- 各 option:
  - pixman: MMX・SSE2・SSSE3（SSSE3 は実行時に選ぶ）、TLS あり。
  - cairo: image・recording・SVG・FreeType・fontconfig・PNG・cairo-gobject。X11・XCB・Quartz・DWrite・zlib（PDF/PS）・LZO は無効。GL は cairo 1.18 に無い。
  - pango: fontconfig・freetype・cairo。Xft・libthai・sysprof・introspection は無効。
- 各 Makefile は check-dynamic-elf の契約（SONAME・NEEDED）で stage を確かめる。
- **cairo patch 0001**: `<alloca.h>` が無ければ `__builtin_alloca` を使う（zedBSD の libc に alloca の宣言が無い）。
- **pango patch 0001**:
  - meson で `getc_unlocked` を検査する。無ければ `getc` に戻す（flockfile を持ったままの読み出し）。
  - LLVM 23 の clang が `G_DEFINE_TYPE` の parent_class を `-Werror=unused-but-set-variable` の群で報告するので、その診断だけ `-Wno-error` にした。
- pixman は TLS（`__tls_get_addr`）を使うので、glib と同じく `-Db_lundef=false`。external.mk の共通化はしていない（2 package）。
- libc の差（alloca.h、getc_unlocked）は port-contract §11 に記録した。libc は変えていない。

### 試験（QEMU）

- `sh plan/ws115/tests/pango-probe.sh build/amd64/packages/toolchain build/packages build/p3-q608/probe`: exit 0、probe の NEEDED の検査を通った。
- test image: `config-amd64-pango.mk`（text の image ＋ pixman・fribidi・cairo・pango）、`BUILD=build/p3-q608/img`、`ZEDBSD_TEST_IMAGE_TAG=pangoprobe`。exit 0。font は Inter・JetBrains Mono・Droid Sans Fallback・Noto Color Emoji。
- guest（`amd64-serial.sh`、serial）:
  - `pango-probe /tmp/pango-probe.png` の出力は `pango-probe: PASS pango 1.56.4 cairo 1.18.6 wrote /tmp/pango-probe.png`、status=0。字の無い glyph は 0。
  - PNG は guest の `uuencode -m` で serial から取り出し、host で base64 を decode した（32814 byte、PNG の signature あり）。
  - **[evidence/q608/pango-probe.png](../evidence/q608/pango-probe.png)**: 960×300 の白地に黒。3 行は「zedBSD: Pango と Cairo」「日本語の文字を描く。」「zedBSD 😀 絵文字」。漢字・かなは Droid Sans Fallback、ラテンは Inter、絵文字は Noto Color Emoji の色つきで、正しく描かれていることを目で確かめた。
  - 1 回目は Arabic（RTL）の 1 語も入れたが、zedBSD の font に Arabic が無く 5 字が hex の箱になった。そのため FAIL pango-coverage になった（[evidence/q608/pango-probe-arabic-no-font.png](../evidence/q608/pango-probe-arabic-no-font.png)。RTL の並べ替えは効いている）。受け入れは日本語なので、probe から Arabic を外した。RTL の font は範囲外で、記録だけ。
- `plan/tools/boot-test.sh build/p3-q608/img/hdd-image.img`: PASS。[evidence/q608/boot.png](../evidence/q608/boot.png) に login prompt が出ていることを目で確かめた。
- 実機: 未実施。

### license（p005・p006 と同じ方法）

- pixman: GPL の文言は 0 件。
- pango: 1 件で、COPYING（Library GPL v2）の本文。compile された pango の source の分類は次のとおり。よって package は LGPL-2.0-or-later（一部 2.1-or-later）。
  - 47: 本文が Library GPL v2 or later
  - 2: Lesser GPL（json）
  - 1: 生成物
- fribidi: 16 件。autotools、COPYING、`bin/getopt*`（`-Dbin=false` で build しない）。
- cairo: 21 件。COPYING-LGPL-2.1、`util/cairo-trace`（GPL-3）・`util/cairo-fdr`・`test/pdiff`（GPL）・`doc/tutorial`。どれも compile されない。compile_commands の util の 4 件は cairo-missing と cairo-gobject で、GPL の一覧に無い。stage に道具も無い。
- 配る license: pixman `COPYING`、fribidi `COPYING`、cairo `COPYING`・`COPYING-LGPL-2.1`・`COPYING-MPL-1.1`、pango `COPYING`。いずれも `/usr/share/licenses/<名前>/` に入る（image で確かめた）。
- 新しい file（pango-probe）は Zlib。

### harfbuzz を hb-cairo つきで、freetype を harfbuzz つきで作り直すか

- GTK 4.18.6 が必須にしているのは harfbuzz と harfbuzz-subset で、harfbuzz-cairo ではない（port-contract §3）。pango 1.56 も hb-cairo を使わない。
- freetype の harfbuzz 統合（auto-hinter の script の範囲）も GTK の要件に無い。
- よって**どちらも作り直さない（記録だけ）**。GTK の build（p002）で要ると分かったら、その時に作り直す。

### 受け入れの判定（P3 の評価。確定は Q1）

- 4 package が build・install され、license の確認を通った: 満たす（手での分類）。
- guest で cairo の image surface に pango で日本語を含む文字列を描いた PNG を作り、host で目視で確かめた: 満たす（QEMU）。PNG は evidence/q608/pango-probe.png。ユーザーに見せるのは Q1。
- boot-test: 満たす（QEMU）。
- 実機: 未実施。

### 残り・申し送り

- libc の `alloca.h` と `getc_unlocked` を足すか（§11、記録だけ）。
- RTL の script の font（Arabic・Hebrew）は無い（範囲外）。
- worktree の build の `-o`（p006 と同じ）。
