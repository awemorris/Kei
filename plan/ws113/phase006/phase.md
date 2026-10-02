<!-- awesome-plan project=zedbsd record=ws113-p006 -->

# ws113-p006: Settings Displayページ

Parent: [WS113](../ws.md)
Status: planned
Disposition: normal
Primary Milestone: MG006（WSから継承）
Queue / attempts: none / 実装未承認
Purpose / goal: モード選択とドラッグ配置を提供
Prerequisites: p005 cleared/libkeiland公開API
Investigation bound: 90分の有限1 Phase Queue案。選定時にscope/時間を再照合する。

## Procedure / affected components

既存読み取り専用Displayページを実設定UIへ置換。出力識別、全拡張/全mirror、配置drag、適用/失敗/変更通知を実装。
[設計](../design.md)の対応段と[全文方針](../../standards/ws113-display.md)を契約とする。材料が変わればWSと影響する他Phaseへ同時反映し、既存Queueの実装scopeを拡張しない。

## Clearance / verification

Settingsからlibkeilandのみで切替と配置変更ができ、hotplug後の一覧を更新する。無効配置時は利用者に理由を表示。
結果は対象環境・source revision・commands・artifactsと結びつける。既存の実機/QEMUを混同しない。未解決の前提で調査上限に達したらattemptをunclearedとし、証拠と再開条件を記録する。

## Standards / limits / evidence

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[scoped full rule](../../standards/ws113-display.md)、[automation](../../standards/automation.md#ws113-multi-display-coverage-2026-10-02)を実装前に読む。formatter/style-checkは補助、意味/所有/イベント順はfull/manual review。HAL API変更は差分ごとの事前承認。compositorのGPU UAPI直接ioctl禁止。無関係なtoolchain変更、aggregate make check、既存WS089/WS099のPhase改変は含めない。

Commands/results/commit/environment/artifacts/skipped checks: 未実施（計画のみ）。Findings: [現状調査](../design.md)。Resume: prerequisiteの実出力を確認し、このPhaseだけを新Queueへ選定・承認後に開始。

## p001契約調査による詳細化（2026-10-02）

[origin p001](../phase001/phase.md)、[契約](../phase001/contracts.md)、[ID/完了比較](../phase001/identity-completion.md)、[fixture](../phase001/fixtures.md)、[WS summary](../ws.md)を入力とする。

Procedure: public libkeilandだけでmode二択、出力一覧、extended配置drag draft/edge snap/Apply/cancel、mirror配置dragの無効化を実装する。Apply前にhardwareを変更しない。hotplug通知・stale再読込・backend failure・適用成功だが保存失敗を実状態に合わせて表示する。

Verification / resume: D02/D03/D08/D09を適用。driver/GPU/Vulkan direct call無しを監査。draftとauthoritative snapshotが競合する場合の利用者操作、接続0/1/2、未対応server、保存失敗を確認。WS089の旧stub履歴を遡及変更しない。

Status/dependenciesは上記のまま。未採択architecture/製品判断とactual prerequisiteを確認し、新QueueでこのPhaseだけを有限選定・承認後に実装する。q586はp001文書のみで後続sourceを許可しない。

## Event

2026-10-02 / ws113-multidisplay-plan-20261002-ws113-p006-created: current userの5条件・3つの追加判断をこのPhaseへ投影。planned/Queue none。GitHub body/comment/Projectへの公開は保留。

2026-10-02 / ws113-contract-design-20261002-a3-ws113-p006: p001のsource/一次仕様で明らかになった不足に合わせ、上記の自Phase procedureと検証/resumeを詳細化。D02/D03/D08/D09を適用。driver/GPU/Vulkan direct call無しを監査。draftとauthoritative snapshotが競合する場合の利用者操作、接続0/1/2、未対応server、保存失敗を確認。WS089の旧stub履歴を遡及変更しない。 origin/WSリンクは上記。planned/Queue noneを保持。GitHub body/comment/Projectはmainへdelivery依頼pending。

2026-10-02 / ws113-technical-choice-20261002-a3-ws113-p006: mainのdelegated technical decision messageからD-BOOT/LAYOUT/REC/AUTH/PORT通常案を採択記録。自Phase影響: 非重複/辺連結edge snapを配置draft/Apply検査へ。 [origin](../phase001/phase.md)/[詳細](../phase001/identity-completion.md)/[WS](../ws.md)。依存/Queue権限不変、main remote delivery pending。
