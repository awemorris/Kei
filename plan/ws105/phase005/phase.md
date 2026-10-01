<!-- awesome-plan project=zedbsd record=ws105-p005 -->

# ws105-p005: libvulkan-compat (3): 画面の WSI（KMS、VK_KHR_display、VK_EXT_acquire_drm_display）

Status: uncleared
Disposition: normal
Parent: [WS105](../ws.md)
Queue: q530 / q530-i01
依存: p004、p001（guest）
実行者: phase-runner（high）。**始める前に [design.md](../design.md) の §4.9 と §7.2 を読む**

## 目的

compositor が画面に出すための VK_KHR_display を、libvulkan-compat が Linux の KMS を直接使って実装する（決定 D8・D9・D20）。DRM の master の fd は compositor の
seat が得て `vkAcquireDrmDisplayEXT` で渡す（p006）。この Phase では、試験の program が自分で開いた fd を渡す形（compositor と同じ）と、root の app（vkdemo）が acquire せずに使う形を確かめる。
**KMS の試験は guest だけで行う**（host の DRM の device を開かない）。

## 作る・変える file（`userland/desktop/libvulkan-compat/`）

| file | 中身 |
| --- | --- |
| `functions.tsv` | design §4.4 の O の表の `VK_KHR_display`（7）・`VK_EXT_direct_mode_display`（1）・`VK_EXT_acquire_drm_display`（2）を `kind=O`・`export=y` に |
| `instance.c` | instance の拡張に `VK_KHR_display`・`VK_EXT_direct_mode_display`・`VK_EXT_acquire_drm_display` を足す |
| `kms.c` | design §4.9 の KMS の ioctl（`<drm/drm.h>`・`<drm/drm_mode.h>`、libdrm を使わない）: 問い合わせの fd の open（`KEILAND_DRM_DEVICE`、`none` なら 0 個、無ければ `/dev/dri/card0`〜`card15` で connector を持つ最初の物）と直後の `DROP_MASTER`、connector・encoder・CRTC・mode の列挙（`DRM_IOCTL_MODE_GETRESOURCES` は 2 回呼ぶ: 数を得て、配列を用意して埋める）、master（`SET_MASTER`・`DROP_MASTER`）、dumb buffer の作成・map（`MAP_DUMB` で offset を得て `mmap`）・破棄、`ADDFB2`（`DRM_FORMAT_XRGB8888`）・`RMFB`、`SETCRTC`、`PAGE_FLIP`（`DRM_MODE_PAGE_FLIP_EVENT`）と event の読み取り（`poll` の上限 100 ms の後に `read`、`struct drm_event_vblank`） |
| `wsi-display.c` | VK_KHR_display の Vulkan の側: `VkDisplayKHR`（connected の connector ごと）・`VkDisplayModeKHR`（mode ごと）の記録、plane は 1 つ（primary）、`vkCreateDisplayPlaneSurfaceKHR`、`vkAcquireDrmDisplayEXT`（fd を `dup`、同じ display に何度でも）・`vkGetDrmDisplayEXT`・`vkReleaseDisplayEXT` |
| `wsi-swapchain.c` | display の surface の swapchain（design §4.9 の複写の道）: image は後段の `OPTIMAL`（usage に `TRANSFER_SRC` を足す）、present で `vkCmdCopyImageToBuffer`（layout を `TRANSFER_SRC_OPTIMAL` に移す barrier と戻す barrier）→ fence を待つ → dumb buffer へ行ごとに memcpy → 最初の 1 回は `SETCRTC`、以後 `PAGE_FLIP`。FIFO（flip の event を待ってから次）。master を失ったら（`EACCES`・`EPERM`）`VK_ERROR_OUT_OF_DATE_KHR`。破棄で fb と dumb buffer を消し、CRTC を acquire の時に `GETCRTC` で覚えた物に戻す |
| `userland/desktop/vkdemo/Makefile.linux` | vkdemo（依存 `libvulkan.so.1`、source は zedBSD の `Makefile` と同じ 4 つ（`userland/base/common/sha256.c` を含む）、`-lm`） |

細部:

- surface の format: `VK_FORMAT_B8G8R8A8_UNORM`・`_SRGB`（dumb buffer は `XRGB8888`、memory の並びは B8G8R8A8 と同じ）。present mode は `FIFO` だけ。
- `vkGetPhysicalDeviceDisplayPropertiesKHR` の `physicalResolution` は mode の preferred の大きさ（`DRM_MODE_TYPE_PREFERRED`、無ければ最初の mode）。
  `displayName` は connector の種類と番号（例 `Virtual-1`）。zedBSD の compositor の `compose_display` が `physicalResolution` を使う。
- `vkGetDisplayModePropertiesKHR` の `refreshRate` は mHz（`clock × 1000 × 1000 / (htotal × vtotal)` を丸めた値。`vrefresh × 1000` でもよい）。
- display の mode の数え方・struct の大きさは linux-libc-dev 6.12 の `<drm/drm_mode.h>` に従う。

## 試験の道具（`plan/tools/keiland-linux/`、main が merge）

`display-probe.c`: 我々の libvulkan-compat で VK_KHR_display を使い、(1) display と mode を出す（`DISPLAY name=... width=... height=... refresh_mhz=...`）、(2) `--acquire` なら `/dev/dri/card0` を
**最初の Vulkan の呼び出しより前に**自分で開き（master を先に取る）、`setenv("KEILAND_DRM_DEVICE", "/dev/dri/card0", 1)` してから instance を作り、`vkAcquireDrmDisplayEXT` に渡す（compositor の形）、
(3) 全画面を 赤 → 緑 → 青 に clear して present し、各色の後に stdout に `DISPLAY color=ff0000` などを出して 5 秒待つ（screenshot の時間）、(4) release して終わり `display-probe: PASS`。

## 手順（host で build し、guest で走らせる）

```
make -j64 keiland-linux && make keiland-linux-install DESTDIR=$PWD/build/keiland-linux/stage
make -j64 keiland-linux CC=clang KEILAND_LINUX_BUILD=build/keiland-linux-clang
sh plan/tools/keiland-linux/elf-check.sh build/keiland-linux/stage && sh plan/tools/keiland-linux/makefile-sync.sh
cc -std=gnu17 -Wall -Wextra -Werror -o build/keiland-linux/stage/opt/keiland/bin/display-probe plan/tools/keiland-linux/display-probe.c \
   -Lbuild/keiland-linux/lib -l:libvulkan.so.1 -Wl,-rpath-link,build/keiland-linux/lib -Wl,-rpath,/opt/keiland/lib
G=plan/tools/keiland-linux/guest.sh
sh $G start
sh plan/tools/keiland-linux/install-guest.sh
sh $G ssh 'rm -f /tmp/display.out; openvt -c 7 -s -- sh -c "KEILAND_SEAT=direct /opt/keiland/bin/display-probe --acquire > /tmp/display.out 2>&1"'
sh $G ssh 'for i in $(seq 30); do grep -q "color=ff0000" /tmp/display.out && break; sleep 1; done; cat /tmp/display.out'
sh $G screenshot $PWD/build/keiland-linux/p005-red.png
python3 plan/tools/keiland-linux/png-probe.py build/keiland-linux/p005-red.png 10 10 640 400 1270 790 10 790
sh $G ssh 'for i in $(seq 10); do grep -q "color=00ff00" /tmp/display.out && break; sleep 1; done'
sh $G screenshot $PWD/build/keiland-linux/p005-green.png
python3 plan/tools/keiland-linux/png-probe.py build/keiland-linux/p005-green.png 10 10 640 400 1270 790 10 790
sh $G ssh 'for i in $(seq 10); do grep -q "color=0000ff" /tmp/display.out && break; sleep 1; done'
sh $G screenshot $PWD/build/keiland-linux/p005-blue.png
python3 plan/tools/keiland-linux/png-probe.py build/keiland-linux/p005-blue.png 10 10 640 400 1270 790 10 790
sh $G ssh 'for i in $(seq 15); do grep -q "display-probe: PASS" /tmp/display.out && break; sleep 1; done; cat /tmp/display.out; chvt 1'
sh $G screenshot $PWD/build/keiland-linux/p005-console.png
```

（screenshot の点の座標は、最初の PNG で `python3 plan/tools/keiland-linux/png-probe.py --size <PNG>` を見て、画面の中に取る。上の例は 1280×800 の物。）

続けて、acquire しない形（`display-probe` を `--acquire` 無しで、同じ手順）と vkdemo（`openvt -c 7 -s -- sh -c "/opt/keiland/bin/vkdemo > /tmp/vkdemo.out 2>&1"`。vkdemo の引数と
終わり方は `userland/desktop/vkdemo/main.c` を読み、数 frame で終わる引数か `timeout 20` で止める）。最後に `sh $G stop`。

## 完了の条件

1. build（gcc・clang）、`elf-check: PASS`、`makefile-sync: PASS`、host の p003・p004 の試験が今も PASS。
2. guest: `display-probe --acquire` の 3 色が screenshot の 4 点（隅 3 つと中央）で `#ff0000`・`#00ff00`・`#0000ff`（PNG をユーザーに見せる）。`display-probe: PASS`。design §10 の V4。
3. guest: `--acquire` 無しでも 2 と同じ（libvulkan-compat が自分で master を取る道）。
4. guest: vkdemo の screenshot の中央の画素が黒（`#000000`）でも console の色でもない（描画が出た）。PNG をユーザーに見せる。
5. 終わった後に `chvt 1` の screenshot に console の文字が出る（CRTC を戻せている）。
6. master を失う確かめ: `display-probe` の途中で SSH から `chvt 1` → 5 秒 → `chvt 7`。root の KMS の master は VT の切り替えで失われないことがある。失われたら `VK_ERROR_OUT_OF_DATE_KHR` で
   `display-probe` が error で終わり固まらないこと、失われなければそう記録する。

## q528 の結果（当時の clearance）

cleared。VK_KHR_display・direct-mode・DRM acquisition の O 10 entry、KMS inquiry/dup master/saved CRTC/double dumb FIFO copy、Linux vkdemo を実装した。

- gcc 14.2 / clang 19.1.7: final build exit0、warning0。elf-check PASS（10 ELF、probe を含む）、makefile-sync PASS、header-check PASS（70 sources）。clang-format19＋定義の改行、scoped style-check（実装・probe）0件。新規 KMS source と変更部分を全文規約で手動点検した。最終全 WS conformance は p011。
- host は DRM=none、display 環境無し。vk-chain-test PASS（API1.0・llvmpipe・1MiB全word一致）、interpose PASS（backend→compat binding0・opt-out）、Wayland FIFO/fallback/resize/MAILBOX 各90 frame、360色/extent/import/private wait PASS。
- guest kernel6.12.107+deb13-amd64、Mesa25.0.7、lavapipe。seat fd duplicate（元fdをclose）とdirectの2経路で Virtual-1 / 1280×800 / 74994mHz。赤・緑・青全6PNG各4点一致、probe exit0/PASS。赤→緑で live oldSwapchain を更新・破棄し、master所有権の継承を確認した。
- vkdemo --time-ms=1000 --hold=10: 320×240、中心 #20c5b0、描画PNGを表示、VKDEMO DONE frames=1。各経路の終了後と最終chvt1はconsole文字のPNGを確認・表示。
- 途中chvt1→5秒→chvt7: direct/root はmasterを失わず、3色を完走してPASS。実際のlogind revoke/OUT_OF_DATE はp009で確認する。ioctlのEACCES/EPERM→OUT_OF_DATEと100ms上限はsourceで確認。
- 完了後 guest stop、overlay廃棄。host package追加0、target toolchain/common zedBSD source変更0。実機GPU・物理monitorのcustom mode・Valgrindは未実施。問合せ/display/mode handleはprocess-lifetime、実機hotplugの動的再列挙は範囲外。
- [ログ・PNG・sha256 manifest](../../history/ws105/q528/evidence/)。不具合残件なし。次はp006（root compositor・wl_shm・入力・VT）、前提WS104とp005を確認。


実装 commit: `18a983dd30b2586f56700113f2a82add518652a5`（WIP）。終了 UTC: 2026-10-01T08:13:06.450181+00:00。GitHub は未公開、Phase / WS event と intended close は outbox に保持。

## 再開設計（q529 の findings、2026-10-01）

p006 の連続描画で OUT_OF_DATE による終了（frames6051、q529 source80eea509）。100ms総期限でflip待ちを打ち切る現実装は、design §4.9 の「poll 1回の上限100ms」を狭く読みすぎた。q528の短い3色試験の結果は保持し、p005の現clearanceをinvalidated/unclearedとする。

main（Q1）が既存WS105完了の委任された技術判断で次を修正する: 各poll≤100ms、単発timeoutはretry、総期限5秒。EACCES/EPERM・revoked fd は即OUT_OF_DATE、総期限のevent未完了はSURFACE_LOST（master喪失と同一視しない）。影響sourceはkms.c、callback/句の規約を点検。productionのtest-only switchは足さない。

検証: gcc/clang・ELF/source/header・host chain/Wayland 回帰、guestのfd/direct 3色。test-only preload observerがpollを1回だけ250ms遅らせて0を返し、2回目から実際のpollへ渡す。両経路で実frame完了/3色とoldSwapchainがPASSすること。sourceの各poll上限100msと総期限5sも点検する。このfixtureは試験の共有ライブラリのみで既定production codeを試す。p006の再開はこの修正・検証のclearanceが前提。

[origin p006](../phase006/phase.md)、[q529 history](../../history/queue-q529.md)。過去のq528 scopeと結果は変更しない。KMSの実機非同期・logind revocationはp009/p011の既存確認へ。

## 今回の結果

cleared（q530-i01）。KMS completionのpollを各100ms以下、総期限5秒にした。単発timeout/EINTRはretry、総期限未完了はSURFACE_LOST、EACCES/EPERMとrevoked fdはOUT_OF_DATEを維持した。productionの試験用switchは無し。

- 旧staged implementationにtest-only flip-delay.soで1回250ms遅延＋poll result0を与え、赤→緑の後にFAIL result=-1000001004を再現した。修正後のseat fd/direct両経路は同じinjection (`requested_ms=100 delayed_ms=250 result=0`) 後に青を表示しPASS/exit0。全6色PNG各4点一致、oldSwapchain破棄後も表示、console復元、guest stop。
- gcc14.2/clang19.1.7 build exit0 warning0、12ELF/source-sync/126header PASS（p006の既存Linuxprogramを含む）。style-check0、clangformat19、変更KMS節の全文規約を点検。全WSの最終規約はp011で実施。
- host DRM=none: chain1MiB/PASS、interposebindings0/optoutPASS、Wayland FIFO/fallback/resize/MAILBOX360frameの画素/extent/import/privatewait PASS。
- q528のvkdemoとrootVT切替の結果は保持。今回変更はその単発SETCRTCの経路に影響しない。logind/revoked fdの実動作は既存p009で確認。実機GPU/物理monitor/Valgrind未実施。
- [新旧ログ・PNG・manifest](../../history/ws105/q530/evidence/)。p005のclearanceを復旧し、p006の同じ受け入れを次のQueueで再開する。q529のunclearedは履歴として保持。


実装 commit: `753b45a0fae9ba22d0ef6d0a7fa0f8a4698b951d`（WIP）。終了 UTC: 2026-10-01T08:31:05.223429+00:00。GitHub は未公開、Phase / WS event と intended close は outbox に保持。

## q534 後の bounded 修復設計

q534 の gdm 起動で kms.c の無条件 SET_MASTER が logind 共有fdに errno13 を返した。p005の「seatが渡すmaster fd」という出力を未達と判断してcurrent clearanceをinvalidated / uncleared（q528/q530は当時の結果を保持）。Q1のWS105完了までの委任された技術判断で、caller supplied fdはAUTH_MAGIC magic0の非破壊probe（current masterだけEINVAL）で検証し、SET_MASTER/DROP_MASTERはlibrary自身のdirect master取得だけに限定する。libraryはsupplied fdのdupとCRTCの復元だけを所有し、masterの制御はseat/logindへ返す。compositorにDRM ioctlは追加しない。影響はkms.cとprivate compat_displayのownership field。product/API/受け入れ目標は不変。

再検証: gcc/clang warning0・ELF/source/header・host chain/interpose/Wayland。guest rootのseat fd/direct 3色、oldSwapchain・console復元。q534のLinux logind sourceをfixture contextとして一般user gdm起動が表示できることを確認（p009のapp/VT/LogOut受け入れは次attempt）。偽の非masterfdはacquireで拒否し、callerfdはcloseされない。p009はp008と修復p005を依存として、同じ全基準で再実行。p006〜p008のroot経路で検証した受け入れは維持し、p011でfinalsourceを再確認。

[origin p009](../phase009/phase.md)、[q534](../../history/queue-q534.md)。

## q535 修復の結果

（実行の後に書く）
