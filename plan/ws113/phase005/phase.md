<!-- awesome-plan project=zedbsd record=ws113-p005 -->

# ws113-p005: compositor拡張とlibkeiland

Parent: [WS113](../ws.md)
Status: planned
Disposition: normal
Primary Milestone: MG006（WSから継承）
Queue / attempts: none / 実装未承認
Purpose / goal: Settings用の照会/変更/通知APIを公開
Prerequisites: p004 cleared/出力状態と適用API
Investigation bound: 90分の有限1 Phase Queue案。選定時にscope/時間を再照合する。

## Procedure / affected components

専用Wayland protocolを設計/実装し、libkeiland公開APIがoutput一覧・mode・座標・適用結果・hotplugを包む。認可/入力検査/失敗を明示。
[設計](../design.md)の対応段と[全文方針](../../standards/ws113-display.md)を契約とする。材料が変わればWSと影響する他Phaseへ同時反映し、既存Queueの実装scopeを拡張しない。

## Clearance / verification

独立clientがlibkeiland経由で二択と配置を照会/適用/購読でき、無効配置や権限不足は状態を壊さず拒否。Settingsにdriver直呼出しなし。
結果は対象環境・source revision・commands・artifactsと結びつける。既存の実機/QEMUを混同しない。未解決の前提で調査上限に達したらattemptをunclearedとし、証拠と再開条件を記録する。

## Standards / limits / evidence

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[scoped full rule](../../standards/ws113-display.md)、[automation](../../standards/automation.md#ws113-multi-display-coverage-2026-10-02)を実装前に読む。formatter/style-checkは補助、意味/所有/イベント順はfull/manual review。HAL API変更は差分ごとの事前承認。compositorのGPU UAPI直接ioctl禁止。無関係なtoolchain変更、aggregate make check、既存WS089/WS099のPhase改変は含めない。

Commands/results/commit/environment/artifacts/skipped checks: 未実施（計画のみ）。Findings: [現状調査](../design.md)。Resume: prerequisiteの実出力を確認し、このPhaseだけを新Queueへ選定・承認後に開始。

## Event

2026-10-02 / ws113-multidisplay-plan-20261002-ws113-p005-created: current userの5条件・3つの追加判断をこのPhaseへ投影。planned/Queue none。GitHub body/comment/Projectへの公開は保留。
