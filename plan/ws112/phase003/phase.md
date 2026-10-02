<!-- awesome-plan project=zedbsd record=ws112-p003 -->

# ws112-p003: Raspberry Pi OS arm64 deb

Parent: [WS112](../ws.md)
Status: planned
Disposition: normal
Primary Milestone: MG007（WSから継承）
Queue / attempts: none / 実装未承認
Goal: make keiland-linux-rpiと実arm64/RPi OS成果物
Prerequisites: ws112-p002 cleared / 共通stage・成果物契約（p001のRPi確定入力を使用）
Investigation bound: 90分の有限1Phase Queue案。具体的なscope/timebox/commandを選定時に再確認する。

## Scope / procedure / affected components

対象RPi OS rootfs/native arm64 toolchain/build環境を実装し、make入口とdeb metadata/依存/入力・CPU記録を追加。汎用Debian packageの名称差替えをしない。

[共通設計](../design.md)の対応節を使用する。自分のPhase以外の受け入れを変更する必要が出たら、依存/foreign Phase/WSを同時に計画修正し、material scopeの同意を確認する。

## Detailed procedure / q585 investigation

[Origin p001](../phase001/phase.md)、[input/source survey](../phase001/survey.md)、[native環境](../native-environments.md)、[形式/CI契約](../package-contract.md)を使用。
公式2026-09-15 RPi Lite Trixie arm64 image/rootfsを候補入力とする。外側既存Debian QEMU VM+内側RPi rootfs/native arm64 compilerのQEMU-user具体案と補助kernelとの差を環境契約に記録、採用/boot条件はmain照合中。
後続command/証拠: make keiland-linux-rpi、RPi imageのcompressed/expanded hashと利用可能な署名、RPi marker/packages/repo、compiler自体と全出力ELF AArch64、native dpkg DB/dependency/encoderと独立deb展開を保存。outer kernelとinner rootfsのidentityを区別、elapsedを工程別記録。
Prerequisitesは上記のcleared Phaseと実出力のまま。p001の未決D1/D2・候補version等が解消され、当Phaseのexact Queueにinput/boot/command/timeboxを保存するまで実装を開始しない。

## Clearance criteria / verification

make keiland-linux-rpiが実RPi OS/arm64でbuildしたdebを生成し、OS/CPU/format/payload/依存とsourceを独立確認。OS boot困難/依存不足/時間超過なら記録し、偽のRPi buildでclearしない。

RPiについてはuserの追加指示「ビルドが通ればOK」により、build/deb生成で受け入れる。user申告のQEMU GPU制約を理由に、GPU/GUI/表示・実機試験をclear条件から除外。CPU/形式等の確認はpackage整合であり、runtime代替試験を追加しない。

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

2026-10-02 / ws112-package-plan-20261002-ws112-p003-created: userの5OS package計画をこの有限Phaseへ分割、Status planned・Queue none。RPi arm64回答を契約に反映。詳細とscopeはWS/design参照。GitHub body/comment/Project公開は保留、local/outboxに記録。

2026-10-02 / ws112-rpi-build-only-20261002: current userのRPi build-only受け入れを反映。影響するp001/p003/p007・WS/design・release方針を更新、Queue/実装許可は追加しない。GitHub event deliveryは保留。

2026-10-02 / ws112-q585-contract-detail: [p001](../phase001/phase.md)の一次資料/実source調査で当Phaseのprocedure/command/証拠を具体化。上記のnative環境/形式/CI契約へ対応づけ、既存prerequisites・受け入れ・Queue none・Status plannedを保持。再開はp001残件と必要な実出力を照合後の当Phaseだけの承認済Queue。[WS要約](../ws.md#event-history)。GitHub deliveryはmain canonical outboxへ。
