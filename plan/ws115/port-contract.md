<!-- awesome-plan project=zedbsd record=ws115-port-contract -->

# WS115 素の GTK4 の zedBSD 移植の契約（ws115-p001 / q597）

作成 2026-10-02、P3（[ws115-p001](phase001/phase.md)、Queue q597-i01）。調査と契約だけで、package・libc・libwayland の source は変えていない。
tarball はすべて P3 の worktree の `build/p3-q597/dist/`（ignored）に取って測った。表の size と SHA-256 はその実測値。版の候補の比較には upstream の `meson.build` を読み、glib については zedBSD target 向けに **meson の dry 構成**も試した（§3.2）。
`[判断]` の印はユーザーが決める点で、全部を §8 にまとめる。

## 1. 版（D-VER）

### 1.1 推奨: GTK 4.18.6

| 版 | tarball（GNOME） | size | SHA-256（公開 `.sha256sum` と一致） | meson | glib | pango | wayland-client | wayland-protocols |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| **4.18.6（推奨）** | `https://download.gnome.org/sources/gtk/4.18/gtk-4.18.6.tar.xz` | 17710412 | `e1817c650ddc3261f9a8345b3b22a26a5d80af154630dedc03cc7becefffd0fa` | ≥ 1.2.0 | ≥ 2.80 | ≥ 1.56 | ≥ 1.23.0 | ≥ 1.41 |
| 4.20.3 | `…/gtk/4.20/gtk-4.20.3.tar.xz` | 16003020 | `2873f2903088a66c71173ea2ed85ffae266a66b972c3a4842bbb2f6f187ec153` | ≥ 1.5.0 | ≥ 2.82 | ≥ 1.56 | ≥ 1.24.0 | ≥ 1.44 |
| 4.22.5 | `…/gtk/4.22/gtk-4.22.5.tar.xz` | 17007008 | `7fd725deb2cb3f8dc218ad862c5056ff8548f49d3b0e4081796e444c22d19686` | ≥ 1.5.0 | ≥ 2.84 | ≥ 1.56 | ≥ 1.24.0 | ≥ 1.44 |
| 4.24.1（最新安定版） | `…/gtk/4.24/gtk-4.24.1.tar.xz` | 17655236 | `e98abe720e16129c8c0f50761dee0a2e9ae2478055e31018f6cf977fcf6513e9` | **≥ 1.8.0** | **≥ 2.89.3** | ≥ 1.58 | ≥ 1.24.0 | ≥ 1.48 |

4.18.6 を推す理由:
1. WS114 で Linux（Debian 13）で実測したのが同じ 4.18.6。zedBSD と Linux の差を、版の差なしに比べられる（WS115 acceptance 3）。
2. host の meson 1.7.0 で足りる（4.24 は 1.8 が要る）。
3. glib を host の道具と同じ 2.84.4 にでき、host 用の glib を別に build しなくてよい（§4）。
4. 要る wayland-client の版が 1.23.0 で、zedBSD の libwayland の ABI の基準（wayland 1.23.1、[API-PROVENANCE](../../userland/desktop/keiland/wayland/API-PROVENANCE.md)）と一致する。4.20 以降は 1.24 を要求する。

4.24 との差（`[判断]` D-VER）:
- 4.24.1 は最新安定版で、上流の修正が入っている。ただし meson ≥ 1.8 と glib ≥ 2.89.3（2.90 系）が要る。
- meson は inventory の meson 1.12.0 を python から直接動かせば済む（build は要らない。tarball は取得済み、SHA-256 `88afe0c2…`）。
- 一方 glib 2.90 だと、host の glib 道具（2.84.4）と版が合わない。同じ 2.90 の host 用 glib 道具を作る手間が増える。
- 4.18 系はもう更新されない（4.18.6 が最後）。移植で得た知見は WS097 に渡すためのもので、長く保守する版ではない。この前提で 4.18.6 を推す。
- 4.24 に移るときの差分は §1.2 の版、wayland-protocols ≥ 1.48、libwayland の 1.24 の機能（§5）。

### 1.2 依存の版（推奨の 4.18.6 の組）

| package | 版 | 入手元 | size | SHA-256 | license | build 系 | Phase | 備考 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| glib | **2.84.4** | `https://download.gnome.org/sources/glib/2.84/glib-2.84.4.tar.xz` | 5618200 | `8a9ea10943c36fc117e253f80c91e477b673525ae45762942858aef57631bb90`（公開値と一致） | LGPL-2.1-or-later（tree に LICENSES/ がある） | meson ≥ 1.4.0 | p005 | host の glib 道具 2.84.4 と同じ版。inventory の 2.90.0 は 4.24 用 |
| libffi | 3.8.0 | inventory §1.2 | 1581626 | `7da3e2d9…c0db4` | MIT | autotools＋libtool | p005 | 共有ライブラリを作るには libtool の patch が要る（inventory §3.2） |
| pcre2 | 10.48 | 同 | 2213024 | `b6c68fdf…c8ed` | BSD-3-Clause WITH PCRE2-exception | CMake | p005 | glib が要る |
| pango | **1.56.4** | `https://download.gnome.org/sources/pango/1.56/pango-1.56.4.tar.xz` | 1883988 | `17065e2fcc5f5a5bdbffc884c956bfc7c451a96e8c4fb2f8ad837c6413cb5a01`（公開値と一致） | LGPL（COPYING は Library GPL v2。SPDX 名は p007 で機械的に確定する） | meson ≥ 1.2.0 | p007 | glib ≥ 2.82、fontconfig ≥ 2.15、cairo ≥ 1.18。1.56 系の最後。1.58.2 でもよい |
| fontconfig | **2.17.1** | `https://gitlab.freedesktop.org/api/v4/projects/890/packages/generic/fontconfig/2.17.1/fontconfig-2.17.1.tar.xz` | 1326312 | `9f5cae93f4fffc1fbc05ae99cdfc708cd60dfd6612ffc0512827025c026fa541`（公開値と一致） | MIT 系（HPND-sell 相当） | meson ≥ 1.6.1 | p006 | inventory の 2.18.3 は meson ≥ 1.11 で、host の 1.7 では足りない。gperf が要る（§4） |
| freetype | 2.14.3 | inventory | 2670220 | `36bc4f1c…785a5f` | FTL OR GPL-2.0-or-later（FTL を選ぶ） | meson | p006 | 最初は harfbuzz なしで作る |
| harfbuzz | 14.5.0 | inventory | 20262956 | `b7132e14…92b9` | MIT | meson（C++） | p006 | glib 付き（pango のため）、`harfbuzz-subset` を作る（GTK に必須）。C++ の runtime は main の `devel/libcxx`（libc++.so.1）で、image に要る |
| libpng | 1.6.58 | inventory | 1070096 | `28eb403f…4775` | Libpng-2.0 | CMake | p006 | |
| pixman | 0.46.4 | inventory | 660536 | `a098c339…1239` | MIT | meson ≥ 1.3 | p007 | |
| cairo | 1.18.6 | inventory | 32725456 | `1c767308…57d4` | LGPL-2.1-only OR MPL-1.1 | meson ≥ 1.3 | p007 | `cairo-gobject` を有効、X11/XCB/quartz は無効 |
| fribidi | 1.0.17 | inventory | 1203284 | `6949dcde…bada2` | LGPL-2.1-or-later | meson | p007 | |
| gdk-pixbuf | 2.44.8 | inventory | 6037756 | `919f5295…a3c` | LGPL-2.1-or-later | meson ≥ 1.5 | p008 | glycin は無効（Linux 専用）。png/jpeg を builtin、`man=false`、thumbnailer なし |
| libjpeg-turbo | 3.2.0 | inventory | 2537858 | `6f30092c…878e` | IJG AND BSD-3-Clause AND Zlib | CMake | p008 | GTK が直接要る |
| libtiff | 4.7.2 | inventory | 2409652 | `4996f0c4…ddb88` | libtiff | CMake | p008 | GTK が必須にしている |
| graphene | 1.10.8 | inventory | 333924 | `a37bb0e7…279a` | MIT | meson | p008 | `graphene-gobject-1.0` |
| libepoxy | 1.5.10 | inventory | 223528 | `072cda4b…e623` | MIT | meson | p008 | GTK が常に要る。zedBSD の library の名前を dlopen させる patch が要る（§6） |
| libxkbcommon | 1.13.2（tag archive） | inventory | 1243485 | `acc4d5f7…5528` | MIT 系 | meson ≥ 1.4、bison ≥ 3.6 | p008 | xkbregistry・x11・wayland の道具は無効、version script も無効 |
| xkeyboard-config | 2.48 | inventory | 953316 | `b7704132…48fd5` | MIT 系 | meson | p008 | データ。GTK は起動時に既定の keymap を名前から作るので、image に要る（§7） |
| wayland-protocols | 1.49 | inventory | 150612 | `ec4c8f74…b14` | MIT | meson | p009 | XML だけ。GTK の meson の wayland module が `find_protocol` で使う |
| zlib・expat | 既存 package | — | — | — | Zlib・MIT | — | 済み | cairo/png/tiff と fontconfig が使う |

inventory の値の出典は [WS034 inventory §1.2](../ws034/package-inventory.md)。main の `build/distfiles` から複写して SHA-256 を測り直し、一致を確認した。
**本家 libwayland（wayland 1.26.0）は使わない。** epoll・timerfd・signalfd が要るうえ（inventory §2.6）、zedBSD には独自の `libwayland-client.so` がある。GTK はこれに足りない分を足して使う（§5、p009）。

### 1.3 無効にする option（GTK 4.18.6）

`-Dx11-backend=false -Dwayland-backend=true -Dbroadway-backend=false -Dmedia-gstreamer=disabled -Dprint-cups=disabled -Dprint-cpdb=disabled -Dcloudproviders=disabled -Dsysprof=disabled -Dtracker=disabled -Dcolord=disabled -Daccesskit=disabled -Dintrospection=disabled -Ddocumentation=false -Dman-pages=false -Dscreenshots=false -Dbuild-tests=false -Dbuild-testsuite=false`。`-Dvulkan` は §6 の判断に従う（推奨は最初 `disabled`）。demo と example は p010 で使うので `-Dbuild-demos=true -Dbuild-examples=false`。
- iso-codes は任意。
- tarball には生成済みの theme の CSS（`gtk/theme/Default/*.css`）が入っているので、sassc は要らない。
- cross build では GTK が objcopy による resource の埋め込みを使わず、C source を生成する（`can_use_objcopy_for_resources` は native の Linux だけ）。
- glib: `-Dtests=false -Dintrospection=disabled -Dman-pages=disabled -Ddocumentation=false -Dnls=disabled -Dselinux=disabled -Dlibmount=disabled -Dxattr=true`、`--wrap-mode=nodownload`（subproject の取得を禁止）。

## 2. 依存の一覧（必須と任意）

- 必須（GTK 4.18.6 の `meson.build` が `required` にしている物）: glib・gobject・gmodule・gio-unix（任意だが使う）、cairo・cairo-gobject、pango・pangocairo・pangoft2（Wayland では必須）、fribidi、harfbuzz・harfbuzz-subset、gdk-pixbuf、libpng、libtiff、libjpeg、libepoxy、xkbcommon、graphene-gobject、wayland-client（≥ 1.23）・wayland-protocols（≥ 1.41）・wayland-egl。
- 推移的に要る物: fontconfig（pango）、freetype、pixman、libffi・pcre2（glib）、zlib・expat。C++ の runtime（harfbuzz）。
- Linux だけで必須: libdrm。zedBSD では要らない（`required: os_linux`）。`linux/dma-buf.h` が無いので dmabuf の経路は build されない。
- 使わない物: X11 一式、broadway、gstreamer、cups/cpdb、cloudproviders、sysprof、tracker、colord、accesskit、introspection（cross では実行できない）、portal（D-Bus。WS114 p004 の判断と同じく、ベータ1では要らない）。

## 3. libc の不足と OS API

### 3.1 zedBSD libc にある物（`libc.so` の export と sysroot の header を調べた）

iconv・libintl（ngettext・bind_textdomain_codeset）・newlocale/uselocale/strtod_l・pthread（condattr_setclock・CLOCK_MONOTONIC）・posix_memalign/aligned_alloc・poll/ppoll・pipe2・accept4・shm_open/shm_unlink・mkostemp・posix_fallocate・getmntent/setmntent/hasmntopt（`mntent.h`）・statvfs・getxattr 系・getrandom・issetugid・closefrom・posix_spawn・dlopen/dladdr・qsort_r・memmem・strndup・clock_gettime・madvise・res_query/dn_expand・getaddrinfo。
rtld は `DT_INIT_ARRAY`（glib の constructor）、rpath/runpath、`LD_LIBRARY_PATH`、`/lib` の次に `/usr/lib` の探索、dlopen の動的 TLS を扱う（`src/rtld/rtld.c`）。

### 3.2 glib 2.84.4 の meson の dry 構成（zedBSD target、`build/p3-q597/probe/`）

既存の cross wrapper `build/amd64/packages/toolchain/bin/zedbsd-clang`、P3 の sysroot、host の meson 1.7.0、`system='zedbsd'` の仮の cross file（p004 の試作。repo には入れていない）で試した。
- **止まる所: `sys/poll.h` が無い**。glib は `sys/poll.h` か `winsock2.h` を要求し、無いと `error('FIX POLL* defines')` になる。zedBSD には `poll.h` だけがある（POLL* の値は標準どおり 1/2/4/8/0x10/0x20）。probe の中だけ `#include <poll.h>` の shim を置いて進めると、libc の検査を全部通り、pcre2 の dependency が無い所まで進んだ（以降の検査は p005 で依存を揃えてから）。
- 無いと判定されたが fallback がある物: eventfd（GWakeup は pipe を使う）・inotify と kqueue（file monitor の backend なし → GPollFileMonitor）・epoll・statfs（statvfs を使う）・getmntent_r（getmntent をロックして使う）・memalign・pthread_setname_np（名前を付けないだけ）・if_nametoindex/if_indextoname（WIN32 と `HAVE_IP_MREQN` の中だけで使う）・getauxval（issetugid を使う）・close_range/fdwalk（closefrom）・splice/copy_file_range・stpcpy（g_stpcpy）・nl_langinfo の拡張・futex（pthread を使う）・pidfd・memfd_create。

| 不足 | 使う側 | 影響 | 対応の案 |
| --- | --- | --- | --- |
| `<sys/poll.h>` | glib の meson（必須） | configure が止まる | **[判断] L1**: (a) libc に `#include <poll.h>` だけの `sys/poll.h` を足す（BSD と Linux にある header、1 file）。(b) glib に `poll.h` も受け付ける 1 hunk の patch を当てる。推奨は (a)。他の package も `sys/poll.h` を読むことが多いため |
| memfd_create | GDK の wl_shm・cursor | 無い場合 GTK は `shm_open(名前)`→`shm_unlink` に落ちる（libkeiui と同じ方式）。`F_ADD_SEALS` も使わない | 足さない（任意） |
| getifaddrs・if_nametoindex | gio の network | 使う所が glib の zedBSD の構成では build されない | 足さない |
| eventfd・inotify・kqueue | glib | fallback で動く。file monitor は polling | 足さない。file monitor の性能が問題になれば後で |
| pthread_setname_np | glib | thread に名前が付かないだけ | 足さない |

**libc に関数を足す必要はない**というのが結論。header の `sys/poll.h` が 1 つ要る（L1）。libc の変更は main の所有なので、L1 は main とユーザーの判断。

### 3.3 GTK 4.18.6 自身の OS API

memfd_create（任意、§3.2）・shm_open・mkostemp・posix_fallocate・memmem・strndup・clock_gettime・madvise・mmap。全部ある。ioctl と DMA_BUF は `HAVE_DMABUF`（`linux/dma-buf.h`）の中だけ。

## 4. host 道具

| 道具 | 要る側 | host（Debian 13）の現状 | 契約 |
| --- | --- | --- | --- |
| meson | 全部 | 1.7.0 | 4.18.6 の組（fontconfig 2.17.1）は 1.7.0 で足りる。4.24 を選ぶなら meson 1.12.0 を python で直接動かす（p004） |
| ninja | 全部 | 1.12.1 | そのまま |
| pkg-config | 全部 | pkgconf 1.8.1 | `zedbsd-pkg-config` の wrapper（`PKG_CONFIG_LIBDIR` を package の prefix と sysroot だけにする。inventory §3.5、p004） |
| gperf | fontconfig（`find_program('gperf')` が必須） | **無い** | gperf 3.3（inventory、SHA-256 `fd87e0ab…`、GPL-3.0-or-later の host 道具で image には入らない）を host 用に source から build（p004） |
| glib-compile-resources・glib-compile-schemas・glib-mkenums・glib-genmarshal・gdbus-codegen | GTK・gdk-pixbuf・pango | 2.84.4 | glib 2.84.4 を選ぶので host の物をそのまま使う。schema は host の glib-compile-schemas で stage の中に `gschemas.compiled` を作る |
| wayland-scanner | GTK（meson の `unstable-wayland` module が `dependency('wayland-scanner', native: true)`） | 1.23.1（pkg-config あり） | host の物を使う。生成された private code は `WL_PRIVATE` を自分で定義し、`wl_proxy_marshal_flags` を呼ぶ（§5） |
| bison ≥ 3.6 | libxkbcommon | 3.8.2 | そのまま |
| python3（packaging） | GTK・glib の生成 script | 3.13.5、packaging あり | そのまま。jinja2 は要らない |
| glslc | GTK の Vulkan renderer | 2025.2 | `-Dvulkan=enabled` のときだけ |
| nasm | libjpeg-turbo の SIMD | ある | そのまま |
| sassc・rst2man・g-ir-scanner | — | — | 要らない（生成済みの CSS、man と introspection は無効） |

## 5. libwayland の ABI（zedBSD `userland/desktop/libwayland` と GTK 4.18.6）

照合の方法: GTK の `gdk/wayland` が呼ぶ `wl_*` と、host の wayland-scanner 1.23.1 が生成する code が呼ぶ extern 関数を、`libwayland-client.so` の動的 export（427 symbol）と照合した。core protocol の名前（listener の数、`*_SINCE_VERSION`、enum）は、zedBSD の header と upstream 1.23.1 の header を比べた。
- GTK は自分の cursor の code（`gdk/wayland/cursor/`）を持つので、libwayland-cursor は要らない。xdg-shell などの protocol code も GTK が自分で生成する（hidden）。zedBSD の libwayland が export する `xdg_*_interface` などと名前は重なるが、GTK の側は `-Bsymbolic` と hidden で自分の物を使う。

| 不足 | 種類 | GTK の使い方 | p009 の対応 |
| --- | --- | --- | --- |
| `wl_log_set_handler_client` | 関数（export なし） | `gdkdisplay-wayland.c:626` で起動時に必ず呼ぶ | 足す（client の log handler） |
| `wl_surface_listener` の要素の数（zedBSD 2、upstream 4: `preferred_buffer_scale`・`preferred_buffer_transform`、v6） | header の構造体 | 位置で初期化しているので、要素が足りないと compile できない | header と event の表を wl_surface v6 まで広げる |
| `wl_surface_offset`・`WL_SURFACE_OFFSET_SINCE_VERSION`・`WL_SURFACE_DAMAGE_BUFFER_SINCE_VERSION`・`WL_SURFACE_SET_BUFFER_SCALE_SINCE_VERSION` | request（v5）と macro | 版を確かめてから呼ぶ | 足す |
| `WL_POINTER_AXIS_VALUE120_SINCE_VERSION` | macro | wl_pointer v8 の判定 | 足す（event の表と listener も v8/v9 の範囲を確かめる） |
| `wl_output_destroy`・`WL_OUTPUT_RELEASE_SINCE_VERSION`・`enum wl_output_transform`（`WL_OUTPUT_TRANSFORM_*`）・`WL_OUTPUT_SUBPIXEL_*` | 関数・macro・enum | output の扱い | 足す |
| `enum wl_shm_format` | enum の型 | buffer の形式 | 足す |
| `wl_data_device_manager_get_version` | 関数 | DnD の版の判定 | 足す |
| pkg-config の `wayland-client.pc`（Version 1.23.1）・`wayland-egl.pc` | metadata | `dependency('wayland-client', version: '>= 1.23.0')`、`dependency('wayland-egl')` | 足す。版は ABI の基準の 1.23.1 とする |

- 足りている物: `wl_display_*`（connect・dispatch 系・prepare_read(_queue)・read_events・cancel_read・roundtrip・flush・get_fd・create_queue）、`wl_event_queue_destroy`、`wl_proxy_*`（marshal_flags・add_listener・destroy・get/set_user_data・get_version・get_class・set_queue・create_wrapper）、`wl_egl_window_*`、core の `wl_*_interface`。
- GTK が使う protocol のうち zedBSD の表に無いもの（gtk-shell・viewporter・xdg-foreign・pointer-gestures・fractional-scale・presentation-time・xdg-activation・xdg-dialog・cursor-shape・single-pixel-buffer・color-management・linux-dmabuf など）の event は、`event.c` の `wlc_event_generic` が渡す。これは各引数を 1 word として最大 20 個を渡し、generated の listener を呼ぶもので、x86-64 SysV では整数と pointer の引数だけなので成り立つ。**p009 で、これらの listener への実際の配送（`new_id` の event で server が作る object を含む）を試験する。** compositor が advertise しない protocol は GTK が bind しないので、届く event の範囲は compositor が決める。

## 6. renderer（WS034 p038 の調査を含む）

| renderer | build 時 | runtime | 判定 |
| --- | --- | --- | --- |
| **Cairo（GskCairoRenderer）＋ wl_shm** | 常に build される | EGL も Vulkan も要らない。`GSK_RENDERER=cairo`、`GDK_DISABLE=gl,vulkan` | **最初の目標（推奨、ベータ1の案 B）**。WS114 の Linux 実測で、同じ 4.18.6 の Cairo は window・入力・clipboard が通った |
| GL（GskGLRenderer）＋ zedBSD の libEGL/libGLESv2（Vulkan の上の GLES 3.1） | libepoxy が必須。epoxy は既定で `libEGL.so.1` と `libGLESv2.so.2` を dlopen するが、zedBSD の SONAME は `libEGL.so` と `libGLESv2.so` | GTK が必須にしている EGL 拡張（`EGL_KHR_create_context`・`EGL_KHR_surfaceless_context`）と `EGL_KHR_platform_wayland`・`EGL_KHR_no_config_context` は zedBSD の EGL が出している | 第 2 段階。epoxy に `#elif defined(__ZEDBSD__)`（target の clang が定義する）で zedBSD の名前を足す patch（p008）。GLES 3.1 の機能の過不足は p010 で実測 |
| Vulkan（GskVulkanRenderer）＋ libvulkan（Venus） | `vulkan.pc` が ≥ 1.3 で、Vulkan 1.3 の header が要る。**zedBSD の `include/libc/vulkan/` は選んだ部分だけの ABI（`VK_HEADER_VERSION_COMPLETE` は 1.0、`VK_VERSION_1_3` なし）** で、GTK の `VK_API_VERSION_1_3`・`VkPhysicalDeviceVulkan11/12Features` が無い | instance 拡張は VK_KHR_surface/wayland_surface、device は swapchain ほか | **[判断] R1**: (a) 最初は `-Dvulkan=disabled`（推奨）、(b) zedBSD の Vulkan header を 1.3 まで広げる（libc/GPU の境界なので main の判断）、(c) GTK の build のときだけ Khronos の Vulkan-Headers を使う（libvulkan の ABI と食い違う危険がある） |

GTK 4.18 は GL が使えなければ Cairo に落ちる。最初の目標は Cairo とし、GL と Vulkan は WS114 の Linux の 3 renderer の比較と並べて段階的に足す。

## 7. demo app と試験

- **app**: GTK の tarball の `gtk4-demo`（`--run=clipboard` などの起動は WS114 q580 と同じ）と `gtk4-widget-factory`（`-Dbuild-demos=true`）。新しい試験 app は書かない。Linux の WS114 と同じ 4.18.6 の app なので、同じ操作の結果をそのまま比べられる。zedBSD には python3 の gi が無いので、WS114 の `gtk4-baseline.py` は使えない。代わりに gtk4-demo の demo（clipboard・dialog・menu・entry・maximize/fullscreen）を使う。
- **代表操作**（WS114 p007 と同じ範囲）: window の表示（CSD）、menu・tooltip・modal、entry への key 入力、別 client への clipboard（gtk4-demo と widget-factory の間）、move・resize・maximize/fullscreen と restore、close。renderer はまず Cairo。
- **runtime の前提**（p010 で image に入れる物）:
  - fontconfig の `fonts.conf`（zedBSD の font の directory を指す）と書ける cache の directory。
  - xkeyboard-config のデータ（GTK は `xkb_keymap_new_from_names` で既定の keymap を作る。Keiland の compositor は完全な keymap の文字列を送る）。
  - GTK の schema の `gschemas.compiled`。
  - D-Bus が無いので `GTK_A11Y=none`、GApplication は single-instance の D-Bus 無しで動く（警告は記録する）。FileDialog は WS114 p001 で `gsettings-desktop-schemas` が無いと落ちたので、範囲に入れるかを p010 で決める。
- **試験の手順**:
  - 起動の確認は `plan/tools/boot-test.sh`。
  - GUI は Venus の QEMU の desktop guest（`plan/ws035/tests/zdesktop-guest.sh start/wait/stop`、[zedbsd-commands §5–7](../tools/keiland-linux/zedbsd-commands.md)）。
  - app の起動は serial（`plan/tools/guest/serial.py`）か SSH、画面と入力は QMP（PNG・input-send-event）。
  - 判定は app の stdout と `WAYLAND_DEBUG=client` の trace、PNG。console と serial の log では判定しない。
- **build の試験**（p004〜p002）:
  - 各 package の stage の ELF を `tools/build/check-dynamic-elf.py` で確かめる（symbol versioning なし、`DT_NEEDED`、SONAME の実体）。
  - license は `plan/tools/packages/audit-licenses.sh`。
  - 最後の image は `boot-test.sh`。

## 8. ユーザーの判断が要る点

| ID | 問い | 推奨 | 影響 |
| --- | --- | --- | --- |
| **D-VER** | GTK の版 | **4.18.6**（glib 2.84.4・pango 1.56.4・fontconfig 2.17.1、host の meson 1.7） | 4.24.1 なら meson 1.12（python）、glib 2.90.0（host の glib 道具も 2.90 で作る）、pango 1.58.2、fontconfig 2.18.3、wayland-protocols ≥ 1.48、libwayland を 1.24 の機能まで広げる。Queue がおよそ 1 つ増える |
| **L1** | libc に `sys/poll.h` を足すか | **足す**（`poll.h` を include するだけの 1 file。main の所有） | 足さないなら glib に patch を当てる（他の package でも同じ patch が要りうる） |
| L2 | libc に関数を足すか | **足さない**（memfd_create・eventfd・inotify・getifaddrs は fallback で動く） | file monitor が polling になる（性能だけ） |
| **R1** | renderer の目標 | **Cairo＋wl_shm を最初に**、次に GL（epoxy の patch）。Vulkan は header の判断の後 | Vulkan 1.3 の header の拡張は libc/GPU の境界にかかる |
| P1 | portal・D-Bus | 入れない（WS114 p004 の推奨と同じ） | FileDialog は gsettings schema しだい |
| S1 | 共有 path の割当 | p004 は `userland/packages/external.mk`・`tools/`、p009 は `userland/desktop/libwayland/`・`userland/desktop/keiland/wayland/` | main が割り当てる。libwayland は全 Keiland app と libvulkan の WSI が使う |

## 9. Phase の範囲（確定と改訂）

| Phase | 範囲（この契約で確定） | 前の計画との差 |
| --- | --- | --- |
| p004 | meson の cross file と native file、`zedbsd-pkg-config`、package の prefix、libtool の zedbsd 対応の共通化、symbol versioning の無効化の共通化、gperf 3.3 の host build、meson は 4.18 の組なら host の 1.7 | glib の host 道具は作らない（host の 2.84.4 を使う）。wayland-scanner も host の 1.23.1 を使う |
| p005 | libffi 3.8.0・pcre2 10.48・**glib 2.84.4**、L1 の結果（`sys/poll.h`） | glib の版を 2.90.0 から 2.84.4 に |
| p006 | libpng・freetype・harfbuzz 14.5.0（glib 付き・subset）・**fontconfig 2.17.1**（gperf） | fontconfig の版。harfbuzz は libc++.so.1（main の devel/libcxx）を要る |
| p007 | pixman・cairo（gobject）・fribidi・**pango 1.56.4** | pango の版 |
| p008 | gdk-pixbuf・libjpeg-turbo・libtiff・graphene・**libepoxy（zedBSD の library 名の patch）**・libxkbcommon・xkeyboard-config | epoxy の patch を明記 |
| p009 | libwayland の §5 の不足（wl_log_set_handler_client、wl_surface v5/v6、wl_output と wl_pointer の名前、enum、pkg-config）と generic 配送の試験、wayland-protocols 1.49 のデータ | 本家 libwayland と wayland-cursor は作らない（GTK が cursor の code を持つ） |
| p002 | GTK 4.18.6 本体（§1.3 の option）、`userland/packages/desktop/gtk4/` | Vulkan は R1 しだい |
| p010 | §7 の app・操作・runtime の前提、Cairo → GL（→ Vulkan） | — |

分ける必要は今のところ無い。p009 は p004 だけに依存し、p005〜p008 と並行できる（共有 path が別だから）。

## 10. 確認したこと・していないこと

- 実施: tarball の取得と SHA-256 の照合（GNOME の公開値、inventory の値）、GTK・依存の `meson.build` の読解、GTK と glib の OS API の grep、zedBSD の `libc.so`・`libwayland-client.so`・`libEGL.so` の export と SONAME の照合、header の比較、glib 2.84.4 の target 向け meson の dry 構成（`sys/poll.h` で止まり、shim を置けば pcre2 まで進むことを確認）、host 道具の版。
- 未実施: どの package の実際の build も、GTK の meson 構成も（依存が無いため）、runtime（zedBSD 上の実行）も、generic 配送の実測も、license の機械監査（p005 以降）もしていない。版を pin すると決めたら、署名の検証は各 Phase で inventory の方法に従う。

## 11. 移植で分かった zedBSD の libc と kernel の差（p005 以降、追記していく）

Q1 の判断（2026-10-02）: POSIX の名前空間にかかる変更（string.h から strings.h を読むかどうか）は WS001 の観点で後に決める。それまでは package ごとの小さな patch で対処し、どの package で起きたかをここに残す。libc の判断の材料にする。

| 差 | 種類 | 見つけた package | 対処 |
| --- | --- | --- | --- |
| `<sys/poll.h>` が無い | libc の header | glib（meson が必須にしている） | **libc に追加済み**（main acb5a2da9、L1） |
| `<arpa/nameser.h>` が無い（定義は `<resolv.h>` にある）。`HEADER` 構造体と `GETSHORT`/`GETLONG` も無い | libc の header | glib（gio の検査、`gthreadedresolver.c`） | glib の patch 0001・0003。record の検索は G_RESOLVER_ERROR_INTERNAL を返す |
| `string.h` が strcasecmp/strncasecmp を宣言しない（POSIX どおり `strings.h` だけ） | libc の header（名前空間） | glib（glib-init.c、gstrfuncs.c） | glib の patch 0002（`HAVE_STRINGS_H` で `strings.h` を読む） |
| gettext 系に `format_arg` の属性が無い | libc の header | glib（-Werror=format-nonliteral/-security） | **libc に追加済み**（P3 150c2f51f、main 871777b34。ユーザーの明示の許可。差分の元は plan/ws115/proposed/libc-libintl-format-arg.diff） |
| `CMSG_NXTHDR` が無い（POSIX が要求する） | libc の header | glib（gsocket.c） | **libc に追加済み**（同上。差分の元は plan/ws115/proposed/libc-cmsg-nxthdr.diff、host 試験 tests/cmsg-nxthdr-test.c） |
| `IN_MULTICAST()`・`SOMAXCONN` が無い | libc の header（BSD と POSIX の定数） | glib（ginetaddress.c、gsocket.c） | glib の patch 0004（通常の定義と 128 を置く） |
| IP 層の socket option（IP_TTL、multicast、group membership、IPv6 版）・`SOCK_SEQPACKET`・`FIONREAD` が無い | kernel（UAPI に無い） | glib（gsocket.c） | glib の patch 0004（G_IO_ERROR_NOT_SUPPORTED、available bytes は -1）。kernel に足すかは別の判断（記録だけ） |
| `__tls_get_addr` が ld.so にだけあり、link のときには見えない | link の契約 | glib（meson の既定 `--no-undefined`） | glib に `-Db_lundef=false`。p006 以降で同じことが起きたら external.mk の共通規則か、link で ld.so を見せる案で決める（Q1） |
| glib の fuzz の `fuzz_resolver` が record の parser（0003 で外した）を呼ぶ | 上の nameser の結果 | glib（fuzzing/meson.build） | glib の patch 0005（`arpa/nameser.h` が無ければその target だけ外す） |
| harfbuzz（C++）が libc++ の header を要る | toolchain の package（devel/libcxx） | harfbuzz | upstream の既定（`with_libstdcxx=false`）で C の linker で link するので、実行時に libc++ は要らない（NEEDED と未定義の C++ の symbol が無いことを確かめた）。header は `ZEDBSD_EXT_harfbuzz_DEPENDS` の `libcxx` の stage から `-nostdinc++ -isystem <view>/usr/include/c++/v1`（lang/clang と同じ形）。subagent の worktree では main の stage の複写を使い、make に `-o <libcxx の stage>/.zedbsd-staged` を渡す（p006） |
| fontconfig の `additional-fonts-dirs=yes` が build の機械の X11 の font の directory を調べる | cross の build の道具 | fontconfig | `no` にし、`default-fonts-dirs=/usr/share/fonts`（p006） |
| `<alloca.h>` が無く、alloca の宣言がどこにも無い | libc の header | cairo（cairo-colr-glyph-render.c） | cairo の patch 0001（header が無ければ `__builtin_alloca`）。libc に `alloca.h` を足すかは別の判断（記録だけ。p007） |
| `getc_unlocked()` が無い（POSIX。flockfile・funlockfile はある） | libc | pango（pango-utils.c） | pango の patch 0001（meson の検査と getc への fallback）。libc に足すかは別の判断（記録だけ。p007） |
| LLVM 23 の clang が `-Wunused-but-set-variable` の群に `-Wunused-but-set-global` を含め、G_DEFINE_TYPE の parent_class を報告する | toolchain の版 | pango（`-Werror=unused-but-set-variable`）。glib・cairo などでは warning だけ | pango の patch 0001（その診断だけ `-Wno-error`）（p007） |
| pixman が TLS を使い `__tls_get_addr` を呼ぶ | link の契約（glib と同じ） | pixman | `-Db_lundef=false` を package ごとに指定（glib と pixman の 2 つ。external.mk の共通化はしていない。p007） |
| `<inttypes.h>` の `PRId64`・`PRIu64` が `"lld"`・`"llu"` で、`int64_t` は `long`（LP64） | libc の header | libtiff・libxkbcommon・glib（gtestutils）・expat の `-Wformat` の warning（実行時は同じ幅で害は無い） | 記録だけ。libc の inttypes.h を `"ld"`・`"lu"` に直す差分の候補（main の判断。p008） |
| `<assert.h>` が C11 の `static_assert` を定義しない | libc の header | libxkbcommon | libxkbcommon の patch 0002（meson の検査で、無ければ config.h で `_Static_assert` に）。libc に足す差分の候補（p008） |
| `<dlfcn.h>` に `RTLD_NOLOAD` が無い（拡張で、POSIX ではない） | libc・rtld | libepoxy | libepoxy の patch 0002（無ければ「読み込まれていない」と扱う）（p008） |
| `CLOCK_PROCESS_CPUTIME_ID` が無い | libc（POSIX の CPU 時間の clock） | libxkbcommon の bench | patch 0003（cross build では試験・bench を作らない）。記録だけ（p008） |
| EGL・GLES・GL の SONAME が版の無い `libEGL.so`・`libGLESv2.so`・`libGL.so`（/lib） | zedBSD の library の名前 | libepoxy（dlopen） | libepoxy の patch 0001（`__ZEDBSD__` で名前を足す）。guest で epoxy 経由の `eglQueryString` が通った（p008） |
| `config.sub` と libtool が zedbsd を知らない | 外部の build 道具 | libffi（autotools だけ） | libffi の patch 0001（OpenSSH の先例と同じ形） |
