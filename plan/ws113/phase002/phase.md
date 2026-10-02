<!-- awesome-plan project=zedbsd record=ws113-p002 -->

# ws113-p002: i915のHPD・複数display出力

Parent: [WS113](../ws.md)
Status: planned
Disposition: normal
Primary Milestone: MG006（WSから継承）
Queue / attempts: none / 実装未承認
Purpose / goal: driverからGPU表示イベントを提供し、複数出力を同時に扱う
Prerequisites: p001 cleared/driver契約
Investigation bound: 120分の有限1 Phase Queue案。選定時にscope/時間を再照合する。

## Procedure / affected components

i915/displayのHPD→表示イベント通知、接続済み出力のgeneration、複数claim/mode/presentを実装。既存UAPI責務で足りなければ変更を局所化。
[設計](../design.md)の対応段と[全文方針](../../standards/ws113-display.md)を契約とする。材料が変わればWSと影響する他Phaseへ同時反映し、既存Queueの実装scopeを拡張しない。

## Clearance / verification

HPD接続/切断で表示イベントと列挙内容が一致し、2出力を同時にpresentできる。host contractと実i915の下層証拠を分ける。
結果は対象環境・source revision・commands・artifactsと結びつける。既存の実機/QEMUを混同しない。未解決の前提で調査上限に達したらattemptをunclearedとし、証拠と再開条件を記録する。

## Standards / limits / evidence

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[scoped full rule](../../standards/ws113-display.md)、[automation](../../standards/automation.md#ws113-multi-display-coverage-2026-10-02)を実装前に読む。formatter/style-checkは補助、意味/所有/イベント順はfull/manual review。HAL API変更は差分ごとの事前承認。compositorのGPU UAPI直接ioctl禁止。無関係なtoolchain変更、aggregate make check、既存WS089/WS099のPhase改変は含めない。

Commands/results/commit/environment/artifacts/skipped checks: 未実施（計画のみ）。Findings: [現状調査](../design.md)。Resume: prerequisiteの実出力を確認し、このPhaseだけを新Queueへ選定・承認後に開始。

## p001契約調査による詳細化（2026-10-02）

[origin p001](../phase001/phase.md)、[契約](../phase001/contracts.md)、[ID/完了比較](../phase001/identity-completion.md)、[fixture](../phase001/fixtures.md)、[WS summary](../ws.md)を入力とする。

Procedure: connectorの固定slot/非0ID、output generationとdevice topology sequenceの分離、HPD workerのcoherent inventory publish→poll_notify、独立open ACKを実装・検証する。単一rdをper-output lease/pipe/PLL/scanoutへ分け、切断中のbuffer retirementと残るhead継続を証明する。EXT control全entryに必要なpower/next-first-pixel/vblank能力、必要時のGPU UUID native query、present completionの不足を差分設計し、共有/HAL所有境界の承認前にAPI変更しない。

Verification / resume: H01–H03/H07–H10を適用。connector数/connected数/plane indexを混同しない。2台の独立claim/presentはactual i915両画面とcounterで確認し、model PASSを代替にしない。D-ID/D-ATOMIC/D-PORTとboot overrideの材料が必要部分を選定前に解決。

Status/dependenciesは上記のまま。未採択architecture/製品判断とactual prerequisiteを確認し、新QueueでこのPhaseだけを有限選定・承認後に実装する。q586はp001文書のみで後続sourceを許可しない。

## Event

2026-10-02 / ws113-multidisplay-plan-20261002-ws113-p002-created: current userの5条件・3つの追加判断をこのPhaseへ投影。planned/Queue none。GitHub body/comment/Projectへの公開は保留。

2026-10-02 / ws113-contract-design-20261002-a3-ws113-p002: p001のsource/一次仕様で明らかになった不足に合わせ、上記の自Phase procedureと検証/resumeを詳細化。H01–H03/H07–H10を適用。connector数/connected数/plane indexを混同しない。2台の独立claim/presentはactual i915両画面とcounterで確認し、model PASSを代替にしない。D-ID/D-ATOMIC/D-PORTとboot overrideの材料が必要部分を選定前に解決。 origin/WSリンクは上記。planned/Queue noneを保持。GitHub body/comment/Projectはmainへdelivery依頼pending。

2026-10-02 / ws113-technical-choice-20261002-a3-ws113-p002: mainのdelegated technical decision messageからD-BOOT/LAYOUT/REC/AUTH/PORT通常案を採択記録。自Phase影響: 初回fixture eDP+HDMIとall-connected初回extendedを入力にする。未移植portを完成扱いにしない。 [origin](../phase001/phase.md)/[詳細](../phase001/identity-completion.md)/[WS](../ws.md)。依存/Queue権限不変、main remote delivery pending。
