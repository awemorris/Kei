<!-- awesome-plan project=zedbsd record=ws113-p008 -->

# ws113-p008: 実i915の全経路受け入れ

Parent: [WS113](../ws.md)
Status: planned
Disposition: normal
Primary Milestone: MG006（WSから継承）
Queue / attempts: none / 実装未承認
Purpose / goal: 接続からSettings・表示・窓移動まで実機で確認
Prerequisites: p002〜p007 cleared/実driver・API・UI・窓出力
Investigation bound: 120分の有限1 Phase Queue案。選定時にscope/時間を再照合する。

## Procedure / affected components

zedBSD i915実機で外部displayの接続/切断、2出力同時、Settingsの両modeと配置drag、異解像度、窓移動/再接続を確認。QEMU/model/実機証拠を分ける。
[設計](../design.md)の対応段と[全文方針](../../standards/ws113-display.md)を契約とする。材料が変わればWSと影響する他Phaseへ同時反映し、既存Queueの実装scopeを拡張しない。

## Clearance / verification

D1〜D5の実i915結果、物理各outputの画面証拠と操作結果を記録。Linux/FreeBSD単一表示回帰とzedBSD起動を該当手順で確認。
結果は対象環境・source revision・commands・artifactsと結びつける。既存の実機/QEMUを混同しない。未解決の前提で調査上限に達したらattemptをunclearedとし、証拠と再開条件を記録する。

## Standards / limits / evidence

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[scoped full rule](../../standards/ws113-display.md)、[automation](../../standards/automation.md#ws113-multi-display-coverage-2026-10-02)を実装前に読む。formatter/style-checkは補助、意味/所有/イベント順はfull/manual review。HAL API変更は差分ごとの事前承認。compositorのGPU UAPI直接ioctl禁止。無関係なtoolchain変更、aggregate make check、既存WS089/WS099のPhase改変は含めない。

Commands/results/commit/environment/artifacts/skipped checks: 未実施（計画のみ）。Findings: [現状調査](../design.md)。Resume: prerequisiteの実出力を確認し、このPhaseだけを新Queueへ選定・承認後に開始。

## p001契約調査による詳細化（2026-10-02）

[origin p001](../phase001/phase.md)、[契約](../phase001/contracts.md)、[ID/完了比較](../phase001/identity-completion.md)、[fixture](../phase001/fixtures.md)、[WS summary](../ws.md)を入力とする。

Procedure: fixture manifestをsource/build/machine/runtime/physical sink/mode/connector/capability/occupancy/commands/evidence/bounds付きで完成し、H01–H10/D01–D10の選定scopeを有限に実行する。現physical fixtureは未確認。historical eDP+HDMIと実GPU passthrough、capture build、host modelを別classに記録する。

Verification / resume: D1–D5のactual zedBSD i915 gate。両physical出力の同時content、異解像度、hotplug、Settings保存/復元、窓全tree境界移動と採択D-ATOMICを確認。boot-test.shのlogin PNG scopeを守り、旧serial/SSH例外を流用しない。0台fixture不在や必要能力不足はuncleared/再開条件。

Status/dependenciesは上記のまま。未採択architecture/製品判断とactual prerequisiteを確認し、新QueueでこのPhaseだけを有限選定・承認後に実装する。q586はp001文書のみで後続sourceを許可しない。

## Event

2026-10-02 / ws113-multidisplay-plan-20261002-ws113-p008-created: current userの5条件・3つの追加判断をこのPhaseへ投影。planned/Queue none。GitHub body/comment/Projectへの公開は保留。

2026-10-02 / ws113-contract-design-20261002-a3-ws113-p008: p001のsource/一次仕様で明らかになった不足に合わせ、上記の自Phase procedureと検証/resumeを詳細化。D1–D5のactual zedBSD i915 gate。両physical出力の同時content、異解像度、hotplug、Settings保存/復元、窓全tree境界移動と採択D-ATOMICを確認。boot-test.shのlogin PNG scopeを守り、旧serial/SSH例外を流用しない。0台fixture不在や必要能力不足はuncleared/再開条件。 origin/WSリンクは上記。planned/Queue noneを保持。GitHub body/comment/Projectはmainへdelivery依頼pending。

2026-10-02 / ws113-technical-choice-20261002-a3-ws113-p008: mainのdelegated technical decision messageからD-BOOT/LAYOUT/REC/AUTH/PORT通常案を採択記録。自Phase影響: eDP+HDMIを初回actual fixture候補にする。現可用性未確認と他port不足を保持。 [origin](../phase001/phase.md)/[詳細](../phase001/identity-completion.md)/[WS](../ws.md)。依存/Queue権限不変、main remote delivery pending。
