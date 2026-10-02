<!-- awesome-plan project=zedbsd record=ws115-p004 -->

# ws115-p004: meson のクロス契約と host 道具

Parent: [WS115](../ws.md)
Status: cleared（q600-i01、P3、2026-10-02。Queue と共有記録への投影は Q1）
Disposition: normal
Primary Milestone: MG002（WSから継承）
Queue / attempts: q600-i01（P3、cleared）
Purpose / goal: GTK4 とその依存を meson で zedBSD target へ cross build できるようにする。
Prerequisites: p001 の契約、共有 path（external.mk・gen-cross-toolchain.sh）の main の割当
Investigation bound: timebox 4h / 1 Queue
Origin: WS034 p025（移管を main に依頼）

## 範囲

- `gen-cross-toolchain.sh` に meson の cross file（compiler wrapper・sysroot・pkg-config・`host_machine` の system 名）を生成させ、`external.mk` に meson/ninja で build する package の共通の規則を足す。CMake/autoconf の既存の契約と同じ wrapper を使う。
- pkg-config の wrapper（sysroot と package の prefix）、libtool の共有ライブラリの問題（inventory §3.2）、symbol versioning の無効化の共通の対応。
- host 道具: p001 で要ると決めたもの（meson の版、gperf の source build、native の glib 道具は p005 で glib と同じ版を作る、wayland-scanner）を `build/` の下の host tools として作る規則。
- 試験: 小さな meson の試験 package（例: graphene を先に通すか、試験用の 1 file の library）で cross file が通ることを確かめる。

## 受け入れ

meson 構成の小さな package が target 向けに build され、ELF の machine・interpreter・NEEDED が正しい。CMake/autoconf の既存 package（zlib・expat・curl）の build に回帰が無い。

## 所有 path

`userland/packages/external.mk`・`userland/packages/tools/`（共有、main の割当が要る）、`plan/ws115/`

## Standards / evidence / resume

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[scoped方針](../../standards/ws114-gtk-qt-learning.md)、[automation](../../standards/automation.md)、[license audit](../../tools/packages/audit-licenses.sh) を実装前に確認する。外部 package は公式 tarball を取得・検証して patch し、source tree へ取り込まない（AGENTS.md）。新コードは全文規約。toolchain（`toolchain/`・`lang/clang`・`devel/libcxx`・共有 `build/llvm` 等）は変更・build しない。共有 `build/` は読取専用、自分の worktree の `build/` を使う。全体の `make check` は禁止。具体コマンド/版/結果/commit/環境/成果物/skip/制限は未実施（計画のみ）。各PhaseはQueueの個別scope承認後に実行する。前提未達・判断待ちならattemptをunclearedとして証拠/再開条件を残す。

Event ws115-beta1-plan-20261002: 2026-10-02 計画担当が依存 package の移植 Phase として作成。版・option は p001 の移植契約で確定する。planning/Queue none。GitHub publication pending。

## q600-i01 の結果（2026-10-02、P3）

承認: Q1 の継続 dispatch（q600、時限 4h）。S1 は Q1 が P3 に割り当てた: `userland/packages/external.mk` と cross file の生成 script（新規）。toolchain は変えていない。base は main 4696a2152、途中で main acb5a2da9（`include/libc/sys/poll.h`）を取り込んだ。版に依らない形で作った（D-VER が変わっても meson の program の変数だけで済む）。

### 作った物

- **`userland/packages/tools/gen-meson-cross.sh`（新規）**: gen-cross-toolchain.sh が作った wrapper を使い、次の 3 つを書く。
  - `meson-cross.ini`: `system=zedbsd`、`needs_exe_wrapper`、`default_library=shared`、`wrap_mode=nodownload`。`sys_root` は置かない。
  - `meson-native.ini`: build machine の cc・c++・pkg-config。
  - `bin/zedbsd-pkg-config`: build ごとの「view」を `PKG_CONFIG_SYSROOT_DIR` にする。build machine の .pc は読まず、system dir による削除も無効にする。
- **`external.mk`**
  - `packages-meson-cross` と `ZEDBSD_EXTERNAL_MESON_STAMP`。
  - `ZEDBSD_EXTERNAL_MESON_PROGRAM`/`_NINJA_PROGRAM`。meson 1.12 などに差し替えられる。
  - `ZEDBSD_EXTERNAL_HOST_BIN_DIRS`: host 道具を PATH に置く。
  - `ZEDBSD_EXTERNAL_CLOSURE`: 依存の推移閉包。
  - `ZEDBSD_EXTERNAL_VIEW`: 依存の stage を symlink で 1 つに集めた directory。同じ file を持つ依存があれば cp が失敗する。
  - `ZEDBSD_EXTERNAL_MESON`: meson package の configure と stage の規則。`ZEDBSD_EXT_<name>_{MAKEFILE,MESON_OPTIONS,DEPENDS,HOST_TOOLS}` を読み、`_CONFIGURED`・`_STAGED` を出す。`-Wl,-rpath-link,<view>/usr/lib` を付ける。
  - `build-jobs.mk` の include（standalone の build でも job の数が決まるように）。
  - 依存の約束: どの package も stage の stamp を `<stage>/.zedbsd-staged` に置き、`ZEDBSD_EXT_<name>_DEPENDS` で直接の依存を書く（無ければ空）。
- **`userland/packages/devel/gperf/Makefile`（新規）**: gperf 3.3（inventory の size と SHA-256、GPL-3.0-or-later）を host の cc/c++ で build する。userland package としては登録しないので、menu にも image にも入らない。target は `gperf-host`、`ZEDBSD_GPERF_HOST` と `ZEDBSD_EXTERNAL_HOST_BIN_DIRS` に登録する。
- 試験（`plan/ws115/tests/`）:
  - `meson-probe/`（C の試験 project）と `meson-probe.sh`: shared library と SONAME、executable、pkg-config で zlib を探す、host の glib が見えない、native の生成 program、.pc の生成、ELF の検査。
  - `meson-package-probe.mk`: 本物の upstream graphene 1.10.8 を `ZEDBSD_EXTERNAL_MESON` の規則で build し、ELF を検査する。

### 確認（P3 の worktree、host の meson 1.7.0・ninja 1.12.1・pkgconf 1.8.1、target は clang 23.1.0）

| 確認 | 結果 | 証拠 |
| --- | --- | --- |
| gperf の host build | `make gperf-host` で exit 0、`gperf --version` は 3.3 | build/p3-q600/gperf-build.log |
| cross file の生成 | `make packages-meson-cross` で 3 file。meson は host を `zedbsd-clang`（clang 23.1.0）と `ld.lld 23.1.0`、build を gcc 14.2.0、cpu family を x86_64 と認識した | build/p3-q600/probe.log |
| pkg-config の分離 | view が無いと zlib は見えない。host の glib-2.0 も見えない。view があると `-I<view>/usr/include -L<view>/usr/lib -lz` | 同 |
| 試験 project | `meson-probe: PASS`。libprobe.so.1.0.0 は SONAME libprobe.so.1、NEEDED は libz.so.1 と libc.so、rpath なし。probe-check は NEEDED が libprobe.so.1 と libc.so、interpreter /lib/ld.so、x86-64。native 生成も通った | 同 |
| upstream の meson package（graphene、`ZEDBSD_EXTERNAL_MESON` の規則） | `meson-package-probe: PASS`。libgraphene-1.0.so.0 の SONAME と NEEDED libc.so。upstream の source の warning が 59（-ffast-math による `-Wnan-infinity-disabled` 58 と `-pthread` の未使用 1）。我々の source ではない | build/p3-q600/package-probe.log |
| 既存 package の回帰 | zlib・expat・curl の `make -n -B` の package の recipe の行は変更の前後で同一。zlib と expat を実際に build し直して exit 0（check-dynamic-elf を通る） | build/p3-q600/{before,after}-*.txt、zlib-expat-build.log |
| sysroot への `sys/poll.h` の反映 | sysroot の入力は `find include/libc …` で決まるので、新しい header は sysroot を out of date にする（`make -q` が 1）。`make sysroot-amd64` で `usr/include/sys/poll.h` が入った。glib 2.84.4 の meson 構成は `sys/poll.h: YES`、POLL* の値も通り、pcre2 の依存が無い所まで進む（p005 の入力） | build/p3-q600/sysroot.log、glib-probe.log |
| 規約 | shell は `sh -n` OK、新しい C は style-check total 0、`git diff --check` OK。clang-format は定義の引数の継続を 4 空白にしたがるが、全文規約の tab のままにした（automation の「定義の引数を戻す」） | — |

### 範囲から外した物・残り

- libtool の zedbsd 対応の共通化: autotools だけで作る package は libffi だけなので、p005 で個別に扱う。symbol versioning の無効化も package ごとの option で行う（inventory §3.3）。
- 本 tree の library（libwayland-client・libvulkan・libEGL など）の .pc: view に入れる「疑似 package の stage」として p009 で作る。
- meson 1.12（4.24 用）は `ZEDBSD_EXTERNAL_MESON_PROGRAM` で切り替えられる。今は host の 1.7。
- curl は openssl の build が要るので、dry-run の recipe の比較だけ。image の build と boot-test は行っていない（image の中身は変わらない）。
- p011（libkeiland の file chooser の backend）と D-Bus 無し（P1）は、GTK の option と patch で後から足せる。p004 の仕組みには影響しない。
