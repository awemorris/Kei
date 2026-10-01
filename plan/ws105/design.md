# WS105 設計: Keiland を Linux で動かす（`/opt/keiland`）

2026-10-01 に Q1 が、ユーザーとの検討（[ws.md](ws.md) の「決定と理由」D1〜D25）と code の調査から書いた。**決定の理由は ws.md に、仕組みはこの文書にある。**
**改訂 2（2026-10-01）**: 6 つの survey（zedBSD の command、WS104 の編集、compositor の編集、host の試しの compile、host の Vulkan・dma-buf・ELF の実験、Linux の guest・protocol）と
design-reviewer の敵対的な見直し（blocker 3・major 15）の結果を入れた。実験の code は [survey/](survey/README.md)。
code の行番号は 2026-10-01 の main（`0eb5e118`）の物で、WS104 の後は変わっている。WS104 が作る境界（`zwl-gpu.h`・`zwl-input.h`・`zwl-os.h`・
`libkeiland/zedbsd/`・`userland/desktop/paths.h`・`userland/desktop/keiland/`）を前提にする。

## 0. 読み方と用語

- **この WS の agent は、Phase を始める前に ws.md の「決定と理由」とこの文書の §1・§2 を必ず読む。** libvulkan-compat を触る Phase（p003〜p005）は §4 の全部を読む。
- 「zedBSD の」と「Linux の」を必ず区別する。同じ名前の物が二つある:

| 名前 | zedBSD | Linux |
| --- | --- | --- |
| libvulkan | `userland/desktop/libvulkan/`: Venus と i915 の **driver そのもの** | `userland/desktop/libvulkan-compat/`: **WSI だけを持ち、他は system の libvulkan に渡す** library（§4）。install の名前は `libvulkan.so.1` |
| GPU の buffer の protocol | `keiland_gpu_buffer_v1`（kernel の image の capability と記述） | `zwp_linux_dmabuf_v1`（Linux の標準。dma-buf と modifier） |
| fence | `keiland_gpu_buffer_v1.set_acquire_fence`（世代つきの fence の fd） | implicit sync（dma-buf に付いた fence。sync_file の ioctl で出し入れ） |
| 画面 | zedBSD の libvulkan の VK_KHR_display（kernel の display の ioctl） | libvulkan-compat の VK_KHR_display（Linux の KMS の ioctl、複写の道） |
| 入力 | `/dev/input/eventN`（evdev と同じ API）、sessiond が持ち主を変える | `/dev/input/eventN`（evdev）、root か logind の `TakeDevice` |
| session | sessiond（greeter・login・Log Out・Shut Down） | gdm（または text console から root で起動） |
| network・音 | networkd・audiod（libkeiland の `zedbsd/`） | wpa_supplicant・ALSA（libkeiland の `wpa/`・`linux/`） |

用語:

- **後段（backend）**: libvulkan-compat が chain する先の system の `libvulkan.so.1`（Debian では `/usr/lib/x86_64-linux-gnu/libvulkan.so.1`、Khronos の loader）。
- **ICD**: Khronos の loader が読む driver（Mesa の lavapipe・anv・radv・venus など）。
- **WSI**: Vulkan の画面に出す部分（`VK_KHR_surface`・`VK_KHR_swapchain`・`VK_KHR_wayland_surface`・`VK_KHR_display`）。
- **dma-buf**: Linux の kernel の、device の間・process の間で共有できる buffer の fd。
- **modifier**: dma-buf の画素の並べ方（tiling）を表す 64 bit の値。`DRM_FORMAT_MOD_LINEAR`（0）は行の順の素直な並び。
- **sync_file**: Linux の kernel の fence の fd。`poll` で readable になったら済み。
- **implicit sync**: fence を protocol で送らず、dma-buf 自体に付けておく方法。`DMA_BUF_IOCTL_IMPORT_SYNC_FILE` で付け、`DMA_BUF_IOCTL_EXPORT_SYNC_FILE` で取り出す（Linux 6.0 から）。
- **KMS**: Linux の kernel の画面の設定と表示の ioctl（DRM の一部）。**DRM master** は KMS の操作をしてよい 1 つの fd。
- **logind**: systemd の session の管理。gdm の下の session の process に、DRM と入力の device の fd を `TakeDevice` で渡す（D-Bus）。
- **seat の backend**: compositor の中で、DRM の master の fd と入力の device の fd を得る部分（`seat-direct`: root で直接 open、`seat-logind`: logind から）。

## 1. 全体の形

```
                       zedBSD（今、WS104 の後）                         Linux（WS105 の後）
 app        /bin/terminal など                                  /opt/keiland/bin/terminal など（同じ source、Makefile.linux）
 library    /lib/libkeiland.so（zedbsd/ の backend）            /opt/keiland/lib/libkeiland.so（wpa/・linux/ の backend）
            /lib/libwayland-client.so（我々の）                 /opt/keiland/lib/libwayland-client.so（同じ source）
            /lib/libvulkan.so（Venus・i915 の driver）          /opt/keiland/lib/libvulkan.so.1（libvulkan-compat）→ /usr/lib/.../libvulkan.so.1（system）→ Mesa
 compositor /bin/wayland（zedbsd/ の module）                   /opt/keiland/bin/wayland（linux/ の module）
 起動       sessiond → greeter → session                         gdm → keiland.desktop → /opt/keiland/bin/wayland（または text console から root）
```

- **同じ source を zedBSD と Linux で build する。** OS で違う所は `<package>/zedbsd/`・`<package>/linux/`・`<package>/wpa/` の file の入れ替えで、
  macro の block は `wayland/zwl-evdev.h` の 1 つ（と install の path の `-D`）だけ。
- Linux の build は zedBSD の build（cross の toolchain・sysroot・image）と完全に別で（D2）、host の compiler（既定 `cc`）と host の header（glibc・
  linux-libc-dev・`/usr/include/vulkan`）を使う。
- install は全て `/opt/keiland/` の下（D1）。例外は gdm に見せる `/usr/share/wayland-sessions/keiland.desktop` の 1 file（D11、p009、`make keiland-linux-install-session`）。

## 2. install の配置（`/opt/keiland`、`KEILAND_PREFIX` で変えられる）

| directory | 中身 |
| --- | --- |
| `bin/` | `wayland`（compositor）、`terminal`・`files`・`settings`・`notes`・`textedit`・`imageview`・`pdfviewer`・`kuidemo`・`mview`・`vkdemo`・`wltest`・`wlshm` |
| `libexec/` | `keiland-ime` |
| `lib/` | `libwayland-client.so`・`libkeiland.so`・`libkeiui.so`・`libtruetype.so`・`libpdf.so`・`libz-compat.so`・`libpng-compat.so`・`libjpeg-compat.so`・`libgif-compat.so`・`libvulkan.so.1` |
| `share/fonts/` | `keiland.ttf`・`keiland-mono.ttf`・`keiland-fallback.ttf`・`keiland-emoji.ttf`（zedBSD の `KEILAND_FONT_DATA` と同じ対応） |
| `share/licenses/keiland-fonts/` | font の license |
| `share/keiland/` | `wallpaper.ppm`・`wallpapers/`（`userland/desktop/wallpapers/generate.py` で作る） |
| `share/kei/ime/ja/` | IME の辞書（`SKK-JISYO.X` は外部の package の取得物、`userland/desktop/ime/dict/Makefile`。p008） |
| `share/mview/` | mview の model（`qs40`。p007） |
| `etc/keiland/` | `apps.conf`・`desktop`・`open-with` など（zedBSD の image の `/etc/keiland` に入る物があれば） |
| `etc/vulkan-backend` | 後段の path を変えたいときだけ（§4.3。既定では作らない） |

- path の対応は WS104 p007 の `userland/desktop/paths.h`（`KEILAND_BINDIR` → `bin`、`KEILAND_LIBEXECDIR` → `libexec`、`KEILAND_DATADIR` → `share`、`KEILAND_SYSCONFDIR` → `etc`）。
- SONAME は zedBSD と同じ版の番号の無い名前（`libkeiland.so` など）。system の library（`libz.so.1` など）と名前が重ならない。**例外は `libvulkan.so.1`**
  （普通の Vulkan の app と同じ名前で見つかるように。§4）。
- 全ての program と library に `RUNPATH=/opt/keiland/lib`（`-Wl,-rpath,$(KEILAND_PREFIX)/lib -Wl,--enable-new-dtags`）。
- **host の `/opt/keiland` には install しない**（host の system を変えない）。試験は `DESTDIR=$PWD/build/keiland-linux/stage` の tree を、host では `LD_LIBRARY_PATH` で、
  guest では `install-guest.sh` で `/opt/keiland` に写して使う。
- 2026-10-01 の host（Debian 13、glibc 2.41）で build した物は glibc 2.38 以上を要る（`_GNU_SOURCE` の `__isoc23_strtol` など）。古い distribution で動かすには、その distribution で build する。

## 3. build（`make keiland-linux`、D1・D2）

2026-10-01 に、下の `keiland-linux.mk` の雛形で libz-compat・libpng-compat・libwayland-client を gcc 14 と clang 19 で build・install できることを確かめた（scratch で）。
また、Linux の build の対象の 303 個の source を host の gcc と clang で compile して、問題を全て洗った（§3.5）。

### 3.1 入口（top-level の `Makefile`）

`Makefile` の `managed-lan-host-test` の規則（426 行の近く）の後に足す:

```make
# The Keiland desktop for Linux (WS105, plan/ws105/design.md §3): a build of its own with the host's compiler, which
# shares nothing with the zedBSD build.  DESTDIR and KEILAND_PREFIX (default /opt/keiland) choose where install goes.
.PHONY: keiland-linux keiland-linux-install keiland-linux-install-session keiland-linux-clean
keiland-linux:
	$(MAKE) -f userland/desktop/keiland-linux.mk all
keiland-linux-install:
	$(MAKE) -f userland/desktop/keiland-linux.mk install
keiland-linux-install-session:
	$(MAKE) -f userland/desktop/keiland-linux.mk install-session
keiland-linux-clean:
	$(MAKE) -f userland/desktop/keiland-linux.mk clean
```

- 4 つの goal を `ZEDBSD_CONFIG_OPTIONAL_GOALS`（`Makefile:44-53`）に足す（`config.mk` 無しで走れるように）。`help`（440-455 行）に 1 行ずつ足す。
- 変数（`CC`・`DESTDIR`・`KEILAND_PREFIX`・`KEILAND_LINUX_BUILD`）は command line から sub-make に渡る（GNU make の既定）。
- top-level の `Makefile` は `userland/*/*/Makefile` を全て include する（`Makefile:260-264`）が、`Makefile.linux` という名前は include しない。**Linux の build の file を
  `Makefile` という名前にしてはいけない。**
- 使い方（repo の root から）:
  ```
  make -j64 keiland-linux                                                  # build（gcc）
  make -j64 keiland-linux CC=clang KEILAND_LINUX_BUILD=build/keiland-linux-clang   # clang でも warning 0 を確かめる
  make keiland-linux-install DESTDIR=$PWD/build/keiland-linux/stage        # 試験の tree（host の /opt には入れない）
  make keiland-linux-clean
  ```

### 3.2 `userland/desktop/keiland-linux.mk`（Linux の build の本体。検証済みの雛形）

zedBSD の `ZEDBSD_USERLAND_PACKAGE` を使わない、独立した GNU make の file。**p002 でこの雛形をそのまま `userland/desktop/keiland-linux.mk` に置き、
`KEILAND_LINUX_PACKAGES` の既定の値と `install-session`（p009）を足す。**

```make
# The Linux build of the Keiland desktop (WS105, plan/ws105/design.md §3).  It shares no rule with the zedBSD build:
# the host's compiler, headers and C library build every package listed in KEILAND_LINUX_PACKAGES, each with its
# own Makefile.linux, into $(KEILAND_LINUX_BUILD), and install copies the result under $(DESTDIR)$(KEILAND_PREFIX).
#
#   make keiland-linux                       (from the top-level Makefile: make -f userland/desktop/keiland-linux.mk all)
#   make keiland-linux-install DESTDIR=...   (install)
# Copyright (C) 2026 Awe Morris; SPDX-License-Identifier: Zlib

KEILAND_LINUX_BUILD ?= build/keiland-linux
KEILAND_PREFIX ?= /opt/keiland
DESTDIR ?=
KEILAND_LINUX_OPT ?= -O2 -g
# The Debian multiarch name of the host (clang has no -print-multiarch on Debian 19).
KEILAND_LINUX_MULTIARCH ?= $(shell gcc -print-multiarch 2>/dev/null || dpkg-architecture -qDEB_HOST_MULTIARCH 2>/dev/null)
KEILAND_LINUX_EXTRA_CPPFLAGS ?=

KEILAND_LINUX_CPPFLAGS := -D_GNU_SOURCE \
	-DKEILAND_BINDIR='"$(KEILAND_PREFIX)/bin"' -DKEILAND_LIBEXECDIR='"$(KEILAND_PREFIX)/libexec"' \
	-DKEILAND_DATADIR='"$(KEILAND_PREFIX)/share"' -DKEILAND_SYSCONFDIR='"$(KEILAND_PREFIX)/etc"' \
	$(KEILAND_LINUX_EXTRA_CPPFLAGS) -I. -Iuserland/desktop/keiland -I$(KEILAND_LINUX_BUILD)/include
# -Wno-format-truncation: gcc's guess that a display string may be cut short (the strings are cut on purpose; D24).
KEILAND_LINUX_CFLAGS := $(KEILAND_LINUX_OPT) -std=gnu17 -Wall -Wextra -Werror -Wno-format-truncation -fPIC
KEILAND_LINUX_LDFLAGS := -Wl,-rpath,$(KEILAND_PREFIX)/lib -Wl,--enable-new-dtags \
	-Wl,-rpath-link,$(KEILAND_LINUX_BUILD)/lib -L$(KEILAND_LINUX_BUILD)/lib

# The headers the Linux build takes from zedBSD's C library side: copies, never -Iinclude/libc (it would hide glibc's).
KEILAND_LINUX_HEADERS := $(shell find include/libc/compat -type f -name '*.h') \
	include/libc/pdf.h include/libc/sha2.h include/libc/md5.h include/libc/sha1.h
KEILAND_LINUX_HEADER_COPIES := $(patsubst include/libc/%,$(KEILAND_LINUX_BUILD)/include/%,$(KEILAND_LINUX_HEADERS))

$(KEILAND_LINUX_BUILD)/include/%: include/libc/%
	@mkdir -p $(dir $@)
	cp $< $@

$(KEILAND_LINUX_BUILD)/obj/%.o: %.c | $(KEILAND_LINUX_HEADER_COPIES)
	@mkdir -p $(dir $@)
	$(CC) $(KEILAND_LINUX_CFLAGS) $(KEILAND_LINUX_CPPFLAGS) $(KEILAND_LINUX_CPPFLAGS_$(subst /,_,$(dir $<))) -MMD -MP -c $< -o $@

KEILAND_LINUX_ALL :=
KEILAND_LINUX_INSTALL :=

# $(1) name, $(2) SONAME, $(3) sources, $(4) our libraries it links (SONAMEs or a .a), $(5) system libraries,
# $(6) the version script or empty, $(7) more link flags
define KEILAND_LINUX_LIBRARY
KEILAND_LINUX_OBJS_$(1) := $$(patsubst %.c,$(KEILAND_LINUX_BUILD)/obj/%.o,$(3))
-include $$(KEILAND_LINUX_OBJS_$(1):.o=.d)
$(KEILAND_LINUX_BUILD)/lib/$(2): $$(KEILAND_LINUX_OBJS_$(1)) $$(addprefix $(KEILAND_LINUX_BUILD)/lib/,$(4)) $(6)
	@mkdir -p $$(dir $$@)
	$$(CC) -shared -Wl,-soname,$(2) -Wl,-z,defs $$(if $(6),-Wl$$(comma)--version-script=$(6)) $(7) $$(KEILAND_LINUX_LDFLAGS) \
		$$(KEILAND_LINUX_OBJS_$(1)) $$(addprefix -l:,$(4)) $(5) -o $$@
KEILAND_LINUX_ALL += $(KEILAND_LINUX_BUILD)/lib/$(2)
KEILAND_LINUX_INSTALL += lib/$(2)
endef

# $(1) name, $(2) bin or libexec, $(3) sources, $(4) our libraries it links, $(5) system libraries
define KEILAND_LINUX_PROGRAM
KEILAND_LINUX_OBJS_$(1) := $$(patsubst %.c,$(KEILAND_LINUX_BUILD)/obj/%.o,$(3))
-include $$(KEILAND_LINUX_OBJS_$(1):.o=.d)
$(KEILAND_LINUX_BUILD)/$(2)/$(1): $$(KEILAND_LINUX_OBJS_$(1)) $$(addprefix $(KEILAND_LINUX_BUILD)/lib/,$(4))
	@mkdir -p $$(dir $$@)
	$$(CC) -pie $$(KEILAND_LINUX_LDFLAGS) $$(KEILAND_LINUX_OBJS_$(1)) $$(addprefix -l:,$(4)) $(5) -o $$@
KEILAND_LINUX_ALL += $(KEILAND_LINUX_BUILD)/$(2)/$(1)
KEILAND_LINUX_INSTALL += $(2)/$(1)
endef

# $(1) the path under the prefix, $(2) the source file
define KEILAND_LINUX_DATA
$(KEILAND_LINUX_BUILD)/$(1): $(2)
	@mkdir -p $$(dir $$@)
	cp $$< $$@
KEILAND_LINUX_ALL += $(KEILAND_LINUX_BUILD)/$(1)
KEILAND_LINUX_INSTALL += $(1)
endef

comma := ,

KEILAND_LINUX_PACKAGES ?=
include $(KEILAND_LINUX_PACKAGES)

.PHONY: all install clean
all: $(KEILAND_LINUX_ALL)

install: all
	@set -e; for f in $(KEILAND_LINUX_INSTALL); do \
		mode=0644; case $$f in lib/*|bin/*|libexec/*) mode=0755 ;; esac; \
		install -D -m $$mode $(KEILAND_LINUX_BUILD)/$$f $(DESTDIR)$(KEILAND_PREFIX)/$$f; \
	done

clean:
	rm -rf $(KEILAND_LINUX_BUILD)/obj $(KEILAND_LINUX_BUILD)/lib $(KEILAND_LINUX_BUILD)/bin $(KEILAND_LINUX_BUILD)/libexec \
		$(KEILAND_LINUX_BUILD)/include $(KEILAND_LINUX_BUILD)/share $(KEILAND_LINUX_BUILD)/etc $(KEILAND_LINUX_BUILD)/gen \
		$(KEILAND_LINUX_BUILD)/stage
```

要点:

- `-I.`（repo の root）: tree の慣習の `#include "userland/..."` のため。`-Iuserland/desktop/keiland`: desktop の公開の header（WS104 p001 で移した物）。
- `-I$(KEILAND_LINUX_BUILD)/include`: zedBSD の libc の側にあって Linux の build にも要る header の**複写**: `include/libc/compat/`、`pdf.h`、`sha2.h`・`md5.h`・`sha1.h`
  （glibc に無い OpenBSD の digest の API。`sha1.h` は `src/libc/openbsd-digest.c` が要る、2026-10-01 確かめ）。**`include/libc` を `-I` に入れてはいけない**（glibc と衝突する）。
- Vulkan・DRM・dma-buf・evdev・ALSA の header は host の物（`/usr/include/vulkan`、linux-libc-dev の `/usr/include/drm`・`linux/`・`sound/`）。
- **`-Wl,-rpath-link,$(KEILAND_LINUX_BUILD)/lib` は必ず要る**: GNU ld は、link する library の NEEDED（例 libpng-compat → libz-compat）を `-L` でなく rpath-link で探す。
  無いと「libz-compat.so, needed by libpng-compat.so, not found」で失敗する（design-reviewer が確かめた）。試験の道具の command にも全て付ける。
- `-Wno-format-truncation`（D24）: gcc 14 だけが出す「表示の文字列が切れるかもしれない」の警告（2026-10-01 に 10 箇所）。文字列は意図して切っているので抑える。clang 19 もこの option を受ける。
- 依存の我々の library は `-l:libkeiland.so` の形で名前を固定する（system の同名の物を拾わないように）。
- `KEILAND_LINUX_CPPFLAGS_<directory>`（例 `KEILAND_LINUX_CPPFLAGS_userland_desktop_libvulkan-compat_`）で directory ごとの `-I` を足せる（生成物の directory など）。
- `KEILAND_LINUX_EXTRA_CPPFLAGS` は試験と調べ物のため（通常は空）。

### 3.3 `Makefile.linux`（package ごと）

```make
# userland/desktop/libkeiland/Makefile.linux -- the Linux build of libkeiland (WS105).  The zedBSD build is Makefile.
# keiland-linux-sync: skip userland/desktop/libkeiland/zedbsd/*  (the zedBSD backend)
# keiland-linux-sync: skip userland/base/net/*  (networkd's protocol, zedBSD only)
LIBKEILAND_LINUX_SOURCES := userland/desktop/libkeiland/version.c userland/desktop/libkeiland/menu.c ... \
	userland/desktop/libkeiland/wpa/network-wpa.c userland/desktop/libkeiland/linux/network-link-linux.c userland/desktop/libkeiland/linux/audio-linux.c
$(eval $(call KEILAND_LINUX_LIBRARY,keiland,libkeiland.so,$(LIBKEILAND_LINUX_SOURCES),libwayland-client.so,-lm,userland/desktop/libkeiland/exports.map))
```

- source の一覧は `Makefile` と別に持つ（D2）。ずれは `plan/tools/keiland-linux/makefile-sync.sh` が見つける: `Makefile` の `userland/...*.c` のうち、`zedbsd/` の下でなく、
  `Makefile.linux` に無く、`# keiland-linux-sync: skip <path or glob> <理由>` の行にも無い物があれば FAIL。逆向き（`Makefile.linux` だけにある物）は `linux/`・`wpa/` の下か、
  `# keiland-linux-sync: only <path> <理由>` の行があること。
- 依存と system の library（2026-10-01 の試しの link で確かめた。`-lm` 以外の system の library（`-lpthread`・`-ldl`・`-lutil`・`-lrt`）は glibc 2.34 から libc に入ったので要らない）:

| package | SONAME・名前 | 我々の依存（`$(4)`） | system（`$(5)`） |
| --- | --- | --- | --- |
| `userland/base/libz-compat` | `libz-compat.so` | — | — |
| `userland/base/libpng-compat` | `libpng-compat.so` | `libz-compat.so` | `-lm` |
| `userland/base/libjpeg-compat`・`libgif-compat` | `libjpeg-compat.so`・`libgif-compat.so` | — | `-lm` |
| `userland/desktop/linux-compat` | `libkeiland-compat.a`（static） | — | — |
| `userland/desktop/libwayland` | `libwayland-client.so` | — | — |
| `userland/desktop/libtruetype` | `libtruetype.so` | — | `-lm` |
| `userland/desktop/libkeiland` | `libkeiland.so` | `libwayland-client.so` | `-lm` |
| `userland/desktop/libvulkan-compat` | `libvulkan.so.1` | `libwayland-client.so` | `-ldl`（無害）、`-Wl,-Bsymbolic`（`$(7)`） |
| `userland/desktop/libkeiui` | `libkeiui.so` | `libkeiland.so libwayland-client.so libtruetype.so libpng-compat.so libvulkan.so.1` | `-lm` |
| `userland/base/libpdf` | `libpdf.so` | `libz-compat.so libjpeg-compat.so libtruetype.so libkeiland-compat.a` | `-lm` |
| compositor `userland/desktop/wayland` | `bin/wayland` | `libkeiland.so libtruetype.so libpng-compat.so libz-compat.so libvulkan.so.1` | `-lm` |
| `terminal` | `bin/terminal` | `libkeiui.so libkeiland.so libwayland-client.so libtruetype.so libvulkan.so.1` | `-lm` |
| `files` | `bin/files` | `libkeiland.so libwayland-client.so libtruetype.so libpng-compat.so libjpeg-compat.so libgif-compat.so libz-compat.so libvulkan.so.1 libkeiland-compat.a` | `-lm` |
| `settings` | `bin/settings` | `libkeiland.so libwayland-client.so libtruetype.so libvulkan.so.1` | `-lm` |
| `notes` | `bin/notes` | `libkeiui.so libkeiland.so libwayland-client.so libtruetype.so libpdf.so libvulkan.so.1 libkeiland-compat.a` | `-lm` |
| `textedit` | `bin/textedit` | `libkeiui.so libkeiland.so libtruetype.so libpng-compat.so libz-compat.so libwayland-client.so libvulkan.so.1` | `-lm` |
| `imageview` | `bin/imageview` | `libkeiui.so libkeiland.so libtruetype.so libpng-compat.so libjpeg-compat.so libgif-compat.so libz-compat.so libwayland-client.so libvulkan.so.1` | `-lm` |
| `pdfviewer` | `bin/pdfviewer` | `libkeiui.so libkeiland.so libtruetype.so libpdf.so libwayland-client.so libvulkan.so.1` | `-lm` |
| `kuidemo` | `bin/kuidemo` | `libkeiui.so libkeiland.so libwayland-client.so libtruetype.so libvulkan.so.1` | `-lm` |
| `ime` | `libexec/keiland-ime` | `libwayland-client.so` | — |
| `mview` | `bin/mview` | `libwayland-client.so libvulkan.so.1` | `-lm` |
| `vkdemo` | `bin/vkdemo` | `libvulkan.so.1` | `-lm` |
| `wltest` | `bin/wltest` | `libwayland-client.so libvulkan.so.1` | `-lm` |
| `wlshm` | `bin/wlshm` | `libwayland-client.so` | — |

  （program の依存は余分でもよい: Debian の gcc は `--as-needed` が既定で、使わない物は NEEDED に入らない。足りないと link が失敗するので分かる。）

### 3.4 Linux だけの小さな部品

- `userland/desktop/linux-compat/`（Linux の build だけ）: glibc に無い OpenBSD の digest（`sha2.h`・`md5.h`・`sha1.h`）を、zedBSD の libc の source
  （`src/libc/openbsd-sha2.c`・`openbsd-digest.c`）を compile して static な library `libkeiland-compat.a` にする。`Makefile.linux` だけを持つ（static の library の規則は
  `keiland-linux.mk` に `KEILAND_LINUX_STATIC`（`ar rcs`）を足す）。2026-10-01 に、2 つの source が glibc で、header の複写だけで compile できることを確かめた。
  使う物: libpdf・files・notes。
- header の漏れの確かめ（`plan/tools/keiland-linux/header-check.sh`、p002）: host には libwayland-dev が入っていて `/usr/include/wayland-client.h` などがある。我々の header が
  見つからないと、error にならずに host の物が使われる（-MMD は system の header を記録しない）。script は Linux の build の全ての source を `-M`（system の header を含む）で
  前処理し、`/usr/include/wayland-` と `/usr/include/EGL`・`/usr/include/GLES` が出たら FAIL。

### 3.5 host の試しの compile の結果（2026-10-01、gcc 14.2.0・clang 19.1.7・glibc 2.41）

303 個の source（package ごとの `Makefile` の一覧）を上の flag で compile した。直す必要がある物と、その Phase:

| 問題 | 直し方 | Phase |
| --- | --- | --- |
| `src/libc/openbsd-digest.c` が `sha1.h` を要る | `sha1.h` も複写する（上の雛形に入れた） | p002 |
| `mview/renderer.c:2461-2469` が `printf`・`fflush`・`stdout` を `<stdio.h>` 無しで使う（zedBSD では他の header から来る） | `#include <stdio.h>` を足す（共通の file、振る舞いは変わらない） | p007 |
| compositor の `zwl-gpu.h` の `<vulkan/vulkan_external.h>`（zedBSD だけの header） | WS104 p004 で消す（[edits-compositor.md](../history/ws104/design/edits-compositor.md)） | WS104 p004 |
| compositor の `zwl.h` の `<uapi/input.h>` | WS104 p005 の `zwl-evdev.h` | WS104 p005 |
| compositor の `compose.c:1949` が `vkGetFenceFdKHR` を直接呼ぶ（Linux の loader は拡張の関数を export しないので link できない） | WS104 p004 で `vkGetDeviceProcAddr` から得る形にする | WS104 p004 |
| gcc だけの `-Wformat-truncation`（10 箇所） | `-Wno-format-truncation`（D24） | p002 |
| gcc だけの `-Wmaybe-uninitialized`: `userland/base/libpdf/font.c:2773`（`control`）、`userland/desktop/wayland/titlebar-shell.c:1342`（`colour`） | 初期値を入れる（`control[0] = 0.0; control[1] = 0.0;` を `convert_contours()` の loop の前、`float colour[4] = { 0.0f, 0.0f, 0.0f, 0.0f };`。振る舞いは変わらない。clang は `-Wno-maybe-uninitialized` を知らないので flag では抑えない） | p008（font.c）・p006（titlebar-shell.c） |
| `libkeiland/network-link.c` は Linux で compile できない（`ifc_buf`・`ifr_hwaddr`・`SIOCGIFSTATS` の違い） | Linux の版を `linux/` に書く（設計どおり） | p002（仮）・p010 |
| `-O3` だけで出る gcc の警告（`charstrings.c:476`・`files/ui-drag.c:600`・`wayland/network.c:561`） | 既定の `-O2` では出ない。`KEILAND_LINUX_OPT` を変えるときの注意として記録 | — |

clang 19 は、上の error（header・`stdio.h`）の他に警告を 1 つも出さなかった。compile の時間は 64 core で 3 秒以内。

## 4. libvulkan-compat（Linux の app が使う libvulkan）

**この節が WS105 で一番わかりにくい仕組みである。** 実装する前に全部を読むこと。迷ったら §4.2 の図に戻る。
2026-10-01 の改訂 2: design-reviewer と host の実験（[survey/](survey/README.md)）の結果で、§4.3・§4.6〜§4.11 を直した。特に「後段を `RTLD_DEEPBIND` で開く」（§4.11、D10 の改訂）。

### 4.1 一言で

`/opt/keiland/lib/libvulkan.so.1` は、**WSI（画面に出す部分）だけを自分で実装し、それ以外の Vulkan の全ての関数を、system の
`libvulkan.so.1`（後段、backend）にそのまま渡す library** である。後段は Khronos の loader（Mesa の ICD を読む）でも、組み込みの
ベンダーの単体の libvulkan（Mali など）でもよい。描画・compute・memory は全て後段の driver が行う。

zedBSD の `userland/desktop/libvulkan/` は Venus・i915 の driver そのもの（Vulkan の全てを自分で実装する）なので、**全く別の物**である。
source は共有しない（D7）。名前の似た `libvulkan` と `libvulkan-compat` を取り違えないこと。

### 4.2 図

```
 app（Terminal・vkdemo・compositor など、/opt/keiland/bin/*）
   │  DT_NEEDED libvulkan.so.1、RUNPATH /opt/keiland/lib
   ▼
 /opt/keiland/lib/libvulkan.so.1  ＝ libvulkan-compat（我々）
   │   ├ WSI: VK_KHR_surface・VK_KHR_wayland_surface・VK_KHR_swapchain・VK_KHR_display・VK_EXT_direct_mode_display・VK_EXT_acquire_drm_display を自分で実装
   │   │        → Wayland（我々の libwayland-client）で zwp_linux_dmabuf_v1 を話す / KMS の ioctl で画面に出す
   │   └ それ以外: 後段の関数の pointer（dlsym で得た物）を呼ぶだけ
   │  dlopen("/usr/lib/x86_64-linux-gnu/libvulkan.so.1", RTLD_NOW | RTLD_LOCAL | RTLD_DEEPBIND)（絶対 path）
   ▼
 後段 /usr/lib/<multiarch>/libvulkan.so.1（Khronos の loader、またはベンダーの libvulkan）
   │
   ▼
 ICD（Mesa の lavapipe・anv・radv・venus、ベンダーの driver）→ GPU
```

- app は普通の Vulkan の app と同じく `libvulkan.so.1` に link する。`RUNPATH` が `/opt/keiland/lib` なので、動的 linker は我々の物を先に見つける。
- 我々の物は後段を**絶対 path で** `dlopen` する。名前（`libvulkan.so.1`）で開くと自分自身が返る（2026-10-01 に確かめた）ので、絶対に名前で開かない。
- 後段の WSI（後段の `VK_KHR_wayland_surface` など）は**使わない**。理由: ベンダーの libvulkan は Wayland の WSI を持たないことや、
  別の版の libwayland を前提にすることがある。WSI を我々が持てば、後段は「描画できる Vulkan」でさえあればよい（D5）。

### 4.3 後段の見つけ方と開き方（`backend.c`）

順に試し、最初に開けた物を使う。

1. 環境変数 `KEILAND_VULKAN_BACKEND`（絶対 path）。
2. file `$(KEILAND_PREFIX)/etc/vulkan-backend` の 1 行目（絶対 path）。
3. build の時に決めた既定の一覧（`KEILAND_VULKAN_BACKEND_PATHS`、`-D` で渡す、`:` 区切り）。既定の値は
   `/usr/lib/$(KEILAND_LINUX_MULTIARCH)/libvulkan.so.1:/usr/lib/x86_64-linux-gnu/libvulkan.so.1:/usr/lib/aarch64-linux-gnu/libvulkan.so.1:/usr/lib64/libvulkan.so.1:/usr/lib/libvulkan.so.1`。
   `KEILAND_LINUX_MULTIARCH` は `gcc -print-multiarch` か `dpkg-architecture -qDEB_HOST_MULTIARCH` の値（**clang の `-print-multiarch` は Debian の clang 19 では使えない**、§3.2）。

開き方: glibc では `dlopen(path, RTLD_NOW | RTLD_LOCAL | RTLD_DEEPBIND)`。環境変数 `KEILAND_VULKAN_NO_DEEPBIND=1` のときだけ `RTLD_DEEPBIND` を外す
（理由は §4.11）。glibc でない C library（musl など、`RTLD_DEEPBIND` が無い）は WS105 の範囲の外（§8）で、`#ifdef RTLD_DEEPBIND` で外す。

開いた後に必ず確かめる:

- `dladdr` で自分の file の path を得て、`realpath` が後段の `realpath` と同じなら断る（自分を開いた）。
- `dlsym(handle, "vkGetInstanceProcAddr")` が我々自身の `vkGetInstanceProcAddr` の address と違うこと。
- 見つからない・確かめに失敗したときは、stderr に `libvulkan-compat: no backend libvulkan (KEILAND_VULKAN_BACKEND, <prefix>/etc/vulkan-backend, <既定の一覧>)` を 1 回出し、
  `vkCreateInstance` は `VK_ERROR_INCOMPATIBLE_DRIVER`、`vkEnumerateInstanceExtensionProperties`・`vkEnumerateInstanceLayerProperties` は 0 個と `VK_SUCCESS`
  （これらの関数の規格の戻り値に INCOMPATIBLE_DRIVER は無い）、`vkEnumerateInstanceVersion` は `VK_API_VERSION_1_0` と `VK_SUCCESS` を返す。

後段は process の中で 1 回だけ開き（`pthread_once`）、閉じない。我々の library は `-Wl,-Bsymbolic` で link する（我々の中の、自分の関数の address の参照が
他の物に結び付かないように）。

### 4.4 関数の 3 つの種類

Vulkan の関数は全て、次のどれかに入る。**どれに入るかの一覧は `functions.tsv` の 1 か所で持ち、`gen-forward.sh` で `forward.inc` を作る（§4.10）。**

| 種類 | 何をするか | 例 |
| --- | --- | --- |
| **F: 素通し（forward）** | 我々の export した関数は、後段の同じ名前の関数の pointer を呼ぶだけ。引数も戻り値も触らない | `vkCmdDraw`・`vkQueueSubmit`・`vkCreateImage`・`vkAllocateMemory` などほぼ全部（Vulkan 1.0〜1.4 の core の 234 個のうち I・O でない物） |
| **I: 横取り（intercept）** | 後段に渡す前後で手を入れる | `vkCreateInstance`・`vkDestroyInstance`・`vkEnumerateInstanceExtensionProperties`・`vkEnumerateDeviceExtensionProperties`・`vkCreateDevice`・`vkDestroyDevice`・`vkGetInstanceProcAddr`・`vkGetDeviceProcAddr`・`vkGetDeviceQueue`・`vkGetDeviceQueue2`（p003 の missing-backend fallback に `vkEnumerateInstanceLayerProperties`・`vkEnumerateInstanceVersion` も I として入れる） |
| **O: 自前（own）** | 後段に渡さず、我々が全部実装する（WSI） | 下の表 |

O の関数（我々が実装し、export する物）:

| 拡張 | 関数 |
| --- | --- |
| `VK_KHR_surface` | `vkDestroySurfaceKHR`・`vkGetPhysicalDeviceSurfaceSupportKHR`・`vkGetPhysicalDeviceSurfaceCapabilitiesKHR`・`vkGetPhysicalDeviceSurfaceFormatsKHR`・`vkGetPhysicalDeviceSurfacePresentModesKHR` |
| `VK_KHR_wayland_surface` | `vkCreateWaylandSurfaceKHR`・`vkGetPhysicalDeviceWaylandPresentationSupportKHR` |
| `VK_KHR_swapchain` | `vkCreateSwapchainKHR`・`vkDestroySwapchainKHR`・`vkGetSwapchainImagesKHR`・`vkAcquireNextImageKHR`・`vkQueuePresentKHR`、Vulkan 1.1 からの `vkGetDeviceGroupPresentCapabilitiesKHR`・`vkGetDeviceGroupSurfacePresentModesKHR`・`vkGetPhysicalDevicePresentRectanglesKHR`・`vkAcquireNextImage2KHR`（device group は 1 つの device として答える: mask 1、`LOCAL` だけ、rect は surface の全体） |
| `VK_KHR_display` | `vkGetPhysicalDeviceDisplayPropertiesKHR`・`vkGetPhysicalDeviceDisplayPlanePropertiesKHR`・`vkGetDisplayPlaneSupportedDisplaysKHR`・`vkGetDisplayModePropertiesKHR`・`vkCreateDisplayModeKHR`・`vkGetDisplayPlaneCapabilitiesKHR`・`vkCreateDisplayPlaneSurfaceKHR` |
| `VK_EXT_direct_mode_display` | `vkReleaseDisplayEXT` |
| `VK_EXT_acquire_drm_display` | `vkAcquireDrmDisplayEXT`・`vkGetDrmDisplayEXT` |

**WSI の仲間で我々が実装しない関数**（`vkCreateXlibSurfaceKHR`・`vkCreateXcbSurfaceKHR`・`vkCreateHeadlessSurfaceEXT`・`vkCreateSharedSwapchainsKHR`・
`vkGetPhysicalDeviceSurfaceCapabilities2KHR`・`vkGetPhysicalDeviceSurfaceFormats2KHR`・`vkGetPhysicalDeviceDisplayProperties2KHR` など、後段の loader が export する
WSI の関数の全て）は、**export しない**。`vkGet*ProcAddr` で名前を聞かれたら **NULL を返す**（後段に聞かない）。後段に聞くと、後段の loader は我々の
`VkSurfaceKHR` を自分の struct と思って読み、壊れる。この「拒む名前」の一覧も `functions.tsv` に種類 **N（deny）** として持つ（host の `vulkan_core.h`・
`vulkan_wayland.h`・`vulkan_xlib.h`・`vulkan_xcb.h` の `VK_KHR_*surface*`・`VK_KHR_*swapchain*`・`VK_KHR_*display*`・`VK_EXT_*display*`・`VK_EXT_*surface*`・
`VK_EXT_swapchain*`・`VK_KHR_present*`・`VK_GOOGLE_display_timing` の block の関数のうち、O でない物）。

**F の素通しの仕組み**（ここが「包まない」の意味）:

- Vulkan の dispatchable な handle（`VkInstance`・`VkPhysicalDevice`・`VkDevice`・`VkQueue`・`VkCommandBuffer`）は、**後段が返した値をそのまま app に返す**。
  我々の struct で包まない（wrap しない）。
- 我々が export する F の関数（例 `vkCmdDraw`）は、**後段が export している同じ名前の関数**（`dlsym(backend, "vkCmdDraw")`）を呼ぶ。
  後段の export の関数は、後段自身の trampoline であり、handle から正しい driver の関数を自分で選ぶ。我々は選ばなくてよい。
- だから F の関数には instance・device ごとの表が要らない。process に 1 つの表（`static PFN_vkCmdDraw next_vkCmdDraw` の集まり）を、
  後段を開いたときに `dlsym` で埋める。
- 結果として、**WSI 以外の Vulkan の動きは後段と全く同じ**になる。app のバグや driver の差を我々が吸収することは無い。
- 後段が export していない core の関数（例: Vulkan 1.1 の後段に 1.3 の関数）を app が我々の export から呼んだときは、stderr に
  `libvulkan-compat: the backend has no vkXxx` を出して `abort()` する（版を確かめずに呼ぶ app の誤り。void の関数は error を返せないため）。

**I の横取りの内容**（§4.6・§4.7 で詳しく）:

- 拡張の一覧から後段の WSI の拡張を消し、我々の WSI の拡張を足す。
- instance・device の作成で、app が求めた我々の WSI の拡張を後段に渡さず（後段は知らない）、我々の WSI が内部で要る拡張を後段に足す。
- `vkGet*ProcAddr` で、O・I の関数の名前には我々の関数を、N の名前には NULL を、それ以外には後段の答えを返す。
- `vkGetDeviceQueue*` で、`VkQueue` → `VkDevice` の対応を覚える（`vkQueuePresentKHR` は queue しか受け取らないため）。

**I・O の関数の中から後段を呼ぶ規則（必ず守る）**: 後段の関数は、**I の関数の後段の版は `dlsym(backend, 名前)` で得た pointer だけ**で呼ぶ。
後段の `vkGetInstanceProcAddr`・`vkGetDeviceProcAddr` の答えを I の関数の「次」に使ってはいけない（§4.11: `RTLD_DEEPBIND` を外した場合、後段はそこで
**我々の**関数を返し、無限の循環になる）。WSI が内部で使う instance・device の関数（`vkGetPhysicalDeviceFormatProperties2`・`vkCreateImage` など。F の物）は、
後段の `vkGet*ProcAddr` で得てよい（driver の関数を直接指す。DEEPBIND を外した場合でも、高々我々の F の関数を 1 回通るだけ）。

### 4.5 我々が持つ記録（handle を包まない代わり）

| 記録 | key | 中身 | いつ作り、いつ消すか |
| --- | --- | --- | --- |
| instance の記録 | 後段の `VkInstance` の値 | app の `pApplicationInfo->apiVersion`（無ければ 1.0）、後段の `vkGetInstanceProcAddr` で得た instance の関数の表（WSI が内部で使う物: `vkGetPhysicalDeviceProperties`・`vkGetPhysicalDeviceFormatProperties2(KHR)`・`vkGetPhysicalDeviceImageFormatProperties2(KHR)`・`vkGetPhysicalDeviceMemoryProperties`・`vkGetPhysicalDeviceQueueFamilyProperties`・`vkGetPhysicalDeviceExternalSemaphoreProperties(KHR)`・`vkGetPhysicalDeviceExternalFenceProperties(KHR)`）、app が有効にした我々の WSI の拡張 | `vkCreateInstance` の成功で作り、`vkDestroyInstance` で消す |
| device の記録 | 後段の `VkDevice` の値 | physical device、instance の記録、実際に使う Vulkan の版（app の apiVersion と後段の physical device の apiVersion の小さい方）、後段の `vkGetDeviceProcAddr` で得た device の関数の表（WSI が使う: `vkCreateImage`・`vkDestroyImage`・`vkGetImageMemoryRequirements2(KHR)`・`vkAllocateMemory`・`vkFreeMemory`・`vkBindImageMemory`・`vkGetMemoryFdKHR`・`vkGetImageDrmFormatModifierPropertiesEXT`・`vkGetImageSubresourceLayout`・`vkCreateCommandPool`・`vkAllocateCommandBuffers`・`vkBeginCommandBuffer`・`vkCmdPipelineBarrier`・`vkCmdCopyImageToBuffer`・`vkEndCommandBuffer`・`vkQueueSubmit`・`vkCreateSemaphore`・`vkGetSemaphoreFdKHR`・`vkImportSemaphoreFdKHR`・`vkCreateFence`・`vkImportFenceFdKHR`・`vkWaitForFences`・`vkResetFences`・`vkCreateBuffer` など）、**WSI の道**（下の §4.6 の表）、app が `VK_KHR_swapchain` を有効にしたか | `vkCreateDevice` の成功で作り、`vkDestroyDevice` で消す |
| queue の記録 | 後段の `VkQueue` の値 | 属する device の記録、queue family の番号 | `vkGetDeviceQueue*` で作る。device とともに消す |
| surface | 我々の `VkSurfaceKHR`（我々の struct の pointer を `uint64_t` にした値） | Wayland: `wl_display *`・`wl_surface *`・WSI の event queue・bind した `zwp_linux_dmabuf_v1` と受けた (format, modifier) の組。display: plane と mode | `vkCreate*SurfaceKHR`〜`vkDestroySurfaceKHR` |
| swapchain | 我々の `VkSwapchainKHR` | §4.8・§4.9 | `vkCreateSwapchainKHR`〜`vkDestroySwapchainKHR` |
| display・mode | 我々の `VkDisplayKHR`・`VkDisplayModeKHR` | DRM の connector・CRTC・mode、master の fd（acquire された物） | §4.9 |

- 記録は小さな配列か連結 list に持ち、`pthread_mutex` で守る。数は少ない（instance・device は普通 1 つ）。
- `VkSurfaceKHR`・`VkSwapchainKHR`・`VkDisplayKHR`・`VkDisplayModeKHR` は non-dispatchable な handle なので、我々の値を返してよい。
  **後段にこれらの handle を渡してはいけない**（後段は知らない）。app が pNext で渡すことがある構造体（`VkImageSwapchainCreateInfoKHR`・
  `VkBindImageMemorySwapchainInfoKHR`、Vulkan 1.1 の `VK_KHR_swapchain` の一部）は、device group の present を我々が 1 device としてしか扱わないので、
  app が使うことは無い前提にする。使われたら（`vkCreateImage`・`vkBindImageMemory2` の pNext に見つけたら）後段は壊れるので、WS105 では範囲の外（§8）として記録する。

### 4.6 instance と device の作成（I の横取りの細部）

**我々の WSI の拡張**（我々が名乗る物）:

- instance: `VK_KHR_surface`、`VK_KHR_wayland_surface`、`VK_KHR_display`、`VK_EXT_direct_mode_display`、`VK_EXT_acquire_drm_display`。
  `VK_KHR_get_surface_capabilities2`・`VK_EXT_surface_maintenance1` は名乗らない（WS105 の範囲の外）。
- device: `VK_KHR_swapchain`。

**`vkEnumerateInstanceExtensionProperties(pLayerName = NULL)`**: 後段の一覧から、次を**消し**、我々の instance の WSI の拡張を**足す**。

- 消す: 名前が `VK_KHR_surface`・`VK_KHR_*_surface`・`VK_EXT_*_surface`・`VK_KHR_display`・`VK_KHR_get_display_properties2`・
  `VK_EXT_acquire_*_display`・`VK_EXT_direct_mode_display`・`VK_EXT_display_surface_counter`・`VK_KHR_get_surface_capabilities2`・
  `VK_EXT_swapchain_colorspace`・`VK_EXT_surface_maintenance1`・`VK_KHR_surface_protected_capabilities`・`VK_GOOGLE_surfaceless_query` の物。
- 足す: 上の我々の instance の拡張（重複させない）。
- `pLayerName` が NULL でなければ後段にそのまま渡す（layer は後段の物）。
- 数を問う呼び出し（`pProperties == NULL`）と埋める呼び出しで同じ数になるように、一覧は毎回同じ規則で作る。足りない配列には `VK_INCOMPLETE`。

**`vkEnumerateDeviceExtensionProperties`**: 後段の一覧から `VK_KHR_swapchain`・`VK_KHR_swapchain_mutable_format`・`VK_EXT_swapchain_maintenance1`・
`VK_KHR_present_id`・`VK_KHR_present_wait`・`VK_KHR_incremental_present`・`VK_EXT_display_control`・`VK_KHR_display_swapchain`・
`VK_EXT_full_screen_exclusive`・`VK_KHR_shared_presentable_image`・`VK_GOOGLE_display_timing`・`VK_EXT_hdr_metadata` を消し、
`VK_KHR_swapchain` を足す。**ただし** 下の「WSI の道」が「無し」になる physical device では `VK_KHR_swapchain` を足さない。

**`vkCreateInstance`**:

1. 後段を開く（§4.3）。
2. app の `ppEnabledExtensionNames` から我々の WSI の instance の拡張を**取り除いた**一覧を作る。
3. 内部で要る instance の拡張を足す（後段が持つ物だけ。**app の apiVersion で決める**: 2026-10-01 の調べで、Keiland の app は全て `VK_API_VERSION_1_0` を使う。
   実際の版は app の apiVersion と後段の版の小さい方なので、1.1 の core の機能は拡張として足さないと使えない）:
   app の apiVersion が 1.1 未満なら `VK_KHR_get_physical_device_properties2`・`VK_KHR_external_memory_capabilities`・`VK_KHR_external_semaphore_capabilities`・
   `VK_KHR_external_fence_capabilities`。
4. 後段の `vkCreateInstance`（`dlsym` の pointer）を呼ぶ。`VkInstanceCreateInfo` は複写して拡張の一覧だけ差し替える（app の構造体を書き換えない）。
5. 成功したら instance の記録を作る（§4.5）。instance の関数の表は、app の apiVersion が 1.1 未満なら `...2KHR` の名前で、1.1 以上なら core の名前で得る。

**WSI の道**（physical device ごとに、`vkCreateDevice` の前に決めて device の記録に持つ。**拡張の名前の有無でなく、実際の能力で決める**）:

| 問い合わせ | 使う関数 |
| --- | --- |
| dma-buf で export できる image が作れるか | `vkGetPhysicalDeviceImageFormatProperties2` + `VkPhysicalDeviceExternalImageFormatInfo{DMA_BUF}`（modifier の拡張があれば `VkPhysicalDeviceImageDrmFormatModifierInfoEXT{LINEAR}` も）→ `VkExternalImageFormatProperties.externalMemoryProperties.externalMemoryFeatures` に `EXPORTABLE` |
| SYNC_FD の semaphore を export・import できるか | `vkGetPhysicalDeviceExternalSemaphoreProperties(SYNC_FD)` の `EXPORTABLE`・`IMPORTABLE` |
| SYNC_FD の fence を import できるか | `vkGetPhysicalDeviceExternalFenceProperties(SYNC_FD)` の `IMPORTABLE` |

| 道 | 条件 | swapchain の image | sync |
| --- | --- | --- | --- |
| `modifier` | dma-buf の export ができ、`VK_EXT_image_drm_format_modifier` がある | `VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT` | 下 |
| `linear` | dma-buf の export ができ、modifier の拡張が無い | `VK_IMAGE_TILING_LINEAR`（compositor には `DRM_FORMAT_MOD_LINEAR` と送る） | 下 |
| 無し | dma-buf の export ができない（例: `/dev/udmabuf` を開けない lavapipe、2026-10-01 に確かめた） | `VK_KHR_swapchain` を名乗らない（wl_shm の予備の道は範囲の外、§8） | — |

sync: SYNC_FD の semaphore の export と fence・semaphore の import ができれば implicit sync（§4.8）。できなければ「CPU で待つ」道（§4.8 の予備の道）。

**`vkCreateDevice`**:

1. app の一覧から `VK_KHR_swapchain` を取り除く。**後段がその physical device で `VK_KHR_swapchain` を持つなら、後段にもそれを有効にする**（後段の WSI の関数は呼ばないが、
   app が image を `VK_IMAGE_LAYOUT_PRESENT_SRC_KHR` に移すのは `VK_KHR_swapchain` を有効にした device でないと規格の誤用になる。validation layer の誤検出も避ける）。
   後段が `VK_KHR_swapchain` を持たない（ベンダーの blob など）ときは有効にしない（その場合 `PRESENT_SRC_KHR` の誤用は後段に見えるが、動きには影響しない前提。§10 V8）。
2. app が `VK_KHR_swapchain` を求めたときだけ、WSI の道に要る device の拡張を足す。**実際の版（device の記録）が 1.1 未満なら** 1.1 の core の物も拡張として足す:
   `VK_KHR_external_memory`・`VK_KHR_external_memory_fd`・`VK_EXT_external_memory_dma_buf`・`VK_KHR_dedicated_allocation`・`VK_KHR_get_memory_requirements2`・
   `VK_KHR_external_semaphore`・`VK_KHR_external_semaphore_fd`・`VK_KHR_external_fence`・`VK_KHR_external_fence_fd`、
   `modifier` の道なら `VK_EXT_image_drm_format_modifier` とその前提（`VK_KHR_image_format_list`・`VK_KHR_bind_memory2`・`VK_KHR_sampler_ycbcr_conversion`・
   `VK_KHR_maintenance1`・`VK_KHR_get_physical_device_properties2` は instance の側）、後段が持てば `VK_EXT_queue_family_foreign`。
   1.1 以上の版なら core に入っている物（external_memory・external_semaphore・external_fence・dedicated_allocation・get_memory_requirements2・bind_memory2・
   sampler_ycbcr_conversion・maintenance1）は足さない。1.2 以上なら `image_format_list` も足さない。
3. 後段の `vkCreateDevice`（`dlsym` の pointer）を呼び、device の記録を作る。device の関数の表は、実際の版に合わせて `...KHR` の名前か core の名前で得る。

### 4.7 `vkGetInstanceProcAddr`・`vkGetDeviceProcAddr`

- `vkGetInstanceProcAddr(instance, name)`:
  - `instance` が NULL: global な関数（`vkCreateInstance`・`vkEnumerateInstanceExtensionProperties`・`vkEnumerateInstanceLayerProperties`・`vkEnumerateInstanceVersion`・
    `vkGetInstanceProcAddr`）だけ我々の物を返し、他は NULL。
  - `name` が N（拒む名前）→ NULL。
  - `name` が I か O の関数 → 我々の関数。ただし instance の拡張の O（surface・display の関数）は、app がその拡張を有効にしていなければ NULL。
    device の拡張の O（swapchain の関数）は instance の時点では device の拡張が分からないので、常に我々の関数を返す（規格はこれを許す）。
  - それ以外（F の関数、後段の拡張の関数）→ 後段の `vkGetInstanceProcAddr(instance, name)` の答えをそのまま返す。
- `vkGetDeviceProcAddr(device, name)`:
  - `name` が N → NULL。
  - `name` が device の I の関数（`vkGetDeviceProcAddr`・`vkDestroyDevice`・`vkGetDeviceQueue`・`vkGetDeviceQueue2`）→ 我々の物。
  - `name` が swapchain の O の関数 → app がその device で `VK_KHR_swapchain` を有効にしていれば我々の物、していなければ NULL。
  - それ以外 → 後段の `vkGetDeviceProcAddr(device, name)`。これは driver の関数を直接指すので、app の呼び出しは我々を通らず速い。
- 我々の export の一覧は、`functions.tsv` で export が `y` の物（F の 1.0〜1.4 の core 全部と I と O）。後段の loader の export の一覧と同じにはしない
  （N の関数は export しない）。`exports.map` は `gen-forward.sh` が `functions.tsv` から作る（手で書かない）。

### 4.8 Wayland の WSI（`wsi-wayland.c`・`wsi-swapchain.c`）

**protocol**: core の `wl_surface`・`wl_buffer`・`wl_callback` と、`zwp_linux_dmabuf_v1` の **version 3**（server の版が 3 より大きくても 3 で bind する。
4 以上では `format`・`modifier` の event が来ず、`zwp_linux_dmabuf_feedback_v1` の format の表（mmap）を読む必要がある。feedback は WS105 では作らない）。
`zwp_linux_dmabuf_v1` は core ではないので、その client の stub（`linux-dmabuf-v1-client-protocol.h` と `wl_interface` の表）は
libvulkan-compat の**内部**に置く（F-065 の決定 3 と同じ扱い。libwayland-client にも libkeiland にも入れない）。stub は wayland-scanner の
出力ではなく、我々の libwayland の手書きの形（`userland/desktop/libwayland/*-protocol.c`）に合わせて書く。

**event queue**: app の event の処理を邪魔しないため、WSI は surface ごとに自分の `wl_event_queue` を作り（`wl_display_create_queue`）、
`wl_display` と `wl_surface` の wrapper（`wl_proxy_create_wrapper` + `wl_proxy_set_queue`）を通して request を送る。zedBSD の
`userland/desktop/libvulkan/wsi-wayland.c` が同じ形なので、queue・frame callback・release の扱いはそれを手本に読む（code の複写はしない、D7）。

**待つ時の形（必ず時間の上限を付ける）**: WSI の queue の event を待つときは、`wl_display_dispatch_queue` を使わず、
`wl_display_prepare_read_queue` → `wl_display_flush` → `poll(wl_display_get_fd, POLLIN, 残り時間)` → readable なら `wl_display_read_events`、時間切れなら
`wl_display_cancel_read` → `wl_display_dispatch_queue_pending` の形で書く（全て我々の libwayland にある: `userland/desktop/libwayland/client.c`）。

**surface の作成**（`vkCreateWaylandSurfaceKHR`）: `wl_display *` と `wl_surface *` を覚え、WSI の queue を作り、registry を取って `zwp_linux_dmabuf_v1` を
**version 3 で bind し、1 回 roundtrip**（`wl_display_roundtrip_queue`）して `format`・`modifier` の event を受ける。surface の問い合わせ（下）は swapchain より
前に呼ばれるので、bind はここで行う。compositor が `zwp_linux_dmabuf_v1` を持たなければ surface は作れるが、問い合わせは format 0 個・`vkCreateSwapchainKHR` は
`VK_ERROR_SURFACE_LOST_KHR`（stderr に 1 行）。**`format` の event に頼らない**（weston・wlroots は v3 で `modifier` だけを送る）: (format, modifier) の組は
`modifier` の event から集める。`format` の event しか来ない server（v1・v2）には bind しない（v3 未満は対象の外）。

**surface の問い合わせ**:

- `vkGetPhysicalDeviceSurfaceSupportKHR`: graphics の queue family で、device の WSI の道が「無し」でなければ真。
- `vkGetPhysicalDeviceWaylandPresentationSupportKHR`: 同じ。
- `vkGetPhysicalDeviceSurfaceFormatsKHR`: `VK_FORMAT_B8G8R8A8_UNORM`・`VK_FORMAT_B8G8R8A8_SRGB`（`DRM_FORMAT_ARGB8888`・`XRGB8888`）と
  `VK_FORMAT_R8G8B8A8_UNORM`・`_SRGB`（`DRM_FORMAT_ABGR8888`・`XBGR8888`）のうち、compositor が告げた組と後段が作れる物（WSI の道で使う tiling で）の共通部分。
  color space は `VK_COLOR_SPACE_SRGB_NONLINEAR_KHR` だけ。
- `vkGetPhysicalDeviceSurfaceCapabilitiesKHR`: `currentExtent` は `0xFFFFFFFF`（Wayland では client が大きさを決める）、
  `minImageCount` 2、`maxImageCount` 8、`maxImageArrayLayers` 1、usage は `COLOR_ATTACHMENT`・`TRANSFER_SRC`・`TRANSFER_DST`・`SAMPLED`、composite alpha は
  `OPAQUE`・`PRE_MULTIPLIED`・`INHERIT`、transform は `IDENTITY` だけ。
- `vkGetPhysicalDeviceSurfacePresentModesKHR`: `FIFO` と `MAILBOX`。
- `vkGetPhysicalDevicePresentRectanglesKHR`: 1 個、(0,0)〜surface の大きさ（不明なら `currentExtent` と同じく大きさを返せないので、swapchain の大きさ）。

**swapchain の image の作り方**（`vkCreateSwapchainKHR`、image ごと）:

1. format の modifier を選ぶ（`modifier` の道）: compositor が告げた modifier の集合 ∩ 後段の `VkDrmFormatModifierPropertiesListEXT`
   （`vkGetPhysicalDeviceFormatProperties2` の pNext。**v1 の struct を使う**: lavapipe は v2 の `...List2EXT` を埋めない、2026-10-01 確かめ）で、
   `drmFormatModifierPlaneCount == 1` で、tiling features に `COLOR_ATTACHMENT` があり、`vkGetPhysicalDeviceImageFormatProperties2` で export できる物。
   無ければ LINEAR。lavapipe は LINEAR だけを返す（確かめ済み）。
2. `vkCreateImage`: `VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT` + `VkImageDrmFormatModifierListCreateInfoEXT`（選んだ modifier）+
   `VkExternalMemoryImageCreateInfo{DMA_BUF_BIT_EXT}`（`linear` の道では `VK_IMAGE_TILING_LINEAR`）。
3. memory: `vkGetImageMemoryRequirements2` + dedicated（`VkMemoryDedicatedAllocateInfo`）+ `VkExportMemoryAllocateInfo{DMA_BUF_BIT_EXT}` で `vkAllocateMemory`、`vkBindImageMemory`。
4. `vkGetMemoryFdKHR(DMA_BUF)` で dma-buf の fd を得て、**`fcntl(fd, F_SETFD, FD_CLOEXEC)` を付ける**（lavapipe の fd は CLOEXEC でない、確かめ済み）。
   modifier と plane 0 の offset・stride を `vkGetImageDrmFormatModifierPropertiesEXT` と `vkGetImageSubresourceLayout(MEMORY_PLANE_0)`（`linear` の道は COLOR の aspect）で得る。
5. `zwp_linux_dmabuf_v1.create_params` → `add(dup(fd), 0, offset, stride, modifier_hi, modifier_lo)` → `create_immed(width, height, fourcc, 0)` で `wl_buffer` を作る
   （送った dup は閉じる）。元の fd は image の記録に残す（implicit sync の ioctl に使う）。

**present**（`vkQueuePresentKHR`、image ごと）— **implicit sync**（D6）:

1. 描画の完了を表す semaphore を作る: `vkQueueSubmit`（command buffer 1 個: 後段が `VK_EXT_queue_family_foreign` を持つなら、image の queue family の所有を
   `VK_QUEUE_FAMILY_FOREIGN_EXT` に渡す barrier（`oldLayout = newLayout = PRESENT_SRC_KHR`）。持たなければ command buffer 0 個、`pWaitSemaphores` = app の物、
   `pSignalSemaphores` = 我々の export できる semaphore（`VkExportSemaphoreCreateInfo{SYNC_FD}`）。
2. `vkGetSemaphoreFdKHR(SYNC_FD)` で sync_file の fd を得る（-1 なら既に signal 済み。lavapipe は常に本物の fd を返す、確かめ済み）。
3. その sync_file を dma-buf に付ける: `ioctl(dmabuf_fd, DMA_BUF_IOCTL_IMPORT_SYNC_FILE, {flags = DMA_BUF_SYNC_WRITE, fd})`（fd は消費されないので後で閉じる）。
   これで「この buffer への書き込みは、この fence が済むまで終わっていない」と kernel が知る。compositor はそれを待てる（§5.2）。
4. `wl_surface_attach` → `wl_surface_damage_buffer`（全体）→ FIFO なら `wl_surface_frame` → `wl_surface_commit` → `wl_display_flush`。
5. **予備の道（CPU で待つ）**: WSI の道の sync が「CPU で待つ」か、ioctl が `ENOTTY`・`EINVAL`（Linux 6.0 より前の kernel）・`EPERM` を返したとき: 手順 1 の
   submit に我々の fence を付け、`vkWaitForFences` で描画の完了を CPU で待ってから commit する（遅いが正しい）。ioctl が一度失敗したら、その swapchain では以後ずっと予備の道。
   試験は専用の LD_PRELOAD shim から IMPORT_SYNC_FILE の ENOTTY を返し、production の既定の予備の道を実行する。試験専用の production 環境変数は作らない（coding-style.md §12、p004 の検証手順の補い）。
6. **FIFO**: 前の present の frame callback が来るまで次の commit を送らない。待ちは上の「待つ時の形」で、**上限 100 ms**（compositor が隠れた surface に
   callback を送らないときに app が止まらないように。Mesa の WSI と同じ考え方）。上限を過ぎたら待たずに commit する。MAILBOX は待たない。
7. lavapipe では、手順 1 の submit の時点で CPU が描画の完了まで待つ（2026-10-01 確かめ）ので、`vkQueuePresentKHR` は CPU を止める。正しい動き。

**acquire**（`vkAcquireNextImageKHR`）:

- `wl_buffer.release` を受けた image だけが空き。空きが無ければ、上の「待つ時の形」で `timeout`（ns）まで待つ（0 なら待たない → `VK_NOT_READY`、
  時間切れ → `VK_TIMEOUT`、`UINT64_MAX` は上限無し）。
- 返す image の、compositor の読み終わりを待つ: `ioctl(dmabuf_fd, DMA_BUF_IOCTL_EXPORT_SYNC_FILE, {flags = DMA_BUF_SYNC_WRITE})` で
  「この buffer を読む・書く全ての fence」を sync_file にし、`vkImportSemaphoreFdKHR(SYNC_FD, TEMPORARY)` で app の semaphore に入れる
  （成功で fd は Vulkan の物。app が fence を渡したら、その fence にも同じ fd の dup を `vkImportFenceFdKHR(SYNC_FD, TEMPORARY)` で入れる）。
  予備の道では、sync_file の代わりに -1（済み）を import する（release の時点で compositor が読み終えている、§5.2 の約束）。
  import ができない後段（WSI の道の sync が「CPU で待つ」）では、空の submit で app の semaphore・fence を signal する。

**大きさの変化**: Wayland では client が大きさを決めるので、`VK_ERROR_OUT_OF_DATE_KHR` は返さない（`VK_SUBOPTIMAL_KHR` も返さない）。app が
新しい大きさで swapchain を作り直す。古い swapchain（`oldSwapchain`）の image は、release を受けたものから消す。

**dma-buf が使えないときの予備（wl_shm の複写）**: WSI の道が「無し」の場合。WS105 では作らない（範囲の外、§8）。

**試験で確かめられないこと（§10 V3・V9）**: host と guest の lavapipe は CPU で描き、submit の時点で完了まで待つので、compositor の側の implicit sync の待ちは常に 0 ms になる。
implicit sync の受け渡しが正しいこと（fence が dma-buf に付いていること）は、`dmabuf-probe` が export した sync_file を `SYNC_IOC_FILE_INFO`（`<linux/sync_file.h>`）で
読み raw fence 数/driver/timeline を記録する。空/完了時も kernel が stub=1 を返すため、受け渡しは試験 observer の IMPORT_SYNC_FILE flags=WRITE 成功で確かめ、fallback は ENOTTY 後の再試行無しと CPU wait の成功を全 frame で確かめる。実際に待つことの確かめは GPU の後段（実機）が要り、WS105 では未実施になる。

### 4.9 画面の WSI（VK_KHR_display と VK_EXT_acquire_drm_display、`wsi-display.c`・`kms.c`）

compositor（`/opt/keiland/bin/wayland`）が画面に出すための物。後段の VK_KHR_display は使わない（D8）。

**どの DRM の device か**: 環境変数 `KEILAND_DRM_DEVICE`（例 `/dev/dri/card0`）。無ければ `/dev/dri/card0`〜`card15` の中で connector を持つ最初の物。
`KEILAND_DRM_DEVICE=none` なら display を 1 つも見せない（**host での全ての試験はこれを設定する**。host の `/dev/dri/card0`（Matrox の console）を開かないため）。
compositor の seat（§5.1）は、自分が開いた DRM の device の path を、Vulkan の instance を作る**前に** `setenv("KEILAND_DRM_DEVICE", path, 1)` する
（seat の master の fd と libvulkan-compat の問い合わせの fd が同じ card を指すように）。

**二つの fd**:

| fd | 誰が開くか | 何に使うか |
| --- | --- | --- |
| 問い合わせの fd | libvulkan-compat 自身が、最初の display の問い合わせ（`vkGetPhysicalDeviceDisplayPropertiesKHR` など）で開く。`O_RDWR | O_CLOEXEC`。**開いた直後に `DRM_IOCTL_DROP_MASTER` を呼ぶ**（Linux は master の居ない primary node を最初に開いた fd を自動で master にするので、問い合わせの fd が master を奪わないように。master でなければ ioctl は失敗し、それは無視する） | connector・mode・CRTC の列挙（`VkDisplayKHR`・`VkDisplayModeKHR` を作る）。DRM の問い合わせの ioctl は master でなくても通る |
| master の fd | **compositor の seat の backend** が得る（root なら直接 open、gdm の下なら logind の `TakeDevice`）。`vkAcquireDrmDisplayEXT(physicalDevice, fd, display)` で渡される。**libvulkan-compat は `dup` して自分の複写を持つ**（呼んだ側は自分の fd を自由に閉じてよい） | mode の設定・dumb buffer・page flip（master が要る操作） |

- compositor の手順（WS104 p006 の `zwl_os_display_acquire`・`zwl_os_display_release` の Linux の実装）:
  ```
  seat が /dev/dri/card0 を開く（master）、setenv KEILAND_DRM_DEVICE         ← zwl_os_open（Vulkan より前）
  vkGetPhysicalDeviceDisplayPropertiesKHR → display を選ぶ（今の compose.c・vkdemo/display.c と同じ）
  vkAcquireDrmDisplayEXT(physicalDevice, seat の master の fd, display)      ← zwl_os_display_acquire（swapchain の前）
  vkCreateDisplayPlaneSurfaceKHR → vkCreateSwapchainKHR → vkQueuePresentKHR …
  vkReleaseDisplayEXT(physicalDevice, display)                               ← zwl_os_display_release（swapchain を消した後）
  ```
  compositor は fd を渡すだけで、DRM の ioctl は呼ばない（WS103 の「compositor は GPU を Vulkan だけで扱う」を Linux でも保つ）。
- `vkAcquireDrmDisplayEXT` は何度呼んでもよい（同じ display に新しい fd を渡すと、前の dup を閉じて新しい物に替える）。logind の resume で fd が変わったときに使う（§5.5）。
- acquire されていない display に swapchain を作ったとき（vkdemo など root で直接走る app）: libvulkan-compat が問い合わせの fd で master を取ろうとする
  （`DRM_IOCTL_SET_MASTER`）。取れなければ `VK_ERROR_INITIALIZATION_FAILED`。
- `vkGetDrmDisplayEXT(physicalDevice, drmFd, connectorId, &display)` も実装する（規格の関数。connector の番号から display を返す）。compositor は使わない。
- display は全ての physical device に同じ物を見せる（lavapipe のように DRM の device と関係の無い後段があるため。複写の道なのでどの device でも出せる）。
- KMS の操作（`kms.c`）は Linux の kernel の DRM の ioctl を直接使う（`<drm/drm.h>`・`<drm/drm_mode.h>`、linux-libc-dev の header、MIT）。
  **libdrm には link しない**（D9）。使う ioctl: `DRM_IOCTL_SET_MASTER`・`DROP_MASTER`・`MODE_GETRESOURCES`・`GETCONNECTOR`・`GETENCODER`・`GETCRTC`・
  `CREATE_DUMB`・`MAP_DUMB`・`DESTROY_DUMB`・`ADDFB2`・`RMFB`・`SETCRTC`・`PAGE_FLIP`（`DRM_MODE_PAGE_FLIP_EVENT`）。flip の完了の event は fd の `read`。
  **logind から来た fd は `O_NONBLOCK`** なので、event は `poll(fd, POLLIN, 上限 100 ms)` してから `read` する。
- **image の出し方は「複写の道」（v1、全ての後段で動く、D20）**: swapchain の image は後段の普通の image（`OPTIMAL`、usage に `TRANSFER_SRC` を足す）。
  present のたびに `vkCmdCopyImageToBuffer` で host-visible・host-coherent の buffer に写し、fence を待ち、KMS の dumb buffer（`CREATE_DUMB`・`MAP_DUMB` + mmap）に
  行ごとに memcpy し（stride が違う）、`PAGE_FLIP` する（最初の 1 回は `SETCRTC`）。dumb buffer は 2 つ（前と後）。flip の完了の event を読んでから次の flip を出す（FIFO）。
  1920×1080 で 1 frame 約 8 MB の memcpy。QEMU で 60 Hz を目標にしない（受け入れは「表示が正しい」こと）。
- **複写しない道**（dumb buffer の dma-buf を後段に import して直接描く、または後段の image の dma-buf を `ADDFB2` する）は WS105 では作らない（§8、Future Work）。
- master を失ったとき（logind の `PauseDevice`、root での `chvt`）: `PAGE_FLIP`・`SETCRTC` が `EACCES`・`EPERM` になる。
  `vkQueuePresentKHR` は `VK_ERROR_OUT_OF_DATE_KHR` を返す。compositor は pause の間は present を止め、resume で swapchain を作り直す（§5.5）。

### 4.10 file の構成（`userland/desktop/libvulkan-compat/`）

| file | 役割 |
| --- | --- |
| `Makefile.linux` | Linux の build だけ（zedBSD の `Makefile` は**作らない**。zedBSD では build しない、D4） |
| `README.md` | この節の要約と、§4.2 の図 |
| `functions.tsv` | 全ての関数の名前と種類（F・I・O・N）、export するか。`plan/ws105/survey/vkprotos.sh` の出力（host の `/usr/include/vulkan/vulkan_core.h` の `VK_VERSION_1_0`〜`VK_VERSION_1_4`、1.4.309 で 137・28・13・37・19 個）に種類の列を足した物。**生成した後に手で直す file** として source に入れる（header の版が変わったら作り直して差を見る） |
| `gen-forward.sh` | `functions.tsv` と host の `vulkan_core.h` から `forward.inc`（F の関数の定義と dlsym の表）と `exports.map` を作る sh の script。build の時に走らせる（生成物は `$(KEILAND_LINUX_BUILD)/gen/libvulkan-compat/` に置き、source に入れない）。手本: `plan/ws105/survey/gen-forward.sh` |
| `backend.c` | §4.3（後段を開く、確かめ） |
| `dispatch.c` | F の表の初期化、`vkGetInstanceProcAddr`・`vkGetDeviceProcAddr`（§4.7）、再入の検出（§4.11） |
| `instance.c` | `vkCreateInstance`・`vkDestroyInstance`・instance の拡張の一覧（§4.6）、instance の記録 |
| `device.c` | `vkCreateDevice`・`vkDestroyDevice`・device の拡張の一覧・`vkGetDeviceQueue*`、WSI の道の判定、device と queue の記録 |
| `wsi-wayland.c` | Wayland の surface、`zwp_linux_dmabuf_v1` の bind と (format, modifier) の受け取り、event queue、時間の上限つきの待ち |
| `wsi-swapchain.c` | swapchain（Wayland と display の共通の部分: image の配列、acquire・present の入口）、device group の 3 関数 |
| `wsi-display.c` | VK_KHR_display・VK_EXT_direct_mode_display・VK_EXT_acquire_drm_display の Vulkan の側 |
| `kms.c` | KMS の ioctl（§4.9） |
| `linux-dmabuf-v1-protocol.c`・`linux-dmabuf-v1-client-protocol.h` | protocol の stub（内部） |
| `compat.h` | 内部の struct と関数の宣言 |

### 4.11 名前の衝突（symbol interposition）と確かめ

我々の library と後段は、どちらも `vkCreateInstance` などの同じ名前を export する。

- **我々 → 後段**: 我々は後段の関数を `dlsym(backend_handle, name)` の pointer で呼ぶので、名前の解決に頼らない。安全。
- **後段 → 我々（2026-10-01 の実験で実際に起きた）**: Debian の Khronos の loader（1.4.309、`-Bsymbolic` 無し）は、自分の export の `vk*` の address を
  269 個の `R_X86_64_GLOB_DAT` の relocation で取っている。我々の library が先に載っていると、それらは process の global な scope の**我々の**関数に結び付く
  （DT_NEEDED で載せた試験で 215 個）。その結果:
  - 後段の `vkGetInstanceProcAddr` は、instance の名前（`vkCreateInstance`・`vkCreateDevice`・`vkEnumeratePhysicalDevices` など）に**我々の関数**を返し、
    `vkGetDeviceProcAddr` は `vkGetDeviceProcAddr`・`vkDestroyDevice`・`vkGetDeviceQueue` に我々の関数を返す。
  - 後段の `vkCreateInstance` の中から、我々の `vkEnumeratePhysicalDevices` が実際に呼ばれた（design-reviewer の実験）。
  - I の関数が「次」を後段の `vkGet*ProcAddr` から得ると、自分自身を呼んで無限に循環する。
- **対策（D10、2026-10-01 改訂）**:
  1. **後段を `RTLD_DEEPBIND` で開く（glibc の既定）**。後段とその依存は、自分の symbol を global な scope より先に見る。実験で結び付きは 0 になり、
     後段の `vkGetInstanceProcAddr` は後段自身の trampoline を返した。`KEILAND_VULKAN_NO_DEEPBIND=1` で外せる（app が malloc などを preload で差し替えていて、
     DEEPBIND の下の後段がそれを見ないと困る場合のため。外した場合は下の 2・3 で循環を防ぐ）。
  2. **I の関数の後段の版は `dlsym(handle, 名前)` の pointer だけで呼ぶ**（§4.4 の規則）。
  3. 我々の F・I の入口に thread-local の再入の数を持ち、**同じ関数への再入を見つけたら** stderr に
     `libvulkan-compat: the backend called back into vkXxx (symbol interposition); unset KEILAND_VULKAN_NO_DEEPBIND` を出して `abort()` する（無限の循環で固まるより良い）。
  4. 我々の library は `-Wl,-Bsymbolic` で link する（§4.3）。
- **確かめ（p003 の受け入れ、`interpose-check.sh`）**:
  - (a) 既定（DEEPBIND）で `LD_DEBUG=bindings` の出力に、後段の file から我々の file への `vk` の symbol の結び付きの行が **0 個**。行の形（2026-10-01 確かめ）:
    `     2776112:	binding file /usr/lib/x86_64-linux-gnu/libvulkan.so.1 [0] to /…/lib/libvulkan.so.1 [0]: normal symbol `vkCreateDescriptorSetLayout'`（行頭は空白と pid と TAB）。
  - (b) `KEILAND_VULKAN_NO_DEEPBIND=1` でも `vk-chain-test` が PASS し、再入の検出が働かない（2 の規則が守られている）。
  - (c) 試験の program は我々の library に **DT_NEEDED で** link する（`dlopen` で載せると結び付きは起きないので確かめにならない）。
    Debian の gcc は既定で `--as-needed` なので、`vk*` を直接呼ばない試験の program は `-Wl,--no-as-needed` を付ける。
- 同じ SONAME（`libvulkan.so.1`）の 2 つの file が 1 つの process に別々に載ることは 2026-10-01 に確かめた（V2）。
- **我々の libwayland-client と system の libwayland-client**: 後段の ICD（Mesa の lavapipe など）と implicit layer（`libVkLayer_MESA_device_select.so`）は
  `libwayland-client.so.0`（system の物）に依存している。我々の libwayland-client の SONAME は `libwayland-client.so`（版の番号が無い。zedBSD と同じ）で名前が違うので、
  ICD を載せると system の物も同じ process に載る。DEEPBIND の下では ICD は system の物を使い、DEEPBIND を外すと先に載った我々の物に結び付く。どちらでも、
  **後段の instance で WSI の拡張を有効にしない**（§4.6、我々が取り除く）限り、ICD と layer の Wayland の code は実行されない（不変条件。layer の device_select は
  WSI の拡張が無いと Wayland を開かない、2026-10-01 の design-reviewer の観察）。Mesa 25.0.7 の 4 つの ICD が参照する `wl_*` の symbol が全て我々の
  libwayland-client にあることを `nm` で確かめた（D19）。
- **我々の compat の library と system の library**: 我々の `libz-compat.so` は `crc32` などを版無しで export する。lavapipe・libLLVM は版無しの `crc32` などを参照するので、
  DEEPBIND を外すと我々の物に結び付く（DEEPBIND の下では後段の依存の system の zlib を使う）。我々の物が zlib と ABI まで互換なら害は無い（F-065 の確かめ 2）。§10 V10。
- **system の libwayland を使う app（Keiland の外の app）は対象の外**: そういう app の `wl_display *` は system の libwayland の物で、我々の WSI が我々の
  libwayland の関数で扱うと壊れる。libvulkan-compat は `/opt/keiland/bin` の Keiland の app（我々の libwayland-client に link した物）のためだけの物である。
  `LD_LIBRARY_PATH` で `vkcube` などを我々の library で走らせることは試験に使わない。
- **`vulkaninfo` について**: `vulkaninfo` は `libvulkan.so.1` に link せず、`dlopen("libvulkan.so.1")` と `vkGetInstanceProcAddr` だけを使う（確かめ済み）。
  `--summary` でも、Wayland の socket（`$XDG_RUNTIME_DIR/wayland-0`）があれば WSI を使う。我々の library で走らせるときは
  `env -u WAYLAND_DISPLAY -u DISPLAY XDG_RUNTIME_DIR=<空の directory> KEILAND_DRM_DEVICE=none LD_LIBRARY_PATH=<stage>/opt/keiland/lib vulkaninfo --summary` とする。
  名前の結び付きの確かめには使えない（dlopen なので）。

## 5. compositor の Linux の module（`userland/desktop/wayland/linux/`）

WS104 の後、compositor の OS の部分は `wayland/zedbsd/` の file と、共通の code が呼ぶ 3 つの header（`zwl-gpu.h`・`zwl-input.h`・`zwl-os.h`）と
`zwl_handoff_*` の関数になっている。Linux では同じ関数を `wayland/linux/` の file で実装する。共通の code の変更は、下の §5.6 に挙げた物だけにする。
2026-10-01 の改訂 2: guest の survey と design-reviewer の結果で §5.1・§5.2・§5.5・§5.6 を直した。

| file | 実装する物 | Phase |
| --- | --- | --- |
| `linux/os-linux.c` | `zwl-os.h`（seat の選択と open・close、VT、既定の socket、`KEILAND_DRM_DEVICE` の setenv、`zwl_os_display_acquire`・`zwl_os_display_release`、logind の poll） | p006（direct）、p009（logind） |
| `linux/seat-direct-linux.c` | root で DRM と入力の device を直接 open する seat | p006 |
| `linux/seat-logind-linux.c`・`linux/dbus-linux.c`・`linux/dbus-linux.h` | logind の seat と最小の D-Bus | p009 |
| `linux/input-linux.c` | `zwl-input.h`（`/dev/input/eventN`、device の fd は seat から） | p006 |
| `linux/handoff-linux.c` | `zwl_handoff_*`（sessiond が無い） | p006 |
| `linux/gpu-linux.c` | `zwl-gpu.h`（`zwp_linux_dmabuf_v1` の server、dma-buf の import、implicit sync） | p006（global 無しの空の形）、p007（全部） |
| `linux/seat-linux.h` | linux の file の間の内部の header（seat の open・close、device の open・close、DRM の fd と path、pause の状態） | p006 |

### 5.1 seat（`os-linux.c`・`seat-direct-linux.c`）

- seat の選び方: 環境変数 `KEILAND_SEAT=direct|logind`。無ければ、`XDG_SESSION_TYPE` が `wayland` で `XDG_SESSION_ID` があれば `logind`、それ以外は `direct`。
  （`XDG_SESSION_ID` だけで決めてはいけない: root の SSH の session も `XDG_SESSION_ID` を持ち（`XDG_SESSION_TYPE=tty`）、そこから `openvt` で起動した compositor は
  logind を選んでしまう。2026-10-01 の guest の survey で確かめた。）gdm の `gdm-wayland-session` は `XDG_SESSION_TYPE=wayland` を設定する。
  試験の command では常に `KEILAND_SEAT` を明示する（p006〜p008 は `KEILAND_SEAT=direct`）。
  p006 では `direct` だけ（`logind` を選んだら `ENOTSUP` で起動の失敗、message `ZWL OS unavailable errno=95` と stderr に `seat logind is not built yet`）。
- `direct`（root で走る。組み込みと開発用）:
  - DRM: `open(KEILAND_DRM_DEVICE or "/dev/dri/card0", O_RDWR | O_CLOEXEC)`。master の居ない primary node を最初に開いた fd は自動で master になる
    （`zwl_os_open` は Vulkan の instance を作る前に呼ばれるので、compositor の fd が先に master を取る）。開いた path を `setenv("KEILAND_DRM_DEVICE", path, 1)` する
    （libvulkan-compat の問い合わせの fd が同じ card を開くように、§4.9）。
  - `zwl_os_display_acquire(server, physical, display)`: `vkGetInstanceProcAddr(compose の instance, "vkAcquireDrmDisplayEXT")` で関数を得て、DRM の fd を渡す
    （libvulkan-compat が dup する）。`zwl_os_display_release`: `vkReleaseDisplayEXT`。**compositor は DRM の ioctl を呼ばない。**
  - 入力: `input-linux.c` が `open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC)`。
  - VT: 標準入力が VT（`ioctl(0, KDGETMODE, &mode)` が成功）なら、`KDSETMODE KD_GRAPHICS`（console の文字を描かせない）と `KDSKBMODE K_OFF`（key を tty に流さない）にし、
    元の値を覚えて `zwl_os_close` で戻す。VT でなければ何もしない（SSH から直接起動した場合。そのときは key が console にも流れる）。
    `K_OFF` の間、kernel は Ctrl+Alt+Fn の VT の切り替えをしない（試験では SSH の `chvt` を使う）。
    試験では `openvt -c 7 -s -- sh -c "..."` で VT 7 で起動する（§7）。
  - 終わり方: SIGINT・SIGTERM → 今の `stop_service` → `service_cleanup` → `zwl_os_close`（VT を戻す、fd を閉じる）。
- 既定の socket: `--socket` が無ければ `$XDG_RUNTIME_DIR/wayland-keiland`、`XDG_RUNTIME_DIR` が無ければ `/tmp/wayland-keiland`（`zwl_os_open` で決める。
  共通の `main.c` に「`--socket` が与えられたか」の flag を足す、§5.6）。
- 起動の引数: compositor は `--session` も `--greeter` も無いと 150 秒で終わる（`main.c:77`、試験の既定）。Linux の起動では必ず `--session --glass` を付ける
  （zedBSD の `/etc/keiland/session` と同じ）。

### 5.2 GPU の buffer（`gpu-linux.c`、p007）

**global**: `zwl_gpu_global_interface()` → `"zwp_linux_dmabuf_v1"`、`zwl_gpu_global_version()` → 3。

**protocol**（`/usr/share/wayland-protocols/stable/linux-dmabuf/linux-dmabuf-v1.xml` が正。interface の版は 5 だが、我々は version 3 の範囲だけを出す）:

| object | request（opcode） | event（opcode） |
| --- | --- | --- |
| `zwp_linux_dmabuf_v1`（kind `ZWL_FACTORY`） | 0 `destroy`、1 `create_params(new_id params)` | 0 `format(uint format)`、1 `modifier(uint format, uint hi, uint lo)` |
| `zwp_linux_buffer_params_v1`（新しい kind `ZWL_GPU_OBJECT`、§5.6） | 0 `destroy`、1 `add(fd, uint plane_idx, uint offset, uint stride, uint modifier_hi, uint modifier_lo)`、2 `create(int width, int height, uint format, uint flags)`、3 `create_immed(new_id wl_buffer, int width, int height, uint format, uint flags)` | 0 `created(new_id wl_buffer)`、1 `failed` |

- error の値（params）: `already_used` 0、`plane_idx` 1、`plane_set` 2、`incomplete` 3、`invalid_format` 4、`invalid_dimensions` 5、`out_of_bounds` 6、`invalid_wl_buffer` 7。flags: `y_invert` 1、`interlaced` 2、`bottom_first` 4（我々はどれも受けず、0 以外は `create` で `failed`、`create_immed` で `invalid_format`）。
- bind の時: 受け付ける format ごとに `format` の event を 1 回、(format, modifier) の組ごとに `modifier` の event を 1 回送る（v3 では両方を送ってよい。我々の client は `modifier` だけを読む）。
- 受け付ける format: `DRM_FORMAT_ARGB8888`（`0x34325241`）→ `VK_FORMAT_B8G8R8A8_UNORM`・合成は alpha、`DRM_FORMAT_XRGB8888`（`0x34325258`）→ 同じ VkFormat・合成は opaque。
- 受け付ける modifier: 後段の `vkGetPhysicalDeviceFormatProperties2(B8G8R8A8_UNORM)` + `VkDrmFormatModifierPropertiesListEXT`（v1 の struct）のうち、plane が 1 つで
  `drmFormatModifierTilingFeatures` に `SAMPLED_IMAGE` がある物、かつ `vkGetPhysicalDeviceImageFormatProperties2`（`VkPhysicalDeviceImageDrmFormatModifierInfoEXT` と
  `VkPhysicalDeviceExternalImageFormatInfo{DMA_BUF}`）で `IMPORTABLE` の物。起動の時（`zwl_gpu_device_extensions` の後、device を作った後の最初の bind の前）に 1 回求めて覚える。
  lavapipe は LINEAR（0）だけ（2026-10-01 確かめ）。
- 複数の plane の buffer（YUV など）は WS105 では受けない。
- **`create`（非同期）の `created` の event**: server が wl_buffer の id を割り当てる。compositor には既に `zwl_create_server(client, kind, version)`
  （`userland/desktop/wayland/objects.c:102`、server の id の範囲 `ZWL_SERVER_ID_FIRST 0xff000000`（`zwl.h:56`）、`wl_data_offer` が使っている（`data.c:801`）、
  server の id には `delete_id` を送らない（`objects.c:369`））がある。それで `ZWL_BUFFER` を作り、`created(new_id)` の event で id を送る。
- import（`create`・`create_immed`）:
  1. 検査: plane 0 が add 済み、plane 1 以上が無い、format と modifier が受け付ける組、`0 < width, height ≤ server->gpu_limits.max_dimension`、
     `stride ≥ width × 4`、`offset + stride × height ≤ lseek(fd, 0, SEEK_END)`（dma-buf の大きさ。lavapipe では page に丸めた値）。違反は上の error（`out_of_bounds` など）。
  2. `vkCreateImage`: 2D、mip 1、layer 1、sample 1、`tiling = VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT`、usage `SAMPLED`、pNext に
     `VkImageDrmFormatModifierExplicitCreateInfoEXT{modifier, 1, &VkSubresourceLayout{offset, 0, stride, 0, 0}}` と `VkExternalMemoryImageCreateInfo{DMA_BUF_BIT_EXT}`。
  3. `vkGetMemoryFdPropertiesKHR(DMA_BUF, fd)`（関数は `vkGetDeviceProcAddr` で得る）の `memoryTypeBits` と `vkGetImageMemoryRequirements` の bits の共通の最も低い bit を memory type にする（無ければ失敗）。
  4. `vkAllocateMemory`: `VkImportMemoryFdInfoKHR{DMA_BUF, dup(fd)}` + `VkMemoryDedicatedAllocateInfo{image}`、**大きさは requirements の size**（`lseek` の値でない）。
     成功すると Vulkan が dup の fd を持つ（lavapipe はすぐ閉じる）。失敗なら dup を閉じる。
  5. `vkBindImageMemory`、`zwl_import_adopt(buffer, image, memory, width, height, format)`、ARGB なら `zwl_import_set_alpha(buffer, 1)`。
  6. plane の fd の 1 本（`dup`、`FD_CLOEXEC`）を buffer の OS の記録（`gpu_private`）に残す（implicit sync の ioctl に使う）。buffer が消えるときに `zwl_gpu_object_free` で閉じる。
  7. log: `ZWL IMPORT client=%llu buffer=%u width=%u height=%u bytes=%llu`（zedBSD と同じ形。bytes は dma-buf の大きさ）。失敗の log は zedBSD と同じ `ZWL IMPORT_ERROR client=%llu errno=%d`。
- **implicit sync**（acquire）: `zwl_gpu_commit(surface, buffer)` で、残した fd に `ioctl(DMA_BUF_IOCTL_EXPORT_SYNC_FILE, {flags = DMA_BUF_SYNC_READ})`
  （読む前に待つべき fence = client の描画）。得た sync_file を surface の `acquire[]` に入れる（generation は 1）。後は今の共通の code（`commit_fence`・`zwl_fence_ready` の
  poll）がそのまま待つ。`--log-frames` なら zedBSD と同じ形の `ZWL ACQUIRE_FENCE client=... surface=... generation=1` を出す。ioctl が `ENOTTY` なら（古い kernel）fence 無し。
- release: 共通の code は、合成の frame の完了（`zwl_compose_complete`）の後に `wl_buffer.release` を送る。つまり release の時点で compositor は読み終えている。
  client（libvulkan-compat）は release の後に `EXPORT_SYNC_FILE(WRITE)` で念のため待つ（§4.8）。compositor が読みの fence を dma-buf に付ける必要は無い。
- **queue family の所有**: client は present で image を `VK_QUEUE_FAMILY_FOREIGN_EXT` に渡す（§4.8、後段が `VK_EXT_queue_family_foreign` を持つとき）。compositor の側の受け取り
  （FOREIGN から自分の queue family への barrier）は共通の `import.c` の layout の移行を変える必要があるので、WS105 では行わない（LINEAR の image では影響が無い。§8・§10 V8）。
- Vulkan の拡張（`zwl_gpu_instance_extensions`・`zwl_gpu_device_extensions`）: **compositor は `VK_API_VERSION_1_0` で instance を作る**（`compose.c:612`）ので、
  1.1 以上の core の機能も拡張として要る。instance: `VK_EXT_direct_mode_display`・`VK_EXT_acquire_drm_display`（`VK_KHR_get_physical_device_properties2` は compose が既に有効にする）。
  device: `VK_EXT_external_memory_dma_buf`・`VK_EXT_image_drm_format_modifier`・`VK_KHR_image_format_list`・`VK_KHR_bind_memory2`・`VK_KHR_sampler_ycbcr_conversion`・
  `VK_KHR_maintenance1`（`VK_KHR_external_memory`・`_fd`・`get_memory_requirements2`・`dedicated_allocation` は compose が既に有効にする）。
  `zwl_gpu_device_extensions` は physical device を受け取り（WS104 p004 の hook の形）、`vkEnumerateDeviceExtensionProperties` で後段にある物だけを返す。
  `VK_EXT_image_drm_format_modifier` が無い後段では、import は `VK_IMAGE_TILING_LINEAR` で行い、受け付ける modifier は LINEAR だけ（stride は client の値と照らし、違えば `failed`）。
- frame の fence: `zwl_gpu_frame_fence_type()` → 0（fd にしない。D22。SYNC_FD の export は fence を reset する（2026-10-01 確かめ）ので、今の
  `zwl_compose_complete` の `vkWaitForFences` と合わない。lavapipe には OPAQUE_FD の fence も無い。main loop は今の `vkGetFenceStatus` の poll の道を使う）。

### 5.3 入力（`input-linux.c`）

- `zwl_input_scan`: `/dev/input` の `eventN` を列挙し、まだ開いていない物を seat の関数で開き（direct は `open`、logind は `TakeDevice`）、`EVIOCGBIT` で bit を読んで
  `zwl_input_probe` に渡す。zedBSD の `zedbsd/input-zedbsd.c` と同じ形で、header は `zwl-evdev.h`（Linux では `<linux/input.h>`）。hotplug は zedBSD と同じ 2 秒ごとの再走査
  （inotify は使わない）。**seat が pause の間は走査しない。**
- device を開いた直後に `ioctl(fd, EVIOCSCLOCKID, &(int){CLOCK_MONOTONIC})` をする（Linux の evdev の時刻の既定は `CLOCK_REALTIME`。compositor は event の時刻を
  `zwl_milliseconds()`（`CLOCK_MONOTONIC`）と比べるので揃える。zedBSD の evdev は起動からの時間）。
- `zwl_input_device_absinfo`・`_name`・`_id`・`_read`・`_close`: `EVIOCGABS`・`EVIOCGNAME`（Linux は長さを返すので 0 以上は成功）・`EVIOCGID`・`read`・seat の close。
- `EVIOCGRAB` はしない（direct は `K_OFF`、logind は logind が VT を管理する）。
- guest の `/dev/input/eventN` の番号は起動ごとに変わる（2026-10-01 確かめ）。名前（`EVIOCGNAME`）で判断する試験を書かない。

### 5.4 session（`handoff-linux.c`）

| 関数 | Linux の動き |
| --- | --- |
| `zwl_handoff_wait` | 何もしない（すぐ画面を取る） |
| `zwl_handoff_release` | `zwl_compose_output_close(server)`（zedBSD の release から sessiond への書き込みを除いた物） |
| `zwl_handoff_logout` | 0 を返す（呼ぶ側の `home.c` が `zwl_request_stop()` で compositor を終える → gdm が greeter に戻す） |
| `zwl_handoff_tick` | 何もしない |

- lock 画面（`zwl_lock`）は `control_fd < 0` で何もしない（zedBSD の sessiond 無しと同じ）。App Home に Lock Screen は出ない。
- session の中の Shut Down は無い（zedBSD の session にも無く、greeter だけが持つ）。Linux では gdm の greeter が持つ（D11 の補い）。

### 5.5 logind と pause・resume（p009）

**D-Bus**（`dbus-linux.c`。手本は `plan/ws105/survey/dbusprobe.py`、host の system bus で通った物）:

1. `socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC)` で `/run/dbus/system_bus_socket` に接続。
2. **NUL の 1 byte を送る**（無いと dbus-daemon は切る）。
3. `AUTH EXTERNAL <getuid() の 10 進の文字列を 16 進の ASCII にした物>\r\n` → `OK <guid>\r\n`。
4. `NEGOTIATE_UNIX_FD\r\n` → `AGREE_UNIX_FD\r\n`（fd を受けるのに要る）。
5. `BEGIN\r\n`。
6. `Hello`（path `/org/freedesktop/DBus`、interface・destination `org.freedesktop.DBus`）を呼ぶ。返事の前に `NameAcquired` の signal が来ることがある（**signal と method の返事は混ざって来る**）。
7. 読みは常に `recvmsg` と cmsg の buffer（`SCM_RIGHTS` の fd が来る）。

message の形: header は endian の byte `l`、type（1 method_call、2 method_return、3 error、4 signal）、flags、version 1、body の長さ（u32）、serial（u32）、
header の field の配列 `a(yv)`（1 PATH `o`、2 INTERFACE `s`、3 MEMBER `s`、4 ERROR_NAME `s`、5 REPLY_SERIAL `u`、6 DESTINATION `s`、7 SENDER `s`、8 SIGNATURE `g`、
9 UNIX_FDS `u`）、header の後を 8 byte に揃える。型は `s`・`o`・`g`・`u`・`b`・`h`（fd の配列の index の u32）と、それらの struct（reply の `(hb)`）だけ扱う。
`TakeDevice(226, 0)` の method_call の byte の並び（serial 5、176 byte、host で通った物）:

```
0000 6c 01 00 01 08 00 00 00 05 00 00 00 98 00 00 00   'l' CALL flags=0 v1 bodylen=8 serial=5 fieldsarraylen=0x98
0010 01 01 6f 00 23 00 00 00 "/org/freedesktop/login1/session/_32" 00 (8 に揃える)
0040 06 01 73 00 16 00 00 00 "org.freedesktop.login1" 00 00
0060 02 01 73 00 1e 00 00 00 "org.freedesktop.login1.Session" 00 00
0088 03 01 73 00 0a 00 00 00 "TakeDevice" 00 + 0xa0 まで揃える
00a0 08 01 67 00 02 75 75 00                             SIGNATURE g "uu"
00a8 e2 00 00 00 00 00 00 00                             body u32 226, u32 0
```

返事は method_return（field 5 REPLY_SERIAL、field 8 SIGNATURE `hb`、field 9 UNIX_FDS 1）、body は fd の index の u32 と bool の u32。

**logind**（`seat-logind-linux.c`。`man 5 org.freedesktop.login1`、systemd 257）:

1. `org.freedesktop.login1.Manager.GetSession(s $XDG_SESSION_ID)` → session の object path（`_3` の escape がある。例 `/org/freedesktop/login1/session/_3208`。**返った path をそのまま使う**。
   `session/self`・`session/auto` では signal が来ない）。
2. `AddMatch`: `type='signal',sender='org.freedesktop.login1',interface='org.freedesktop.login1.Session',path='<返った path>'`。
3. `org.freedesktop.login1.Session.TakeControl(b false)`（呼ぶ process の uid が session の uid か root であること）。
4. DRM: `TakeDevice(u major, u minor)` → `(h fd, b inactive)`（major・minor は `stat(KEILAND_DRM_DEVICE or /dev/dri/card0)` の `st_rdev`、DRM は 226）。
   **logind の fd は `O_NONBLOCK`**。入力: device ごとに同じく。**1 つの device は 1 回しか TakeDevice できない**（ReleaseDevice するまで次は失敗する）。close は `ReleaseDevice`。
5. signal `PauseDevice(u major, u minor, s type)`: type が `pause` なら、その device の使用を止めてから `PauseDeviceComplete(u, u)` を送る。`force`・`gone` は返事しない。
6. signal `ResumeDevice(u major, u minor, h fd)`: 新しい fd に替え、古い fd を閉じる。
- 経験上の動き（systemd の source から。p009 で確かめる）: pause で DRM は master を外され、evdev は `EVIOCREVOKE` されて read が `ENODEV` になる。
  `Seat.SwitchTo` による切り替えは `pause`、kernel の VT の切り替え（`chvt`）は `force` を送る。

**pause・resume の扱い**（§5.6 の共通の変更）:

- DRM の pause: `server->os_paused = 1`。`zwl_compose_output_close`（swapchain を消し、`zwl_os_display_release` で `vkReleaseDisplayEXT`）。paused の間は合成と present をしない
  （`zwl_schedule` が描かない）。`pause` なら `PauseDeviceComplete`。
- DRM の resume: 新しい fd を seat に持ち、`os_paused = 0`、`output_open` の道（`enter_window_mode` と同じ）で swapchain を作り直す（`zwl_os_display_acquire` が新しい fd で
  `vkAcquireDrmDisplayEXT` を呼ぶ）。
- 入力の pause: その device の fd を読まない（poll の set から外す）。**閉じない・ReleaseDevice しない**（resume で同じ device の新しい fd が来る）。`gone` なら閉じて記録を消す。
- 入力の resume: `ResumeDevice` の fd を device の新しい fd にする（古い fd は閉じる）。
- pause の間は 2 秒ごとの入力の再走査をしない。
- VT の切り替えの key（Ctrl+Alt+Fn → `org.freedesktop.login1.Seat.SwitchTo(u)`、`/org/freedesktop/login1/seat/seat0`）は WS105 では作らない（§8）。試験は SSH から
  `chvt N`（`force`）と `busctl call org.freedesktop.login1 /org/freedesktop/login1/seat/seat0 org.freedesktop.login1.Seat SwitchTo u N`（`pause`）で行う。

**gdm**（Debian 13、gdm3 48.0-2）:

- gdm は `/usr/share/wayland-sessions/*.desktop` の session を `/usr/libexec/gdm-wayland-session [--register-session] "<Exec>"` で起動する。`--register-session` は
  `.desktop` に `X-GDM-SessionRegisters=true` が**無い**ときに付き、そのとき gdm-wayland-session が我々の代わりに session を登録する（Keiland は登録も sd_notify も要らない）。
  `DesktopNames` は `XDG_CURRENT_DESKTOP` になるだけ（任意）。
- `keiland.desktop`（`/usr/share/wayland-sessions/`、`make keiland-linux-install-session`）:
  ```
  [Desktop Entry]
  Name=Keiland
  Comment=The Keiland desktop
  Exec=/opt/keiland/bin/wayland --session --glass
  Type=Application
  DesktopNames=Keiland
  ```
  （`--wallpaper` などは zedBSD の `/etc/keiland/session` を見て、Linux の既定（`KEILAND_DATADIR`）で足りない物を Exec に足す。）
- gdm の udev の規則は virtio-vga を仮想の GPU と見なす。**GPU が 2 つ以上あると gdm は Wayland を止める**ので、guest に GPU を足さない。
- 自動 login と session の選び方は §7.2。

### 5.6 共通の code の変更（WS105 で許す物だけ）

| 変更 | 理由 | Phase |
| --- | --- | --- |
| `protocol.c`: `zwl_gpu_global_interface()` が NULL なら GPU の global を出さない | p006 の Linux の module はまだ global を持たない | p006 |
| `main.c`: `--socket` が与えられたかの flag | 既定の socket を OS が決める | p006 |
| `userland/desktop/wayland/titlebar-shell.c:1299`: `float colour[4] = { 0.0f, 0.0f, 0.0f, 0.0f };` | gcc の `-Wmaybe-uninitialized`（誤検出。振る舞いは変わらない） | p006 |
| `zwl.h`・`protocol.c`: object の kind `ZWL_GPU_OBJECT`（OS の GPU の protocol の factory 以外の object）を足し、`zwl_dispatch` で `zwl_gpu_request` に送る | linux-dmabuf の params の object | p007 |
| `struct zwl_object` に OS の module の記録の pointer（`void *gpu_private`）と、`object_free` の中で OS の module を呼ぶ `zwl_gpu_object_free(object)`（zedBSD の module は何もしない） | dma-buf の fd を buffer に残すため | p007 |
| `userland/desktop/mview/renderer.c`: `#include <stdio.h>` | glibc では他の header から来ない | p007 |
| `userland/base/libpdf/font.c` の `convert_contours()`: `control[0] = 0.0; control[1] = 0.0;` を loop の前に | gcc の `-Wmaybe-uninitialized`（誤検出） | p008 |
| `userland/desktop/keiland/keiui.h:73` の `KUI_TEXT_EMOJI` と `libkeiui/text.c:595`: emoji の font の path を `paths.h` から（公開の header は repo の `paths.h` を include できないので、`text.c` の中で `KEILAND_DATADIR "/fonts/keiland-emoji.ttf"` を使い、header の macro はそのまま残す） | WS104 p007 で残した物（公開の header の path） | p008 |
| `userland/desktop/files/apps.c:111` の `apps_program_folders[]`（Open With の program を探す directory）に `KEILAND_BINDIR` を先頭に足す | Linux では program が `/opt/keiland/bin` にある（WS104 p007 で残した物） | p008 |
| `struct zwl_server` の `os_paused` と、`zwl_schedule` の paused の扱い、resume の swapchain の作り直し、paused の間の入力の再走査の停止 | logind の pause | p009 |
| gcc だけが出す警告の、振る舞いを変えない直し（上に無い物が出たとき） | gcc 14 と clang 19 の差 | 出た Phase。直した file と警告を結果に書く |

**それ以外の共通の file を変える必要が出たら、Phase を止めて main に報告する**（WS104 の境界の欠けなので、WS104 の考え方に沿って直すかを main が決める）。
zedBSD の側の module（`zedbsd/`）に要る小さな追加（`zwl_gpu_object_free` の空の実装など）は、その Phase で一緒に行い、zedBSD の回帰（§9.2）を流す。

## 6. libkeiland の Linux の backend

WS104 の p003 で、libkeiland の OS の部分は `libkeiland/zedbsd/` に移してある（`network-zedbsd.c`・`network-link-zedbsd.c`・`audio-zedbsd.c`）。Linux では同じ関数（`keiland.h` の `keiland_network_*`・`keiland_audio_*`）を
別の source で実装する。**`keiland.h` の公開の API と意味は変えない。** app（zdesktop の system bar・Settings）は何も変えずに Linux で動く。

### 6.1 WiFi（`libkeiland/wpa/network-wpa.c`、FreeBSD と共有できる形）

- 相手: wpa_supplicant の制御の socket（`/run/wpa_supplicant/<ifname>`、`ctrl_interface=DIR=/run/wpa_supplicant GROUP=netdev`）。
  protocol は 1 つの datagram の text の command と応答（wpa_supplicant 2.10 の source で確かめた、2026-10-01）:
  - `PING` → `PONG\n`。成功は `OK\n`、失敗は `FAIL\n`、知らない command は `UNKNOWN COMMAND\n`。
  - `STATUS` → `key=value` の行（`bssid=`・`freq=`・`ssid=`・`id=`・`mode=`・…・`wpa_state=`・`ip_address=`・`address=`）。`wpa_state` は
    `DISCONNECTED`・`INACTIVE`・`INTERFACE_DISABLED`・`SCANNING`・`AUTHENTICATING`・`ASSOCIATING`・`ASSOCIATED`・`4WAY_HANDSHAKE`・`GROUP_HANDSHAKE`・`COMPLETED`・`UNKNOWN`。
  - `SCAN` → `OK`、scan 中は `FAIL-BUSY`、interface が無効なら `FAIL`。
  - `SCAN_RESULTS` → 見出しの行 `bssid / frequency / signal level / flags / ssid` の後に `MAC\tfreq\tlevel\tflags\tssid` の行。応答の buffer は 4096 byte（長い一覧は切れる）。
  - `LIST_NETWORKS` → 見出し `network id / ssid / bssid / flags` の後に `id\tssid\tany|MAC\t[CURRENT][DISABLED][TEMP-DISABLED]`。
  - `ADD_NETWORK` → `N\n`（新しい network は無効の状態）。`SET_NETWORK N ssid <値>`: 値は `"text"`、`P"printf の escape"`、引用無しの 16 進のどれか。**16 進を使う**（ssid の
    文字の escape の誤りを避ける）。`SET_NETWORK N psk "<8〜63 文字>"`（64 桁の 16 進も可）。
  - `ATTACH` → `OK`。以後その socket に `<level>` の前置きの event: `<3>CTRL-EVENT-CONNECTED - Connection to MAC completed [id=N id_str=]`、
    `<3>CTRL-EVENT-DISCONNECTED bssid=MAC reason=N[ locally_generated=1]`、`CTRL-EVENT-SCAN-STARTED `、`CTRL-EVENT-SCAN-RESULTS `。
    （`CTRL-EVENT-STATE-CHANGE` は Android の build だけ。状態の変化は event の後に `STATUS` を問い直して知る。）
  client の側の socket は `/tmp` などに自分で `bind` した unix の datagram の socket（wpa_supplicant の `wpa_ctrl.c` と同じ形。この file は BSD license だが、
  **複写せずに自分で書く**）。
- `keiland_network_open`: `/run/wpa_supplicant/` の中の最初の socket（`p2p-` で始まる物を除く）に接続し、`ATTACH` する。無ければ
  `reachable = 0`（今の zedBSD の「daemon が居ない」と同じ）で、update が 1 秒ごとに試す。
- 状態の対応:

  | `STATUS` の `wpa_state` | `KEILAND_WIFI_*` |
  | --- | --- |
  | `INTERFACE_DISABLED` | `OFF` |
  | `SCANNING` | `SEARCHING` |
  | `AUTHENTICATING`・`ASSOCIATING`・`ASSOCIATED`・`4WAY_HANDSHAKE`・`GROUP_HANDSHAKE` | `CONNECTING` |
  | `COMPLETED` | `CONNECTED` |
  | `DISCONNECTED`・`INACTIVE` | `DISCONNECTED` |
  | socket が無い | `ABSENT` |

- request の対応: `SCAN` → `SCAN`（結果は `CTRL-EVENT-SCAN-RESULTS` の後に `SCAN_RESULTS`）、`JOIN ssid` → `LIST_NETWORKS` で ssid の番号を探して
  `ENABLE_NETWORK n` と `SELECT_NETWORK n`（無ければ ENOENT。鍵の保存は §6.2 で `ADD_NETWORK` 済みの前提。**`SELECT_NETWORK` は他の全ての network を無効にし、
  `SAVE_CONFIG` はその `disabled=1` を保存する**ので、JOIN の後に `SAVE_CONFIG` しない）、`DISCONNECT` → `DISCONNECT`、`WIFI_OFF` → `DISCONNECT` の後
  interface を down（`SIOCSIFFLAGS`、権限が無ければ EPERM）、`WIFI_ON` → interface を up して `RECONNECT`（`RECONNECT` は `DISCONNECT` の後でだけ効く）、
  `PROFILES` → 何もしない（成功）。
- `SCAN_RESULTS` の行: `signal level` を `rssi`（dBm）、`flags` に `WPA`（`[WPA-PSK-…]`・`[WPA2-PSK-CCMP]` など）か `WEP` か `SAE` があれば `secured = 1`
  （`RSN` は mesh の時だけ出る）。同じ ssid は強い方だけ。ssid の escape を戻す: `\"`・`\\`・`\e`・`\n`・`\r`・`\t`・`\xNN`（32〜126 の外の byte、UTF-8 の全てを含む）。
- `connected`・`kind`・`interface`・`wired` は §6.2 の interface の情報から作る。

### 6.2 interface・DNS・鍵（`libkeiland/linux/network-link-linux.c`）

- `keiland_network_get_links`: `getifaddrs`（address、MAC は `AF_PACKET` の `sockaddr_ll`）と `/sys/class/net/<if>/statistics/{rx,tx}_bytes`。
  `/sys/class/net/<if>/wireless` か `phy80211` があれば WiFi。
- `keiland_network_get_dns`: `/etc/resolv.conf` の `nameserver`（zedBSD と同じ。zedBSD の source の読み方の部分は同じでよいが file を分ける）。
- `keiland_network_save_key(ssid, key)`: wpa_supplicant に `ADD_NETWORK` → `SET_NETWORK n ssid <ssid の 16 進>` → `SET_NETWORK n psk "<key>"` →
  `SAVE_CONFIG`（`update_config=1` のときだけ効く。失敗しても wpa_supplicant の memory に残る）。**`ENABLE_NETWORK` はしない**（wpa_supplicant が手が空いていると
  有効にした network に接続を始める。接続は JOIN の request で行う）。既に同じ ssid があれば `SET_NETWORK` で鍵だけ変える。
- `keiland_network_get_saved`: `LIST_NETWORKS` の ssid。

### 6.3 音（`libkeiland/linux/audio-linux.c`）

- 相手: ALSA の kernel の control の interface `/dev/snd/controlC<N>`（`<sound/asound.h>`、linux-libc-dev の header）。**alsa-lib に link しない**（D15）。
- 使う ioctl: `SNDRV_CTL_IOCTL_CARD_INFO`、`SNDRV_CTL_IOCTL_ELEM_LIST`、`SNDRV_CTL_IOCTL_ELEM_INFO`、`SNDRV_CTL_IOCTL_ELEM_READ`、`SNDRV_CTL_IOCTL_ELEM_WRITE`、
  `SNDRV_CTL_IOCTL_SUBSCRIBE_EVENTS`（他の process の変更を `read` の event で知る）。
- 音量の element: 名前が `Master Playback Volume`（無ければ `PCM Playback Volume`、`Speaker Playback Volume`）の INTEGER。mute は `Master Playback Switch`（BOOLEAN、1 が音あり）。
  左右 2 channel。範囲は `SNDRV_CTL_IOCTL_ELEM_INFO` の `value.integer.min`・`max` で読む（QEMU の `intel-hda` + `hda-duplex` の codec「QEMU Generic」は
  `Master Playback Volume`・`Master Playback Switch`・`Capture Volume`・`Capture Switch` を持ち、範囲は 0〜74 と推定、2026-10-01 の survey）。`keiland_audio_state` の音量（0〜100）に線形で写す。
- struct の大きさ（amd64、`<sound/asound.h>` 6.12）: `snd_ctl_elem_id` 64・`snd_ctl_elem_list` 80・`snd_ctl_elem_info` 272・`snd_ctl_elem_value` 1224（整数の値は `long`）・
  `snd_ctl_event` 72 byte。event は `read()` で `struct snd_ctl_event{int type; data.elem{unsigned mask; struct snd_ctl_elem_id id;}}`、mask は VALUE 1・INFO 2・ADD 4・TLV 8・REMOVE ~0。
  `SNDRV_CTL_IOCTL_SUBSCRIBE_EVENTS` の引数は int（1 で購読）。手本: `plan/ws105/survey/asnd.c`。
- `keiland_audio_fd`: control の fd（event が来ると readable）。
- `keiland_audio_feedback`（確かめの音）: PCM の再生が要る（`/dev/snd/pcmC<N>D<M>p`、`SNDRV_PCM_IOCTL_*`）。**WS105 では作らず、0 を返して鳴らさない**
  （`keiland.h` の約束「an audiod without it stays silent」の範囲。§8）。
- `keiland_audio_available`（WS104 p002 で足した関数）: `/dev/snd/controlC*` のどれかが開けて、音量の element があれば 1。
- `reachable` は control の device が開けている間 1、`device` は音量の element があれば 1、`rate`・`channels` は 0（不明）と 2。

## 7. 試験の環境

### 7.1 二つの場所

| 場所 | 何を試すか | 理由 |
| --- | --- | --- |
| **host**（この開発機。Debian 13、kernel 6.12、Mesa 25.0.7 の lavapipe、Khronos の loader 1.4.309） | build、libvulkan-compat の chain と Wayland の WSI（試験用の Wayland server `dmabuf-probe` を相手に）、host の単体の試験 | 速い。host の GPU は Matrox（3D 無し）なので Vulkan は lavapipe（software）だけ。lavapipe は `VK_EXT_external_memory_dma_buf`・`VK_EXT_image_drm_format_modifier`・`VK_KHR_external_semaphore_fd`・`VK_KHR_external_fence_fd` を持つ（2026-10-01 確かめ）。**lavapipe の dma-buf は `/dev/udmabuf`（`root:kvm 0660`）を要る**（kvm の group が無いと dma-buf の export・modifier・SYNC_FD が消える、2026-10-01 確かめ）。利用者 awe は kvm に入っている |
| **Linux の guest**（QEMU の中の Debian 13。`plan/tools/keiland-linux/` で作る、p001） | compositor（KMS・evdev・seat）、app、gdm、WiFi（`mac80211_hwsim`）、音（QEMU の `intel-hda`） | host の画面と入力を触らない。root で DRM master を取ってよい。gdm を入れてよい |

- **host の画面（`/dev/dri/card0`・`card1`、Matrox の console）と host の入力の device を Keiland の試験で開かない。** host での試験の command は全て
  `KEILAND_DRM_DEVICE=none` を付ける（§4.9）。`vulkaninfo` などは `env -u WAYLAND_DISPLAY -u DISPLAY XDG_RUNTIME_DIR=<空の directory>` で走らせる（§4.11）。
  （host の `/dev/dri/renderD128` は vgem で、zedBSD の Venus の試験の QEMU が使う。Linux の guest の試験では使わない。）
- zedBSD の試験の規則（`plan/tools/boot-test.sh` だけで起動を確かめる、serial の log で判定しない）は zedBSD の image の物。Linux の guest の起動の判定は
  SSH が通ることと QMP の `screendump` の PNG で行う（D25。boot-test.sh は zedBSD の image の UEFI の起動の物で、Linux の guest には使えない）。
  guest の serial の console の log は残すが判定に使わない。

### 7.2 Linux の guest（p001 で作る。手本は `plan/ws105/survey/build-guest.sh`、2026-10-01 に通った物）

**image の作り方**（root は要らない。host の `/etc/subuid` に `awe:100000:65536` がある）:

- `mmdebstrap --mode=unshare --variant=important --include=<package> <hook...> trixie $OUT/rootfs.tar http://deb.debian.org/debian`
  （`deb.debian.org` は host から届く、2026-10-01 確かめ。60 秒ほど）。
- `LC_ALL=C.UTF-8` を付ける（host の `ja_JP` の locale は生成されていないので、無いと perl が警告する）。`PATH` に `/usr/sbin:/sbin` を足す（`mkfs.ext4` が awe の PATH に無い）。
- package（base）: `linux-image-amd64,systemd-sysv,udev,dbus,libpam-systemd,openssh-server,sudo,kmod,iproute2,mesa-vulkan-drivers,libvulkan1,vulkan-tools,weston,wpasupplicant,hostapd,iw,alsa-utils,kbd,rsync`。
  **`dbus` と `libpam-systemd` は必ず入れる**（mmdebstrap は Recommends を入れないので、無いと system bus が無く、logind の D-Bus も SSH の session の `XDG_RUNTIME_DIR` も無い）。
  `gdm` の variant では `gdm3` を足す（`gnome-session` などが入り重い）。
- hook（`--customize-hook`。外の file は `upload OUTSIDE INSIDE` で入れる。hook の中の `cp` は awe の 0700 の directory を読めない。`upload` は所有と mode を落とすので後で直す）:
  hostname `keiland-guest`、`/etc/hosts`、`/etc/resolv.conf`（`nameserver 10.0.2.3`）、`/etc/fstab`（`/dev/vda / ext4 defaults 0 1`）、systemd-networkd の DHCP
  （`/etc/systemd/network/20-wired.network`: `[Match] Name=en*`、`[Network] DHCP=yes`、`systemctl enable systemd-networkd`）、**`systemctl disable wpa_supplicant.service`**
  （Debian の unit は `RuntimeDirectory=wpa_supplicant` で、止めると `/run/wpa_supplicant` を消し、p010 の試験の wpa_supplicant の socket も消える）、root の password を消す、
  `useradd --uid 1000 --groups video,input,audio,render,netdev,kvm kei`（**`kvm` は lavapipe の `/dev/udmabuf` のため**、§7.1）、password `kei`、両者の `authorized_keys`、
  `PermitRootLogin prohibit-password`、`download /vmlinuz $OUT/vmlinuz`・`download /initrd.img $OUT/initrd.img`（Debian の initramfs は virtio_blk と ext4 を持つ）。
- disk: `truncate -s 8G $OUT/guest.img.new && mkfs.ext4 -q -F -L keiland-root -d $OUT/rootfs.tar $OUT/guest.img.new && mv $OUT/guest.img.new $OUT/guest.img`
  （`mkfs.ext4 -d` は tar を直接読み、tar の uid・gid を保つ。directory に展開してから `-d` すると unshare の 100000 台の uid になる。mmdebstrap の `.ext4` の出力は大きさが足りない）。

**起動**（QEMU 10.0.11 の option を 2026-10-01 に全て確かめた。KVM で SSH は 9 秒で通る）:

```
qemu-system-x86_64 -machine q35,vmport=off -accel kvm -cpu host -m 4G -smp 4 -display none \
  -kernel $GUEST_DIR/vmlinuz -initrd $GUEST_DIR/initrd.img -append "root=/dev/vda rw console=ttyS0 quiet" \
  -drive file=$GUEST_RUN/overlay.qcow2,format=qcow2,if=virtio \
  -device virtio-vga,id=video0 -device virtio-keyboard-pci,display=video0 -device virtio-tablet-pci,display=video0 \
  -audiodev none,id=snd0 -device intel-hda -device hda-duplex,audiodev=snd0 \
  -netdev user,id=n0,hostfwd=tcp:127.0.0.1:$SSH_PORT-:22 -device virtio-net-pci,netdev=n0 \
  -qmp unix:$GUEST_RUN/qmp.sock,server=on,wait=off -serial file:$GUEST_RUN/serial.log -pidfile $GUEST_RUN/qemu.pid -daemonize
```

- **`overlay.qcow2`**: 起動のたびに `qemu-img create -f qcow2 -b $GUEST_IMAGE -F raw $GUEST_RUN/overlay.qcow2` で作り直す（共有の `guest.img` は読むだけ。2 つの QEMU が同じ raw の
  image を書くと壊れるか lock で失敗する。subagent の worktree からも main の checkout の image を読むだけで使える）。
- `-machine q35,vmport=off`: 既定の `pc` は floppy を探して起動が 34 秒遅れ、`vmport` の vmmouse が絶対座標の event を取る。
- `virtio-mouse-pci` は入れない（入れると button の event が tablet でなく mouse に行く）。keyboard と tablet は `display=video0` に結び、QMP の event は全て
  `"device":"video0","head":0` を付けて送る（key は QEMU Virtio Keyboard、絶対座標と button は QEMU Virtio Tablet に届く、確かめ済み）。
- 画面の既定の大きさは 1280×800（virtio-vga の既定）。
- `console=tty0` にしない（kernel の message が login の画面に出る）。
- `poweroff`（SSH）で QEMU は 3 秒ほどで終わり、pidfile を消す。
- **Venus（`virtio-vga-gl,venus=true`）は使わない**: `egl-headless` は host の render node（`/dev/dri/renderD128`）を開く必要があり、§7.1 の規則に反する。guest の Vulkan は guest の lavapipe。

**QMP**（`plan/tools/qmp.py SOCK COMMAND ARGS_JSON`）:

- screenshot: `qmp.py $GUEST_RUN/qmp.sock screendump '{"filename":"/ABS/PATH.png","format":"png"}'`（**path は絶対 path**。daemon の QEMU は `/` に cd しているので相対 path は失敗する）。RGB の PNG。
- key: `qmp.py SOCK input-send-event '{"device":"video0","head":0,"events":[{"type":"key","data":{"down":true,"key":{"type":"qcode","data":"meta_l"}}}]}'`（離すときは `"down":false`）。
  QKeyCode の名前: `meta_l`（super。`super` は無い）、`ctrl`、`alt`、`shift`、`ret`、`spc`、`esc`、`tab`、`f1`〜、`a`〜`z`、`1`〜`0`。
- pointer: `{"type":"abs","data":{"axis":"x","value":0..32767}}`・`{"type":"abs","data":{"axis":"y","value":...}}`、button `{"type":"btn","data":{"down":true,"button":"left"}}`。
  画素の座標 → 値: `x * 32767 / (幅 - 1)`。
- 終わり: `qmp.py SOCK quit`。

**VT と起動**: root で SSH から `openvt -c 7 -s -- sh -c "<command> > /tmp/<log> 2>&1"`（**redirect は `sh -c` の中に書く**。外に書くと openvt の出力になり、子の出力は VT に行く）。
process は SSH が切れても残る。`chvt N` で VT を切り替える。

**gdm**（`gdm` の variant、p009）:

- `/etc/gdm3/daemon.conf` の `[daemon]`: `AutomaticLoginEnable=true`・`AutomaticLogin=kei`（`WaylandEnable` の既定は true）。`DefaultSession` の key は無い。
- 利用者の session の選択は AccountsService: 起動した guest で
  `busctl call org.freedesktop.Accounts /org/freedesktop/Accounts/User1000 org.freedesktop.Accounts.User SetSession s keiland` と `... SetSessionType s wayland`
  （file `/var/lib/AccountsService/users/kei` の `[User]`・`Session=keiland`・`SessionType=wayland` でもよいが、daemon が覚えているので書いた後に `systemctl restart accounts-daemon`）。
  これをしないと自動 login は GNOME を起動する。

### 7.3 host の試験の道具（`plan/tools/keiland-linux/`、p003・p004 で作る）

**全ての command に `KEILAND_DRM_DEVICE=none` と、Wayland・X の環境変数の除去を付ける**（§7.1）。link の command は `-Wl,-rpath-link,build/keiland-linux/lib` を付ける（§3.2）。

- `vk-chain-test.c`: libvulkan-compat を通して instance・device を作り、transfer で buffer を埋め、結果を読む。後段の名前を出す。我々の library に **DT_NEEDED で** link する（§4.11）。
  ```
  cc -o build/keiland-linux/test/vk-chain-test plan/tools/keiland-linux/vk-chain-test.c -ldl \
     -Wl,--no-as-needed -Lbuild/keiland-linux/lib -l:libvulkan.so.1 -Wl,-rpath-link,build/keiland-linux/lib -Wl,-rpath,/opt/keiland/lib
  env -u WAYLAND_DISPLAY -u DISPLAY KEILAND_DRM_DEVICE=none LD_LIBRARY_PATH=build/keiland-linux/stage/opt/keiland/lib build/keiland-linux/test/vk-chain-test
  ```
- `dmabuf-probe`（試験用の Wayland server、**host の libwayland-server と wayland-scanner を使う。試験の道具だけで、出荷しない**。手本 `plan/ws105/survey/dmabuf-probe.c`）:
  - global: `wl_compositor`（v4）、`zwp_linux_dmabuf_v1`（v3。`format` と `modifier` の event で `ARGB8888`・`XRGB8888` の `LINEAR` を告げる）。
  - commit された dma-buf の buffer ごとに: plane の fd に `DMA_BUF_IOCTL_EXPORT_SYNC_FILE(READ)` をかけ、得た sync_file を `SYNC_IOC_FILE_INFO`（`<linux/sync_file.h>`）で読んで
    fence の数を数え、`poll` で待ち、`mmap`（LINEAR だけ）して中央の画素を読み、`PROBE frame=N pixel=0xAARRGGBB fences=F waited_ms=M` を stdout に出し、`wl_buffer.release` を送る。
  - `--frames N` で N frame 受けたら終わる。`--timeout S`（既定 60 秒）で何も来なければ `PROBE TIMEOUT` で終わる。
  - build（2026-10-01 に通った command）:
    ```
    X=$(pkg-config --variable=pkgdatadir wayland-protocols)/stable/linux-dmabuf/linux-dmabuf-v1.xml
    SC=$(pkg-config --variable=wayland_scanner wayland-scanner)
    mkdir -p build/keiland-linux/test/gen
    $SC server-header $X build/keiland-linux/test/gen/linux-dmabuf-v1-server-protocol.h
    $SC private-code  $X build/keiland-linux/test/gen/linux-dmabuf-v1-protocol.c
    cc -std=c11 -Wall -Wextra -Werror -O2 -Ibuild/keiland-linux/test/gen $(pkg-config --cflags wayland-server) -o build/keiland-linux/test/dmabuf-probe \
       plan/tools/keiland-linux/dmabuf-probe.c build/keiland-linux/test/gen/linux-dmabuf-v1-protocol.c $(pkg-config --libs wayland-server)
    ```
- `wsi-probe-client.c`: libvulkan-compat の Wayland の WSI で、frame ごとに決まった色（frame 番号で赤・緑・青を巡る）で clear して present する client。
  我々の libwayland-client に link する（**system の `wayland-client.h` を使わない**: `-Iuserland/desktop/keiland` を先に置き、`pkg-config wayland-client` を使わない）。xdg-shell は使わない。
- `interpose-check.sh`: §4.11 の確かめ (a)〜(c)。行の形と grep:
  ```
  env -u WAYLAND_DISPLAY -u DISPLAY KEILAND_DRM_DEVICE=none LD_LIBRARY_PATH=$STAGE LD_DEBUG=bindings LD_DEBUG_OUTPUT=$OUT/ld build/keiland-linux/test/vk-chain-test
  cat $OUT/ld.* | grep -c "binding file /usr/lib/x86_64-linux-gnu/libvulkan.so.1 \[0\] to .*/opt/keiland/lib/libvulkan.so.1 \[0\]: normal symbol \`vk"    # 0 であること
  ```
- 全ての試験の command は `timeout`（例 `timeout 120`）で上限を付ける。

## 8. 範囲の外（WS105 では作らない。Future Work か後の WS）

- FreeBSD（F-065 に残す）。`wpa/` の module は FreeBSD でも使える形にしておくだけ。
- browser（libbrowser）・xserver・EGL と GLES（libegl・libglesv2・egltest・glescompute・gpudemo）の Linux の build。
- libvulkan-compat の、複写しない画面の出力（dumb buffer の dma-buf の import、image の dma-buf の `ADDFB2`）と、dma-buf の無い後段のための wl_shm の予備の道（§4.8）。
- explicit sync（`linux-drm-syncobj-v1`）と、sync_file の ioctl の無い Linux 6.0 より前の kernel での試験（予備の道の code は作るが、試験は 6.12 だけ）。
- 確かめの音（`keiland_audio_feedback` の PCM の再生）。Linux では 0 を返して鳴らさない（`keiland.h` の約束「an audiod without it stays silent」の範囲）。
- amd64 の外の Linux（compositor の `zwl_cycles` が `rdtsc` を使う。組み込みの ARM へは後の WS）。
- 実機の Linux（WS105 の証拠は host と QEMU の guest だけ）。
- Keiland の外の app（system の libwayland を使う app）の libvulkan-compat での動作（§4.11）。
- sessiond・greeter・lock 画面の Linux の版、session の中の Shut Down。
- 互換の Qt6・GTK4（WS096・WS097）、XDG の規則、Linux の distribution の package（`.deb` など）。
- glibc でない C library（musl など。`RTLD_DEEPBIND` が無い）。
- device group の swapchain（`VkImageSwapchainCreateInfoKHR`・`VkBindImageMemorySwapchainInfoKHR` を app が使う場合）、`VK_KHR_get_surface_capabilities2` などの WSI の拡張。
- compositor の側の `VK_QUEUE_FAMILY_FOREIGN_EXT` からの所有の受け取り（LINEAR の image では要らない。tiled・圧縮の modifier の GPU で要る）。
- `zwp_linux_dmabuf_v1` の version 4 以上（feedback）。我々の client は 3 で bind し、我々の compositor は 3 を出す。
- VT の切り替えの key（Ctrl+Alt+Fn → logind の `Seat.SwitchTo`）。
- Linux の guest の Venus（host の render node を開く必要がある）。

## 9. 確かめの約束

### 9.1 Linux の側

- build: `make keiland-linux`（既定の `cc` = Debian の gcc 14）と `make keiland-linux CC=clang KEILAND_LINUX_BUILD=build/keiland-linux-clang` の両方で warning 0（`-Werror`）。
- install: `make keiland-linux-install DESTDIR=$PWD/build/keiland-linux/stage`。`readelf -d` で全ての ELF に `RUNPATH [/opt/keiland/lib]`、SONAME が §2 のとおり、
  `NEEDED` に我々の library が SONAME で並ぶこと（`plan/tools/keiland-linux/elf-check.sh`、p002 で作る）。
- host の試験と guest の試験は §7。**判定は数（PROBE の行、終了の code）と PNG（`png-probe.py` の画素）で行い、目で見ただけで PASS にしない。**
  PNG はユーザーに見せる（AGENTS.md の「撮れた PNG はユーザーに見せる」に合わせる）。

### 9.2 zedBSD の側（共通の file を変えた Phase で必ず）

WS105 は共通の source（compositor の `protocol.c`・`main.c`・`display.c`、libkeiland の共通の file など）を少し変える（§5.6）。変えた Phase では zedBSD の回帰を流す:

- command は [WS104 の commands.md](../ws104/commands.md) のとおり（build は `make -j64 disk-image` と warning の数え、boot test は専用の `OUTPUT`）。
- `plan/tools/gpu-boundary/v1-check.sh`、`criteria.sh ... C1 C2 C9`（compositor を変えたとき。commands.md の §5・§6）。
- `plan/tools/keiland-os-boundary/check.sh`（WS104 p008 の物）。
- boot test。

## 10. 作業の中で確かめる点（未確認の前提）

2026-10-01 の survey で確かめた物は「確かめ済み」と書いた。

| # | 前提 | どこで確かめるか | 外れたら |
| --- | --- | --- | --- |
| V1 | 後段を `RTLD_DEEPBIND` で開くと、後段の中の参照が我々の `vk*` に結び付かない（確かめ済み: 0 個。DEEPBIND 無しでは 215 個、§4.11） | p003 の `interpose-check.sh`（他の後段・新しい loader の版で崩れていないか） | main に報告 |
| V2 | 同じ SONAME `libvulkan.so.1` の我々の物と後段が 1 つの process に別々に載る（確かめ済み） | p003 の `vk-chain-test` | main に報告 |
| V3 | lavapipe が dma-buf の image を LINEAR の modifier で export・import でき、SYNC_FD の semaphore・fence を export・import できる（確かめ済み、host。kvm の group が要る） | p004（host）、p007（guest の kei の利用者） | WSI の道が「無し」になる。guest の kvm の group を確かめる |
| V4 | guest の virtio-gpu の KMS で dumb buffer の page flip が動き、QMP の `screendump` に出る | p005 | `SETCRTC` だけで出す道（flip の event 無し）に落とす |
| V5 | lavapipe が別の process の lavapipe が export した dma-buf（udmabuf）を import できる（確かめ済み、host） | p007（guest） | main に報告 |
| V6 | gdm（Debian 13、gdm3 48）が `keiland.desktop` の session を logind の session として起動し、`XDG_SESSION_ID`・`XDG_SESSION_TYPE=wayland` が届き、`TakeControl` が通る | p009 | gdm の版の差を調べて main に報告 |
| V7 | `mac80211_hwsim` と hostapd で guest の中に WiFi の AP と client ができ、wpa_supplicant の制御 socket で scan・接続できる（`mac80211_hwsim` の module は確かめ済み） | p010 | network の試験を「wpa_supplicant の制御 socket の偽物の server」で行う形に落とす |
| V8 | 我々の compositor・client が `PRESENT_SRC_KHR` と所有の移り（FOREIGN）を後段の都合どおりに扱わなくても、LINEAR の image では正しく表示される | p007 | tiled の modifier の GPU（実機）では要る。§8 |
| V9 | implicit sync の fence が dma-buf に付く（IMPORT_SYNC_FILE flags=WRITE の成功を試験 observer で記録、実画素と照合。raw `fences` は空/完了時も kernel stub=1 になる）。lavapipe は CPU で完了まで待つので、実際に待つことは見えない | p004 | IMPORT_SYNC_FILE の失敗・CPU fallback の実行と kernel の版を記録。raw fence 数だけを成功の根拠にしない |
| V10 | 我々の compat の library（`libz-compat.so` など）が版無しで export する名前（`crc32` など）に、`KEILAND_VULKAN_NO_DEEPBIND=1` の時に Mesa・LLVM が結び付いても壊れない（zlib と ABI が同じ） | p008（Image Viewer などで compat の library と Vulkan が同じ process に居る状態で app が動くこと） | DEEPBIND の既定のままなら起きない。外す場合の注意として記録 |
| V11 | `logind` の pause で DRM の master が外され evdev が revoke され、resume で新しい fd が来る（systemd の source の知識。未確かめ） | p009 | 実際の動きに合わせて §5.5 を直す（main に報告） |

### p003 の実装時の確認（q525）

V1・V2 verified: 既定の backend → compat binding 0、opt-out の chain も PASS、同じ SONAME の別 backend で 1 MiB fill / copy / fence と全 word 一致。明示した backend の壊れた指定は authoritative として診断・失敗し、既定候補で隠さない（Phase の自己参照 / missing-backend の検証を保持）。pthread key の thread-local record によって必要な lifetime / 再入検出を保ち、NEEDED は glibc runtime の libc.so.6 だけ。fake backend の直接再帰は gcc が local alias にしたため、extern assembler alias から vkCreateInstance@PLT を呼ぶ試験に直し、診断と exit134 を確認。詳細は [p003](phase003/phase.md)。

### p004 検証の改訂（q526 後、2026-10-01）

[Phase 改訂](phase004/phase.md): kernel 6.12 の export は空の reservation に stub を補い、lavapipe の完了済み payload も stub と観測された。raw fences≥1 / fallback=0 の区別は成立しない。raw 値を保持し、試験 observer の IMPORT_SYNC_FILE の成功/ENOTTY と CPU fence wait、全 frame 実画素を受け入れの根拠とする。D6 の production API/同期方式に変更無し。実機 GPU の非同期待ちは従来通り未実施。
