# GTK 4.18.6 の Vulkan renderer と zedBSD の Vulkan の差（2026-10-02 調査）

ユーザーの問い（2026-10-02）:「描画の順番：Cairo → Vulkanにできますか？ Vulkan 1.3にしたいのですが、ブロッキング要素があるかチェックしてください。カーネルの機能が足りないのか、libvulkanのAPIが足りないだけなのか。」
読み取り専用の調査（source 不変）。GTK の展開と GSK の SPIR-V は host の一時領域で確認した。

## 結論

- GTK 4.18.6 は build で `vulkan >= 1.3`（vulkan.pc）、実行で `vkCreateInstance` の `apiVersion = VK_API_VERSION_1_3` を要求するが、実際に使う API は Vulkan 1.0 の render pass の描画と 1.1 core の query・alias だけ（dynamic rendering・sync2・timeline・descriptor indexing などは使わない。shader も vulkan1.0）。根拠: `meson.build:34,670-673`、`gdk/gdkvulkancontext.c:302,611-664,1586-1636,1775`、`gsk/gpu/gskvulkandevice.c:137-180`。
- **Venus**: kernel・UAPI・HAL の不足は無い。足りないのは header（`include/libc/vulkan/vulkan_core.h` は 1.3.269 から選んだ部分だけ、`VK_API_VERSION_1_1..1_3` や Vulkan11/12Features などが無い）、`vulkan.pc`、instance の版（`userland/desktop/libvulkan/instance.c:58-65` が 1.0 以外を INCOMPATIBLE_DRIVER で拒否 → GTK は必ずここで落ちる）、1.1 core の symbol（vkBindImageMemory2、vkGetPhysicalDeviceExternalSemaphoreProperties、Ycbcr、core 名の alias、vkEnumerateInstanceVersion）。分類は header/API の追加（a）と libvulkan の実装（b）だけ。
- **i915 native**: 上に加えて kernel の SPIR-V→GEN compiler（`src/drivers/gpu/i915/compiler/spirv.c`）が GSK の shader の OpFunctionCall・OpFunctionParameter・OpReturnValue・OpSpecConstant・OpSpecConstantOp・OpAny を受け付けない（分類 c）。HAL・UAPI の変更は要らない。
- GL は build から外せない（libepoxy・wayland-egl は GTK の必須の依存）。実行時に `GDK_DISABLE=gl`・`GSK_RENDERER=vulkan` で Cairo → Vulkan にできる。Wayland では自動選択でも Vulkan が GL より先。dmabuf の zero-copy は無く、提示は libvulkan の WSI。
- libvulkan 自体が本当の Vulkan 1.2/1.3 を名乗るのは GTK には不要で、別の大きな作業（Venus の codec に約 50 の構造体・command、timeline semaphore の host wait は kernel の fence が binary（`include/uapi/gpu-fence.h:20-31`）なので emulation か UAPI 変更の調査、i915 native は executor と compiler の大きな実装）。

## 推奨の Phase 案

1. header を pinned 1.3.269 から広げ、`vulkan.pc` を足す（a、libc/GPU 境界なので main の判断）。
2. libvulkan の instance を 1.1 の実装に（apiVersion 1.x の受理、vkEnumerateInstanceVersion、1.1 core の約 28 command を alias・emulation・最小実装で。physical device の apiVersion は 1.0 のまま正直に報告）。
3. WS115: `-Dvulkan=enabled` で build し、QEMU の Venus で `GSK_RENDERER=vulkan GDK_DISABLE=gl` の gtk4-demo を確認（Cairo の後）。
4. i915 の compiler: GSK の 34 個の SPIR-V を host の corpus にし、function call の inline、spec constant、OpAny を実装、5330 で確認。
5. （任意・別 WS）libvulkan に本当の 1.2/1.3 を名乗らせる。
