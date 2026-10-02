<!-- awesome-plan project=zedbsd record=ws129 -->

# WS129: ベータ1 のリリース作業

<!-- awesome-plan-current:start -->
Status: planning
Primary Milestone: MG007
Related Milestones: MG003, MG006
Objectives: O1, O2
Parent: [Master](../master.md)
Focused goal: fg019（ベータ1、2026-10-17）
Queue: none
Resume point: [p001](phase001/phase.md)（release の定義: 版の付け方、配布物、CI の release、release notes・既知の問題・license の一覧の作り方、凍結と最終回帰・実機の確認の日程、planned）。版の名前はユーザーの判断。
凍結は無し（2026-10-02 user（作業開始の指示）「凍結はしません。できたところまででベータ1にします。安定化はベータの最後の方のバージョンで行います。」）。p001 の日程案（10/13 凍結）を置き換える。デモの image は CI の設定を土台に変える（同日 user）。
2026-10-02 user（リリースの流れ）:「CIはPrereleaseを生成、それをダウンロードして動作確認した私が、PrereleaseからLatest Releaseに手動昇格します。」→ CI の release の job はベータ1 の版の Prerelease を作る（nightly とは別の tag）。ユーザーが download して動作確認し、手で Latest Release に昇格する。エージェントは昇格・公開をしない。版の名前・tag の形・配布物（Windows の zip を載せるか）は未決。
<!-- awesome-plan-current:end -->

## 目標（2026-10-02 ユーザー「次のFeature Goalはベータ1のリリースにします」「リリース目標は10/17です」）

nightly（`zedbsd-amd64.img.gz`・`Kei-nightly.zip`、「Not an official release」）から、版のついたベータ1 のリリースを作る。fg019 の各 WS の成果を集め、既知の問題を明示し、最終回帰を通す。
push・GitHub release の公開はユーザーの指示で行う。

## 既知の事実（2026-10-02、source を読んで）

- 版は source に直書き: libc の `uname`（`userland/base/libc/posix.c:5324-5325`、`0.0.1`・`zedBSD 0.0.1`）、i386 の HAL の起動の表示（`src/hal/i386/cmain.c:42,46`、HAL の実装なので承認なしで直せる範囲）。Settings の About（`userland/desktop/settings/about.c`）は WS089 の file。一つの源（Makefile の変数から生成）が無い。
- CI（`.github/workflows/ci.yml`）: main への push ごとに build → `nightly-${run_number}`（prerelease、「Not an official release」）に `zedbsd-amd64.img.gz`・`Kei-nightly.zip`・Keiland の deb を載せる。版のついた release の job、checksum（img の SHA-256）、tag の trigger は無い。WS112 も release の job に package を足す計画（同じ file の衝突）。
- CI の image の config `config/ci/config-amd64.mk` は `CONFIG_DRIVER_PCI_INTEL_AX211 := y`（BUG-134: driver を有効にすると 5330 で起動が止まる）で、`settings`・`audiod` を含まない（デモの config `plan/ws075/demo/config-demo-hdmi.mk` は含む）。**今の nightly は 5330 で起動が止まる可能性がある**（未確認）。
- license: 外部 package は `plan/tools/packages/audit-licenses.sh` と `plan/ws032/provenance.md` で監査する。image に入る license の文書は package ごと（`userland/base/licenses/` は browser・AX211 の driver・llvm runtime・RTL8822B の表だけ）。image 全体の一覧（OS・package・firmware・font）は無い。
- 既知の問題の源は [Bug Board](../known-bugs.md)（Q1 の file、読むだけ）。

## ベータ1 の到達目標と受け入れ（測れる形）

| # | 条件 | 証拠 |
| --- | --- | --- |
| R1 | 版の名前が一つの源から uname・About・起動の表示・`/etc/os-release`（作るなら）・release の名前に出る | QEMU の SSH の `uname -a`、About の PNG |
| R2 | release の image の config が fg019 の成果を含み、5330 と 5320 で起動を止める driver を含まない | config の差分の review、`plan/tools/boot-test.sh` |
| R3 | CI に版のついた release の job があり、tag（またはユーザーの手動の実行）でだけ動き、img.gz・zip と SHA-256 を載せ、release notes を本文にする。nightly は今のまま | YAML の review と、local で job の shell の部分を再現した結果。push・公開はユーザーの指示で |
| R4 | release notes（新機能、対象 platform 5330/5320、USB への書き方、既知の問題、license の一覧への link）と既知の問題の一覧（Bug Board の open の行から選んだもの）がある | `docs/` か `tools/release/` の文書（置き場所は p001） |
| R5 | image に入る全ての component の license の一覧と本文が image と release にある | 一覧の生成の script と audit の結果 |
| R6 | release candidate の commit で最終回帰（QEMU の boot test、領域ごとの host の試験、5330 の passthrough の smoke）が通る | 回帰の記録 |
| R7 | 5330 と 5320 の実機で USB から起動し、確認の一覧をユーザーが通す | ユーザーの報告 |

## Phase

| Phase | 目的 | Status | 依存 | 目安 |
| --- | --- | --- | --- | --- |
| [p001](phase001/phase.md) | release の定義（版の付け方の案、配布物、CI の release の設計、文書の置き場所、凍結と回帰と実機の日程の案）。ユーザーへの質問を出す | planned | なし | 2h |
| [p002](phase002/phase.md) | license の一覧（image の全ての component、本文の収集、生成の script、audit） | planned | なし（最後に p007 で再生成） | 3〜4h |
| [p003](phase003/phase.md) | 版の一つの源（Makefile の変数 → uname・起動の表示・`/etc/os-release`、About は WS089 に依頼） | planning | p001 とユーザーの版の名前 | 2h |
| [p004](phase004/phase.md) | release の image の config（fg019 の成果、Settings・audiod 等、AX211 の扱い）と CI の release の job | planning | p001、BUG-134 の結果（AX211 の扱い） | 3h |
| [p005](phase005/phase.md) | release notes・既知の問題の一覧・利用の手引き（USB への書き方、対象 platform、WiFi の adapter） | planning | p001、各 WS の成果（10/13 頃に集める） | 2〜3h |
| [p006](phase006/phase.md) | 凍結した release candidate で最終回帰（QEMU・host の試験・5330 の passthrough の smoke） | planning | p003〜p005、凍結の日（ユーザー） | 3h |
| [p007](phase007/phase.md) | 実機の確認（5330・5320、ユーザーと一緒に、ws005-p023・ws033-p002・ws118-p004 とまとめる） | planning | p006、ユーザーの時期 | 2h（立会い） |
| [p008](phase008/phase.md) | 公開（tag、CI の実行、配布物の確認）。ユーザーの指示でだけ | planning | p007、ユーザーの公開の指示 | 1h |

日程の案（p001 で確定）: 10/13 機能の凍結の候補 → 10/14 RC → 10/14〜15 最終回帰 → 10/15〜16 実機の確認 → 10/17 公開。
