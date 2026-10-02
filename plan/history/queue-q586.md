<!-- awesome-plan project=zedbsd record=q586 -->

# q586 / WS113 p001 display contracts

Status: finished / q586-i01 uncleared
Period: 2026-10-02 07:12–08:42 UTC（90min）。終端docs08:43。
Phase: [ws113-p001](../ws113/phase001/phase.md) uncleared/normal、WS113 incomplete。
Approval/scope: 下記lane・byte-preserved Phase snapshot（SHA256 41bedea04dd6fd471756bb91bcf43ab03ef0232ee2d20c06c57e5147e3f3d309）、A/N=3 user開始指示。read-only source/一次仕様と設計資料だけ。
Outcome/reason: i915/Vulkan/source能力20行、HPD/ID/世代/notify/fence/出力状態/Settingsプロトコル/窓所属/失敗/有限fixtureを保存。D-ATOMICの要求解釈が未回答で契約確定criteria未達。
Evidence: [ledger](../ws113/phase001/evidence.md)、[contracts](../ws113/phase001/contracts.md)、[ID/完了](../ws113/phase001/identity-completion.md)、[native能力案](../ws113/phase001/native-contract.md)。A3-001〜006 cumulative2ad9c951 → root83b111c63 ACK。
Checks: worker/root docs diff-check/local links/例port key長さ、実sourceとprimary specifications/manual確認。製品C/HAL/UAPI変更、build/guest/SSH/hardware/installation無し。
Decisions: D-ID local PCI portkey/standard displayName、bootpreferred/allconnected、layout/reconnect/auth/初回fixtureはmain委任技術判断。新HAL/APIは未承認。D-ATOMIC unresolved、historical physical fixture可用性はp008 readiness未確認。
Resume: D-ATOMIC採択後、同p001残scopeを新finite attemptへ選定。q592/p002は候補だけ、未投入。08:42有限attempt終了後、userのA3 wrap-up/全agent区切り終了指示によりsession終了。worktree clean/未commit・ignored証拠・owned process無しをfinal receiptで確認。GitHub publication pending、WIP/no push。

## Exact lane before terminal projection

# Agent A3 Queue q586

Status: active
Attempt: q586-i01 / in-progress
Owner: Agent A canonical writer / A3 display executor
Approval: current user / 2026-10-02「エージェントA、あなたもN=3で作業を開始してください」。A3の既定担当WS113の最初の契約調査のみ選定。
Started UTC: 2026-10-02T07:12:00Z
Timebox: 最大90分 / 1 Phase。
Phase: [ws113-p001](../../ws113/phase001/phase.md)
Snapshot: [approved phase](approved-phase.md) / SHA256 `41bedea04dd6fd471756bb91bcf43ab03ef0232ee2d20c06c57e5147e3f3d309`
Exact scope: i915→Vulkan Display通知の実能力、安定ID/出力generation、全拡張/全mirror、pointer越境時の窓単一出力、配置/保存/切断と失敗の契約、後続API/実機fixtureを実source/一次仕様から設計。production source/driver/HAL API変更と実機占有・実装は含めない。
Dependencies: WS075/WS089/WS103実成果はcontext。Bのsource変更と同時に編集しない。
Worktree: /home/awe/zedBSD-worktrees/a3 / codex/a3-display
Next Queue: q592 ID予約のみ。p001採択済み契約・依存とexact scope確認まで実装しない。
Merge requests / ACK: A3-001 requested → integrated `6e34d1bb9690685342b2916bc6addc4f3369c606` → main `8021bc210` / ACK delivered。18行能力表・実sourceとVulkan一次仕様を照合。製品source/hardware変更なし、whole criteria未達。
Sync: GitHub publication保留。commitはWIP、pushなし。

2026-10-02 / MR ACK reconciliation: A3-002 6a0a532b/A3-003 addd2583 → root8cf8a8ea6 integrated/ACK delivered。20行能力表とcontracts/identity/fixtures、変更foreign p002–p009のprocedure/verification/event確認。D-BOOT/LAYOUT/REC/AUTH/PORT採択、D-ID full PCI portkeyと旧boot anchorはrootが追加判断を送付済み、次docs MRへ投影。D-ATOMIC user返答待ち、deadline08:42 UTC保持。

2026-10-02 / MR A3-004 ACK: df2f36ff → root ec870f856 integrated/ACK delivered。採択済local PCI portkey/旧bootpreferredのcurrent契約・foreign p001–008、native capability proposalを確認。local paths237/0error、keylength/diff-check PASS。D-ATOMIC回答待ち、source/HAL/API/hardware実装なし。

## Approved Phase snapshot

```markdown
<!-- awesome-plan project=zedbsd record=ws113-p001 -->

# ws113-p001: 契約・能力と実機fixture

Parent: [WS113](../ws.md)
Status: in-progress
Disposition: normal
Primary Milestone: MG006（WSから継承）
Queue / attempts: q586 / q586-i01 / A3（契約調査のみ）
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
```
