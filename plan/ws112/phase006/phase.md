<!-- awesome-plan project=zedbsd record=ws112-p006 -->

# ws112-p006: CI 5 target・release添付

Parent: [WS112](../ws.md)
Status: planned
Disposition: normal
Primary Milestone: MG007（WSから継承）
Queue / attempts: none / 実装未承認
Goal: 全5 packageを既存CIからrelease filesへ渡す定義と失敗関門
Prerequisites: ws112-p002/p003/p004/p005 cleared / 5種類の実package・記録
Investigation bound: 60分の有限1Phase Queue案。具体的なscope/timebox/commandを選定時に再確認する。

## Scope / procedure / affected components

.github/workflows/ci.ymlのmatrix/needs/download/verifier/release本文・filesを更新。全5 artifactを検査し、runtime smoke/PNG必須関門を外す。既存image/zip jobを保持。

[共通設計](../design.md)の対応節を使用する。自分のPhase以外の受け入れを変更する必要が出たら、依存/foreign Phase/WSを同時に計画修正し、material scopeの同意を確認する。

## Detailed procedure / q585 investigation

[Origin p001](../phase001/phase.md)、[input/source survey](../phase001/survey.md)、[native環境](../native-environments.md)、[形式/CI契約](../package-contract.md)を使用。
既存image/zipと5OS package matrix全件をrelease needsに設定。target_id/CPU/format/source/inputと20file集合を検査し、downloadの重複上書きを防止、runtime PNG/smoke必須を解除する。
後続command/証拠: 全5実package+3sidecars setをnative metadata/manifest/checksumで受理。missing/duplicate/truncated/wrong OS・CPU・source・hash・dirty/path traversal/test payloadの有限各1例を拒否、PNGだけで欠損packageを受理しない。既存CI/YAMLをlocal検査しremote未実施を明記。
Prerequisitesは上記のcleared Phaseと実出力のまま。p001の残存判断と定義済み契約を照合し、当Phaseのexact Queueにinput/適用boot方法/command/timeboxを保存するまで実装を開始しない。D2は[環境契約](../native-environments.md)の採用方式に従い、実成立は担当Phaseで確認する。

## Clearance criteria / verification

YAML/展開script/merged artifact検証で5OS/CPUすべてがrelease対象となる。欠落・破損・重複・別OS/CPU/sourceが拒否される。runtime testがCIに残っていないことを確認、remote実行/公開の実施有無を区別。

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

2026-10-02 / ws112-package-plan-20261002-ws112-p006-created: userの5OS package計画をこの有限Phaseへ分割、Status planned・Queue none。RPi arm64回答を契約に反映。詳細とscopeはWS/design参照。GitHub body/comment/Project公開は保留、local/outboxに記録。

2026-10-02 / ws112-q585-contract-detail: [p001](../phase001/phase.md)の一次資料/実source調査で当Phaseのprocedure/command/証拠を具体化。上記のnative環境/形式/CI契約へ対応づけ、既存prerequisites・受け入れ・Queue none・Status plannedを保持。再開はp001残件と必要な実出力を照合後の当Phaseだけの承認済Queue。[WS要約](../ws.md#event-history)。GitHub deliveryはmain canonical outboxへ。
