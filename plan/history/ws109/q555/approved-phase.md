<!-- awesome-plan project=zedbsd record=ws109p004 -->

# ws109p004: audio・network・WiFi の FreeBSD backend

Status: uncleared
Disposition: normal
Parent: [WS109](../ws.md)
Queue / Attempt: q554 / q554-i01

## 目的・範囲

p001 で選んだ native audio/mixer、interface/link 状態、WiFi 管理を libkeiland の backend に実装。共通 WPA wire が適用できる部分は再利用。

## 完了条件

F4。PCM 再生を含めるかは p001 で確定し、WS105 の音量 backend と取り違えない。

## 前提・未決・実行手順

依存: p002。WS の scope と acceptance、設計の未決を確認する。
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

## 2026-10-02 / ws109-user-decisions-20261002 / このPhaseへの反映

ユーザー「drm-kmodを利用OKです。FreeBSDにも例外を適用します。FreeBSD実機は用意しておくので、作業を進めておいてください。」（2026-10-02 JST、このchat）。D1:既存FreeBSD drm-kmod利用可、GPL-free systemstack条件をこの範囲で置換。Keiland sourceの寛容license/外部実装を取り込まない境界は維持。D2:WS109専用FreeBSD QEMU guestのloopback SSH/QMP PNG検証を承認。D3:実機はユーザーが準備、入手前にnative build/backend実装を進める。実GPU/WiFi結果は将来の実機関門に残し、mock/QEMUbuildで代替しない。

OSS mixer/vtnet/native networkとsupplicant backend実装を先行可。mockwireの結果はcontractだけ。realWiFi scan/connect/disconnect/configのcriteriaはユーザー準備の実機でverify、partial実装Queueと全F4を分ける。 [origin](../phase001/phase.md)・[WS summary](../ws.md)・[scope](../../standards/ws109-native.md)。foreign Phase own検証/再開条件を更新、remote comment pending。

## q551 design reconciliation / ws109-q551-native-environment

Prerequisite is verified p002 L1 headers/libraries, not full F2 integration. Native OSS/AF_LINK/WPA backend implemented ahead of final integration, actual audio/wired VM checks and mocked WPA distinguished from physical WiFi acceptance. Reason: actual native dependencies and backend/link ordering inspected in p001. [Origin](../phase001/phase.md), [WS](../ws.md), [native design](../../history/ws109/q551/environment.md). Own revised verification/resume condition saved; remote structural comment pending.

## q554 OSS audio / exact partial scope

Prerequisite: p002 L1 verified native52source foundation/public headers/library chain q553. No dependency on p002 L2 application integration. User-authorized WS109 implementation ahead of physical GPU/WiFi. Maximum60min/onePhase, audio subset only; complete F4 network/WiFi acceptance retained for subsequent scoped attempts.

Implement libkeiland/freebsd/audio-freebsd.c as real OSS native mixer backend with existing public keiland_audio_* contracts. Native hw.snd.default_unit preference and accessible /dev/mixerN enumeration, real channel volume/mute masks, initial readback, periodic metadata/state refresh and retry after device loss. No PCM addition or stub success. Event descriptor is -1 because native mixer has no event-read/poll ABI; existing periodic keiland_audio_update must keep UI state fresh. Clarify public event-fd comment only if needed; no public ABI changes. Check native volume/master orPCM selection, stereo/mono channels and mute capability, errno and ownership. Tests directly link this production module (no fake libkeiland.so). Actual QEMU HDA/kernel ioctl read/set/independent native mixer command/readback/external change and settings restoration; invalid ranges/nulls and unavailable device behavior. Full Cstandard/clang-format19/stylecheck/manual for new module/probe. Affected Linux/zedBSD audio backends remain separate; final integration/UI/3OS gates in p005. No network implementation in this attempt. Real WiFi/GPU gates unchanged; WIP commit, no push/publication.

Scope: new libkeiland/freebsd/audio-freebsd.c, audio event-fd documentation in keiland/keiland.h, WS109 audio probe/recipes/evidence. Subsequent p004 network/WPA source and p002 libkeiland real link will consume this verified module; no fake provider while network absent. Whole Phase remains uncleared after this partial audio output.

## Result / q554-i01 / 2026-10-01T19:52:10.085559+00:00

Queue item cleared /whole Phase uncleared。FreeBSDOSS audio実装/nativeHDA volume/mute/independent libmixer/外部変更refresh、unprivileged同一subscription再接続を実検証。音量86/86/offとdevice権限を復元。[result](../../history/ws109/q554/result.md)。network/WPA/actualWiFi・全F4/GUI統合は後続、WS incomplete。

Event ws109-q554-cleared: local evidence/outcome saved; remote comment (no Phase close) pending.

## q555 network/WPA / exact partial scope

Verified prerequisite q553 L1 native headers/libraries and q554 OSS source output. Maximum60min/onePhase. Implement libkeiland/freebsd/network-link-freebsd.c for native AF_LINK/getifaddrs addresses/MAC/if_data64bitcounters/MTU/admin/carrier flags, native SIOCGIFFLAGS/SIOCSIFFLAGS preserving high/low flags, and native net80211 read-query WiFi classification (interface type/name alone does not prove wireless). DHCP/IP remains system-owned.

Move shared DNS/credential/connection selection from Linux network-link into wpa/network-config-wpa.c; both native/Linux builds reuse it. Existing Linux radio/sysfs behavior preserved via small private kwpa_link_usable/kwpa_wireless functions. Common WPA uses selected OS control-directory and native sockaddr packing helper: Linux /run/wpa_supplicant, FreeBSD /var/run/wpa_supplicant/native sun_len/actualextent, no large OS macro. Source boundaries/private headers/package Linux selections updated; unchanged public ABI and no Linuxulator. Full C standard applies to moved/changed source, no relocation exception.

Files: libkeiland/freebsd/network-link-freebsd.c, linux/network-link-linux.c, wpa/{network-config-wpa.c,network-wpa.c,network-wpa.h}, Makefile.linux; WS109 native wired and mock-WPA contract probes/evidence. Native direct link of actual network sources (no fake libkeiland provider). Verify native cc warning0, actual vtnet0 addresses/MAC/MTU/counters/carrier and no-supplicant state, real kernel radio flag permission/invalidinterface failures, actual native Unix datagrams vs independent bounded mock peer for scan/profiles/credential escaping/persistence/join/disconnect/timeout. Mock work is wire contract only, actual WiFi/physical device remains F4 gate. Linux affected module/library rebuild/contracts regressions and full Cformatter/style/manual. Actual p002 L2 linking and main GUI apps, p003graphics/seat and p005final3OS remain separate. No hostnetwork mutations/credentials/targettoolchain, WIP commit/pushなし/publicationdeferred.
