<!-- awesome-plan project=zedbsd record=ws119 -->

# WS119: インストーラの作り直し

<!-- awesome-plan-current:start -->
Status: planning
Primary Milestone: MG003
Related Milestones: MG006
Objectives: O2
Parent: [Master](../master.md)
Focused goal: fg019（ベータ1、2026-10-02 user: WS118 の次）
Queue: none
Resume point: [p001](phase001/phase.md)（旧 zedinst・image の構成・base の道具の調査と、要件の案・選択肢をユーザーへ出す、planned）。要件が決まるまで p002 以降は planning。
2026-10-02 user:「ディスク全体のみ。UEFIのみ。」→ インストーラは導入先の disk 全体を使う（他の OS との共存は無し）、UEFI だけ。他の要件（入力の項目・UI の言語・起動の入口）は p001 で案を出す。
<!-- awesome-plan-current:end -->

## 目標（2026-10-02 ユーザー）

「インストーラは作り直しです。Linuxパッケージよりも上の優先度です。」

- ベータ1 に向けてインストーラを作り直す。**Wayland で動くインストーラ**（2026-10-02 user「Waylandで動くインストーラを実装する。」）。順位は WS118（5320）の後。旧インストーラ（[WS019](../ws019/ws.md)、completed）と 4 機種の実機の受け入れ（[WS028](../ws028/ws.md)）は前の世代の証拠として参照し、再利用しない（完了した WS に目標を足さない規則）。
- 要件（GUI か TUI か、partition の方式、対象の機種・媒体、既存の disk の扱い）はユーザーと決める。決まるまで Phase は planning。

## 既知の事実（2026-10-02、plan と source を読んで）

- 旧インストーラ `/bin/zedinst` は Noct（`userland/retro/zedinst/`、約 3600 行、text と BeUI の画面）。共存・専用（whole disk）・PC-98 の FAT を WS019 で受け入れた。現在の image の config（`config.mk`・`config/ci/config-amd64.mk`）には入っていない。
- 再利用できる base の道具: `diskpart`（GPT の init・編集）、`mkfs`（FAT32・UFS）、`mkswap`、`blkid`、`mount`、`cp`/`pax`、`df`。amd64 の既定の image は native（UEFI、ESP に kernel、UFS の root、swap の partition、`Makefile` の 144 行）。
- desktop の app は C と libkeiland/libkeiui（Settings・Files・Terminal）。Wayland の client の作りはこれに倣える。

## ベータ1 の到達目標と受け入れ（案、p001 の後にユーザーと確定）

| # | 条件（案） | 証拠 |
| --- | --- | --- |
| I1 | nightly/ベータ1 の USB image で起動した Keiland の session から、インストーラ（Wayland の窓）を開き、導入先の disk を選び、確認の後に導入できる | QEMU（NVMe の空の disk、GPU 無しの framebuffer の Keiland か Venus）の PNG |
| I2 | 導入先の disk だけから起動し、graphical login に届き、作った利用者で login できる | QEMU で USB を外して `plan/tools/boot-test.sh` 相当（導入先の disk の image） |
| I3 | 導入の失敗（容量不足、書き込みの失敗、取消し）で画面に理由が出て、導入先が中途半端なまま起動可能に見えない | QEMU の PNG と log |
| I4 | 5330 の実機で導入して単独起動する（導入先の disk の扱いはユーザーと決める。実機の内蔵 NVMe には Linux の host がある） | ユーザーの報告 |

範囲（案）: UEFI・GPT・whole disk の専用導入だけ（共存は後）、利用者・password・hostname の入力、言語は英語の UI（日本語の UI は IME/WS095 の後）。

## Phase

| Phase | 目的 | Status | 依存 | 目安 |
| --- | --- | --- | --- | --- |
| [p001](phase001/phase.md) | 旧 zedinst と image の構成・base の道具の調査、要件の案と選択肢、画面の流れの案、backend と frontend の分け方の設計案 | planned | なし | 2〜3h |
| [p002](phase002/phase.md) | 導入の backend（非対話の C の command、導入の計画を受けて partition・format・複写・boot の設定・利用者の作成）と QEMU の NVMe の試験 | planning | p001 とユーザーの要件の判断 | 3〜4h |
| [p003](phase003/phase.md) | Wayland の frontend（画面の流れ、disk の選択、確認、利用者、進捗、完了・再起動） | planning | p001 の判断、p002 の backend の interface | 3〜4h |
| [p004](phase004/phase.md) | live の image への組込み（App Home・desktop からの起動、image の config）と QEMU の通し（I1〜I3） | planning | p002、p003 | 2〜3h |
| [p005](phase005/phase.md) | 全文規約の確認と回帰 | planning | p004 | 2h |
| [p006](phase006/phase.md) | 実機の導入（I4、ユーザーと一緒に） | planning | p005、ユーザーの時期と導入先の disk の判断 | 1〜2h（立会い） |

日程の注意: 10/17 までに p001〜p005 を通すには、p001 の判断を早く得る必要がある。間に合わない場合の扱い（ベータ1 から外す、旧 zedinst の text 版で代える等）もユーザーの判断。

