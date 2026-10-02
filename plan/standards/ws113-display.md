# WS113: 複数ディスプレイの表示・設定契約

Authority: 2026-10-02 current userの本chatのWS追加依頼、および質問への回答（ドラッグ中のポインタが隣画面へ入った時に切替、単一画面制約は拡張表示時、受け入れはまずzedBSD i915）。[WS113](../ws113/ws.md)に適用する全文の設計方針。計画のみで実装Queueは未承認。

- zedBSDのi915で外部displayの接続/切断を検出し、libvulkanの**Vulkan Display拡張**からcompositorへ通知する。compositorはGPU表示操作をlibvulkanのVulkan API/拡張経由で行い、GPU UAPIを直接ioctlしない。現行`VK_KHR_display`の列挙に加え、標準`VK_EXT_display_control`の`VK_DEVICE_EVENT_TYPE_DISPLAY_HOTPLUG_EXT`とfenceによる通知を実装・検証する計画。別の手段が必要なら標準契約との適合をp001で明示して設計改訂する。
- Settingsアプリは`libkeiland`の公開APIを通じてcompositorの専用Wayland拡張と通信する。SettingsがGPU deviceやdriverを直接操作しない。照会・適用・変更通知と権限/入力検査/失敗の扱いを契約化する。
- 全接続displayを**拡張表示**または**全画面ミラーリング**のどちらか一方にする。混在構成は今回のscope外。拡張ではdisplayの配置をSettings上のドラッグで変更し、窓は常に1つのdisplayにだけ描く。ドラッグ中のpointerが隣displayへ入った時点で、窓全体の所属/描画先を原子的に移す。境界にかかる部分をもう一方へ描かない。
- ミラーリングは1つの論理desktopを接続した全物理displayへ複製する。これには同じ窓が全物理displayに見える。ユーザーは「1画面のみ」を拡張表示時の制約として確認した。
- hotplug/切断/再接続時はdisplay一覧とgenerationを再照会し、接続されていないoutputへpresentしない。残るdisplayへ窓を退避させ、1台構成に安全に戻す。設定の永続化、出力ID、異解像度mirrorの縮尺とfallbackはp001で具体化する。
- zedBSD i915の実機が完了の受け入れ対象。Linux/FreeBSDでは既存の単一displayとbuildを維持し、今回の複数display受け入れは課さない。i915以外も今回の実機gateにしない。
- [Guardrail](../guardrail.md)のGPU境界、OS module境界、[C全文](../coding-style.md)を適用する。HAL APIの変更は別途差分事前承認。新しいstyle例外は無し。

一次仕様: [VK_KHR_display](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_display.html)、[VK_EXT_display_control](https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_display_control.html)、[VkDeviceEventTypeEXT](https://docs.vulkan.org/refpages/latest/refpages/source/VkDeviceEventTypeEXT.html)。現実装の対応状況はWS designに記録。
