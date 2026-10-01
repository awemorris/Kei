<!-- awesome-plan project=zedbsd record=ws109 -->

# WS109: Linux 版 Keiland を FreeBSD 15 へ移植

<!-- awesome-plan-current:start -->
Status: incomplete
Primary Milestone: MG006
Related Milestones: MG007
Parent: [Master](../master.md)
Queue: なし（q550 finished / p001 uncleared）
Resume point: p001/q550 uncleared、D1driver license/D2FreeBSD起動方法/D3realgraphics・WiFi環境の回答待ち。
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
GPL の無い構成は F-065 の意図を引き継ぎ、Keiland source と system driver/service の license を分けて確認する。
system の graphics stack に LinuxKPI/DRM がある場合も、GPL が無いと確認前に主張しない。

## WS 自身の完了条件

- F1: 固定 FreeBSD15 version/amd64、graphics device/driver、検証環境、license、native ABI の対応表を確定。
- F2: 独立 native build/DESTDIR install が warning 0、標準公開 header と ELF/後段 Vulkan の symbol chain が利用可能。
- F3: 共通描画から実際の表示・入力・buffer 同期・session/seat の device 所有と解放を確認。
- F4: audio の列挙/音量/mute、network の状態、有線/WiFi の scan/接続/切断/設定を実 backend で検証。mock は実検証と分ける。
- F5: 主な app の起動/操作、Linux と zedBSD の影響範囲の回帰、全文規約と移植/運用文書を確認。

## 依存・所有

WS104/105 の境界・Linux 出力は completed context。共通描画/API の仕様を保ち、WS106 の移動後の対応 app locator を用いる。WS107 は browser を初期対象に加える場合だけ依存候補。WS105 の Linux SSH/QMP 許可を FreeBSD 環境の許可に自動で広げない。

## Phase 表（後続は設計案）

| ID / Phase | 目的 | Goal | Status | 依存 |
| --- | --- | --- | --- | --- |
| [ws109p001](phase001/phase.md) | FreeBSD15 の graphics/OS 契約と環境を調査 | F1 と port の対応表/実現可能な F2〜F5 手順。Linux DMA_BUF sync と同等の能力が無ければ別方式の影響と選択をユーザーに提示してから dependent 実装を選定。 | uncleared / q550 | WS105 output（context） |
| [ws109p002](phase002/phase.md) | native build・library と system Vulkan chain | F2。glibc 固有の loader binding に頼らないことを実際の FreeBSD で検証。 | planning | p001 |
| [ws109p003](phase003/phase.md) | 共有描画と FreeBSD の device/session/input 境界 | F3。Linux source を丸ごと複製した renderer を作らない。device release/fd lifetime も確認。 | planning | p002 |
| [ws109p004](phase004/phase.md) | audio・network・WiFi の FreeBSD backend | F4。PCM 再生を含めるかは p001 で確定し、WS105 の音量 backend と取り違えない。 | planning | p002 |
| [ws109p005](phase005/phase.md) | 全文規約・主な app と3 OS の最終回帰 | F1〜F5。FreeBSD build のみを移植完了としない。未実施の GPU/実機/OS version を記録。 | planning | p003、p004 |


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
