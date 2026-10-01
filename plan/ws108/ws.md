<!-- awesome-plan project=zedbsd record=ws108 -->

# WS108: Debian 13・Ubuntu 26.04 の Keiland deb を CI で作成

<!-- awesome-plan-current:start -->
Status: incomplete
Primary Milestone: MG007
Related Milestones: MG001, MG006
Parent: [Master](../master.md)
Queue: なし（q548 finished）
Resume point: p003 cleared、p004の承認scope/実outputを確認。
<!-- awesome-plan-current:end -->

## 目標・決定の出典

各OSのQEMU guest内でnative buildしたKeiland .debを各guestへ導入して動作を確認し、既存CI/nightly release filesへ組み込む。

2026-10-01 ユーザーの [レビューコメント](../reviews/2026-10-01-review.md)を根拠に計画。
Primary MG007 にこの目標の成果を提供。Related MG001, MG006 は技術/配布/検証の supporting 出力。
WS completion と milestone 全体の acceptance は別に確認する。

## 範囲

[パッケージ設計案](design.md)。architecture は WS105 と同じ amd64。q545で両OSの実guest identity/checksum/readinessを確認。
target distro の固定QEMU guestごとにnative buildし、DESTDIR private stagingからdpkg-debでruntime packageを作る。
既存 zedBSD image/nightly CI は保つ。自動APT repository公開は含めない。2026-10-02ユーザー指定により既存nightly releaseへのdeb/checksum/buildinfo添付を含める。
`/opt/keiland`、system Vulkan の動的後段、gdm/direct の既存の意味を保つ。
最初の runtime package は WS105 で Linux 対応済みの library/compositor/app/data を基準にする。
test app の runtime への混入を避ける。q545でruntimeだけを固定、test/devel packageやbrowser/EGL/GLESの追加は今回の範囲外。
CI job の distro は ubuntu-latest の名前から推測しない。tool/image/action の利用可能な version と digest を実装時に確定する。

## WS 自身の完了条件

- P1: distro/architecture/version、package 内容、依存/競合、license と install manifest を固定。
- P2: 各 target の clean native build と .deb ができ、ELF と runtime の依存/RUNPATH が整合する。
- P3: 両 distro の fresh 環境で install・upgrade・remove と public client/session の smoke を検証。利用者の設定/データを消さない。
- P4: CI の2 target が build/検証の失敗を失敗として扱い、区別可能な .deb と buildinfo/checksum を artifact に保存。
- P5: 最終の全文規約/packaging/CI review と既存 zedBSD CI の維持を確認。

## 依存・所有

WS105 の build/install/ELF と session 契約は completed context。WS106 の test 配置を manifest に反映するため p001 の棚卸しは独立可能、最終 package の検証は実際の確定配置を使う。WS107/browser の Linux 追加は現状必須依存にしない。

## Phase 表（後続は設計案）

| ID / Phase | 目的 | Goal | Status | 依存 |
| --- | --- | --- | --- | --- |
| [ws108p001](phase001/phase.md) | package manifest・環境・依存を設計 | P1 と p002〜p004 の実 command/環境を定義。両OSのQEMU guest内native build、dpkg導入/GUI動作とCI/releaseを設計。 | cleared | WS105 output（context）、WS106 の対象表（context） |
| [ws108p002](phase002/phase.md) | deb packaging と install 検証 | P2/P3 の package/CLI 部分。GUI/session の必要な guest 証拠は最後の Phase までに満たす。 | cleared | p001 |
| [ws108p003](phase003/phase.md) | CI の2 distro job と artifact | P4。実 job の結果または同じ環境/手順の検証を記録し、remote CI 未実行は区別する。 | cleared | p002 |
| [ws108p004](phase004/phase.md) | 全文規約・両 distro の最終 install/session 回帰 | P1〜P5。CI の未実行・GPU/session 未検証が残れば必要な acceptance を満たしたとはしない。 | planning | p003、WS106の確定済み対象配置（scoped output、ime-probe対象外） |


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

## 2026-10-02 / ws108-user-targets

ユーザー（このchat）: make keiland-linux-debianでDebian13のdpkg、make keiland-linux-ubuntu2604でUbuntu26.04のdpkg。ビルド自体もそれぞれQEMU guest内、dpkgを各guestへ導入して動作試験、既存CIに組込みnightly release filesにする。「現在のQueueを完了したら、WS108を実行してください。」をp001〜p004 finite1Phase Queueの実行承認として保存。
旧container案をQEMU nativeへ置換、旧release対象外をrelease filesへ置換。既存/opt layoutとWS105のLinux対応済production内容を維持、test appはruntime package外。amd64は既存対象の具体化。実release publication/pushはこのsessionでは行わずCI定義を作成・local同手順検証、remote runは未実施と分ける。

2026-10-02 / ws108-q545-design: 全Phaseを[確定設計](design.md)へ具体化。QEMU guest build/test、指定targets、runtimeだけのmanifest、既存CI/nightly release files。移動済対象のscoped outputを使用、ime-probe編集/確定を待たない。GitHub delivery pending。

2026-10-01T16:17:58.918952+00:00 / ws108-q545-cleared: p001 cleared。P1 manifest/version/license/dependency/2OS native guest手順/CI release設計を固定。公式pinned image checksum両方一致、actual QEMU10.0.11/KVM cloud-init/SSH/QMP PNGでDebian13/Ubuntu26.04 amd64確認、自分のguest停止。plan/ws108/design.mdとinputs.json。

2026-10-01T16:52:23.158073+00:00 / ws108-q546-cleared: p002 cleared。2OS native package＋fresh overlay install/reinstall/real upgrade/conffile/remove/purge/public Vulkan/actual compositor/Terminal入力 PASS。compiler warning0、private shlibsの27警告を分類。証拠 plan/history/ws108/q546/result.md。p004はcommitted最終sourceの両make/8GiB guestを再検証。

2026-10-02 / ws108-q547-ci-checkpoint: p003がCI/TCG/release verifierを実装、p004の最終scopeをcommitted同source両target＋全差分full reviewへ具体化。dependency/外部契約/acceptance不変、remote未実行のlocal同手順代替はq545設計とp003のP4契約通り。foreign p004 delivery/WS deliveryをlocal outbox保持。

2026-10-02 / ws108-q547-final-source: p003 runnerのTCG input/session envを修正、最終両makeを10362bd3で再実行しp004へ渡す。p004記録にも最終source修正を反映、foreign Phase/WS delivery pending。

2026-10-01T17:46:28.530941+00:00 / ws108-q547-uncleared: p003 uncleared。CI定義/構造/negative gates実装済み、2OS native TCG build/19ELF/clean metadata成功。旧両TCG input失敗を修正しfocused Debian runtime PASS。10362bd3最終Debianはupgrade用deb再圧縮を含む180秒blockでtimeout/exit2、CI検証要件未clear。Ubuntuの同時試験は終了待ち、p004開始不可。次attemptはupgrade圧縮とboundsを修正、2OS actual make＋affected TCG試験へ。

ws108-q548-redesign: p003のupgrade fixture圧縮/boundsを改訂、q547uncleared保持、q548で同Phase再試行。p004依存/最終verifyも改訂、Phase/WS delivery pending。scope/P1〜P5不変。

2026-10-01T18:13:59.373954+00:00 / ws108-q548-cleared: p003 cleared。最終b0e1eaf9の両make exit0、両fresh TCG全smoke PASS、native TCG/KVM payload40全一致、19ELF/manifest/md5/control/root/session audit PASS。CI構造/negative/positive gates、既存build job維持。q547uncleared保持、remote Actions/publish未実行。plan/history/ws108/q548/result.md。
