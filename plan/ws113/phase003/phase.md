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

## p001契約調査による詳細化（2026-10-02）

[origin p001](../phase001/phase.md)、[契約](../phase001/contracts.md)、[ID/完了比較](../phase001/identity-completion.md)、[fixture](../phase001/fixtures.md)、[WS summary](../ws.md)を入力とする。

Procedure: EXT_display_control全4entryとinstance EXT_display_surface_counter依存、public ABI/provenance/dispatch/広告を一体で扱う。独立native openのidle/0output monitor、fence毎cursor、pending reset no-op、signal保持/fresh再登録、status/wait-any/all/timeout/destroy/teardownと外部payload復帰を設計する。coherent snapshot、固定global plane mapping、旧generation mode/surface拒否。D-ID採択済transport、D-ATOMIC採択時だけ標準present_id/wait等の追加完了契約を実装。

Verification / resume: H04–H10を独立Vulkan clientで確認。別fence/device/processが同じeventを独立受信、lease/swapchain無しでも通知。struct-type enumのみを対応証拠にしない。properties2KHRの既存wrapperとi915 opcode148未実装を区別。strict物理消去を標準present_waitの存在だけで宣言しない。

Status/dependenciesは上記のまま。未採択architecture/製品判断とactual prerequisiteを確認し、新QueueでこのPhaseだけを有限選定・承認後に実装する。q586はp001文書のみで後続sourceを許可しない。

## Event

2026-10-02 / ws113-multidisplay-plan-20261002-ws113-p003-created: current userの5条件・3つの追加判断をこのPhaseへ投影。planned/Queue none。GitHub body/comment/Projectへの公開は保留。

2026-10-02 / ws113-contract-design-20261002-a3-ws113-p003: p001のsource/一次仕様で明らかになった不足に合わせ、上記の自Phase procedureと検証/resumeを詳細化。H04–H10を独立Vulkan clientで確認。別fence/device/processが同じeventを独立受信、lease/swapchain無しでも通知。struct-type enumのみを対応証拠にしない。properties2KHRの既存wrapperとi915 opcode148未実装を区別。strict物理消去を標準present_waitの存在だけで宣言しない。 origin/WSリンクは上記。planned/Queue noneを保持。GitHub body/comment/Projectはmainへdelivery依頼pending。

2026-10-02 / ws113-technical-choice-20261002-a3-ws113-p003: mainのdelegated technical decision messageからD-BOOT/LAYOUT/REC/AUTH/PORT通常案を採択記録。自Phase影響: 私有Vulkan identity拡張は不採用。standard短portkeyのpolicyと実GPU UUIDqueryを別gateにする。 [origin](../phase001/phase.md)/[詳細](../phase001/identity-completion.md)/[WS](../ws.md)。依存/Queue権限不変、main remote delivery pending。
