<!-- awesome-plan project=zedbsd record=ws113-p009 -->

# ws113-p009: 最終全文規約とWS受け入れ

Parent: [WS113](../ws.md)
Status: planned
Disposition: normal
Primary Milestone: MG006（WSから継承）
Queue / attempts: none / 実装未承認
Purpose / goal: 全変更sourceと実証結果の最終照合
Prerequisites: p008 cleared/最終source・実機証拠
Investigation bound: 90分の有限1 Phase Queue案。選定時にscope/時間を再照合する。

## Procedure / affected components

driver/libvulkan/compositor/protocol/libkeiland/Settings全変更をC全文でmanual review。該当clang-format/style-check/static/build/絞ったtests/bootを実行し、修正後の影響範囲を再確認。
[設計](../design.md)の対応段と[全文方針](../../standards/ws113-display.md)を契約とする。材料が変わればWSと影響する他Phaseへ同時反映し、既存Queueの実装scopeを拡張しない。

## Clearance / verification

全in-scope違反を解消、commands/versions/files/例外/未実施を記録。WS自身のD1〜D5を独立判定。
結果は対象環境・source revision・commands・artifactsと結びつける。既存の実機/QEMUを混同しない。未解決の前提で調査上限に達したらattemptをunclearedとし、証拠と再開条件を記録する。

## Standards / limits / evidence

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[scoped full rule](../../standards/ws113-display.md)、[automation](../../standards/automation.md#ws113-multi-display-coverage-2026-10-02)を実装前に読む。formatter/style-checkは補助、意味/所有/イベント順はfull/manual review。HAL API変更は差分ごとの事前承認。compositorのGPU UAPI直接ioctl禁止。無関係なtoolchain変更、aggregate make check、既存WS089/WS099のPhase改変は含めない。

Commands/results/commit/environment/artifacts/skipped checks: 未実施（計画のみ）。Findings: [現状調査](../design.md)。Resume: prerequisiteの実出力を確認し、このPhaseだけを新Queueへ選定・承認後に開始。

## Event

2026-10-02 / ws113-multidisplay-plan-20261002-ws113-p009-created: current userの5条件・3つの追加判断をこのPhaseへ投影。planned/Queue none。GitHub body/comment/Projectへの公開は保留。
