<!-- awesome-plan project=zedbsd record=ws113-p001 -->

# ws113-p001: 契約・能力と実機fixture

Parent: [WS113](../ws.md)
Status: planned
Disposition: normal
Primary Milestone: MG006（WSから継承）
Queue / attempts: none / 実装未承認
Purpose / goal: hotplug/複数出力/拡張とmirror/Settings/窓所属の仕様を確定
Prerequisites: 既存WS075/WS089/WS103の実出力を確認（context）
Investigation bound: 90分の有限1 Phase Queue案。選定時にscope/時間を再照合する。

## Procedure / affected components

driver UAPI、Vulkan Display実装、Settings/Waylandの所有を調査。安定output ID、0/1/2台、mode/座標/保存、異解像度mirror、pointer移動閾値、通知順と失敗を設計。
[設計](../design.md)の対応段と[全文方針](../../standards/ws113-display.md)を契約とする。材料が変わればWSと影響する他Phaseへ同時反映し、既存Queueの実装scopeを拡張しない。

## Clearance / verification

標準Vulkan拡張の結線と実機試験fixture、依存・制約・後続のAPI契約が定まり、ユーザーの二択/窓単一所属に矛盾しない。
結果は対象環境・source revision・commands・artifactsと結びつける。既存の実機/QEMUを混同しない。未解決の前提で調査上限に達したらattemptをunclearedとし、証拠と再開条件を記録する。

## Standards / limits / evidence

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[scoped full rule](../../standards/ws113-display.md)、[automation](../../standards/automation.md#ws113-multi-display-coverage-2026-10-02)を実装前に読む。formatter/style-checkは補助、意味/所有/イベント順はfull/manual review。HAL API変更は差分ごとの事前承認。compositorのGPU UAPI直接ioctl禁止。無関係なtoolchain変更、aggregate make check、既存WS089/WS099のPhase改変は含めない。

Commands/results/commit/environment/artifacts/skipped checks: 未実施（計画のみ）。Findings: [現状調査](../design.md)。Resume: prerequisiteの実出力を確認し、このPhaseだけを新Queueへ選定・承認後に開始。

## Event

2026-10-02 / ws113-multidisplay-plan-20261002-ws113-p001-created: current userの5条件・3つの追加判断をこのPhaseへ投影。planned/Queue none。GitHub body/comment/Projectへの公開は保留。
