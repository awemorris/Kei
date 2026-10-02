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

q585-i01実行中。[調査証拠と未決](survey.md)へ実source gap、5OS input候補、公式checksum/index/catalog照合、共通payloadを記録。base `0e68854ace6a84b06eb23268ae75c4cf7b79b6da`。
Skipped: OS image取得、signature/image本体検証、implementation/build/guest/CI/release/導入・runtime。
Resume: 07:12〜08:12 UTCの有限調査内で形式/依存/build環境を具体化。新OS boot方針・RPi環境・trust rootが未知のままclearしない。

## Event history

2026-10-02 / ws112-package-plan-20261002-ws112-p001-created: userの5OS package計画をこの有限Phaseへ分割、Status planned・Queue none。RPi arm64回答を契約に反映。詳細とscopeはWS/design参照。GitHub body/comment/Project公開は保留、local/outboxに記録。

2026-10-02 / ws112-rpi-build-only-20261002: current userのRPi build-only受け入れを反映。影響するp001/p003/p007・WS/design・release方針を更新、Queue/実装許可は追加しない。GitHub event deliveryは保留。

2026-10-02 / ws112-q585-survey-checkpoint-1: 承認済みp001調査を開始。既存source gapと5OS input/hash候補を保存、runtime/新OS合格は未実施。新3OS boot例外・RPi実rootfsのbuild環境・署名trust rootを未決としてmainへ報告。Status in-progress、後続Queueなし、GitHub delivery保留。
