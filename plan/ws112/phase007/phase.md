<!-- awesome-plan project=zedbsd record=ws112-p007 -->

# ws112-p007: 最終全文規約・WS受け入れ

Parent: [WS112](../ws.md)
Status: planned
Disposition: normal
Primary Milestone: MG007（WSから継承）
Queue / attempts: none / 実装未承認
Goal: P1〜P5を最終変更sourceと5成果物で照合
Prerequisites: ws112-p006 cleared / 全最終source・5種類の成果物とCI定義
Investigation bound: 60分の有限1Phase Queue案。具体的なscope/timebox/commandを選定時に再確認する。

## Scope / procedure / affected components

全WS source/config/docsの全文/manual review、該当formatter/lint/syntax/analysis、5 targetのbuild/形式/metadata/manifest/依存、release gateを再照合。最終修正が検証を無効にした箇所だけ再実行。

[共通設計](../design.md)の対応節を使用する。自分のPhase以外の受け入れを変更する必要が出たら、依存/foreign Phase/WSを同時に計画修正し、material scopeの同意を確認する。

## Detailed procedure / q585 investigation

[Origin p001](../phase001/phase.md)、[input/source survey](../phase001/survey.md)、[native環境](../native-environments.md)、[形式/CI契約](../package-contract.md)を使用。
全WS最終source/config/docsと5実成果物の全文/manual conformance。共通契約のnative toolchain/source archive/payload/loader dependency/config/licenseとCI全件gateを実最終revisionへ照合する。
後続command/証拠: 最終変更範囲のPython/shell/Makefile/YAML syntax/manual/git diff --check、該当native build/独立package auditとbounded verifier否定試験結果を記録。追加Cがあれば全文規約/formatter/必要compileを適用。無効化された証拠だけ再確認し、runtime/実機/FreeBSD packageを追加しない。
Prerequisitesは上記のcleared Phaseと実出力のまま。p001の残存判断と定義済み契約を照合し、当Phaseのexact Queueにinput/適用boot方法/command/timeboxを保存するまで実装を開始しない。D2は[環境契約](../native-environments.md)の採用方式に従い、実成立は担当Phaseで確認する。

## Clearance criteria / verification

全変更の規約確認と5 package/CI定義が合格。commands/versions/source revision/input/artifacts/例外/skipped checks/限界を記録し、WS自体のP1〜P5を判定。CI runtime/FreeBSD packageを追加関門にしない。RPiはbuild/deb生成の受け入れに従いGPU/GUI/実機確認を追加しない。

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

2026-10-02 / ws112-package-plan-20261002-ws112-p007-created: userの5OS package計画をこの有限Phaseへ分割、Status planned・Queue none。RPi arm64回答を契約に反映。詳細とscopeはWS/design参照。GitHub body/comment/Project公開は保留、local/outboxに記録。

2026-10-02 / ws112-rpi-build-only-20261002: current userのRPi build-only受け入れを反映。影響するp001/p003/p007・WS/design・release方針を更新、Queue/実装許可は追加しない。GitHub event deliveryは保留。

2026-10-02 / ws112-q585-contract-detail: [p001](../phase001/phase.md)の一次資料/実source調査で当Phaseのprocedure/command/証拠を具体化。上記のnative環境/形式/CI契約へ対応づけ、既存prerequisites・受け入れ・Queue none・Status plannedを保持。再開はp001残件と必要な実出力を照合後の当Phaseだけの承認済Queue。[WS要約](../ws.md#event-history)。GitHub deliveryはmain canonical outboxへ。
