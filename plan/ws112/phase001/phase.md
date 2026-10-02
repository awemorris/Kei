<!-- awesome-plan project=zedbsd record=ws112-p001 -->

# ws112-p001: 共通契約・対象OS入力・形式を確定

Parent: [WS112](../ws.md)
Status: uncleared
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

q585-i01終了、outcome uncleared。2026-10-02 07:12〜08:11 UTC（約59分、60分上限内）。[調査証拠と未決](survey.md)へ実source gap、5OS input候補、公式checksum/index/catalog照合、共通payloadを記録。base `0e68854ace6a84b06eb23268ae75c4cf7b79b6da`。
[環境/署名/後続command](../native-environments.md)と[形式/依存/CI成果物契約](../package-contract.md)を追加。Ubuntu/Fedora/Arch checksum署名は小metadataで実検証、image本体は未検証。
[基準別証拠/commands/results/limits](criteria.md)を保存。D1だけが未解決人間判断、他の実image/native ELF/tool版/経過時間は後続実装verification契約と区別する。
Skipped: OS image取得、image本体署名/hash検証、implementation/build/guest/CI/release/導入・runtime。
Checks: final local links148件/0error、git diff --check PASS、非C近傍形式/full package方針/manual scope・source/一次資料・dependency/event review PASS。全変更はplan/ws112内、C/production source変更無し。
Commits: A2-001 `67c78c0e6`、A2-002 `dbcd3afa9`、A2-003 `79e889209`、A2-004 `4642a7d68`、A2-005 `dff4b7401`（全WIP、main ACK `ed6d2d3c7`まで統合）。終端commit SHAはmain MR/outcome記録へ渡す。
Reason / resume: D1 Fedora/Arch loopback SSH/QMP PNG方式適用がuser返答待ちで「未解決人間判断なし」の基準を満たさない。mainが判断元・共有Guardrailを保存後にp001の全契約を再評価する。D2は委任技術判断で採用済み、実input/guest/ABI/encoderの未検証は後続Phaseの実verificationへ保持。q591は候補のまま、whole p001 clearanceと新exact Queueなしに開始しない。

## Event history

2026-10-02 / ws112-package-plan-20261002-ws112-p001-created: userの5OS package計画をこの有限Phaseへ分割、Status planned・Queue none。RPi arm64回答を契約に反映。詳細とscopeはWS/design参照。GitHub body/comment/Project公開は保留、local/outboxに記録。

2026-10-02 / ws112-rpi-build-only-20261002: current userのRPi build-only受け入れを反映。影響するp001/p003/p007・WS/design・release方針を更新、Queue/実装許可は追加しない。GitHub event deliveryは保留。

2026-10-02 / q585-start: 最新userのAgent A N=3開始指示から、p001調査のみをA2へ最大60minで選定。input/format/native環境/CI契約を文書化し、製品code変更やguest取得/起動は後続。scopeとclearanceはlane/snapshotへ固定。

2026-10-02 / ws112-q585-survey-checkpoint-1: 承認済みp001調査を開始。既存source gapと5OS input/hash候補を保存、runtime/新OS合格は未実施。新3OS boot例外・RPi実rootfsのbuild環境・署名trust rootを未決としてmainへ報告。Status in-progress、後続Queueなし、GitHub delivery保留。

2026-10-02 / ws112-q585-survey-checkpoint-2: 版付きArch URL、公式checksum署名trust root/実暗号検証、RPi公式.infoのnative inventoryを追加。dpkg/RPM/Arch encoderと独立監査、payload/source gzip/CI全5OS集合の契約を具体化。D1/D2判断と実guest照合は未決、Status in-progress。Scope/外部Phaseのclear条件/Queue許可は変更せず、詳細設計の次反映はWSと各foreign Phaseへ記録予定。

2026-10-02 / ws112-q585-contract-detail: 実source/公式input/package仕様から詳細環境/形式/manifest/CI契約を保存し、foreign p002〜p007それぞれにprocedureとcommand/verification/resumeを追記、WS/designへ同eventを記録。依存順/受け入れ/Queue membershipを変更せず、未実行Phaseはplanned。RPiの外側既存Debian VM+内側native rootfs案をmainのdelegated authority照合へ提出。D1/D2は照合終了まで未決、p001 in-progress。

2026-10-02 / ws112-q585-rpi-environment-selected: mainのdelegated technical判断でD2を解決。外側既存pin Debian13 QEMU VM＋内側公式RPi Lite Trixie arm64 rootfs/QEMU-user/native compilerを採用、真のOS/CPU/compiler/headers/libcとbuild-only契約を保持。外側既存Debian例外を用いRPi kernel bootなし、実成立確認/p003実行許可は追加しない。変更したforeign p003、WS/design/環境契約にも根拠とresumeを保存。D1 Fedora/Archはmainがuser確認中、in-progress保持。

2026-10-02 / ws112-q591-p002-candidate: main指示で後続p002 scope/command/verification候補を[文書](../phase002/queue-candidate.md)へ準備、p002/WSにもevent保存。p001残件やQueue承認を代替せず、後続実装は開始しない。

2026-10-02 / ws112-q585-evidence-checkpoint: D2選択後のcurrent procedure参照を更新、source識別version契約とnative parser検証を明示。既存WS108にelapsed未保存/新OS方式未実測の限界を保存、基準別evidence/commands/results/skips/resumeをcriteriaへ追加。D1返答待ちのためin-progress保持、終端clearはまだ宣言しない。

2026-10-02 08:11 UTC / ws112-q585-outcome: q585-i01/p001 uncleared。有限契約調査は終了、5OS inputs/署名/環境/形式/依存/共通payload/source/CI契約と後続commandを保存、mainへ5小checkpoint統合済み。D1 user返答未受領により人間判断の残件を解除しない。実image/guest/build/package/runtime/CI/公開は未実施、理由/基準別証拠/再開はcriteriaと上記。p002〜007 planned、WS incomplete、q591候補だけを保持し同sessionでmain次指示を待機。Phase event/GitHubとQueue/共有投影のcanonical反映はmainへ依頼。
