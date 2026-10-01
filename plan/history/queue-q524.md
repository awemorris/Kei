<!-- awesome-plan project=zedbsd record=q524 -->

# Queue

Status: finished
Active Queue: なし
Last finished Queue: q524

## q524

- Purpose: build の土台（`keiland-linux.mk`・top-level の goal・library の `Makefile.linux`）
- Focus: fg012 / WS105 の既存 Linux の受け入れを満たす。
- Timebox: 60 分。既知の build・必須検証は最大 150 分、未知の問題の調査は 30 分で結果と再開条件を記録する。
- Approval: current user, 2026-10-01「ws105の完了をゴールにして、自走をお願いします。」。既存 WS105 p001〜p011 の範囲を依存順の 1 Phase Queue で実装・検証・記録する。commit は WIP、push / GitHub 公開は行わない。
- Exact approved scope: [ws105-p002](ws105/q524/phase.md) 全範囲、開始前 [snapshot](ws105/q524/scope.md) SHA256 `397f0af993c9c2b151b936c38d2a97276eee72315497f527bfbbef9ddd427a82`。
- Executor: Codex Q1。前 Queue finished、並行 executor なしを確認。
- Applicable rules: AGENTS.md、Guardrail、coding-style.md 全文、WS105 D1〜D25、design.md の指定節。
- Prerequisites: Phase の依存と既存出力を実装・証拠で確認。material scope / criteria の変更・取消なし。
- Started UTC: 2026-10-01T06:04:15.365193+00:00

| Attempt | Phase | Status | Dependency | Selection reason |
| --- | --- | --- | --- | --- |
| q524-i01 | [ws105-p002](ws105/q524/phase.md) | cleared | WS104 の p001（header が `userland/desktop/keiland/`）・p003（libkeiland の `zedbsd/`）（context） | WS105 の既存依存順 |

Dependency graph: Phase の依存（context）→ q524-i01/ws105-p002。他の Phase は次の Queue で選定する。

## Upcoming Work Outlook

WS105 の依存を満たす次の Phase。新しい product / risk 判断はユーザーの決定が前提。既存 WS105 の完了までの自走指示で選定・実行する。
GitHub への publication は deferred、outbox に保持する。

## Outcome

ws105-p002 **cleared**。

Linux の独立 build の土台と、8 package の `Makefile.linux`（7 shared library と install しない digest archive）を実装。仮 network / audio backend は公開 API の署名を保持し、service 不在を返す。DNS は実際の `/etc/resolv.conf` の dotted IPv4 を読む。

- gcc / clang の最終 build exit 0、`-Werror`・warning 0。host gcc 14.2.0 / clang 19.1.7、GNU Make 4.4.1。target toolchain 変更無し、host package 追加無し。
- install した 7 ELF の `RUNPATH [/opt/keiland/lib]` と SONAME / NEEDED を確認、`elf-check: PASS (7 ELF)`。digest archive は stage に無い。
- source token の比較 `makefile-sync: PASS`、system-inclusive dependency の確認 `header-check: PASS (57 sources)`。system の Wayland / EGL / GLES header 混入無し。
- `lib-smoke: PASS`（version 21、network record / unreachable state、audio unavailable、resolver reader）。
- `make keiland-linux-clean` exit 0。Linux の lib / obj / stage を消し、p001 の guest.img は保持。
- `timeout 600 make -j64 disk-image`: exit 0、warning 0。Linux Makefile は target build に include されず、zedBSD source / toolchain に変更無し。共通 C source を直していないため runtime 回帰対象無し。
- 新規 4 C file は clang-format19（ColumnLimit 0）後、定義引数・3 条件の行を全文規約に従い復元。style-check total 0、手動全文レビュー、sh syntax、`git diff --check` PASS。仮 backend の常に拒む API の最終 return は規定の errno を保持。

証拠: `build/ws105-p002/` の gcc/clang（初回・最終）log、install/clean/zedbsd log、verify.log。永続の試験 summary と source manifest は `plan/history/ws105/q524/`。libvulkan と compositor / app は後続 Phase。host の `/opt/keiland` に install していない。host 試験は `KEILAND_DRM_DEVICE=none`、Wayland/X 環境を外し、timeout 付きで実施。


実装 commit `c1c9e48ea7ed636d62f3fb20e0c4179423d5a614`。Finished UTC: 2026-10-01T06:10:50.723980+00:00。次の Phase は WS105 の既存完了までの自走指示で選定。push / GitHub 公開は未実施。
