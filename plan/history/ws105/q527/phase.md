<!-- awesome-plan project=zedbsd record=ws105-p004 -->

# ws105-p004: libvulkan-compat (2): Wayland の WSI（`zwp_linux_dmabuf_v1`、implicit sync）

Status: cleared
Disposition: normal
Parent: [WS105](../../../ws105/ws.md)
Queue: q527 / q527-i01
依存: p003
実行者: phase-runner（high）。**始める前に [design.md](../../../ws105/design.md) の §4（特に §4.4〜§4.8・§4.11）を読む**

## 目的

libvulkan-compat に Wayland の WSI を足す（決定 D6）: `VK_KHR_surface`・`VK_KHR_wayland_surface`・`VK_KHR_swapchain`。swapchain の image は後段で dma-buf を export できる形で作り、
`zwp_linux_dmabuf_v1`（version 3）で compositor に送る。fence は implicit sync（dma-buf に sync_file を付ける）。この Phase の試験の相手は host の上の試験用の Wayland server
（`dmabuf-probe`）で、我々の compositor（p007）より先に WSI を確かめる。

## 手本（[survey/](../q538/survey/README.md)）

- `survey/vkexp.c`: lavapipe での modifier の image の作成・dma-buf の export・`vkGetImageSubresourceLayout(MEMORY_PLANE_0)`・mmap・別の device での import（全て通った）。
- `survey/vksync.c`: SYNC_FD の semaphore の export・import、`DMA_BUF_IOCTL_IMPORT_SYNC_FILE`・`EXPORT_SYNC_FILE`（全て通った）。
- `survey/wsi-client.c`: system の libwayland と lavapipe で dma-buf の image を `create_immed` で送る client（我々の WSI の最小の形）。
- `survey/dmabuf-probe.c`: 試験用の Wayland server（そのまま `plan/tools/keiland-linux/dmabuf-probe.c` の出発点にする）。
- zedBSD の `userland/desktop/libvulkan/wsi-wayland.c`: event queue・wrapper・frame callback・release の扱い（読むだけ、複写しない、D7）。

## 作る・変える file（`userland/desktop/libvulkan-compat/`）

| file | 中身 |
| --- | --- |
| `functions.tsv` | design §4.4 の O の表の `VK_KHR_surface`・`VK_KHR_wayland_surface`・`VK_KHR_swapchain`（device group の 3 つと `vkAcquireNextImage2KHR` を含む）の行を `kind=O`・`export=y` にする。それ以外の WSI の行は N のまま |
| `instance.c` | 我々の instance の拡張（`VK_KHR_surface`・`VK_KHR_wayland_surface`）を一覧に足す。`vkCreateInstance` で app の WSI の拡張を後段に渡さず、app の apiVersion が 1.1 未満なら `VK_KHR_get_physical_device_properties2`・`VK_KHR_external_memory_capabilities`・`VK_KHR_external_semaphore_capabilities`・`VK_KHR_external_fence_capabilities` を足す（後段が持つ物だけ）。instance の関数の表（design §4.5） |
| `device.c` | WSI の道の判定（design §4.6 の 2 つの表）、`VK_KHR_swapchain` を一覧に足す（道が「無し」でなければ）、`vkCreateDevice` の拡張の足し方（design §4.6 の 1〜3: 後段の `VK_KHR_swapchain` を有効にする、1.1 未満の版の KHR の拡張、`VK_EXT_queue_family_foreign`）、device の関数の表 |
| `dispatch.c` | O の関数の名前を `vkGet*ProcAddr` で返す規則（design §4.7） |
| `wsi-wayland.c` | surface の作成（registry、`zwp_linux_dmabuf_v1` を version `min(server, 3)` で bind、roundtrip、`modifier` の event から (format, modifier) の組を集める）・破棄、surface の問い合わせの 5 つと `vkGetPhysicalDevicePresentRectanglesKHR`・`vkGetPhysicalDeviceWaylandPresentationSupportKHR`、時間の上限つきの待ち（design §4.8 の「待つ時の形」） |
| `wsi-swapchain.c` | swapchain（image の作成・dma-buf の export と `FD_CLOEXEC`・`wl_buffer` の作成）、acquire・present（implicit sync と予備の道、FIFO の 100 ms の上限）、release、`oldSwapchain`、device group の 3 関数、`vkAcquireNextImage2KHR` |
| `linux-dmabuf-v1-protocol.c`・`linux-dmabuf-v1-client-protocol.h` | `zwp_linux_dmabuf_v1`・`zwp_linux_buffer_params_v1` の `wl_interface` の表と inline の request の関数（version 3 まで）。我々の libwayland の手書きの形（`userland/desktop/libwayland/*-protocol.c` と `zed-*-client-protocol.h`）に合わせる |
| `Makefile.linux` | 依存（`$(4)`）に `libwayland-client.so`、source に `wsi-wayland.c`・`wsi-swapchain.c`・`linux-dmabuf-v1-protocol.c` |

細部:

- swapchain の image の数: `minImageCount` 以上で、FIFO なら `max(minImageCount, 3)`、MAILBOX なら `max(minImageCount, 4)`。
- present で `VkPresentInfoKHR.pResults` に結果を書く。
- 同じ swapchain は同じ thread から呼ばれる前提でよい（Vulkan の規格: swapchain は外部で同期）。instance・device の記録の表は mutex で守る。
- 失敗の値: surface の compositor が `zwp_linux_dmabuf_v1`（v3 以上）を持たない → `vkCreateSwapchainKHR` が `VK_ERROR_SURFACE_LOST_KHR`、stderr に 1 行。compositor が切れた → `VK_ERROR_SURFACE_LOST_KHR`。
- 予備の道は production の能力不足・sync_file ioctl の失敗で選ぶ。試験専用の production 環境変数は作らない（coding-style.md §12）。試験の LD_PRELOAD shim が DMA_BUF_IOCTL_IMPORT_SYNC_FILE だけに ENOTTY を返し、既定の fallback を実行する。

## 試験の道具（`plan/tools/keiland-linux/`、main が merge）

| file | 中身 |
| --- | --- |
| `dmabuf-probe.c` | design §7.3（`survey/dmabuf-probe.c` から）。`PROBE frame=N pixel=0xAARRGGBB fences=F waited_ms=M` と `PROBE RESULT frames=N`、`--frames N`・`--socket PATH`・`--timeout S`・`--size-log`（frame の幅・高さも出す） |
| `wsi-probe-client.c` | 我々の libwayland-client と libvulkan-compat で、`--frames N` 回 clear して present する client。色は frame の番号で `0xffff0000`・`0xff00ff00`・`0xff0000ff` を巡る（B8G8R8A8 の memory の並びで正しく）。`--resize`（30 frame ごとに 320×240 と 400×300 を替えて swapchain を作り直す）、`--mailbox`、`--timeout S`。終わりに `wsi-probe-client: PASS` |

## 手順

host の独立試験。build / install と `elf-check.sh`・`makefile-sync.sh`・`header-check.sh`、`vk-chain-test`・`interpose-check.sh` に加え、
`timeout 120 bash plan/tools/keiland-linux/wsi-check.sh`。この reusable script が test server/client と observer の 2 版を build し、FIFO/fallback/resize/MAILBOX を各90 frame実行する。
通常 run は sync-observe.so、fallback は compile 時 ENOTTY 指定の sync-unavailable.so を client だけに preload。kernel import flags/result と backend private wait result を全 frame分 assert。
通常の private wait は acquire/reuse の180回、fallback は加えて CPU-before-commit の90回、合計270回。後段の DEEPBIND は維持。
raw fence 数を正規化しない。試験生成 server protocol は build/test/gen のみ、production は自前の手書き stub。各 process90秒、内部deadline60秒。

## 完了の条件

1. build（gcc・clang）、`elf-check: PASS`、`makefile-sync: PASS`、`header-check: PASS`、`vk-chain-test: PASS`（(4) の拡張の確かめは「`VK_KHR_wayland_surface`・`VK_KHR_surface` が**有り**、
   `VK_KHR_xcb_surface` が無い」に直す）、`interpose-check: PASS`。
2. `fifo`: probe が 90 frame を受け（`grep -c` が 90）、各 frame の pixel が client の色の列と一致（frame N の色が `N % 3` の色）、試験 observer の IMPORT_SYNC_FILE が全 90 回 flags=WRITE で成功（kernel への payload の受け渡し）。raw `fences=` と driver/timeline を保存し、stub の数だけから受け渡しを推定しない、client が `wsi-probe-client: PASS`、両方 exit 0。
3. `fallback`（試験 shim の IMPORT_SYNC_FILE の ENOTTY、production の既定の fallback）: 2 と同じ画素/サイズ。IMPORT_SYNC_FILE が最初の 1 回 ENOTTY、以後 import の試行 0 回、CPU fence 待ちが全 90 frame で成功。raw fence 数は kernel の stub を含めそのまま記録し、0 という仮定は使わない。
4. `resize`: 大きさの違う frame（320×240 と 400×300）が probe に届き、client が error なく終わる。
5. `mailbox`: 90 frame。
6. client の process が 90 秒以内に終わる（`timeout` の 124 でない）。`valgrind` が host にあれば `valgrind --leak-check=full --errors-for-leak-kinds=definite` で
   我々の file（`libvulkan.so.1`・`libwayland-client.so`）の definitely lost が 0。無ければ「未実施」。
7. design §10 の V3・V9 の結果（lavapipe の modifier の一覧、`fences=` の値）を記録。

## 結果

- **uncleared**: 4 種の client/server がともに exit 0、各 90 frame。FIFO・resize・MAILBOX の全画素/サイズ一致。fallback の画素/サイズは一致したが、承認済み `fences=0` は不成立。
- host kernel `6.12.101+deb13-amd64`、lavapipe。全 run の raw fences=1、driver/timeline=`stub`、status=1、waited_ms=0。既に完了した writer と空の reservation をこの観測だけでは区別できない。fences≥1 だけによる通常経路の受け入れも根拠が不足する。
- Linux 6.12 の一次資料: [dma-buf.c](https://raw.githubusercontent.com/torvalds/linux/v6.12/drivers/dma-buf/dma-buf.c) の dma_buf_export_sync_file は空の reservation に dma_fence_get_stub を補う。[dma-fence.c](https://raw.githubusercontent.com/torvalds/linux/v6.12/drivers/dma-buf/dma-fence.c) の stub は既に signal 済み。
- gcc の Linux WSI build warning 0。4 run の [観測](../q526/evidence) を保存。clang/全規約/chain 回帰はこの試行では未実施。既存コードの partial resource cleanup と通常 acquire の初期 queue 記録を次試行で見直す。
- 再開条件: 試験だけの ioctl observer で通常 IMPORT_SYNC_FILE の成功/flags=WRITE、fallback ENOTTY 1 回・以後成功 import 0 回、CPU fence 待ち・90 frame 全画素を実測する。raw fence/timeline は加工せず残す。D6 の production 同期方式、L3 の画素/安全性、他 Phase の出力は変更しない。main の委任済み技術判断で検証の観測方法を改訂し、新 Queue snapshot に残す。
- valgrind は host に無く未実施。実機 GPU の非同期 fence 待ちは未実施、lavapipe のみ。push/GitHub 公開なし。


実装 commit: `e4588580404893bdc6f0affd33d68c064ecdeaa3`（WIP）。終了 UTC: 2026-10-01T07:22:11.469836+00:00。GitHub は未公開、Phase / WS event と intended close は outbox に保持。

## 検証手順の補い（2026-10-01、q526 の選定前）

main が全文規約 §12 と手順を照合し、未実装の試験専用 production switch を、試験専用 syscall shim の ENOTTY に置き換えた。D6 の implicit sync / CPU fallback と、90 frame・pixel 一致・fallback fences=0 の受け入れは同じ。外部の product 機能・他の Phase の約束・依存に変更無し。試験の shim は plan/tools/keiland-linux/ に置く。

## q526 後の検証改訂（2026-10-01）

原因: kernel が空/完了済みの reservation を stub にするため raw fence 数は正常と fallback を区別しない。main の委任済み技術判断で、試験専用 observer の ioctl 成功/失敗・CPU fence wait 実行と実画素を照合する。D6 の実装・L3 の安全な呈示・90 frame workload は維持。新 Queue はこの改訂した Phase の snapshot を境界とする。production に試験 switch を追加しない。

通常用 sync-observe.so と fallback 用 sync-unavailable.so は同じ試験 source の compile 時指定。observer は IMPORT_SYNC_FILE の flags/result と vkWaitForFences の result を stderr に記録し、成功数/失敗数を run ごとに厳密に assert する。後段の RTLD_DEEPBIND を外さない。raw fence の stub 情報・確認できない実機 GPU の非同期待ちは制限として残す。

## q527 結果

- **cleared**。q526 の旧条件が不成立だった履歴を保持し、kernel の stub に依存しない改訂検証を実施した。product API / D6 の implicit sync と CPU fallback は同じ。
- gcc 14.2.0 / clang 19.1.7 の最終 source build warning 0。`elf-check: PASS`（8 ELF）、`makefile-sync: PASS`、`header-check: PASS`（64 source）。`git diff --check` PASS。libvulkan-compat 全 C/header、変更した試験 C と generated forward.inc の style-check 合計0、ANSI 宣言・public/static順・callback storage寿命・fd ownership・error unwindを全文規約で照合。
- `timeout 120 bash plan/tools/keiland-linux/wsi-check.sh`: FIFO/fallback/resize/MAILBOX は各90 frame、client/server exit0。全360 frame の実画素がN%3の赤/緑/青と一致。resizeは320×240→400×300→320×240、MAILBOXも90frame。
- 通常3 run: IMPORT_SYNC_FILE flags=WRITE(2) がそれぞれ90回成功。private waitは各180回（acquire/reuse）、全て成功。fallback: ENOTTY(25)は最初の1回のみ、以後import試行無し。private waitは270回、CPU-before-commitが全90frameに加わり全て成功。observerは試験専用、backend DEEPBINDとproduction引数/戻り値を維持。
- raw SYNC_IOC_FILE_INFO は全runでfences=1、driver/timeline=stub、status=1、waited_ms=0。値を加工して0にせず保存。この値だけをimplicit sync成功の根拠にしない。V3: lavapipeの実際のcreated modifier=0x0、single plane、stride=1280/1600、offset=0を確認。V9はkernel importの成功と実画素で検証。
- `vk-chain-test: PASS`: staged SONAME、surface/wayland拡張有り、XCB/Xlib無し、未enableのWayland procedure=NULL、API1.0、llvmpipe、1MiB fill/copy一致。`interpose-check: PASS`、default backend-to-compat bindings=0、NO_DEEPBIND optoutもPASS。
- `make -j4 disk-image` exit0、warning0。Linux固有library/testだけの変更のためzedBSD runtime回帰はp011の全体回帰で実施。host package追加・target toolchain変更・host /opt install無し。
- swapchain destroy時にGPU資源を先に退役し、未releaseのWayland callback storageだけをsurfaceで保持する。deviceが先に破棄されてもcallback dataが残る。初回acquireより前のapplication queue retrievalを必須にしない。
- [raw evidence](evidence) と [ELF manifest](manifest.sha256)を保存。valgrindはhostに無く未実施。実機GPUの非同期待ちは未実施、host lavapipeのみ。GitHub publication/remote closeはdeferred、outboxで保持。commit WIP、push無し。


実装 commit: `cb6a9eacf1dac1a1f6381809ba102558ffc41467`（WIP）。終了 UTC: 2026-10-01T07:35:44.278649+00:00。GitHub は未公開、Phase / WS event と intended close は outbox に保持。


2026-10-01 WS105完了時の所在地更新: 元attemptのStatus/結果/承認は不変。現在のPhase所在地は[WS105履歴索引](../index.md)。approved scope snapshotは改変していない。
