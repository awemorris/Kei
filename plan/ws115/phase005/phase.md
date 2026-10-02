<!-- awesome-plan project=zedbsd record=ws115-p005 -->

# ws115-p005: libffi・pcre2・glib

Parent: [WS115](../ws.md)
Status: in-progress（q602-i01、P3。2026-10-02 の再起動のためのラップアップで中断。glib は build の途中、下の再開点）
Disposition: normal
Primary Milestone: MG002（WSから継承）
Queue / attempts: q602-i01（P3、中断。未完了）
Purpose / goal: GLib（gobject・gio・gmodule）を zedBSD target へ移植する。
Prerequisites: p004（meson の契約）、libc の不足の判断（p001）
Investigation bound: timebox 4h。libc の大きな不足が出たら uncleared で一覧を返す
Origin: WS034 p026（移管を main に依頼）

## 範囲

- `userland/packages/libs/{libffi,pcre2,glib}`。libffi は autotools＋libtool（version script を無効に）。glib の subproject の wrap はネットワークから取らせない（`--wrap-mode=nodownload`）。
- glib の file monitor・network・iconv・libintl の扱いは p001 の方針で patch か option。
- 同じ版の native の glib 道具（glib-compile-resources・glib-compile-schemas・glib-mkenums・gdbus-codegen）を host 用に作る。
- zedBSD guest での最小の実行確認（GMainLoop・GObject の型・gio の file 読み書き・GSettings の memory backend）。

## 受け入れ

3 package が build・install され、license audit を通る。guest で GLib の試験 program が期待どおりに動く（SSH/serial の操作と結果、`boot-test.sh` の起動確認）。

## 所有 path

`userland/packages/libs/{libffi,pcre2,glib}/`、`plan/ws115/`

## Standards / evidence / resume

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[scoped方針](../../standards/ws114-gtk-qt-learning.md)、[automation](../../standards/automation.md)、[license audit](../../tools/packages/audit-licenses.sh) を実装前に確認する。外部 package は公式 tarball を取得・検証して patch し、source tree へ取り込まない（AGENTS.md）。新コードは全文規約。toolchain（`toolchain/`・`lang/clang`・`devel/libcxx`・共有 `build/llvm` 等）は変更・build しない。共有 `build/` は読取専用、自分の worktree の `build/` を使う。全体の `make check` は禁止。具体コマンド/版/結果/commit/環境/成果物/skip/制限は未実施（計画のみ）。各PhaseはQueueの個別scope承認後に実行する。前提未達・判断待ちならattemptをunclearedとして証拠/再開条件を残す。

Event ws115-beta1-plan-20261002: 2026-10-02 計画担当が依存 package の移植 Phase として作成。版・option は p001 の移植契約で確定する。planning/Queue none。GitHub publication pending。

## q602-i01 の途中の記録（2026-10-02、P3、ユーザーの判断による再起動のためのラップアップ）

承認: Q1 の継続 dispatch（q602、時限 4h）、user「まずは素のGTK4を移植してください」。base は main 75e29a2ff。

### 済んだ物

- **pcre2 10.48**（`userland/packages/libs/pcre2/`）: CMake、8-bit だけ、JIT なし、`PCRE2_SYMVERS=OFF`。build は exit 0 で warning 0。check-dynamic-elf は SONAME libpcre2-8.so.0、NEEDED libc.so。commit 24c66c58e。
- **libffi 3.8.0**（`userland/packages/libs/libffi/`、WS126 と共有）: autotools と patch 0001（config.sub と libtool の dynamic linker の節に zedbsd）。`--disable-symvers --disable-exec-static-tramp`。build は exit 0。check-dynamic-elf は SONAME libffi.so.8、NEEDED libc.so。warning は upstream の java_raw_api の deprecated 2 件だけ。commit 24c66c58e。
- **glib 2.84.4**（`userland/packages/libs/glib/`、**build は未完了**）: `ZEDBSD_EXTERNAL_MESON`、依存は libffi・pcre2・zlib、`-Db_lundef=false`。packaging は runtime の library 5 つ。patch:
  - 0001: `<arpa/nameser.h>` が無ければ `<resolv.h>` の C_IN を使い、`HAVE_ARPA_NAMESER_H` を定義する。
  - 0002: `strings.h` を include する。
  - 0003: nameser が無いとき record の検索を refuse する。
  - 0004: IP の socket option・SOCK_SEQPACKET・FIONREAD・SOMAXCONN・IN_MULTICAST。
  - meson の構成は通る。`ninja -k 0` で見た compile の残りは、libintl の format_arg と CMSG_NXTHDR の 2 つだけ（どちらも libc）。
- **GApplication の session bus の警告の出所**: glib ではない。GApplication は session bus が無いと黙って non-unique で動く（`gapplicationimpl-dbus.c` 651–656）。警告は GTK の a11y（`gtkatspiroot.c:615`、`gtkatspicontext.c:1806`）から出る。よって patch は p010 で GTK に当てる。glib 側では、DISPLAY が無ければ autolaunch は spawn せずに error を返す。
- **libc の判断**（Q1 の許可、2026-10-02）:
  1. `include/libc/libintl.h` に `__format_arg__` を付ける。
  2. CMSG_NXTHDR を `include/libc/sys/socket.h` に足す（POSIX）。string.h から strings.h を読む変更は今回しない（WS001 の判断の後）。`-Db_lundef=false` は glib の個別の指定とし、共通化は p006 以降で決める。FIONREAD を kernel に足すかは別の判断（記録だけ）。
  - P3 による libc の編集は Claude Code の権限の仕組みに止められた。差分は `plan/ws115/proposed/libc-libintl-format-arg.diff` と `libc-cmsg-nxthdr.diff` に置いた。
  - libintl の差分は、止められる前に P3 の worktree で sysroot と disk-image を作り直して試した（exit 0、我々の warning 0。libc と base を含む）。その後、worktree の変更は元に戻した。
  - CMSG_NXTHDR は host 試験 `tests/cmsg-nxthdr-test.c` を差分を当てた header に対して走らせ、gcc と clang の両方で exit 0。
  - 再起動後の P3 が 2 つの差分を適用する（Q1）。
- libc と kernel の差の一覧は [port-contract §11](../port-contract.md) に残した。

### 未完了と再開点

1. 2 つの libc 差分を適用する（`include/libc/libintl.h`、`include/libc/sys/socket.h`）。`make sysroot-amd64` を走らせ、disk-image の build で warning 0 を確かめる。cmsg の host 試験も走らせる（compile の方法は試験の file の先頭）。
2. `make <vars> glib` を通す。check-dynamic-elf の NEEDED の契約（Makefile の `ZEDBSD_GLIB_NEEDED_*`）と実物を照合し、違えば直す。library の版の名前（`.8400.4`）も確かめる。
3. `sh plan/ws115/tests/glib-probe.sh <cross> build/packages <work>` で probe を作り、`plan/ws115/tests/config-amd64-glib.mk` と出力された `ZEDBSD_TEST_EXTRA_FILES` で test image を作る。`plan/tools/guest/amd64-serial.sh <image> 'glib-probe /usr/share/glib-probe/schemas /tmp/glib-probe.txt'` で `glib-probe: PASS` を確かめる。最後に `boot-test.sh` で起動を確かめる。
4. license の記録（glib の LICENSES/ と GPL を含む file の分類）と全文規約の確認をし、phase.md を cleared にするか判断する。
- 使う vars: `ZEDBSD_CONFIG=build/p3-q592/config.mk BUILD=build/p3-q592/zedbsd ZEDBSD_LLVM_SOURCE=/home/awe/zedBSD-claude1/build/llvm-source`（`build/p3-q600/vars`）。glib の probe と test image は未試験。
