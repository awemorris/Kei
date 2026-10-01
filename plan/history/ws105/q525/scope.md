<!-- awesome-plan project=zedbsd record=ws105-p003 -->

# ws105-p003: libvulkan-compat (1): 後段への chain（WSI 無し）

Status: planned
Disposition: normal
Parent: [WS105](../ws.md)
Queue: なし
依存: p002
実行者: phase-runner（high）。**始める前に [design.md](../design.md) の §4 を全部読む**（特に §4.3・§4.4 の「I・O の関数の中から後段を呼ぶ規則」・§4.11）

## 目的

`userland/desktop/libvulkan-compat/` を作り、Vulkan の全ての関数を system の libvulkan（後段）に渡す library（`libvulkan.so.1`）にする（決定 D4・D5・D10）。
WSI（surface・swapchain・display）は p004・p005 で足す。この Phase の終わりには、WSI を使わない Vulkan の program が、我々の library を通して後段の lavapipe で動く。

**一番大事な考え方**（design §4.4）: dispatchable な handle（`VkInstance`・`VkDevice`・`VkCommandBuffer` など）を包まない。我々の export した `vkCmdDraw` は、
後段が export している `vkCmdDraw` を `dlsym` で得た pointer で呼ぶだけ。後段の export の関数が handle から正しい driver を選ぶ。
**後段は `RTLD_DEEPBIND` で開く**（Debian の loader は、そうしないと我々の関数を自分の物として使ってしまう。2026-10-01 に確かめた、design §4.11）。

## 手本（[survey/](../survey/README.md)、2026-10-01 に host で通した物）

- `survey/vkprotos.sh`: `vulkan_core.h` から core の prototype を抜き出す（1.0=137・1.1=28・1.2=13・1.3=37・1.4=19、計 234）。
- `survey/gen-forward.sh`: TSV から F の関数の定義と dlsym の表を作る awk。
- `survey/compat.c`・`survey/exports.map`: 最小の forwarder（constructor で後段を開く。我々の版は `pthread_once` で最初の呼び出しの時に開く）。

## 作る file（`userland/desktop/libvulkan-compat/`、design §4.10）

| file | 中身（design の節） |
| --- | --- |
| `README.md` | design §4.1・§4.2 の要約と図。「zedBSD の `libvulkan/` とは別の物」と冒頭に書く |
| `Makefile.linux` | 下の「build の規則」 |
| `functions.tsv` | 列: `block<TAB>name<TAB>return<TAB>params<TAB>param-names<TAB>kind<TAB>export`。最初の 5 列は `sh plan/ws105/survey/vkprotos.sh /usr/include/vulkan/vulkan_core.h "VK_VERSION_1_0 VK_VERSION_1_1 VK_VERSION_1_2 VK_VERSION_1_3 VK_VERSION_1_4"` の出力。`kind` は F・I（design §4.4 の I の 10 個）、`export` は全て `y`。N（拒む名前）の行も足す: `sh plan/ws105/survey/vkprotos.sh /usr/include/vulkan/vulkan_core.h "VK_KHR_surface VK_KHR_swapchain VK_KHR_display VK_KHR_display_swapchain VK_EXT_direct_mode_display VK_EXT_acquire_drm_display VK_KHR_get_surface_capabilities2 VK_KHR_get_display_properties2 VK_EXT_display_surface_counter VK_EXT_display_control VK_KHR_shared_presentable_image VK_EXT_full_screen_exclusive VK_EXT_swapchain_maintenance1 VK_KHR_present_wait VK_GOOGLE_display_timing VK_EXT_hdr_metadata VK_EXT_headless_surface"` と `/usr/include/vulkan/vulkan_wayland.h`・`vulkan_xcb.h`・`vulkan_xlib.h`・`vulkan_xlib_xrandr.h` の block の出力を、この Phase では全て `kind=N`・`export=n` にする（p004・p005 で我々が実装する物を O・`y` に変える）。file の先頭に `#` の注釈の行で作り方を書く |
| `gen-forward.sh` | `functions.tsv` から 2 つを作る POSIX の sh（awk）: (1) `forward.inc`（kind F の関数ごとに `static PFN_vkXxx next_vkXxx;`、export の定義（`compat_enter("vkXxx")` → `if (next_vkXxx == NULL) compat_missing("vkXxx");` → 呼び出し → `compat_leave()`）、表の初期化の関数 `compat_forward_fill(void *handle)`、名前 → kind の表 `compat_names[]`（I・O・N の名前と kind、GetProcAddr が引く））、(2) `exports.map`（`{ global: <export が y の名前>; local: *; };`）。使い方: `gen-forward.sh functions.tsv OUTDIR` |
| `compat.h` | 内部の struct（instance・device・queue の記録、design §4.5）と関数の宣言、`compat_enter`・`compat_leave`・`compat_missing` |
| `backend.c` | design §4.3: 後段を探す順、`dlopen(path, RTLD_NOW | RTLD_LOCAL | RTLD_DEEPBIND)`（`KEILAND_VULKAN_NO_DEEPBIND=1` で外す）、自分を開いていないかの確かめ（`dladdr`・`realpath`・`dlsym` の address）、`compat_forward_fill`、I の関数の後段の版（`dlsym` の pointer）の表、失敗の message。`pthread_once` で 1 回 |
| `dispatch.c` | `vkGetInstanceProcAddr`・`vkGetDeviceProcAddr`（design §4.7。N は NULL）、再入の検出（thread-local の「今入っている関数の名前」の stack、同じ名前に再び入ったら design §4.11 の message と `abort()`）、`compat_missing`（design §4.4 の message と `abort()`） |
| `instance.c` | `vkCreateInstance`・`vkDestroyInstance`・`vkEnumerateInstanceExtensionProperties`・`vkEnumerateInstanceLayerProperties`（後段にそのまま）・`vkEnumerateInstanceVersion`（後段にそのまま）、instance の記録（design §4.6。この Phase では後段の WSI の拡張を一覧から**消す**だけで、我々の WSI の拡張は足さない。内部で要る拡張の追加も p004 から） |
| `device.c` | `vkCreateDevice`・`vkDestroyDevice`・`vkEnumerateDeviceExtensionProperties`（後段の WSI の拡張を消すだけ）・`vkGetDeviceQueue`・`vkGetDeviceQueue2`、device と queue の記録 |

### build の規則（`Makefile.linux`）

```make
# userland/desktop/libvulkan-compat/Makefile.linux -- the Linux libvulkan.so.1: our WSI in front of the system's
# libvulkan (WS105, plan/ws105/design.md §4).  There is no zedBSD Makefile: zedBSD's Vulkan is userland/desktop/libvulkan.
LIBVULKAN_COMPAT_DIR := userland/desktop/libvulkan-compat
LIBVULKAN_COMPAT_GEN := $(KEILAND_LINUX_BUILD)/gen/libvulkan-compat
LIBVULKAN_COMPAT_SOURCES := $(LIBVULKAN_COMPAT_DIR)/backend.c $(LIBVULKAN_COMPAT_DIR)/dispatch.c \
	$(LIBVULKAN_COMPAT_DIR)/instance.c $(LIBVULKAN_COMPAT_DIR)/device.c

$(LIBVULKAN_COMPAT_GEN)/forward.inc $(LIBVULKAN_COMPAT_GEN)/exports.map: $(LIBVULKAN_COMPAT_DIR)/functions.tsv $(LIBVULKAN_COMPAT_DIR)/gen-forward.sh
	@mkdir -p $(LIBVULKAN_COMPAT_GEN)
	sh $(LIBVULKAN_COMPAT_DIR)/gen-forward.sh $(LIBVULKAN_COMPAT_DIR)/functions.tsv $(LIBVULKAN_COMPAT_GEN)

# The generated forwarders are included by dispatch.c; every source sees the generated directory.
KEILAND_LINUX_CPPFLAGS_userland_desktop_libvulkan-compat_ := -I$(LIBVULKAN_COMPAT_GEN) \
	-DKEILAND_VULKAN_BACKEND_PATHS='"/usr/lib/$(KEILAND_LINUX_MULTIARCH)/libvulkan.so.1:/usr/lib/x86_64-linux-gnu/libvulkan.so.1:/usr/lib/aarch64-linux-gnu/libvulkan.so.1:/usr/lib64/libvulkan.so.1:/usr/lib/libvulkan.so.1"'
$(patsubst %.c,$(KEILAND_LINUX_BUILD)/obj/%.o,$(LIBVULKAN_COMPAT_SOURCES)): $(LIBVULKAN_COMPAT_GEN)/forward.inc

$(eval $(call KEILAND_LINUX_LIBRARY,vulkan-compat,libvulkan.so.1,$(LIBVULKAN_COMPAT_SOURCES),,-ldl,$(LIBVULKAN_COMPAT_GEN)/exports.map,-Wl$(comma)-Bsymbolic))

# Our programs link -lvulkan; the build directory gets the development name (never installed).
$(KEILAND_LINUX_BUILD)/lib/libvulkan.so: $(KEILAND_LINUX_BUILD)/lib/libvulkan.so.1
	ln -sf libvulkan.so.1 $@
KEILAND_LINUX_ALL += $(KEILAND_LINUX_BUILD)/lib/libvulkan.so
```

- 我々の program は `-l:libvulkan.so.1` で link する（design §3.3 の表）。`libvulkan.so` の link は system の `-lvulkan` に頼る試験の program のため。
- `KEILAND_LINUX_PACKAGES` の `libtruetype` の後に `userland/desktop/libvulkan-compat/Makefile.linux` を足す（p004 で libwayland-client に依存する）。
- 生成物は source の tree に置かない（`$(KEILAND_LINUX_BUILD)/gen/`）。`makefile-sync.sh` には `# keiland-linux-sync: only userland/desktop/libvulkan-compat/*` の行が要る（zedBSD の `Makefile` が無いので、sync の対象の外にする）。

## 試験の道具（`plan/tools/keiland-linux/`、main が merge）

| file | 中身 |
| --- | --- |
| `vk-chain-test.c` | (1) `dladdr((void *)vkGetInstanceProcAddr, ...)` の path が `.../opt/keiland/lib/libvulkan.so.1` で終わる（我々の物を通っている）。(2) instance を `VK_API_VERSION_1_0` で作り、physical device の名前と driver を出す（`llvmpipe`）。(3) device を作り、queue で 1 MiB の buffer を `vkCmdFillBuffer` で 0x5a5a5a5a に埋め、host-visible の buffer に `vkCmdCopyBuffer` し、fence で待ち、読んで確かめる。(4) instance の拡張の一覧に `VK_KHR_xcb_surface`・`VK_KHR_xlib_surface` が**無い**（p004 の後もずっと無い）。(5) `vkGetDeviceProcAddr(device, "vkCmdFillBuffer")` が NULL でない、`vkGetInstanceProcAddr(instance, "vkCreateXcbSurfaceKHR")` が NULL。(6) 環境変数 `VK_CHAIN_REENTER=1` のとき: 何もしない（下の 7 の偽の後段の試験で使う）。最後に `vk-chain-test: PASS`、exit 0 |
| `interpose-check.sh` | design §4.11・§7.3: (a) 既定で `LD_DEBUG=bindings` の後段から我々への `vk` の結び付きの行が 0、(b) `KEILAND_VULKAN_NO_DEEPBIND=1` でも `vk-chain-test: PASS`、(c) 結果を `interpose-check: PASS`・`FAIL` で出す |
| `fake-backend.c` | 偽の後段: `vkGetInstanceProcAddr` と `vkCreateInstance` と `vkEnumerateInstanceExtensionProperties` だけを export し、`vkCreateInstance` の中で `vkCreateInstance` を**名前で**（PLT を通して）呼ぶ。**gcc で `-fPIC -shared` だけで build する**（`-Bsymbolic`・`-fno-semantic-interposition` を付けない。clang は同じ module の中の呼び出しを結び付けてしまうことがあり、試験が空しく通る） |

## 手順（host、repo の root で。全ての command に `timeout` と host の安全の環境変数）

```
make -j64 keiland-linux && make keiland-linux-install DESTDIR=$PWD/build/keiland-linux/stage
make -j64 keiland-linux CC=clang KEILAND_LINUX_BUILD=build/keiland-linux-clang
sh plan/tools/keiland-linux/elf-check.sh build/keiland-linux/stage
sh plan/tools/keiland-linux/makefile-sync.sh
STAGE=build/keiland-linux/stage/opt/keiland/lib
SAFE="env -u WAYLAND_DISPLAY -u DISPLAY KEILAND_DRM_DEVICE=none XDG_RUNTIME_DIR=$PWD/build/keiland-linux/test/empty-xdg"
mkdir -p build/keiland-linux/test/empty-xdg && chmod 700 build/keiland-linux/test/empty-xdg
cc -std=gnu17 -Wall -Wextra -Werror -o build/keiland-linux/test/vk-chain-test plan/tools/keiland-linux/vk-chain-test.c -ldl \
   -Wl,--no-as-needed -Lbuild/keiland-linux/lib -l:libvulkan.so.1 -Wl,-rpath-link,build/keiland-linux/lib -Wl,-rpath,/opt/keiland/lib
timeout 60 $SAFE LD_LIBRARY_PATH=$STAGE build/keiland-linux/test/vk-chain-test
timeout 60 $SAFE LD_LIBRARY_PATH=$STAGE vulkaninfo --summary | grep -E 'deviceName|driverName'
timeout 120 sh plan/tools/keiland-linux/interpose-check.sh
timeout 30 $SAFE KEILAND_VULKAN_BACKEND=$PWD/$STAGE/libvulkan.so.1 LD_LIBRARY_PATH=$STAGE build/keiland-linux/test/vk-chain-test; echo "exit=$?"   # 自分を開く誤り
timeout 30 $SAFE KEILAND_VULKAN_BACKEND=/nonexistent LD_LIBRARY_PATH=$STAGE build/keiland-linux/test/vk-chain-test; echo "exit=$?"              # 後段が無い
gcc -fPIC -shared -o build/keiland-linux/test/libfake-vulkan.so plan/tools/keiland-linux/fake-backend.c
timeout 10 $SAFE KEILAND_VULKAN_NO_DEEPBIND=1 KEILAND_VULKAN_BACKEND=$PWD/build/keiland-linux/test/libfake-vulkan.so LD_LIBRARY_PATH=$STAGE \
   build/keiland-linux/test/vk-chain-test; echo "exit=$?"                                                                                          # 再入の検出
make -j64 disk-image > build/ws105-p003-zedbsd.log 2>&1; echo "zedbsd make exit=$?"
```

## 完了の条件

1. build（gcc・clang）が通る、`elf-check: PASS`（`libvulkan.so.1` の SONAME は `libvulkan.so.1`、NEEDED は libc と libdl だけ）、`makefile-sync: PASS`。
2. `vk-chain-test: PASS`（host の lavapipe）。
3. `vulkaninfo --summary` が我々の library で走り、`llvmpipe` を出す（vulkaninfo は `dlopen` と `vkGetInstanceProcAddr` だけを使う、design §4.11）。
4. `interpose-check: PASS`（design §10 の V1・V2）。FAIL なら Phase を uncleared で止め、出た行を記録して main に報告。
5. 自分を開く誤りと後段が無い場合: `libvulkan-compat: no backend libvulkan` の message、exit が 0 でない、10 秒以内に終わる（固まらない）。
6. 偽の後段の再入: `libvulkan-compat: the backend called back into vkCreateInstance` の message で `abort`（exit 134）、10 秒以内（`timeout` の 124 でない）。
7. zedBSD の build に影響が無い（`zedbsd make exit=0`。libvulkan-compat は zedBSD の `Makefile` を持たない、D4）。

## 結果

（実行の後に書く。V1・V2 の結果を design §10 の表にも追記する）
