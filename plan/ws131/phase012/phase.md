<!-- awesome-plan project=zedbsd record=ws131-p012 -->

# ws131-p012: libkeiui を libkeiland へ移す（名前は変えない）

Status: planning（p002 第 2 版はユーザーのレビュー済み（2026-10-03、D7 は確認中）。開始はユーザーの承認と P2 の終了の後に Q1 が指示）
Disposition: normal
Parent: [WS131](../ws.md)、計画の正本 [design.md](../design.md)
Queue: none
依存: p011 cleared（D8 の番号の順）、p003 が main に統合済み。WS090 は WS131 の間は動かない（D9 の決定）。判断 D10（承認済み）
目安: 4h（1 Queue）。実行者: Q1 が割り当てる（high）
所有 path: `userland/desktop/libkeiui/` → `userland/desktop/libkeiland/ui/`、`userland/desktop/libkeiland/`（Makefile 3 本・exports.map）、利用者の build の file（Text Editor・Image Viewer・PDF Viewer・Notes・Terminal・keiland-ime・kuidemo・**Files**）、`userland/desktop/files/ui-scrollbar.c` の include、`keiland-linux.mk`・`keiland-freebsd.mk`（package の一覧と公開の header の表 `:184-187`）、`plan/ws131/`。（Q1 の委任が要る: Q1 が Phase の前に委任を記録する）: `platform/amd64/vmunix.mk`（libkeiui.so の規則と 7 つの link）、`config/ci/config-amd64.mk:37` と試験の config 4 本の package 名、`plan/tools/keiland-linux/elf-check.sh`、§2.4 の host の試験 13 file の path

## 目的と結果

libkeiui の source を `libkeiland/ui/` に移し、`libkeiui.so` を無くす。名前はまだ `kui_` のまま（改名は p013）。app の source は include を除き無変更。

## 範囲

1. `git mv userland/desktop/libkeiui/* userland/desktop/libkeiland/ui/`。`picture/color-glyph.c` を libkeiland の source 一覧へ。
2. build: libkeiland の Makefile 3 本に `ui/` と依存（libpng-compat・libz-compat・libvulkan、Linux では libtruetype も）。`vmunix.mk` の libkeiui.so の規則（`:882-896`）と image の data を除き、7 つの app の link から `libkeiui.so` を除く。Files の 3 本の Makefile の `libkeiui/scroll-bar.c` を新しい path に（または libkeiland の link に替える）。`keiland-linux.mk:109`・FreeBSD の同等の include を除く。
3. 取りこぼし（review 2）: package 名 `libkeiui` の 5 か所（design.md §2.4）、FreeBSD の公開の header の表（`keiui.h` は p013 まで残す）、`elf-check.sh:17`、Linux・FreeBSD の source の install で古い `libkeiui.so` を消す規則。最初に `grep -rn libkeiui` を tree 全体で流し、全ての所を phase.md に列挙してから直す。
4. exports.map を公開の header の関数の一覧から生成する形に（design.md §5.4、B4）。

## 受け入れ

- 3 OS の build（zedBSD の amd64 は exit 0・自前の warning 0、`make keiland-linux` の gcc と clang、FreeBSD の native build と `native-build-audit.py`）と `plan/tools/keiland-os-boundary/check.sh` PASS。
- `grep -rn libkeiui` が tree（history と plan の文書を除く）で 0。`nm -D libkeiland.so` が旧 libkeiui.so と旧 libkeiland.so の公開の関数の和と一致し、内部の名前を出さない（B4）。
- zedBSD: ws090 の host 試験（host-draw・host-input・host-widgets）・`plan/tools/keiui/host-chooser.sh`・`plan/ws127/tests/scroll-bar-test.sh`、`textinput-p013.sh`、`viewers-p008.sh`、`demo-s8-s9.sh`、Files の `files-regress.sh`、boot-test。Linux: 8 つの app の起動の PNG、`elf-check.sh`。

## 検証の方法

[zedbsd-commands.md](../../tools/keiland-linux/zedbsd-commands.md)、[keiland-linux/README.md](../../tools/keiland-linux/README.md)、[keiland-freebsd/README.md](../../tools/keiland-freebsd/README.md)。QEMU の console・serial の log で判定しない（不具合は gdbstub・monitor・QMP）。build は自分の `BUILD=build/ws131-pNNN/`、共有の `build/` を消さない。FreeBSD の起動の確認は判断 D11 の範囲（passthrough なし）。QEMU と実機の証拠を分ける。

## 衝突・危険・rollback

- 衝突: 全 app の build の file と `vmunix.mk` を触る。D8 の単独走行の間は他の担当が動かない。 順は Q1 が決める（design.md §7.2）。
- rollback: 前へ直すのを基本にし、後ろに同じ file の変更が無い時だけ統合の commit を revert する（design.md §7.4）。

## Resume

依存の Phase の cleared と main への統合、関係する判断の決定の後に、Q1 が Queue を作る。
