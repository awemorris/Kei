<!-- awesome-plan project=zedbsd record=ws113-p003 -->

# ws113-p003: Vulkan Displayの列挙・通知

Parent: [WS113](../ws.md)
Status: planned
Disposition: normal
Primary Milestone: MG006（WSから継承）
Queue / attempts: none / 実装未承認
Purpose / goal: libvulkanから標準Display API/拡張でhotplugと複数出力を公開
Prerequisites: p002 cleared/実driverイベント
Investigation bound: 120分の有限1 Phase Queue案。選定時にscope/時間を再照合する。

## Procedure / affected components

VK_EXT_display_controlのdevice hotplug fenceとVK_KHR_displayの再列挙、複数surface/swapchain、世代/ACK/lease寿命、切断時の結果を実装。
[設計](../design.md)の対応段と[全文方針](../../standards/ws113-display.md)を契約とする。材料が変わればWSと影響する他Phaseへ同時反映し、既存Queueの実装scopeを拡張しない。

## Clearance / verification

独立Vulkan clientで接続/切断通知、再列挙、2出力同時呈示、切断時refusal/回復を確認。標準entry/拡張advertisementは実装済みのみ。
結果は対象環境・source revision・commands・artifactsと結びつける。既存の実機/QEMUを混同しない。未解決の前提で調査上限に達したらattemptをunclearedとし、証拠と再開条件を記録する。

## Standards / limits / evidence

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[scoped full rule](../../standards/ws113-display.md)、[automation](../../standards/automation.md#ws113-multi-display-coverage-2026-10-02)を実装前に読む。formatter/style-checkは補助、意味/所有/イベント順はfull/manual review。HAL API変更は差分ごとの事前承認。compositorのGPU UAPI直接ioctl禁止。無関係なtoolchain変更、aggregate make check、既存WS089/WS099のPhase改変は含めない。

Commands/results/commit/environment/artifacts/skipped checks: 未実施（計画のみ）。Findings: [現状調査](../design.md)。Resume: prerequisiteの実出力を確認し、このPhaseだけを新Queueへ選定・承認後に開始。

## Event

2026-10-02 / ws113-multidisplay-plan-20261002-ws113-p003-created: current userの5条件・3つの追加判断をこのPhaseへ投影。planned/Queue none。GitHub body/comment/Projectへの公開は保留。
