<!-- awesome-plan project=zedbsd record=ws113-p007 -->

# ws113-p007: 窓の出力所属と画面間移動

Parent: [WS113](../ws.md)
Status: planned
Disposition: normal
Primary Milestone: MG006（WSから継承）
Queue / attempts: none / 実装未承認
Purpose / goal: 拡張表示で窓全体を1出力にだけ表示
Prerequisites: p004 cleared/論理座標・出力描画（p006とは独立）
Investigation bound: 120分の有限1 Phase Queue案。選定時にscope/時間を再照合する。

## Procedure / affected components

top-level窓の所属を持ち、drag pointerが隣画面に入った時点で一括切替。boundaryで二重描画しない。disconnect時は残るoutputへ退避。
[設計](../design.md)の対応段と[全文方針](../../standards/ws113-display.md)を契約とする。材料が変わればWSと影響する他Phaseへ同時反映し、既存Queueの実装scopeを拡張しない。

## Clearance / verification

窓は拡張時に単一outputでのみ見え、境界を跨ぐdragで全体が移る。窓の一部が別displayに現れない。mirror時は全画面複製。
結果は対象環境・source revision・commands・artifactsと結びつける。既存の実機/QEMUを混同しない。未解決の前提で調査上限に達したらattemptをunclearedとし、証拠と再開条件を記録する。

## Standards / limits / evidence

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[scoped full rule](../../standards/ws113-display.md)、[automation](../../standards/automation.md#ws113-multi-display-coverage-2026-10-02)を実装前に読む。formatter/style-checkは補助、意味/所有/イベント順はfull/manual review。HAL API変更は差分ごとの事前承認。compositorのGPU UAPI直接ioctl禁止。無関係なtoolchain変更、aggregate make check、既存WS089/WS099のPhase改変は含めない。

Commands/results/commit/environment/artifacts/skipped checks: 未実施（計画のみ）。Findings: [現状調査](../design.md)。Resume: prerequisiteの実出力を確認し、このPhaseだけを新Queueへ選定・承認後に開始。

## p001契約調査による詳細化（2026-10-02）

[origin p001](../phase001/phase.md)、[契約](../phase001/contracts.md)、[ID/完了比較](../phase001/identity-completion.md)、[fixture](../phase001/fixtures.md)、[WS summary](../ws.md)を入力とする。

Procedure: root output_token/ownership_epochにpopup/subsurface/transient/decoration/shadowを従属させ、owner以外のrender listへ一切追加しない。global pointerのshared-edge越境でgrab offsetを保持してtree全体を1回切替、motion更新の後にbutton処理。大delta/負origin/nonshared edge/小target/未ACK resizeを扱う。disconnectは残るownerへtree退避、0台park。D-ATOMIC採択のphysical transition順を実装し、未採択解釈を勝手に緩和しない。

Verification / resume: D04–D07/D10を適用。root boundsが境界を跨いでも別outputに一部を描かない。logical ownerと実scanoutを別証拠にする。旧window有りqueued frame/高速往復/切断during drag/grab cancelを確認。p006との新依存は追加しない。

Status/dependenciesは上記のまま。未採択architecture/製品判断とactual prerequisiteを確認し、新QueueでこのPhaseだけを有限選定・承認後に実装する。q586はp001文書のみで後続sourceを許可しない。

## 採択済local port ID / native capability入力

[main採択A2とsource](../phase001/identity-completion.md)、[native capability結線](../phase001/native-contract.md)を使う。自Phaseへの影響: 退避先のdeterministic key順はA2の検証済local key、persist不可ならsession tokenの決定的順。reconnect同port新generation、window奪回無し。
後続の実装権限/依存は不変。actual API番号/layout/共有callback差分は選定前にowner/main review、HAL変更なら事前承認。

## Event

2026-10-02 / ws113-multidisplay-plan-20261002-ws113-p007-created: current userの5条件・3つの追加判断をこのPhaseへ投影。planned/Queue none。GitHub body/comment/Projectへの公開は保留。

2026-10-02 / ws113-contract-design-20261002-a3-ws113-p007: p001のsource/一次仕様で明らかになった不足に合わせ、上記の自Phase procedureと検証/resumeを詳細化。D04–D07/D10を適用。root boundsが境界を跨いでも別outputに一部を描かない。logical ownerと実scanoutを別証拠にする。旧window有りqueued frame/高速往復/切断during drag/grab cancelを確認。p006との新依存は追加しない。 origin/WSリンクは上記。planned/Queue noneを保持。GitHub body/comment/Projectはmainへdelivery依頼pending。

2026-10-02 / ws113-technical-choice-20261002-a3-ws113-p007: mainのdelegated technical decision messageからD-BOOT/LAYOUT/REC/AUTH/PORT通常案を採択記録。自Phase影響: 退避窓の自動奪回無し、edge連結入力を採用。D-ATOMIC回答前にphysical解釈を採択しない。 [origin](../phase001/phase.md)/[詳細](../phase001/identity-completion.md)/[WS](../ws.md)。依存/Queue権限不変、main remote delivery pending。

2026-10-02 / ws113-local-port-id-20261002-a3-ws113-p007: mainのD-ID A2/旧bootpreferred技術採択messageを受領。退避先のdeterministic key順はA2の検証済local key、persist不可ならsession tokenの決定的順。reconnect同port新generation、window奪回無し。 [origin](../phase001/phase.md)/[sourceと範囲](../phase001/identity-completion.md)/[WS](../ws.md)。既往eventを保存し、該当current designを更新。p001 in-progress、他Phase planned/Queue none。main remote delivery pending。
