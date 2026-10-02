<!-- awesome-plan project=zedbsd record=ws112-p002 -->

# ws112-p002: Debian/Ubuntu package生成を動作試験から分離

Parent: [WS112](../ws.md)
Status: planned
Disposition: normal
Primary Milestone: MG007（WSから継承）
Queue / attempts: none / 実装未承認
Goal: 既存2 targetでbuild/形式検証だけのdeb生成と共通stage/記録契約を提供
Prerequisites: ws112-p001 cleared / 確定したinputsとmanifest
Investigation bound: 90分の有限1Phase Queue案。具体的なscope/timebox/commandを選定時に再確認する。

## Scope / procedure / affected components

tools/release/keiland-linux-debとroot targetの必須経路を整理。native QEMU buildを保ち、fresh guest/runtime工程を任意経路へ分離。共通launcher収録とartifact metadata/verifierの拡張点を整える。

[共通設計](../design.md)の対応節を使用する。自分のPhase以外の受け入れを変更する必要が出たら、依存/foreign Phase/WSを同時に計画修正し、material scopeの同意を確認する。

## Detailed procedure / q585 investigation

[Origin p001](../phase001/phase.md)、[input/source survey](../phase001/survey.md)、[native環境](../native-environments.md)、[形式/CI契約](../package-contract.md)を使用。
既存native QEMU buildをruntimeから分離し、共通stageとsource archiveの外側gzip時刻/filename固定を提供。既存2inputを保持、launcher/test app除外・conffiles・native dpkg-shlibdeps/dpkg-debと独立dpkg query/extractionを照合する。
後続command/証拠: make keiland-linux-debian / make keiland-linux-ubuntu2604、専用outputの各deb+3sidecars、source hash再現、readelf CPU/SONAME/RUNPATH、package展開と共通manifest比較。任意runtimeの入口と過去WS108証拠は保存。
Prerequisitesは上記のcleared Phaseと実出力のまま。p001の未決D1/D2・候補version等が解消され、当Phaseのexact Queueにinput/boot/command/timeboxを保存するまで実装を開始しない。

## Clearance criteria / verification

make keiland-linux-debian / make keiland-linux-ubuntu2604で各debと付随記録を生成、独立format/payload/CPU/ELF依存監査が成立。runtime/PNGなしでもpackage検証が成立し、既存test-only payloadを含まない。

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
[q591 scope/verification候補](queue-candidate.md)をmainの依頼で準備。同documentは実行承認ではなく、p001 clearanceとexact Queue保存まで開始しない。

## Event history

2026-10-02 / ws112-package-plan-20261002-ws112-p002-created: userの5OS package計画をこの有限Phaseへ分割、Status planned・Queue none。RPi arm64回答を契約に反映。詳細とscopeはWS/design参照。GitHub body/comment/Project公開は保留、local/outboxに記録。

2026-10-02 / ws112-q585-contract-detail: [p001](../phase001/phase.md)の一次資料/実source調査で当Phaseのprocedure/command/証拠を具体化。上記のnative環境/形式/CI契約へ対応づけ、既存prerequisites・受け入れ・Queue none・Status plannedを保持。再開はp001残件と必要な実出力を照合後の当Phaseだけの承認済Queue。[WS要約](../ws.md#event-history)。GitHub deliveryはmain canonical outboxへ。

2026-10-02 / ws112-q591-p002-candidate: main指示で後続候補のexact scope/必要出力/資源/command/有限verificationを別documentへ具体化。現行Status planned・Queue none、実装未実行を保持。origin p001とWSへ準備eventを保存、mainが後続Queue選択/承認を所有する。
