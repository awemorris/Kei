<!-- awesome-plan project=zedbsd record=ws115-p005 -->

# ws115-p005: libffi・pcre2・glib

Parent: [WS115](../ws.md)
Status: cleared（2026-10-02 Q1。q602-i01、958b6c060 まで main に統合。license は手での分類で可、audit-licenses.sh の拡張は ws129-p002）
Disposition: normal
Primary Milestone: MG002（WSから継承）
Queue / attempts: q602-i01（P3 generation1 が中断、generation2 が継続して完了）
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

### 未完了と再開点（generation1 の時点。下の結果で済んだ）

1〜4 は下の「q602-i01 の継続の結果」で済んだ。

## q602-i01 の継続の結果（2026-10-02、P3 generation2）

承認: Q1 の継続 dispatch（q602）。libc の 2 つの差分はユーザーの明示の許可（このセッションのユーザーの発言「libcへの変更をあなたに明示的に許可します。」）。base は main 798ad97bc。vars は `build/p3-q600/vars`（`ZEDBSD_CONFIG=build/p3-q592/config.mk BUILD=build/p3-q592/zedbsd ZEDBSD_LLVM_SOURCE=/home/awe/zedBSD-claude1/build/llvm-source`）。

### commit

- 150c2f51f: libc の 2 差分（`include/libc/libintl.h`、`include/libc/sys/socket.h`）。`plan/ws115/proposed/` の差分のとおりに当てた。main が 871777b34 に統合（ACK 済み）。
- 31155f84d・6ca63b326: glib。patch 0004 の改訂、patch 0005 の追加、libgthread の NEEDED の契約、PATCH_LEVEL zedbsd7。
- 本 commit: probe の書き直し、cmsg の試験の compile の方法、port-contract §11、この記録。

### libc

- host 試験 `tests/cmsg-nxthdr-test.c`: gcc と clang の両方で exit 0（compile の方法は file の先頭。`-DKERN_UAPI_NATIVE -nostdinc -isystem include/libc -isystem include`）。
- `make <vars> sysroot-amd64`: exit 0。sysroot の `usr/include/libintl.h` に format_arg が 13 か所、`sys/socket.h` に CMSG_NXTHDR が入った。
- `make <vars> -j disk-image`: exit 0。libc と base を含めて 826 の compile、すべて `-Werror`。compiler の warning は upstream の NoctLang `interpreter.c:2395 -Wreturn-type` の 1 件だけで、この header とは関係が無い（toolchain の範囲で、触らない）。check-amd64-native-image OK。

### glib 2.84.4

- 初回の `make <vars> glib` は fuzz の `fuzz_resolver` の link で止まった。この target は record の parser `g_resolver_records_from_res_query` を呼ぶが、0003 がその parser を外している。**patch 0005**: `arpa/nameser.h` が無ければ、`fuzzing/meson.build` でこの target だけ外す（`have_arpa_nameser_h` は 0001 が決める）。
- 次は check-dynamic-elf で止まった。libgthread の実物の NEEDED は libglib-2.0.so.0 だけ。libc の symbol を使わないので、`--as-needed` で libc.so が外れる。**契約を実物に合わせた**（`ZEDBSD_GLIB_NEEDED_gthread`）。ほかの 4 つは契約どおり（glib: pcre2-8・c、gobject: glib・ffi・c、gio: glib・gobject・gmodule・z・c、gmodule: glib・c）。SONAME はどれも `lib*-2.0.so.0`、実体は `.so.0.8400.4`。
- **patch 0004 の改訂**: 0004 のせいで gsocket.c に warning が 7 件出ていた。
  - getter 3 つの `-Wuninitialized`: option が無いときは `value = 0` にする。
  - group membership の local 3 つが unused: その宣言を `G_SOCKET_HAVE_IP_OPTIONS` で囲む。
  - `g_socket_get_adapter_ipv4_addr` が unused: 条件に `G_SOCKET_HAVE_IP_OPTIONS` を足した。option がある platform では元の条件と同じになる。
  - 改訂後、patch を当てた file（gsocket.c・ginetaddress.c・gthreadedresolver.c・glib-init.c・gstrfuncs.c・fuzzing）の warning は 0。残りの約 50 件は upstream の物（G_DEFINE_TYPE の parent_class、gtestutils の format、gnulib）。
- 最終の `make <vars> glib`: exit 0、`.zedbsd-checked` ができた（`build/p3-q602/glib5.log`）。libgirepository-2.0 も build されるが、package には入れない。

### 試験 program（QEMU）

- `tests/glib-probe/glib-probe.c`: generation1 の版は compile されていなかった。`g_memory_settings_backend_new` には `G_SETTINGS_ENABLE_BACKEND` と `gio/gsettingsbackend.h` が要る。parent_class は使われていなかった。そこで全文規約に合わせて step ごとの関数に書き直した。call の結果を直接 return しない、finalize は parent に chain up する、失敗した GLib の call は GError の message を出す。
- `sh plan/ws115/tests/glib-probe.sh build/amd64/packages/toolchain build/packages build/p3-q602/probe`: exit 0。probe の NEEDED は check-dynamic-elf を通った。schema は host の glib-compile-schemas 2.84.4（Debian 2.84.4-3~deb13u3）で `--strict` で compile した。
- test image: `ZEDBSD_CONFIG=plan/ws115/tests/config-amd64-glib.mk BUILD=build/p3-q602/img ZEDBSD_TEST_IMAGE_TAG=glibprobe ZEDBSD_TEST_EXTRA_FILES=<probe の出力>` で disk-image を作り、exit 0。rootfs に 5 つの library、libffi・pcre2・zlib、probe、schema、`/usr/share/licenses/{glib,libffi,pcre2,zlib}` が入った。
- guest（QEMU q35・KVM・8 GiB、serial で操作）: `glib-probe /usr/share/glib-probe/schemas /tmp/glib-probe.txt; echo status=$?` の結果は `glib-probe: PASS glib 2.84.4` と `status=0`。`cat /tmp/glib-probe.txt` は `zedBSD GIO`。確かめたもの: GObject の signal（libffi の generic marshaller）、GMainLoop と timeout、GIO の file の書き込みと読み戻し、GRegex（PCRE2）、GSettings の memory backend（default の 7 と、書いた 9）、GThread。判定は probe の stdout で、console の log は読んでいない。
  - 道具の問題: `plan/tools/guest/amd64-serial.sh` は固定の `sleep 2` の後に serial の socket へつなぐ。この host では `snapshot=on` の drive の open に 8〜11.5 秒かかり（`-S` で測った）、socket がまだ無くて失敗した。共有の道具は main の所有なので変えず、scratch の写しの `sleep 2` を「socket ができるまで最大 60 秒待つ」に置き換えて走らせた（ほかは同じ）。道具の修正は Q1 に依頼する。
- `plan/tools/boot-test.sh`（uefi-nvme）を 2 つの image で走らせた。`BOOT_TEST_WORK` を scratchpad にすると QMP の socket の path が 108 byte を超えたので、`/tmp/p3bt` を使った。
  - 通常の image（libc を変えた物、`build/p3-q592/zedbsd/hdd-image.img`）: PASS、`build/p3-q602/boot-test-main/login.png`。
  - glib の test image: PASS、`build/p3-q602/boot-test-glib/login.png`。
  - どちらの PNG も login prompt が出ていることを目で確かめた。

### license

- `plan/tools/packages/audit-licenses.sh` は openssl と openssh の tarball だけを見る（main の道具）。glib・pcre2・libffi は同じ検索（`GNU General Public License|SPDX-License-Identifier: *GPL`）を 3 つの tarball に当て、手で分類した。
  - glib の 11 件: LICENSES/ の本文 3、build の時の script（gen-unicode-tables.pl、tests の gen-case*.py、glib-genmarshal.in、glib-mkenums.in、po2tbl.sed.in、glib-gettextize.in）、`glib/valgrind.h`（その file だけ BSD 型だと明記してある）。
  - libffi の 34 件と pcre2 の 12 件: autotools の生成物と道具（config.guess・ltmain.sh・m4 など。例外付きで、build にしか使わない）、libffi の testsuite、ChangeLog.old、LICENSE-BUILDTOOLS。
  - どれも配る library には入らない。
- 配る glib の 5 library に compile された source（compile_commands.json）の分類: SPDX が LGPL-2.1-or-later の物が 334。tag の無い物が 8 で、本文が LGPL-2.1-or-later（ggtknotificationbackend.c、gallocator.c、guuid.c、gmarshal.c）、gdbus-codegen と glib-mkenums の生成物（LGPL と、同じ license という明記）、notice の無い gmodule-deprecated.c（project の COPYING が LGPL-2.1-or-later）。GPL の物は無い。
- 配布物: glib は COPYING（LGPL-2.1）、libffi は LICENSE（MIT）、pcre2 は LICENCE.md（BSD-3-Clause と例外）を `/usr/share/licenses/` に入れる。LGPL の library は共有 library として配り、置き換えられる。

### 範囲の一点

- 範囲の「同じ版の native の glib 道具を host 用に作る」は、p004 の契約（port-contract §4 の host 道具の表と §9 の p004 の行）で「作らない。host の 2.84.4 を使う」に決まっている。schema の compile は host の 2.84.4 で行った。

### 受け入れの判定（P3 の評価。確定は Q1）

- 3 package が build・install され、license の確認を通った: 満たす。ただし audit-licenses.sh はこの 3 つの tarball を見ないので、同じ検索と手での分類で代えた。道具を広げるかは Q1 が決める。
- guest で試験 program が期待どおりに動いた: 満たす（QEMU。serial の操作と probe の PASS）。
- boot-test.sh の起動の確認: 満たす（QEMU。2 つの image）。
- 実機: 未実施。

### 残り・申し送り

- `amd64-serial.sh` の socket を待つ処理（main の道具）。
- `audit-licenses.sh` を GTK の依存の tarball に広げるか（main の道具）。
- FIONREAD と IP の socket option を kernel に足すか、`-Db_lundef=false` を共通の規則にするか（§11。別の判断）。
- `plan/ws115/proposed/` の 2 つの差分は適用済みで、記録として残した。
