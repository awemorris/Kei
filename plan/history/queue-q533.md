<!-- awesome-plan project=zedbsd record=q533 -->

# Queue

Status: finished
Active Queue: なし
Last finished Queue: q533

## q533

- Purpose: app の Linux の build と install の data
- Focus: fg012 / WS105 の既存 Linux の受け入れを満たす。
- Timebox: 60 分。既知の build・必須検証は最大 150 分、未知の問題の調査は 30 分で結果と再開条件を記録する。
- Approval: current user, 2026-10-01「ws105の完了をゴールにして、自走をお願いします。」。既存 WS105 p001〜p011 の範囲を依存順の 1 Phase Queue で実装・検証・記録する。commit は WIP、push / GitHub 公開は行わない。
- Exact approved scope: [ws105-p008](ws105/q533/phase.md) 全範囲、開始前 [snapshot](ws105/q533/scope.md) SHA256 `75831c8d4516dbf20a6fd0ecade15b3d4a698d8fc355a9f09a60c2d06ad30398`。
- Executor: Codex Q1。前 Queue finished、並行 executor なしを確認。
- Applicable rules: AGENTS.md、Guardrail、coding-style.md 全文、WS105 D1〜D25、design.md の指定節。
- Prerequisites: Phase の依存と既存出力を実装・証拠で確認。material scope / criteria の変更・取消なし。
- Started UTC: 2026-10-01T09:45:05.481795+00:00

| Attempt | Phase | Status | Dependency | Selection reason |
| --- | --- | --- | --- | --- |
| q533-i01 | [ws105-p008](ws105/q533/phase.md) | cleared | p007（context） | WS105 の既存依存順 |

Dependency graph: Phase の依存（context）→ q533-i01/ws105-p008。他の Phase は次の Queue で選定する。

## Upcoming Work Outlook

WS105 の依存を満たす次の Phase。新しい product / risk 判断はユーザーの決定が前提。既存 WS105 の完了までの自走指示で選定・実行する。
GitHub への publication は deferred、outbox に保持する。

### Q1の技術補完（承認済み目的・受け入れ不変）


## Terminal 起動の bounded 補完（2026-10-01、Q1）

App HomeのTerminal childがstatus139、直接起動も同じ。source/objdumpでmain_start→main_menu_stateが最初のmain_tab_newより先、main_screenはNULLのままselection/rangeを読むと確認。OS分岐の問題ではない。D21のTerminal起動・入力という既存受け入れに必要な普通の技術修正として、common terminal/main.c:main_menu_stateを「screenが無ければ選択なし」にする。起動順、menu/tabs/shellの所有、product、依存、受け入れは不変。広いTerminal改修はしない。Linuxの修正前139→修正後Home起動・10秒生存・echo入力、zedBSDのTerminal起動/文字/終了と必須回帰で検証。ユーザーのWS105完了まで自走指示の委任を適用し、move/resizeのbug移管判断とは分ける。

元snapshotを保持。[補完snapshot](history/ws105/q533/supplement-terminal.md) SHA256 `ab34e1faacf374c61458059dfd3aa84ac1a01f6d888949dad86a29bf787a4ea0`。

## Outcome

ws105-p008 **cleared**。

Linux app の build/install、Home の9app・IME、data を検証し p008 cleared。source `ba46edf8` + Terminal guard `7dd3ad9e`（WIP）。gcc14.2 / clang19.1.7 warning0、26ELF（24 production + 2 test fixture）、source-sync / 329 header PASS、changed common source 4file style-check0。Homeの全appは起動10秒生存、Terminal echo、Files directory、Settingsページ、Text Editor文字、Image Viewer画像、PDF Viewer 2ページ、kuidemo / mviewを確認。IME Alt+Space→kanji→漢字→確定→直接入力PASS。Notesは起動/終了PASS、keyboard文字入力のみ理由つき未実施（既存手書き専用UI）。全appのcloseでchild status0、最後のpsはdesktop FilesとIMEのみ。compositor SIGTERM2310frame error0 / cleanup_failed0、guest停止。辞書SHA256、5gradient、既定wallpaperはtargetとbyte一致、host package追加0。

zedBSD: disk-image warning0、OS boundary / V1（54source）、host dedicated18 / decoder17 ordinary+sanitize、boot login PNG、C1/C2/C9 13/13、forge拒否→3import、fence600すべてgeneration1 PASS。3commonfileのtarget path stringsはWS104 q521と3/3一致。全criteria imageはTerminal guard前（startupを試験しない）；その後final sourceを含むforge imageでTerminalの10秒生存、echo keiland-zedbsd-ok表示、timeout終了（TAB count0）、compositor継続を別に確認、BUG-128 resolved。代表PNGを目視・ユーザーに提示、実機未実施。p072 / p076今回はPASSだが修理とはしない。BUG-125 / BUG-127のtrackingとq532の元のFAILは保持。

証拠: [q533 manifest](../../history/ws105/q533/evidence/SHA256SUMS)、original `build/ws105-p008/`。全guest停止。GitHub未公開、bug disposition / Phase / WS eventはoutboxで保持、pushなし。次はp009 logind / gdm。

実装 commit `7dd3ad9e26639cb9c17517b06d0fe977927b70ca`。Finished UTC: 2026-10-01T10:24:11.027596+00:00。次の Phase は WS105 の既存完了までの自走指示で選定。push / GitHub 公開は未実施。
