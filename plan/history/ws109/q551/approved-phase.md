<!-- awesome-plan project=zedbsd record=ws109p001 -->

# ws109p001: FreeBSD15 の graphics/OS 契約と環境を調査

Status: uncleared
Disposition: normal
Parent: [WS109](../ws.md)
Queue / Attempt: q550 / q550-i01

## 目的・範囲

Linux headers/ioctl/loader/service を棚卸し、FreeBSD15 の実 headers/library/driver と照合する。graphics 本体を共有できる境界、seat/input、audio/network/WiFi backend、GPL 条件、固定 guest/実機の検証方法を設計。

## 完了条件

F1のfixed software/environment/license/native ABIとF2〜F5の実手順を確定。実機device modelと実GPU能力の結果はユーザー準備の実機でp003/p005の最終受け入れに残す。Linux DMA_BUF syncと同等のUAPI/driver契約はnative headersで確認、actual GPU fencesは後段。ユーザー2026-10-02の実装先行承認に基づく責務分配で、WS自身F1の全証拠をこの設計Phaseで達成したとはしない。

## 前提・未決・実行手順

依存: WS105 output（context）。WS の scope と acceptance、設計の未決を確認する。
技術的な細部は委任範囲で決める。対象/受け入れ/外部契約を変える結果は実装前に計画と承認範囲へ反映する。
調査→変更表と手順の確定→有限 Queue の承認→実装→指定検証→結果・WS・Master・Queue の照合。
最初の p001 は調査の案（1 session / 最大60分、満たせない点と再開条件を残す）。後続の timebox/command は設計後に選定する。

## 適用規則・影響する部品・検証

[WS の制約/部品/受け入れ](../ws.md)、[Guardrail](../../guardrail.md)、
[C 規約全文](../../coding-style.md)、[自動化](../../standards/automation.md)を適用。
新規/変更 C は全文該当節を読み、clang-format-19 と style-check の限界を補う。
最終 conformance は全 WS の source を全文で review。build warning 0、必要な契約検証、diff-check を記録する。
具体的な build/config/tool version と script は p001 の結果で固定する。`make check` は禁止。
zedBSD の起動は boot-test.sh の PNG。Linux の既存検証は WS105 の手順と許可範囲、FreeBSD は検証環境の確定が必要。

## 証拠・結果・再開条件

未実行。command/result/commit/artifact/skipped checks はまだ無い。計画の作成を clearance としない。
再開: prerequisite の実 output と変更の所有、scope snapshot、Queue 承認を確認する。

## イベント

2026-10-01 / review-20261001-planning: 新設した Phase 案。親 WS の目標への寄与と依存を記録。実装の選定は未実施。
GitHub の Phase 作成/comment/Project の projection は公開保留、local outbox に保持する。

## 2026-10-02 実行指示 / 選定scope

ユーザー「WS108の完了後、WS109の実行をお願いします。」。WS108 completed/q549 finishedを確認。q550はp001のみ、最大60分。実sourceと公式15.x release/header/driver/API/licenseを照合し、fixed amd64 imageとnative build、graphics/session/input/audio/network/WiFiの変更対応表および検証環境を具体化。OS image/hashの取得と未起動guest設定の準備を含む。production変更・driver install・guest起動は本scopeに含めない。必要な人間の判断を明示してdependent実装を止める。実WiFi環境の質問をこのchatで提出、回答待ち。FreeBSD起動にはAGENTSの検証例外を確定してから実行。

## 2026-10-02 / ws109-q550-survey / 判断要求

[調査結果](../../history/ws109/q550/survey.md)と[未起動guest](../guest-plan.json)。321uniqueLinuxsource、fixed15.1image/hash/nativeheaders、syncFilehandlers/RTLD_DEEPBIND/OSS/AF_LINK/evdev、Filesxattr/pty/WPAsocketの変更表を保存。D1既存drm-kmodのGPLv2利用、D2FreeBSD起動SSH/QMP例外、D3actualWiFi/graphics検証環境が未確定でF1未充足。3質問を現在のchatで提出、回答待ち。初期audioは既存Linuxと同じmixer範囲でPCMを追加しない。dependent実装を開始しない。質問への回答はこのPhase/WS/Guardrailへ反映、必要なforeignPhasesの検証/依存を再設計してから新attemptを選定する。remote decision comment pending。

## 結果 / q550-i01 / 2026-10-01T18:41:24.330785+00:00

uncleared。321 unique source/公式native ABIの対応表、FreeBSD15.1amd64 image/hash検証と未起動8GiB guest準備を保存。F1はdriver GPLv2利用・FreeBSD起動SSH/QMP例外・real graphics/WiFi検証環境が未確定。3質問への回答待ち、production実装/native build/runtime未実施。

[詳細と証拠](../../history/ws109/q550/survey.md)。再開条件: D1ライセンスとD2起動方法の回答、D3実検証device/受け入れ範囲を確定しnative実environmentを照合。回答が来たら同じp001を新attemptで選定、旧q550のunclearedを保持する。起動規則の例外は回答前に適用しない。独立に進められる本Queueの調査/準備は実施済み、後続は前提未充足。clearance/port完了/bug修正/remotecloseを主張しない。

Event ws109-q550-uncleared: Phase結果と判断要求をlocal保存、remote comment pending。Phaseはactive uncleared、closeしない。

## 2026-10-02 / ws109-user-decisions-20261002 / このPhaseへの反映

ユーザー「drm-kmodを利用OKです。FreeBSDにも例外を適用します。FreeBSD実機は用意しておくので、作業を進めておいてください。」（2026-10-02 JST、このchat）。D1:既存FreeBSD drm-kmod利用可、GPL-free systemstack条件をこの範囲で置換。Keiland sourceの寛容license/外部実装を取り込まない境界は維持。D2:WS109専用FreeBSD QEMU guestのloopback SSH/QMP PNG検証を承認。D3:実機はユーザーが準備、入手前にnative build/backend実装を進める。実GPU/WiFi結果は将来の実機関門に残し、mock/QEMUbuildで代替しない。

origin: q550の未決解消を受け、native softwareのF1 output/guest ABI/license/後続手順を再attemptで確定。実機model/graphics受け入れは実機到着後のfinal gateへ明示して残す。p001の設計関門とWS自身F1の全実機証拠を区別する。 [origin](../phase001/phase.md)・[WS summary](../ws.md)・[scope](../../standards/ws109-native.md)。foreign Phase own検証/再開条件を更新、remote comment pending。

## q551 exact scope

FreeBSD15.1 own guestをSSH/QMP PNGで起動確認、native cc/headers/loader/libmixer/interface/API、pkg dependenciesとライセンス、evdev/HDA/vtnet/DRM availabilityを照合。native準備のgmake/Python/Vulkan headers/loader/Mesa等をown guestに導入し、後続build用outputを保存。production port実装は本Phase外。max60min、image/kernel更新なし、serialnull、実機までのboundary/commandsを固定。
