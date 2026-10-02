<!-- awesome-plan project=zedbsd record=ws109 -->

# WS109: Linux 版 Keiland を FreeBSD 15 へ移植

<!-- awesome-plan-current:start -->
Status: incomplete
Primary Milestone: MG006
Related Milestones: MG007
Parent: [Master](../master.md)
Queue: なし（q572 finished）
Resume point: p005 cleared; Actual sevennativeGPU mainapps/PTY/fileopens plus AppHome PASS; fullsource C fixes/review/nativeheaderELF/3OS builds/finalboot/docs verified. [result](/home/awe/zedBSD-claude1/plan/history/ws109/q572/result.md). F1..F5 satisfied under userselectedi915/physicalWiFiwaiver; BUG130 upstreamtracking, ownVMstopped/VFIOpreserved.
<!-- awesome-plan-current:end -->

## 目標・決定の出典

Linux 版の共通描画を再利用して、FreeBSD 15 の native Keiland compositor と主なアプリを動かし、audio/network/WiFi の FreeBSD backend を用意する。

2026-10-01 ユーザーの [レビューコメント](../reviews/2026-10-01-review.md)を根拠に計画。
Primary MG006 にこの目標の成果を提供。Related MG007 は技術/配布/検証の supporting 出力。
WS completion と milestone 全体の acceptance は別に確認する。

## 範囲

[移植設計・先行調査](design.md)。F-065 の FreeBSD 分を promote。初期 target は FreeBSD 15.x amd64 の案。
Linuxulator に頼らず native libc/toolchain と system Vulkan を使い、独自の WSI/Wayland/libkeiland、`/opt/keiland` を保つ。
描画・composition・renderer は Linux で使っている共通実装を再利用する。
Linux 専用の fd/sync ioctl、VT/seat/device、入力、動的 loader の部分は p001 で可用性を照合して OS module に閉じる。
audio/network/WiFi は FreeBSD backend。既存 WPA の共通 wire は再利用候補。
FreeBSD kernel/driver の移植、未知の GPU 全機種対応、互換 Qt/GTK、FreeBSD pkg の配布はこの初期目標に足さない。
2026-10-02ユーザー承認により、既存drm-kmod（GPLv2を含む）をsystem driverとして利用可。Keiland sourceとsystem driver/serviceのlicenseを分けて確認する。
system の graphics stack に LinuxKPI/DRM がある場合も、GPL が無いと確認前に主張しない。

## WS 自身の完了条件

- F1: 固定 FreeBSD15 version/amd64、ユーザー指定の QEMU i915 PCI passthrough device/driver、検証環境、license、native ABI の対応表を確定。
- F2: 独立 native build/DESTDIR install が warning 0、標準公開 header と ELF/後段 Vulkan の symbol chain が利用可能。
- F3: ユーザー指定の FreeBSD QEMU i915 PCI passthroughで実GPUを使用し、共通描画/入力/同期/sessionの利用可能な経路を確認。元の実機準備・WiFi実機関門はユーザー免除、bootだけでclearとしない。
- F4: audioの列挙/音量/mute、network/有線をQEMU実backendで確認。WiFi backendのnativeABI/拒否/WPAwireを確認し、mockを実WiFiから区別。実機radio操作はユーザー免除。
- F5: FreeBSD QEMU i915 PCI passthroughで主なapp/操作、Linux/zedBSD影響範囲の回帰、全文規約/運用文書を確認。Venus未実行を成功としない。

## 依存・所有

WS104/105 の境界・Linux 出力は completed context。共通描画/API の仕様を保ち、WS106 の移動後の対応 app locator を用いる。WS107 は browser を初期対象に加える場合だけ依存候補。FreeBSD SSH/QMP検証は2026-10-02の直接のユーザー承認による（[全文](../standards/ws109-native.md)）。

## Phase 表（後続は設計案）

| ID / Phase | 目的 | Goal | Status | 依存 |
| --- | --- | --- | --- | --- |
| [ws109p001](phase001/phase.md) | FreeBSD15 の graphics/OS 契約と環境を調査 | F1 と port の対応表/実現可能な F2〜F5 手順。Linux DMA_BUF sync と同等の能力が無ければ別方式の影響と選択をユーザーに提示してから dependent 実装を選定。 | cleared / q551 | WS105 output（context） |
| [ws109p002](phase002/phase.md) | native build・library と system Vulkan chain | F2。glibc 固有の loader binding に頼らないことを実際の FreeBSD で検証。 | cleared / q565 | p001 |
| [ws109p003](phase003/phase.md) | 共有描画と FreeBSD の device/session/input 境界 | F3。Linux source を丸ごと複製した renderer を作らない。device release/fd lifetime も確認。 | cleared / q570 | p002 L1 verified output |
| [ws109p004](phase004/phase.md) | audio・network・WiFi の FreeBSD backend | F4。PCM 再生を含めるかは p001 で確定し、WS105 の音量 backend と取り違えない。 | cleared / q571 | p002 L1 verified output |
| [ws109p005](phase005/phase.md) | 全文規約・主な app と3 OS の最終回帰 | F1〜F5。FreeBSD build のみを移植完了としない。未実施の GPU/実機/OS version を記録。 | cleared / q572 | p002 F2 verified + p003/p004 implementation outputs for subset; actual QEMU i915 GPU output for whole |


依存は表の prerequisite → dependent。context は選定された作業ではない。
後続 Phase の detail は p001 の確定設計から作る。表/実 record/Queue を一緒に整える。

## 制約・標準・検証の扱い

[Guardrail](../guardrail.md)、[AGENTS.md](../../AGENTS.md)、[C 規約全文](../coding-style.md)、
[自動化の対応](../standards/automation.md)を適用する。簡約版は無い。コード生成前に全文の該当節を読み、
最後の conformance Phase では本 WS の全 source 変更を全文でレビューする。
clang-format 19 / style-check は補助。無関係な一括整形、`make check`、`.internal/` の参照は行わない。
意味を変えない移動は warning 0 の build と最後の boot を関門にする。API・platform・packaging の変更には意味のある契約検証を追加する。
image build は直列。zedBSD の起動は boot-test.sh と PNG、console/serial log を起動の証拠にしない。
commit は `WIP`、push なし。GitHub 公開・Issue/Project 更新は現在 deferred、local cache/outbox に記録する。

## 現状・再開

2026-10-01、source `01c754a0` を調査して計画を新設。実装・移動・CI job 追加は未実施。
Queue は無し。p001 の計画を確認して有限 Queue を選定する。後続 Phase は案で、調査結果によって詳細化する。
既存 q538 は finished、WS105 の完了範囲を拡張しない。

## イベント

2026-10-01 / review-20261001-planning: ユーザーのレビューコメントから WS を新設。
範囲・受け入れ・Phase 案を保存、Master / Outlook と照合した。新規実装の Queue 承認は未取得。
[決定の出典と関連 WS](../reviews/2026-10-01-review.md)。公開時にはこのイベントを WS に届ける（現在 outbox 保留）。

2026-10-02 / ws109-q550-start: 上記ユーザー指示を受け、WS108完了後のq550でp001調査を開始。後続はdraftのまま、未決の製品/ライセンス/検証判断を黙って変えない。PhaseとWSのremote event publication pending。

2026-10-02 / ws109-q550-survey: p001の[実source/公式ABI調査](../history/ws109/q550/survey.md)を保存。fixed15.1amd64 image/hashと未起動guest準備済み。D1drm GPLv2 /D2FreeBSD起動方法 /D3realWiFi・graphicsdeviceの判断待ち。F1未充足、p002〜p005はplanningを保つ。Linux描画のcopy実装、productionstub、WiFimockによる受け入れ置換は行わない。

2026-10-01T18:41:24.330785+00:00 / ws109-q550-uncleared: p001は調査/準備を実施したがF1環境・license未確定でuncleared。WSはincomplete、p002〜p005はplanning。321 unique source/公式native ABIの対応表、FreeBSD15.1amd64 image/hash検証と未起動8GiB guest準備を保存。F1はdriver GPLv2利用・FreeBSD起動SSH/QMP例外・real graphics/WiFi検証環境が未確定。3質問への回答待ち、production実装/native build/runtime未実施。 [履歴](../history/queue-q550.md)・[調査](../history/ws109/q550/survey.md)。GitHub Phase/WS event publication pending。

## 2026-10-02 / ws109-user-decisions-20261002

ユーザー「drm-kmodを利用OKです。FreeBSDにも例外を適用します。FreeBSD実機は用意しておくので、作業を進めておいてください。」（2026-10-02 JST、このchat）。D1:既存FreeBSD drm-kmod利用可、GPL-free systemstack条件をこの範囲で置換。Keiland sourceの寛容license/外部実装を取り込まない境界は維持。D2:WS109専用FreeBSD QEMU guestのloopback SSH/QMP PNG検証を承認。D3:実機はユーザーが準備、入手前にnative build/backend実装を進める。実GPU/WiFi結果は将来の実機関門に残し、mock/QEMUbuildで代替しない。

p001の環境/ABI調査を再attempt。実機の詳細確定/実表示/実WiFiはp003/p004/p005の後段に残し、native build用のverified outputから先行実装を進める。WS F1〜F5の受け入れを減らさず、各部分の検証場所を明示する。後続Phaseは自分の必要なprerequisite outputを確認し、実機未着の部分をpartial scopeとして分ける。旧q550 unclearedは保持。

ws109-q551-native-environment: p001 native design saved; p002 L1/L2 scoped outputs clarify p003/p004 prerequisites and p005 final integration/physical gate. Every affected Phase own structural event saved; remote delivery pending.

2026-10-01T19:21:04.454764+00:00 / ws109-q551-cleared: p001 cleared。FreeBSD15.1-p4 amd64 nativeClang19/headers/OSS/evdev/vtnet/pkg/licenseと後続native手順を確認。[environment](../history/ws109/q551/environment.md)。実GPU/seat/fence/WiFiの実機関門はp003/p004/p005に保持、WS incomplete。

2026-10-01T19:26:37.868096+00:00 / ws109-q552-uncleared: p002 uncleared。L1独立native Makefile/52sources、OSsync wrappersと共有WSI変更を実装。Linux Vulkan library buildはwarning0/exit0。FreeBSD buildはlibwayland/wire.cの未計画CMSG_ALIGN依存で失敗、runtime/DESTDIR未実施。[失敗](../history/ws109/q552/l1-build.txt)。新attemptで標準CMSG_SPACE/CMSG_LENによる整列とnative/Linux fd受渡しをscopeに加え、残るnative buildを再検証する。

ws109-q553-redesign: same p002 L1 retry adds wire.c standard CMSG correction and real native/Linux fd contract checks; public interfaces/dependencies/WS acceptance unchanged. q552 failure preserved.

2026-10-01T19:40:30.365350+00:00 / ws109-q553-cleared: p002 uncleared。L1 native52sources/sevenELF＋DESTDIR/publicheaders/nativeVulkan1MiBchain、actualfd/CLOEXEC/truncation/errno所有を検証。Linux library/chain/fullWaylandsuite回帰PASS。[result](../history/ws109/q553/result.md)。fullF2のlibkeiland/compositor/apps統合はp003/p004後のL2、全source規約/zedBSD/physical gatesはp005に保持。

ws109-q554-detail: p004 audio-only nativeOSS implementation/probe selected from verified q553 L1 output; fullF4 network/WiFi and p002 L2 remain. No dependency/scope change to other Phases.

2026-10-01T19:52:10.087368+00:00 / ws109-q554-cleared: p004 uncleared。FreeBSDOSS audio実装/nativeHDA volume/mute/independent libmixer/外部変更refresh、unprivileged同一subscription再接続を実検証。音量86/86/offとdevice権限を復元。[result](../history/ws109/q554/result.md)。network/WPA/actualWiFi・全F4/GUI統合は後続、WS incomplete。

ws109-q555-detail: p004 nativeAF_LINK/net80211/WPA socket/DNS/credential shared mechanism selected from q553/q554 verified outputs. fullF4 actualWiFi and p002L2/p003/p005 acceptance preserved. Internal backend/private boundary changes; no changed foreign Phase criteria.

2026-10-01T20:19:34.541983+00:00 / ws109-q555-cleared: p004 uncleared。nativeAF_LINK/net80211/共有WPA実装、realvtnet/DNS/権限/flag保持/carrierdown→up、nativeUnix/mockWPAwire/期限とLinuxlibrary/link契約PASS。[result](/home/awe/zedBSD-claude1/plan/history/ws109/q555/result.md)。actualWiFi/fullF4・mainGUI/全source規約/3OS/実GPUは保持、WS incomplete。

ws109-q556-ui-libraries: p002 L2 library subset consumes verified p004 q554/q555 source before whole physicalF4 clearance. Existing p003/p005 dependencies and fullWS acceptance unchanged.

2026-10-01T20:26:55.542146+00:00 / ws109-q556-cleared: p002 uncleared。real native113sources/10ELF＋DESTDIR/publicheaders/native libc/私有library chainを検証。installed publicclientでOSS/network/全64CPU pixels/PDFwrite-read-digest PASS。[result](/home/awe/zedBSD-claude1/plan/history/ws109/q556/result.md)。fullF2 compositor/apps・p003seat/graphics/全source規約/physical gatesは保持。

ws109-q557-shared-devices: p003 shared mechanism/native sync/header subset selected; native seat and fullF3 retained, unchanged p002L2 integration/p005 acceptance.

2026-10-01T20:40:05.932372+00:00 / ws109-q557-cleared: p003 uncleared。Linux共有dma-buf/evdev/sessionを再利用、FreeBSDsync/nativeinput header境界を実装。native4objects/exportownershipとLinuxfullcompositor/境界契約PASS。[result](/home/awe/zedBSD-claude1/plan/history/ws109/q557/result.md)。FreeBSDseat/fullcompositor/actualGPU・positiveDMA/全F3は保持。

ws109-q558-permissive-seat-client: existing p001 MITclient option concretized with pinnedarchive/license; p003 native seat/module consumes verified client output next. No changed foreignPhase acceptance or new GPLexception.

2026-10-01T22:56:18.386314+00:00 / ws109-q558-cleared: p003 uncleared。MIT seatd-only libseat nativebuild/privateinstall・実seatdで非特権evdev fd/capability/release PASS。[result](/home/awe/zedBSD-claude1/plan/history/ws109/q558/result.md)。native compositor callbacks・VT/display/actualGPU/F3は保持。

2026-10-01T23:11:23.569523+00:00 / ws109-q559-cleared: p003 uncleared。FreeBSD seatd/OS backend実装、nativeobject warning0、実VT通知/input fd停止・再取得・daemon切断/partialcleanup PASS。[result](/home/awe/zedBSD-claude1/plan/history/ws109/q559/result.md)。描画/通知collaboratorは順序観測のみ、fullnativecompositor/実GPU/physicalVT/F3は保持。

2026-10-01T23:17:08.182550+00:00 / ws109-q560-cleared: p002 uncleared。native compositor本体/shared renderer＋realFreeBSDseatをlink/DESTDIR、168uniqueC/nativeheaders/privateELF/実起動拒否cleanup PASS。[result](/home/awe/zedBSD-claude1/plan/history/ws109/q560/result.md)。apps/data/nativeGUI/actualdisplay/全F2/threeOS・p005は保持。

2026-10-01T23:30:23.236432+00:00 / ws109-q561-cleared: p002 uncleared。native PTY header/libutil・extattr adapter、実UFSのfd/path/link/コピー/権限/ERANGEと実PTY child入出力PASS。[result](/home/awe/zedBSD-claude1/plan/history/ws109/q561/result.md)。fullapps/nativeGUI/F2・threeOS/p005/physical gatesは保持。

2026-10-01T23:32:57.301647+00:00 / ws109-q562-uncleared: p002 uncleared。13apps/dataのnative Makefileを追加。実-j16buildはfiles/places.cの未計画mntent.h依存で停止。[failure](/home/awe/zedBSD-claude1/plan/history/ws109/q562/result.md)。native mount backendを別有限Queueで設計・検証し、残りbuild/install/runtimeを再開。全F2/physical/p005保持。

2026-10-01T23:37:55.818779+00:00 / ws109-q563-uncleared: p002 uncleared。native mount iterator/sharedPlacesを実装・nativecompile。fullappsはfiles/tags.cの未計画ENODATA宣言差で停止。[failure](/home/awe/zedBSD-claude1/plan/history/ws109/q563/result.md)。native ENOATTR互換名を追加し、mount/Linux契約と残りapps/data検証を次attemptで再開。

2026-10-01T23:49:51.391199+00:00 / ws109-q564-cleared: p002 uncleared。13apps/compositor/nativeall warning0、25ELF/private install/data/refusal・native辞書150tests PASS。実mount contracts/LinuxFiles回帰PASS。[result](/home/awe/zedBSD-claude1/plan/history/ws109/q564/result.md)。F2最終一式audit・p005全文規約/threeOSと実GPU/WiFi関門は保持。

2026-10-01T23:53:09.700338+00:00 / ws109-q565-cleared: p002 cleared。F2一式fresh native324unique C/all/install/header closure/25ELF・public64pixels/PDFと実Vulkan1MiBchain verified。[result](/home/awe/zedBSD-claude1/plan/history/ws109/q565/result.md)。nativeGUI/F3/F4実機とp005全文規約/threeOS/docsは未達、WS incomplete。

2026-10-02 / ws109-q566-conformance-design: p005 full standards/regression/docs subset ready using p002 cleared/q565 and verified backend sources; full physical/app acceptance retained. Origin [p005](phase005/phase.md). Remote structural event pending.

## 2026-10-02 / ws109-20261002-qemu-venus-acceptance

Current user, this chat async reply: 「実機検証は不要です。qemuでVenusが使えればclearとします。」
This explicitly replaces the earlier user-provided-machine gate. Physical GPU/display/WiFi
acceptance is waived for WS109; do not request hardware or reintroduce those gates. Required
replacement evidence is actual FreeBSD QEMU Venus usage; mere host support, headless lavapipe or
Linux/zedBSD Venus does not establish that evidence. Native backend/build/standards and affected
regression obligations remain. p004 hardware radio operations become waived, not falsely tested;
native audio/wired and native radio ABI/refusal/WPA wire evidence remains classified accurately.
p003/F3 and p005/F5 replace physical display/main-app checks with owned FreeBSD QEMU Venus-backed
checks. If the native guest stack lacks a required Venus driver, investigate a bounded actual
capability chain and expose the remaining platform/scope choice; kernel/driver port is still outside
WS109's agreed scope. Current q566 standards/regression/docs subset stays authorized; Venus
configuration/implementation is selected separately after q566. No automatic WS/Phase clearance.
Origin user decision reconciled to WS/all changed Phase own criteria, Guardrail/scoped standard,
Queue supplement and docs; remote decision/structural events pending publication.

2026-10-02T00:22:28.923483+00:00 / ws109-q566-cleared: p005 uncleared。Final changed-source standards, wire/Places fixes, native docs and affected native/Linux/zedBSD checks PASS. [result](/home/awe/zedBSD-claude1/plan/history/ws109/q566/result.md). Physical tests waived; whole p005/WS pending actual FreeBSD QEMU Venus.

ws109-q567-design: p003 bounded actual native QEMU Venus investigation replaces former physical waiting; actual kernel/ICD prerequisites checked, q566 conformance subset cleared. No kernel port authorization.

2026-10-02T00:26:10.544991+00:00 / ws109-q567-uncleared: p003 uncleared。Actual Venus-configured FreeBSD QEMU boots, but native DRM/Venus ICD absent and latest upstream virtio driver lacks HOST_VISIBLE. [result](/home/awe/zedBSD-claude1/plan/history/ws109/q567/result.md). Kernel/driver port outside WS109; concrete acceptance/scope decision requested, physical tests waived.

## 2026-10-02 / ws109-20261002-user-i915-passthrough

Current user chat reply: 「awe@10.0.10.25 でi915をPCIパススルーして、FreeBSDゲストを実行してみましょう。」
This authorizes SSH to that specified host and an owned FreeBSD QEMU guest with existing
IrisXe0000:00:02.0 passthrough. The prior loopback-only rule has a scoped host-control exception;
guest SSH remains via remote127.0.0.1 forward, QMP PNG and real native observations, no serial
logs. Physical WiFi/user-supplied-machine request remains waived. Try this explicit native i915
GPU route to address q567 Venus prerequisite; boot alone does not clear F3/F5 or claim Venus.
Remote survey: chaos/Linux6.19.13/QEMU10.0.11/7.4GiBmemory; GPU8086:46a8 alreadyvfio-pci,
IOMMUgroup0 GPUalone, no other QEMU. Use4GiB guest, owned disk copy/overlay/endpoint. No host
GPU unbinding/reboot/kernel/library replacement or unrelated VM/process changes. Existing
native i915/drm-kmod may be installed in own guest under prior permission. New driver port
still excluded. Actual native Vulkan/DRM/provider/render/lease checks required where possible.

2026-10-02T01:05:38.624771+00:00 / ws109-q568-uncleared: p003 uncleared。Native i915/Intel Vulkan1MiB/offscreen and realunpriv compositor-shm PASS; liveDMA_BUF zeroaccessflags block ioctl/Vulkanwindow. [result](/home/awe/zedBSD-claude1/plan/history/ws109/q568/result.md) / [BUG-130](/home/awe/zedBSD-claude1/plan/bugs/BUG-130.md). Nextbounded native capability adaptation; no driverpatch/falseclear. Own remoteVMrunning, baselineVFIOretained.

ws109-q569-native-zeroaccess-capability: p003 native-only recognition of BUG-130 zeroaccessfd transport, reuse existing safeCPU wait/release semantics, no kernelrepair or hiddenEBADF success; realGPU window verification selected. p005 will revalidate changed source.

2026-10-02T01:21:16.714469+00:00 / ws109-q569-cleared: p003 uncleared。Native zeroaccess capability adaptation and actual mappedVulkan3frames/ownership PASS; realGPU VT notification/lease test failed, remains F3. [result](/home/awe/zedBSD-claude1/plan/history/ws109/q569/result.md). Next finite nativeVT integration investigation, no falseclear/kernelrepair.

ws109-q570-native-vt-integration: nativeGPU/VT integration investigation selected after q569 positivewindow but failedleasepause. F3/p005 retain actuallifecycle/input gates; boundedtechnical fixture/native adapter corrections delegated.

2026-10-02T01:24:59.323020+00:00 / ws109-q570-cleared: p003 cleared。Actual nativeGPU window/VT asynchronous lease retirement→reacquisition/input/93frames/cleanup PASS; q569 DMAownership/window output retained. [result](/home/awe/zedBSD-claude1/plan/history/ws109/q570/result.md). Whole F3 cleared, F4closure/F5mainapps/finalstandards remain.

2026-10-02T01:26:11.774458+00:00 / ws109-q571-cleared: p004 cleared。F4 verified using unchanged actualOSS/nativewired/radioABI/WPAwire evidence and current nativeguest; physicalradio userwaived, mock correctlyclassified. [result](/home/awe/zedBSD-claude1/plan/history/ws109/q571/result.md). F5mainapps/finalreview remains.

ws109-q572-final-native-gui-conformance: p002/p003/p004 actualoutputs verified; p005 actualmainapps/finalnativeconformance/docs/WSacceptance selected. Earlier3OS/source receipts retained when hashes match.

2026-10-02T01:47:37.311779+00:00 / ws109-q572-cleared: p005 cleared。Actual sevennativeGPU mainapps/PTY/fileopens plus AppHome PASS; fullsource C fixes/review/nativeheaderELF/3OS builds/finalboot/docs verified. [result](/home/awe/zedBSD-claude1/plan/history/ws109/q572/result.md). F1..F5 satisfied under userselectedi915/physicalWiFiwaiver; BUG130 upstreamtracking, ownVMstopped/VFIOpreserved.
