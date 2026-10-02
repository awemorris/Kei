<!-- awesome-plan project=zedbsd record=q585 -->

# Queue q585 / A2 WS112 p001 contract research

Status: finished
Attempt: q585-i01 / uncleared
Period: 2026-10-02 07:12–08:11 UTC（調査59min、終端docs08:13、上限60min）。
Approval/source: current userのAgent A N=3開始指示。exact scopeは下のlane/Phase snapshotに保存。
Phase: [ws112-p001](../ws112/phase001/phase.md) / uncleared。WS112 incomplete、後続p002〜007 planned。
Outcome: 5OS inputs/checksum/署名metadata、native環境、形式/依存/manifest/source/CI契約と後続commandを保存。未解決D1 Fedora/Arch boot適用により全criteria未達。
Evidence: [criteria](../ws112/phase001/criteria.md)、[survey](../ws112/phase001/survey.md)、[native環境](../ws112/native-environments.md)、[package contract](../ws112/package-contract.md)。A2-001〜006のWIP成果をrootへ統合、final worker SHA8b78da1c535914f78d0e9938af8fea200e556fd7。
Verification: worker local links148/0error、manual全文方針/実source/一次資料/依存とイベント、root changed docs diff-check PASS。実OS image/hash/native ELF/build/package/CI/remote未実施、metadata署名成功をimage成立としない。
Decision: D2は委任技術判断で既存Debian VM内公式RPi arm64 rootfs/native compiler/QEMU-userを採用。D1 user返答待ち。回答・Guardrail反映後に基準を再評価し、別有限Queueのexact scope/依存を確認する。
Residual: q591/p002は[候補](../ws112/phase002/queue-candidate.md)のみ。workerは同runtime/contextで待機、Queue終了はagent終了ではない。GitHub publication保留、pushなし。

## Exact approved lane before terminal projection

# Agent A2 Queue q585

Status: active
Attempt: q585-i01 / in-progress
Owner: Agent A canonical writer / A2 package executor
Approval: current user / 2026-10-02「エージェントA、あなたもN=3で作業を開始してください」。A2の既定担当WS112の最初の調査Phaseのみ選定。
Started UTC: 2026-10-02T07:12:00Z
Timebox: 最大60分 / 1 Phase。
Phase: [ws112-p001](../../ws112/phase001/phase.md)
Snapshot: [approved phase](approved-phase.md) / SHA256 `d6930ba46e8d4339a3454f37f93a21693f550c52e477d7343b1345d89672e5c0`
Exact scope: 5OS targetのnative build環境、公式input/version/hash、CPU、package format/依存、共通payload/launcher、CI artifact契約と後続commandを実source/一次資料で調査・文書化。OS/CPU変更、環境取得/guest起動、production build/package/CI source変更は含めない。
Dependencies: WS108/WS105/WS111実成果はcontextとして確認、必須API追加なし。
Worktree: /home/awe/zedBSD-worktrees/a2 / codex/a2-packages
Next Queue: 未投入。
Merge requests / ACK: A2-001 requested → integrated `67c78c0e6c5ec8d9db055bad682251fc73e82632` → main `5acb47a9c` / ACK delivered。source gap/payload/input候補と未検証区分review。未決D1–D4保持、Phase開始とcheckpointの両イベント保存。
Sync: GitHub publication保留。commitはWIP、pushなし。

## Byte-preserved approved Phase snapshot

```markdown
<!-- awesome-plan project=zedbsd record=ws112-p001 -->

# ws112-p001: 共通契約・対象OS入力・形式を確定

Parent: [WS112](../ws.md)
Status: in-progress
Disposition: normal
Primary Milestone: MG007（WSから継承）
Queue / attempts: q585 / q585-i01 / A2（契約調査のみ）
Goal: 5 targetのpayload/CPU/format/依存・build環境・成果物/CI契約を具体化
Prerequisites: なし（WS108/WS105/WS111の実出力をcontextとして照合）
Investigation bound: 60分の有限1Phase Queue案。具体的なscope/timebox/commandを選定時に再確認する。

## Scope / procedure / affected components

root Makefile/GNUmakefile、release driver、native Linux mk、CI、公式OS/package仕様を調査。OS入力URL/hash/版、RPi arm64環境、RPM/Arch encoder、各OSの依存情報、QEMU boot検証方針と時間見積もりを記録。

[共通設計](../design.md)の対応節を使用する。自分のPhase以外の受け入れを変更する必要が出たら、依存/foreign Phase/WSを同時に計画修正し、material scopeの同意を確認する。

## Clearance criteria / verification

RPi buildのみの受け入れを保持し、GPU/GUIが不要なbuild環境を選ぶ。5 target仕様・共通manifest・個別依存/CPU/version/format・入力pinsと検証commandが定義され、後続Phaseに未解決の人間判断を持ち込まない。今回の計画作成だけでは環境は取得/起動しない。

形式/metadata/checksum/build確認と実行時の動作確認は別。CI runtimeはユーザー指定で対象外、合格したとは記録しない。
調査上限で未知の依存や未決定事項が残れば、当該attemptをunclearedとし証拠・再開条件を残す。未実行Phaseをclearしない。

## Standards / exceptions / constraints

[Guardrail](../../guardrail.md)、[package方針全文](../../standards/ws112-linux-packages.md)、[C規約全文](../../coding-style.md)、[automation](../../standards/automation.md#ws112-linux-package-coverage-2026-10-02)。
非Cは既存近傍形式と全文/manual review。新Cが必要になれば編集前に全文を読む。移動styleの過去WS例外を流用しない。
専用stage/buildで処理し、HAL/toolchain/共有build/.internal/host /optを変更しない。make check禁止。FreeBSD source install/既存Linux GDM direct entryを維持。
push/remote release/Issues公開は本計画では承認されていない。

## Evidence / findings / resume

Commands/results/commit/environment/artifacts: 未実施（計画のみ）。Skipped: implementation/build/guest/CI/release/導入・runtime。
Resume: prerequisitesの実出力と判断を照合し、当PhaseだけのQueueに実行承認を記録してから開始する。

## Event history

2026-10-02 / ws112-package-plan-20261002-ws112-p001-created: userの5OS package計画をこの有限Phaseへ分割、Status planned・Queue none。RPi arm64回答を契約に反映。詳細とscopeはWS/design参照。GitHub body/comment/Project公開は保留、local/outboxに記録。

2026-10-02 / ws112-rpi-build-only-20261002: current userのRPi build-only受け入れを反映。影響するp001/p003/p007・WS/design・release方針を更新、Queue/実装許可は追加しない。GitHub event deliveryは保留。
```
