<!-- awesome-plan project=zedbsd record=ws108 -->

# WS108: Debian 13・Ubuntu 26.04 の Keiland deb を CI で作成

<!-- awesome-plan-current:start -->
Status: planning
Primary Milestone: MG007
Related Milestones: MG001, MG006
Parent: [Master](../master.md)
Queue: なし（新規実装未承認）
Resume point: p001 の調査・設計を選定する前に scope と検証環境を確定。
<!-- awesome-plan-current:end -->

## 目標・決定の出典

CI が Debian 13 と Ubuntu 26.04 向けの install 可能な Linux Keiland の .deb をそれぞれ作成し、artifact として保存する。

2026-10-01 ユーザーの [レビューコメント](../reviews/2026-10-01-review.md)を根拠に計画。
Primary MG007 にこの目標の成果を提供。Related MG001, MG006 は技術/配布/検証の supporting 出力。
WS completion と milestone 全体の acceptance は別に確認する。

## 範囲

[パッケージ設計案](design.md)。初期の architecture は WS105 と同じ amd64 の案（未確定）。
target distro の固定された環境ごとに native build し、DESTDIR staging から dpkg/debhelper で package を作る。
既存 zedBSD image/nightly CI は保つ。Debian package の自動 repository/release 公開はこの範囲に含めない。
`/opt/keiland`、system Vulkan の動的後段、gdm/direct の既存の意味を保つ。
最初の runtime package は WS105 で Linux 対応済みの library/compositor/app/data を基準にする。
test app の runtime への混入を避ける。test/devel の分割、browser/EGL/GLES 等の追加は p001 の package manifest で明記して合意する。
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
| [ws108p001](phase001/phase.md) | package manifest・環境・依存を設計 | P1 と p002〜p004 の実 command/環境を定義。container では DRM/session の実 GUI が確認できない限界を残す。 | planning | WS105 output（context）、WS106 の対象表（context） |
| [ws108p002](phase002/phase.md) | deb packaging と install 検証 | P2/P3 の package/CLI 部分。GUI/session の必要な guest 証拠は最後の Phase までに満たす。 | planning | p001 |
| [ws108p003](phase003/phase.md) | CI の2 distro job と artifact | P4。実 job の結果または同じ環境/手順の検証を記録し、remote CI 未実行は区別する。 | planning | p002 |
| [ws108p004](phase004/phase.md) | 全文規約・両 distro の最終 install/session 回帰 | P1〜P5。CI の未実行・GPU/session 未検証が残れば必要な acceptance を満たしたとはしない。 | planning | p003、WS106 の確定配置（scoped output） |


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
