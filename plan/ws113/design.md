# WS113: 現状調査・実装順・検証設計

Status: planned。2026-10-02読取調査のみ。実装/driver変更/実機試験は未実施。

## 現状の根拠

- `src/drivers/gpu/i915/display/hotplug.c`にはHPD処理があるが、`i915_hpd_drm_kms_helper_*hotplug_event()`は現在logのみでuserspace listenerが無い。
- `include/uapi/gpu-display.h`には`GPU_DISPLAY_EVENTS`とsequence/ACKの契約がある。`userland/desktop/libvulkan/wsi-display.c`はswapchainの進行時にPOLLPRI→QUERY→inventory→ACKを行う。既存のイベント経路がそのままVulkanのhotplug fenceを提供するとは確認されていない。
- `userland/desktop/libvulkan/wsi.c`は`vkGetPhysicalDeviceDisplayPropertiesKHR`で接続済みdisplayの列挙と安定handleを扱う。Vulkan header/API tableに標準Display APIがある一方、`VK_EXT_display_control`/`vkRegisterDeviceEventEXT`の実装は今回の調査で見つからなかった。
- `userland/desktop/wayland/compose.c`のwindow modeは最初の表示可能な1 displayを選び、`zwl.h`も単一composeを所有する。複数出力のsurface/swapchain/座標/presentは未実装。
- `userland/desktop/settings/page-look.c`のDisplayページは読み取り専用の案内、[WS089](../ws089/ws.md)でstubにする旧決定。`libkeiland`には独自Wayland拡張を包む例（keyboard-inset等）がある。

これらはsourceの現状であり、新しいWSが既に動作している根拠ではない。

## 設計の境界と順番

p001: 物理display ID、モード/refresh、全拡張/全mirror、drag座標、単一窓所属、異解像度mirror、保存/復元、0台・1台・hotplugの状態機械と受け入れfixtureを確定。実i915がVulkan Display拡張に通知できることを技術調査し、欠落した標準拡張の仕様と既存GPU_DISPLAY_EVENTSの結線を決める。

p002: i915 HPD→GPU表示イベントと複数outputの同時claim/presentの下層。UAPI/driverの既存責務を使い、必要な追加は最小に。compositorへGPU ioctlを追加しない。p003: libvulkanがVK_KHR_displayとVK_EXT_display_controlで列挙/通知/再列挙、generation/ACK/fence寿命、喪失・再接続、複数surface/swapchainを扱う。

p004: compositorの出力ごとのcompose/present/論理座標と全拡張/全mirror。p005: 専用compositor拡張の照会/通知/設定適用を`libkeiland`で包む。p006: Settings Displayページでモード選択と配置drag/適用/失敗表示。p007: 拡張表示の窓の1出力所有、pointer境界で窓全体の移動、再配置とdisconnect退避。p008: zedBSD i915実環境で接続/切断・2出力同時表示・両mode・Settings・drag・復帰を検証。p009: 最終変更全体の全文規約/該当回帰とWS自身の受け入れを確認。

設定の保存はoutputの安定IDに結び、再接続で有効な配置を復元する案。ミラーの異解像度は同じ論理画面の全内容を各outputに表示できる縮尺をp001で決める。モードに共通解像度がない場合は利用可能なmodeを選んでaspect比を保持する案。これらは実装時の技術設計で具体化し、ユーザーの全拡張/全mirror二択は変えない。

受け入れはzedBSD i915実機。既存Linux/FreeBSDの単一表示/buildの後退を検査する。実機なしでのmodel/QEMUの成功を実i915の合格と書かない。環境が利用不能ならPhase uncleared、観察と再開条件を記録する。

## 依存・標準・時間の境界

[全文方針](../standards/ws113-display.md)、[Guardrail](../guardrail.md)、[C全文](../coding-style.md)、[automation](../standards/automation.md)。既存[WS075](../ws075/ws.md) i915実行器、[WS103](../ws103/ws.md) compositor GPU境界、[WS089](../ws089/ws.md) Settings stubがcontext。WS089の既往Phaseを遡及的に書き換えない。将来の各Phaseを有限1 Queueで実行する。未知のdriver能力やGPU特性を仮定して次Phaseをclearしない。
