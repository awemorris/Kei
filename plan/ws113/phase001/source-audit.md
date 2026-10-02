# ws113-p001 / source・一次仕様の能力調査

調査日: 2026-10-02 UTC。Queue: q586-i01 / A3。Source baseline: `0e68854ac`（`codex/a3-display`）。読取調査だけで、driver/API/source変更、実機占有、SSH、host変更、build、QEMU起動は行っていない。

## 実装済みの能力と欠落

| 層 / source | 実際の契約 | WS113で必要な追加 / 限界 |
| --- | --- | --- |
| `include/uapi/gpu-display.h:44` | `GPU_DISPLAY_EVENTS`は非破壊QUERY→inventory再列挙→exact sequence ACK。sequence初期値1、open description単位、dup/SCM_RIGHTSは共有。sequenceとoutput generationは別 | 入力ABIをそのまま活かせる。通知が実際に増え、pollを起こす下層が必要 |
| `src/drivers/gpu/gpu.c:1805` | QUERY copyout成功後だけobserved更新。ACKはそのopenが観測したsequence以下。新イベントはPOLLPRIを保つ。cap/read権限/flags/予約欄を検査 | i915の通知publication、coalescing、同時ACKの独立client試験が必要 |
| `src/drivers/gpu/i915/display/hotplug.c:990` | `drv_i915_hpd_events`は常に1を返す。1351/1362のconnector/device hotplug helperはlogだけ | HPD workerによるcoherent inventory変更→sequence増加→poll wakeが未実装 |
| `src/drivers/gpu/i915/display/display.c:2496` | queryは0または1 output。ID=1/generation=1、1 plane、接続flag。modeは既定の1 timing、claimは単一lease | connector別ID/epoch、複数lease、mode/pipe/clock/DBUF競合の実検証が必要 |
| `src/drivers/gpu/i915/display/output.c:202` | 起動時にeDPかHDMIを選ぶ。HDMI DDI B/pipe B/DVI、eDPは暗い。後のhotplugを追わない。名前は`HDMI`/`eDP panel` | boot時選択を複数connectorのinventory/実出力へ一般化。既存bootパラメータとの優先規約を確定する必要 |
| `src/drivers/gpu/i915/display/present.c:10` | resident出力/scanout/workerは単一。FIFOのflipとlease引継ぎの最後の画像保持。source自身がhotplugなしと明記 | per-output worker/scanout lifetime、disconnect drain、複数pipe同時動作が未実装 |
| `src/drivers/gpu/i915/display/capture.c:10` | capture buildはpanelを点灯せず仮想1 output/1920×1080/固定epoch。vblankなし | capture成功は実HPD・同時物理scanout・refresh/first pixel・実LCDの証拠にならない |
| `src/drivers/gpu/gpu.c:615` | device identityは`gpu_handle_allocate`の非再利用handle | 登録寿命の識別には使える。再起動に跨がるハードウェアIDとは確認できない |
| `userland/desktop/libvulkan/wsi-display-nodes.c:59` | instance毎のimmutable native-node inventory、renderer pairing、(device_id, display_id)でordinal混同を防止。output countsは動的QUERY | connector hotplugには既存nodeで追従可能。新GPU node追加/再登録はinstance再構築の別問題 |
| `userland/desktop/libvulkan/wsi.c:50` | 接続済みdisplayだけ列挙、同native keyはinstance寿命の同handle。modeはgeneration別、古いmode identityは保持 | handleをpersistしない。mode/surface再作成、enumeration中のcount不変の入替も検出するevent前後確認が必要 |
| `userland/desktop/libvulkan/wsi-display.c:1515` | swapchain進行のPOLLPRI→event QUERY→surface検証→ACK。driver ioctlはlibvulkan内にある | active lease/swapchainの無い0台・idleでも通知するdevice event sourceが必要。旧surface失効時ACKしない経路と新watcherを混同しない |
| `userland/desktop/libvulkan/sync.c:174`, `sync-internal.h:21` | ordinary/native/software/external fence payloadあり | display event専用payloadをstatus/wait/destroy/reset/teardown全てへ統合する必要 |
| `userland/desktop/libvulkan/instance.c:429`, `internal.h:237`、`api-commands.tsv`、`include/libc/vulkan/vulkan_core.h` | KHR_display/KHR_display_swapchainは登録済み。EXT_display_control、EXT_display_surface_counterのextension名/struct定義/prototype/entry/広告を非検出。汎用VkStructureType値は579–583に存在する（初回「enumも無し」を訂正） | pinned registry宣言の生成手順、instance/device依存、有効時のproc-addr、全command実装をp003で検査。enum値だけを対応証拠としない。全source生成の承認は本Queueに含まれない |
| `userland/desktop/libvulkan/queue.c:108`, `wsi-swapchain.c:2792`、i915 `present.c:393` | QueueWaitIdleはaccepted present jobをdrainするがcurrent front bufferを除外。i915はPRESENTでflip arm、WAITで最新latchを観測 | enqueue/render fence/idleを実display完了へ代用しない。present_id/waitのstruct定義/runtimeは未検出（汎用enum値は存在） |
| `userland/desktop/libvulkan/external-properties.c:320`, i915 `render/instance.c:83`, `render/dispatch.c:179` | properties2KHR/ID wrapperは存在、native opcode148でUUID取得。i915 opcode switchに148無し、未実装builtinはENOTSUP/reader poison。wrapper失敗時はID出力0 | standard GPU UUIDとport keyを組む案は、i915の実query能力を追加/検証して初めて利用可能。GPU UUID自体はconnectorを識別しない |
| `userland/desktop/wayland/compose.c:754`, `zwl.h:595` | 最初の表示可能な1 displayを選択。0台は初期化失敗。serverは1 compose/width/height/refresh | 出力table・0台の待機・出力毎swapchain/scene clipping/damage/input/Wayland outputが必要 |
| `userland/desktop/wayland/protocol.c:51` | 固定registryの単一`wl_output`、独自keyboard-inset managerの例がある | dynamic output global/add/remove、snapshot-batched display managerが必要 |
| `userland/desktop/libkeiland/keyboard-inset.c:67` | public wrapperが独自managerをbindし、未対応はENOTSUP、callbackはapplication queue | display query/apply/通知もpublic wrapperで包む。Settingsからdriver/Vulkanを直接操作しない |
| `userland/desktop/settings/page-look.c:183` | Displayはread-only mode/100%/graphicsを描くstub | WS113で後続拡張。WS089の既往stub clearanceは変更しない |
| `userland/desktop/libkeiland/preferences.c:748` | 同directory tempへ書きflush/renameして保存する既存手順 | displays専用保存のauthorityと適用/保存の結果を分離。新geometry keyを旧desktop.conf pollingへ混ぜない |

検索は`rg -n`で対象source/header全域を調査し、関係functionを`sed -n`/`cat`で読んだ。検索非検出はruntime未対応の独立client証明ではない。lineはbaselineに対する参照で、後続source改訂で再確認する。

## 一次仕様（2026-10-02確認）

以下はKhronos Vulkan Documentation Projectの`latest` referenceを実際に取得した。固定版ではなく、p003実装選定時にはchecked-in registry revisionとの宣言/validity差を再確認する。標準CTS合格は今回主張しない。

| 一次URL | 読み取った規約 / 設計への影響 |
| --- | --- |
| [VK_EXT_display_control](https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_display_control.html) | device extension revision 1、VK_EXT_display_surface_counterとVK_KHR_swapchainを要する。4 commands: power、swapchain counter、device event、display event。hotplugだけ実装して全extension対応と広告しない |
| [vkRegisterDeviceEventEXT](https://docs.vulkan.org/refpages/latest/refpages/source/vkRegisterDeviceEventEXT.html) / [VkDeviceEventTypeEXT](https://docs.vulkan.org/refpages/latest/refpages/source/VkDeviceEventTypeEXT.html) | 新しいVkFenceを返し、DISPLAY_HOTPLUGはplug/unplug時にsignalして再列挙を促す。device/info/allocator/output pointerのvalidityを守る。poll callbackやoutput IDを公開するAPIではない |
| [VK_EXT_display_surface_counter](https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_display_surface_counter.html) / [VkSurfaceCapabilities2EXT](https://docs.vulkan.org/refpages/latest/refpages/source/VkSurfaceCapabilities2EXT.html) | instance extension revision 1、KHR_display依存。surface counter能力を別照会し、非display surfaceにVBLANKを広告しない。実vblank能力なしでcounterを捏造しない |
| [vkGetSwapchainCounterEXT](https://docs.vulkan.org/refpages/latest/refpages/source/vkGetSwapchainCounterEXT.html) | present engineが少なくとも1 presentを処理したswapchainに対する照会。out of dateでcounter不可ならOUT_OF_DATE_KHRを許す |
| [vkRegisterDisplayEventEXT](https://docs.vulkan.org/refpages/latest/refpages/source/vkRegisterDisplayEventEXT.html) | display/deviceの同physical parentが必要。FIRST_PIXEL_OUTは次refreshのfirst pixel。render completion/present submitと同じ意味にはしない |
| [vkDisplayPowerControlEXT](https://docs.vulkan.org/refpages/latest/refpages/source/vkDisplayPowerControlEXT.html) | valid displayのpowerを設定。同physical parent。成功は実power状態の変更を伴う。未実装をVK_SUCCESSにしない |
| [VK_KHR_display](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_display.html) / [VkDisplayPropertiesKHR](https://docs.vulkan.org/refpages/latest/refpages/source/VkDisplayPropertiesKHR.html) | display/built-in modeはinstance寿命、planeはphysical device全体のindex。displayNameはNULL可のUTF-8名称で、同instance中は不変。再起動永続ID・固有名称・EDID raw dataの保証はない |
| [vkResetFences](https://docs.vulkan.org/refpages/latest/refpages/source/vkResetFences.html) | resetはunsignalであって新たなdisplay event登録ではない。pending queue fenceはreset不可、host accessはexternal synchronization |

extension full coverage、永続IDのVulkan伝達規約、現在利用可能な実機fixtureは[契約](contracts.md)/[詳細比較](identity-completion.md)/[検証fixture](fixtures.md)で具体化する。未知の能力を実装済みと扱わない。表は初回18行、追加照合後20行。
