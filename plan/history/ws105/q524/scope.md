<!-- awesome-plan project=zedbsd record=ws105-p002 -->

# ws105-p002: build の土台（`keiland-linux.mk`・top-level の goal・library の `Makefile.linux`）

Status: planned
Disposition: normal
Parent: [WS105](../ws.md)
Queue: なし
依存: WS104 の p001（header が `userland/desktop/keiland/`）・p003（libkeiland の `zedbsd/`）
実行者: phase-runner（high）。`plan/tools/keiland-linux/` の script は main が merge する

## 目的

決定 D1・D2。Linux の build を、zedBSD の build と完全に別の GNU make の file で作る（[design.md](../design.md) §2・§3）。この Phase では library と小さな部品だけを
build・install し、program（compositor・app）は後の Phase で足す。**`keiland-linux.mk` の検証済みの雛形が design §3.2 にある。** そのまま置く。

## 作る・変える file

| file | 内容 |
| --- | --- |
| `Makefile`（top-level） | design §3.1 の block（4 つの goal）を `managed-lan-host-test` の規則の後に。4 つの goal を `ZEDBSD_CONFIG_OPTIONAL_GOALS`（44-53 行）に足す。`help` に 1 行ずつ |
| `userland/desktop/keiland-linux.mk` | design §3.2 の雛形をそのまま。加えて: (1) `KEILAND_LINUX_PACKAGES ?=` の既定の値を下の 8 つの `Makefile.linux` の path（この順）、(2) static の library の macro `KEILAND_LINUX_STATIC`（下）、(3) `install-session:`（p009 まで中身は `@:`） |
| `userland/base/libz-compat/Makefile.linux`・`libpng-compat`・`libjpeg-compat`・`libgif-compat` | design §3.3 の表の SONAME・依存・system の library。exports.map は各 directory の物 |
| `userland/desktop/linux-compat/Makefile.linux` | design §3.4。`src/libc/openbsd-sha2.c`・`openbsd-digest.c` を `KEILAND_LINUX_STATIC` で `lib/libkeiland-compat.a` に（install しない） |
| `userland/desktop/libwayland/Makefile.linux` | `libwayland-client.so`（zedBSD の `Makefile` の 20 個の source、exports.map） |
| `userland/desktop/libtruetype/Makefile.linux` | `libtruetype.so`（`-lm`） |
| `userland/desktop/libkeiland/Makefile.linux` | `libkeiland.so`: 共通の source（`userland/desktop/libkeiland/Makefile` の `LIBKEILAND_SOURCES`）と、Linux の仮の backend（下）。依存 `libwayland-client.so`、system `-lm`。`# keiland-linux-sync: skip` の行で `zedbsd/*` と `userland/base/net/*` を外す |
| `userland/desktop/libkeiland/wpa/network-wpa.c`（仮） | `keiland_network_open`・`close`・`update`・`get_state`・`get_scan`・`request`・`get_request` の最小の実装: open は記録を `calloc` するだけ（NULL なら ENOMEM）、update は `*changed = 0` で 0、state は全て 0（`reachable = 0`・`wifi = KEILAND_WIFI_ABSENT`）、scan は 0 件、request は `ENOTCONN`、get_request は `KEILAND_NETWORK_REQUEST_NONE`。本物は p010 |
| `userland/desktop/libkeiland/linux/network-link-linux.c`（仮） | `keiland_network_get_links`・`get_dns`・`save_key`・`get_saved` の最小: links 0 件、DNS は `/etc/resolv.conf` の `nameserver` を読む（`zedbsd/network-link-zedbsd.c` の DNS の部分と同じ読み方で書いてよい）、save_key は `ENOTSUP`、saved 0 件 |
| `userland/desktop/libkeiland/linux/audio-linux.c`（仮） | `keiland_audio_*` の最小: open は記録を `calloc`、`fd` は -1、update は `*changed = 0`、state は `reachable = 0`、set_volume・feedback は `ENOTCONN`、`keiland_audio_available` は 0 |
| `plan/tools/keiland-linux/makefile-sync.sh` | design §3.3 の source の一覧の確かめ |
| `plan/tools/keiland-linux/elf-check.sh` | design §9.1 の ELF の確かめ |
| `plan/tools/keiland-linux/header-check.sh` | design §3.4 の header の漏れの確かめ |
| `plan/tools/keiland-linux/lib-smoke.c` | 下の確かめ 4 の program |

`KEILAND_LINUX_STATIC` の macro（`keiland-linux.mk` に足す）:

```make
# $(1) name, $(2) the archive's file name, $(3) sources
define KEILAND_LINUX_STATIC
KEILAND_LINUX_OBJS_$(1) := $$(patsubst %.c,$(KEILAND_LINUX_BUILD)/obj/%.o,$(3))
-include $$(KEILAND_LINUX_OBJS_$(1):.o=.d)
$(KEILAND_LINUX_BUILD)/lib/$(2): $$(KEILAND_LINUX_OBJS_$(1))
	@mkdir -p $$(dir $$@)
	rm -f $$@
	ar rcs $$@ $$(KEILAND_LINUX_OBJS_$(1))
KEILAND_LINUX_ALL += $(KEILAND_LINUX_BUILD)/lib/$(2)
endef
```

仮の backend の file の先頭の注釈に「placeholder until ws105-p010」と書く。仮でも `keiland.h` の約束（返す errno の種類、NULL を返す条件）に従い、coding-style.md を適用する。

### 道具の中身

- `makefile-sync.sh`（POSIX の sh）: 引数無しで、`userland/*/*/Makefile.linux` がある全ての package について、`Makefile` の中の `userland/[^ ]*\.c` の token の集合 A と
  `Makefile.linux` の中の同じ token の集合 B を作る。A − B のうち `/zedbsd/` を含まず、`Makefile.linux` の `# keiland-linux-sync: skip <glob>` のどれにも当たらない物があれば
  `makefile-sync: FAIL <package> missing <path>`。B − A のうち `/linux/`・`/wpa/` を含まず、`# keiland-linux-sync: only <glob>` にも当たらない物があれば `... extra <path>`。
  無ければ `makefile-sync: PASS`。
- `elf-check.sh STAGE`: `find $STAGE -type f` の ELF（`file` か先頭の 4 byte）ごとに `readelf -d`: RUNPATH が `[/opt/keiland/lib]`、library なら SONAME が design §3.3 の表の物、
  NEEDED の我々の library が `$STAGE/opt/keiland/lib` にある。違反を 1 行ずつ出し、最後に `elf-check: PASS`・`FAIL`。
- `header-check.sh`: `make -s -f userland/desktop/keiland-linux.mk print-sources`（`keiland-linux.mk` に `print-sources:` の goal を足す: 全ての package の source を 1 行ずつ出す）の
  source ごとに、同じ flag で `$(CC) -M` し、出力に `/usr/include/wayland-`・`/usr/include/EGL`・`/usr/include/GLES` があれば `header-check: FAIL <source> <header>`。無ければ `header-check: PASS`。

## 手順（repo の root で）

```
make -j64 keiland-linux 2>&1 | tee build/keiland-linux-build.log | tail -3; echo "exit=${PIPESTATUS[0]}"
make -j64 keiland-linux CC=clang KEILAND_LINUX_BUILD=build/keiland-linux-clang 2>&1 | tail -3
make keiland-linux-install DESTDIR=$PWD/build/keiland-linux/stage
ls build/keiland-linux/stage/opt/keiland/lib/
sh plan/tools/keiland-linux/elf-check.sh build/keiland-linux/stage
sh plan/tools/keiland-linux/makefile-sync.sh
sh plan/tools/keiland-linux/header-check.sh
mkdir -p build/keiland-linux/test
cc -std=gnu17 -Wall -Wextra -Werror -o build/keiland-linux/test/lib-smoke plan/tools/keiland-linux/lib-smoke.c -Iuserland/desktop/keiland \
   -Lbuild/keiland-linux/lib -l:libkeiland.so -Wl,-rpath-link,build/keiland-linux/lib -Wl,-rpath,/opt/keiland/lib
timeout 30 env LD_LIBRARY_PATH=build/keiland-linux/stage/opt/keiland/lib build/keiland-linux/test/lib-smoke
make keiland-linux-clean && ls build/keiland-linux/
make -j64 disk-image > build/ws105-p002-zedbsd.log 2>&1; echo "zedbsd make exit=$?"
```

`lib-smoke.c`: `keiland_version()` が 21、`keiland_network_open()` が NULL でなく `keiland_network_get_state` の `reachable` が 0、`keiland_audio_available()` が 0、`keiland_network_get_dns` が
落ちない。全て通れば `lib-smoke: PASS`、exit 0。

## 完了の条件

1. gcc と clang の build が `exit=0`（`-Werror` なので warning 0）。
2. stage の `lib/` に 7 つの `.so`（`libz-compat`・`libpng-compat`・`libjpeg-compat`・`libgif-compat`・`libwayland-client`・`libtruetype`・`libkeiland`）。`libkeiland-compat.a` は install されない。
3. `elf-check: PASS`、`makefile-sync: PASS`、`header-check: PASS`、`lib-smoke: PASS`。
4. `make keiland-linux-clean` が `build/keiland-linux/` の build の物だけを消し、`build/keiland-linux/guest`（p001）を残す。
5. zedBSD の build に影響が無い（`zedbsd make exit=0`。`Makefile.linux` は include されない）。共通の source を直したなら design §9.2 の回帰も（直した file と警告を結果に書く）。

## 結果

（実行の後に書く）
