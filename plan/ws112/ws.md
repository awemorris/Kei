<!-- awesome-plan project=zedbsd record=ws112 -->

# WS112: Linux 5種類のパッケージ作成とCIリリース配布

Status: incomplete
Primary Milestone: MG007
Related Milestones: MG001（追跡可能なbuild/配布記録）、MG006（既存Keilandの配布）
Parent: [Master](../master.md)
Queue: q585 / A2（p001契約調査のみ）
Resume point: q585-i01/p001の有限契約調査中。5OS公式input/署名/形式契約を記録、RPiは外側既存Debian QEMU＋内側公式arm64 rootfs/QEMU-user方式採用。Fedora/Arch boot条件D1はmainのuser返答待ち。実装は後続Queue未承認。

## Objective / scope

指定5つのmake targetでDebian 13、Ubuntu 26.04、Raspberry Pi OS、Fedora 44、Arch LinuxのKeiland binary packageを作り、既存CIで全5種類をrelease filesへ含める。
Raspberry Pi OSはuser回答によりarm64、追加指示でbuild/deb生成のみで受け入れ、GPU/GUI/表示確認は不要。Debian/Ubuntuは既存amd64、Fedora/Archは既存方針と同じx86_64を計画案とする。
必要なファイル/metadataをad hocに直接生成・圧縮してよく、正式devtool一式は必須にしない。CIで導入・GUI等の動作確認は不要。
FreeBSDはソースからbuild/installする前提、package作成は対象外。

対象: root make入口、Linux native stage/production manifest、release tools/入力、5OS package metadata、CI/release定義・検証・文書。
renderer/OSbackend修理・新CPU版・HAL/toolchain・FreeBSD packaging・source package/署名基盤・--testing/--login実装は含めない。

## Acceptance（将来の実装）

| ID | 完了条件 |
| --- | --- |
| P1 | 指定5 make targetがそれぞれ対応OS/CPUの実binary packageを生成する（deb3、rpm1、Arch1）。package形式/metadata/依存/owner/modeとbuild由来を確認できる |
| P2 | 共通production payload、/opt/keiland、keiland-desktop、Linux GDM直接wayland、設定/ライセンスを収録。test/development payloadとsystem libcの置換を含めない |
| P3 | 既存CIで全5種類を作成・保存し、全packageの存在/整合をrelease前に検証。5つすべてと付随記録をrelease filesへ渡し既存image/zip配布を保持 |
| P4 | CI runtime/導入/GUI確認を必須経路から外し、FreeBSD packageを作らない。未実施のruntimeやremote releaseを検証済みと主張しない |
| P5 | 全変更の最終全文規約/manual reviewと該当build/syntax/package/CI検証に合格し、commands/versions/証拠/限界を記録する |

## Standards / dependencies / decisions

[設計](design.md)、[Guardrail](../guardrail.md)、[package方針全文](../standards/ws112-linux-packages.md)、[C全文](../coding-style.md)、[automation](../standards/automation.md#ws112-linux-package-coverage-2026-10-02)。
[入力と実sourceの調査](phase001/survey.md)、[native環境/署名/commands](native-environments.md)、[形式/依存/manifest/CI成果物契約](package-contract.md)。
WS108の実QEMU native build/payload/CIとWS105 Linux、WS111 launcherを出発点にする。completed WSを再利用せず、旧結果・失敗履歴を保持。
Debian/Ubuntu QEMU buildは継承し、新OS環境/入力/依存/format/pinsをp001で具体化。RPi OSの版とArch snapshot等は技術調査後に固定する。
既存demo focus/相対順位とWS106保留を変えない。WIP local commit可、push/実装/remote publishなし。GitHub Issue/Project同期は保留。

## Phases

| ID / link | Purpose | Goal | Status | Dependencies |
| --- | --- | --- | --- | --- |
| [ws112-p001](phase001/phase.md) | 共通契約・対象OS入力・形式を確定 | 5 targetのpayload/CPU/format/依存・build環境・成果物/CI契約を具体化 | in-progress / q585 | なし（WS108/WS105/WS111の実出力をcontextとして照合） |
| [ws112-p002](phase002/phase.md) | Debian/Ubuntu package生成を動作試験から分離 | 既存2 targetでbuild/形式検証だけのdeb生成と共通stage/記録契約を提供 | planned | ws112-p001 cleared / 確定したinputsとmanifest |
| [ws112-p003](phase003/phase.md) | Raspberry Pi OS arm64 deb | make keiland-linux-rpiと実arm64/RPi OS成果物 | planned | ws112-p002 cleared / 共通stage・成果物契約（p001のRPi確定入力を使用） |
| [ws112-p004](phase004/phase.md) | Fedora 44 rpm | make keiland-linux-fedora44とFedora 44 rpm成果物 | planned | ws112-p002 cleared / 共通stage・成果物契約（p001のFedora確定入力を使用） |
| [ws112-p005](phase005/phase.md) | Arch Linux binary package | make keiland-linux-archとArch成果物 | planned | ws112-p002 cleared / 共通stage・成果物契約（p001のArch確定入力を使用） |
| [ws112-p006](phase006/phase.md) | CI 5 target・release添付 | 全5 packageを既存CIからrelease filesへ渡す定義と失敗関門 | planned | ws112-p002/p003/p004/p005 cleared / 5種類の実package・記録 |
| [ws112-p007](phase007/phase.md) | 最終全文規約・WS受け入れ | P1〜P5を最終変更sourceと5成果物で照合 | planned | ws112-p006 cleared / 全最終source・5種類の成果物とCI定義 |

Dependency graph: WS105/WS108/WS111（context）→ p001 → p002 → {p003, p004, p005} → p006 → p007。
p006はp002を含む全4package Phaseの実出力を必要とする。各Phaseは1 Queueずつ、表は実行許可ではない。

## Event history

2026-10-02 / ws112-package-plan-20261002-created: current userの指定5OS/targets/ad hoc方式/FreeBSD source-only/CI runtime不要を保存しp001〜p007を追加。
同日のRPi回答「64 bit（arm64、推奨）」を確定契約へ反映。旧WS108の完了は保持し今後のCI runtime方針だけを置換する。
Master/Guardrail/Outlookへ反映。実装の指示ではなく新focus/最優先/Queueを作らない。GitHub body/comment/Projectはlocal outbox pending。

2026-10-02 / ws112-rpi-build-only-20261002: current userのRPi build-only受け入れを反映。影響するp001/p003/p007・WS/design・release方針を更新、Queue/実装許可は追加しない。GitHub event deliveryは保留。

2026-10-02 / ws112-plan-review-20261002: planning review PASS。7 Phaseの親/状態/Primary/依存、8 record IDの重複無し、全新規Markdown localリンク、指定5 target契約、q576 finished/WS108 completed保持を確認。git diff --check PASS、全変更はplan内のみ。28件のprepared/pending journal payloadを検査、GitHub/publication未実施。コード/build/package/GUI試験は計画scopeのため未実施。

2026-10-02 / ws112-q585-contract-detail: p001調査から5OS input/署名/環境/encoder/依存と共通source/payload/CI契約を詳細化。影響するp002〜p007のprocedureに各自のcommand/証拠/再開条件を反映、依存順・受け入れ・Queue権限は維持。RPi rootfs native build具体案と新OS boot条件はmain照合中、p001 in-progress/WS incomplete、未実行Phaseはplanned。GitHub deliveryはmainのcanonical outboxに委ねる。

2026-10-02 / ws112-q585-rpi-environment-selected: p001でmainのdelegated判断により外側既存Debian13 QEMU VM＋内側公式RPi rootfs/QEMU-user native arm64方式を採用、影響するp003とdesign/環境証拠へ根拠/境界/再開commandを反映。p003実行/実環境成立は未承認/未検証、D1 Fedora/Arch boot適用はmainのuser返答待ち。WS incomplete保持。

2026-10-02 / ws112-q591-p002-candidate: main指示で[p002候補](phase002/queue-candidate.md)を具体化しorigin p001/p002にも準備eventを保存。後続Queue scope/承認はmain所有、p002 planned、実装未実行。
