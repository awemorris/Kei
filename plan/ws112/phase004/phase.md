<!-- awesome-plan project=zedbsd record=ws112-p004 -->

# ws112-p004: Fedora 44 rpm

Parent: [WS112](../ws.md)
Status: planned
Disposition: normal
Primary Milestone: MG007（WSから継承）
Queue / attempts: none / 実装未承認
Goal: make keiland-linux-fedora44とFedora 44 rpm成果物
Prerequisites: ws112-p002 cleared / 共通stage・成果物契約（p001のFedora確定入力を使用）
Investigation bound: 90分の有限1Phase Queue案。具体的なscope/timebox/commandを選定時に再確認する。

## Scope / procedure / affected components

Fedora native buildと軽量RPM生成、CPU/version/依存/config/license metadata、make入口・build記録を追加。p003の成果物への依存はない。

[共通設計](../design.md)の対応節を使用する。自分のPhase以外の受け入れを変更する必要が出たら、依存/foreign Phase/WSを同時に計画修正し、material scopeの同意を確認する。

## Detailed procedure / q585 investigation

[Origin p001](../phase001/phase.md)、[input/source survey](../phase001/survey.md)、[native環境](../native-environments.md)、[形式/CI契約](../package-contract.md)を使用。
Fedora44 Generic44-1.7 x86_64の公式image/hashとsigned CHECKSUM検証済みtrust rootを候補入力とする。native gcc/repo/rpm-buildでproduction stageをbinary rpm化、auto system Requiresとprivate self-contained依存の境界、config(noreplace)/licenseを設定する。
後続command/証拠: make keiland-linux-fedora44、native RPM headerのOS/CPU/version/Requires/Provides/scripts/file owner/modeをrpm -qpでquery、rpm2cpio+cpioの独立展開とmanifest比較。BRP/strip/debugによるstage変更は拒否、新OS boot条件を次Queue承認前に照合。
Prerequisitesは上記のcleared Phaseと実出力のまま。p001の未決D1/D2・候補version等が解消され、当Phaseのexact Queueにinput/boot/command/timeboxを保存するまで実装を開始しない。

## Clearance criteria / verification

make keiland-linux-fedora44で実rpmを生成、RPM query/extraction等でformat/CPU/payload/mode/依存を独立確認。ホストやFreeBSD packageの生成は行わない。

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

2026-10-02 / ws112-package-plan-20261002-ws112-p004-created: userの5OS package計画をこの有限Phaseへ分割、Status planned・Queue none。RPi arm64回答を契約に反映。詳細とscopeはWS/design参照。GitHub body/comment/Project公開は保留、local/outboxに記録。

2026-10-02 / ws112-q585-contract-detail: [p001](../phase001/phase.md)の一次資料/実source調査で当Phaseのprocedure/command/証拠を具体化。上記のnative環境/形式/CI契約へ対応づけ、既存prerequisites・受け入れ・Queue none・Status plannedを保持。再開はp001残件と必要な実出力を照合後の当Phaseだけの承認済Queue。[WS要約](../ws.md#event-history)。GitHub deliveryはmain canonical outboxへ。
