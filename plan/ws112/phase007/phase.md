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
