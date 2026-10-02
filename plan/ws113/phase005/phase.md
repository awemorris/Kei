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

## p001契約調査による詳細化（2026-10-02）

[origin p001](../phase001/phase.md)、[契約](../phase001/contracts.md)、[ID/完了比較](../phase001/identity-completion.md)、[fixture](../phase001/fixtures.md)、[WS summary](../ws.md)を入力とする。

Procedure: 専用managerのbegin/output/done snapshot、changed、expected topology/config serial+token/genのconfiguration/apply/result、request_idを確定。public libkeilandがregistry/version/object/snapshot/callback寿命とENOTSUPを包む。active session peer credentialはOS moduleで確認する通常案。compositor-owned displays.confのversion/persistent keyとtemp/flush/rename、appliedとsavedを別結果にする。

Verification / resume: D07–D09を適用。partial snapshotをUIへ公開しない、stale/duplicate/missing/overflow/unsupported/unauthorized/busy/backend/rollback/persistence結果を分ける。私有Vulkan identity APIはmain不採用、A2 local keyを採用。snapshot clone/draftとclient切断退役、独立clientの変更通知を確認。

Status/dependenciesは上記のまま。未採択architecture/製品判断とactual prerequisiteを確認し、新QueueでこのPhaseだけを有限選定・承認後に実装する。q586はp001文書のみで後続sourceを許可しない。

## 採択済local port ID / native capability入力

[main採択A2とsource](../phase001/identity-completion.md)、[native capability結線](../phase001/native-contract.md)を使う。自Phaseへの影響: snapshot key/label/persistableはA2 schema/一意性を検査。保存を同machine/PCI port scopeに限定、kind/portの人向けlabelを公開、UUID問い合わせを必須にしない。
後続の実装権限/依存は不変。actual API番号/layout/共有callback差分は選定前にowner/main review、HAL変更なら事前承認。

## Event

2026-10-02 / ws113-multidisplay-plan-20261002-ws113-p005-created: current userの5条件・3つの追加判断をこのPhaseへ投影。planned/Queue none。GitHub body/comment/Projectへの公開は保留。

2026-10-02 / ws113-contract-design-20261002-a3-ws113-p005: p001のsource/一次仕様で明らかになった不足に合わせ、上記の自Phase procedureと検証/resumeを詳細化。D07–D09を適用。partial snapshotをUIへ公開しない、stale/duplicate/missing/overflow/unsupported/unauthorized/busy/backend/rollback/persistence結果を分ける。私有Vulkan identity APIは未採択。snapshot clone/draftとclient切断退役、独立clientの変更通知を確認。 origin/WSリンクは上記。planned/Queue noneを保持。GitHub body/comment/Projectはmainへdelivery依頼pending。

2026-10-02 / ws113-technical-choice-20261002-a3-ws113-p005: mainのdelegated technical decision messageからD-BOOT/LAYOUT/REC/AUTH/PORT通常案を採択記録。自Phase影響: active session同UID peer検査、Settings限定secret無し、保存keyはstandard policy詳細を待つ。 [origin](../phase001/phase.md)/[詳細](../phase001/identity-completion.md)/[WS](../ws.md)。依存/Queue権限不変、main remote delivery pending。

2026-10-02 / ws113-local-port-id-20261002-a3-ws113-p005: mainのD-ID A2/旧bootpreferred技術採択messageを受領。snapshot key/label/persistableはA2 schema/一意性を検査。保存を同machine/PCI port scopeに限定、kind/portの人向けlabelを公開、UUID問い合わせを必須にしない。 [origin](../phase001/phase.md)/[sourceと範囲](../phase001/identity-completion.md)/[WS](../ws.md)。既往eventを保存し、該当current designを更新。p001 in-progress、他Phase planned/Queue none。main remote delivery pending。
