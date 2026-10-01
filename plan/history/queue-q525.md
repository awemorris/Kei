<!-- awesome-plan project=zedbsd record=q525 -->

# Queue

Status: finished
Active Queue: なし
Last finished Queue: q525

## q525

- Purpose: libvulkan-compat (1): 後段への chain（WSI 無し）
- Focus: fg012 / WS105 の既存 Linux の受け入れを満たす。
- Timebox: 60 分。既知の build・必須検証は最大 150 分、未知の問題の調査は 30 分で結果と再開条件を記録する。
- Approval: current user, 2026-10-01「ws105の完了をゴールにして、自走をお願いします。」。既存 WS105 p001〜p011 の範囲を依存順の 1 Phase Queue で実装・検証・記録する。commit は WIP、push / GitHub 公開は行わない。
- Exact approved scope: [ws105-p003](ws105/q525/phase.md) 全範囲、開始前 [snapshot](ws105/q525/scope.md) SHA256 `58df6ba610e951b6efd3d12f01af6b2e3293abcce72e627c25566b34c6374f23`。
- Executor: Codex Q1。前 Queue finished、並行 executor なしを確認。
- Applicable rules: AGENTS.md、Guardrail、coding-style.md 全文、WS105 D1〜D25、design.md の指定節。
- Prerequisites: Phase の依存と既存出力を実装・証拠で確認。material scope / criteria の変更・取消なし。
- Started UTC: 2026-10-01T06:10:50.891003+00:00

| Attempt | Phase | Status | Dependency | Selection reason |
| --- | --- | --- | --- | --- |
| q525-i01 | [ws105-p003](ws105/q525/phase.md) | cleared | p002（context） | WS105 の既存依存順 |

Dependency graph: Phase の依存（context）→ q525-i01/ws105-p003。他の Phase は次の Queue で選定する。

## Upcoming Work Outlook

WS105 の依存を満たす次の Phase。新しい product / risk 判断はユーザーの決定が前提。既存 WS105 の完了までの自走指示で選定・実行する。
GitHub への publication は deferred、outbox に保持する。

## Outcome

ws105-p003 **cleared**。

Linux 専用 libvulkan-compat を実装。後段への F 222 関数、I 12 関数、禁止する WSI N 51 関数を maintained TSV から生成。dispatchable handle を包まず、後段の dlsym の trampoline を使い、instance / physical-device / device / queue の ownership を保持する。mutex・pthread_once・pthread の thread-local record で publication / 再入 / lifetime を扱う。

- gcc / clang の最終 build exit 0、warning 0。8 ELF の RUNPATH / SONAME / NEEDED が PASS。libvulkan.so.1 の NEEDED は glibc の libc.so.6 だけ（libdl / pthread は glibc 2.34 以降 libc に統合）。初版の __thread の dynamic TLS による ld-linux 直接依存を pthread key に替えて、計画の依存条件を満たした。
- `vk-chain-test: PASS`: staged library の dladdr、Vulkan 1.0 instance、llvmpipe device / driver、1 MiB の fill → copy → fence、262144 word 全一致、GetProcAddr と禁止した X11 surface 名の NULL。
- 我々の library で `vulkaninfo --summary` が llvmpipe を表示。空 XDG_RUNTIME_DIR、Wayland/X を外し、host の DRM は none。
- `interpose-check: PASS`: 既定の backend → compat vk binding 0、NO_DEEPBIND opt-out の command も PASS（V1・V2 verified）。同じ SONAME の別 backend を同 process で利用できた。
- 自分自身の backend 指定と /nonexistent は両方、no backend libvulkan の診断で exit 1、timeout 124 ではない。明示した backend 選択は失敗を既定候補で隠さず、その指定を authoritative とする。
- fake backend の PLT 再入は指定の診断と exit 134。初回 fake は gcc が直接の再帰を local alias にしたため SIGSEGV、試験の extern assembler alias で PLT call を確認して直した。production の再入検出を既定のまま検証した。
- `elf-check: PASS (8 ELF)`、`makefile-sync: PASS`、`header-check: PASS (61 sources)`。system loader が export する WSI 名と TSV を照合し、未分類名 0。
- 新規 source と generated forward.inc の style-check total 0。clang-format19 後、定義引数と3条件を復元、全文を手動レビュー。sh syntax / git diff --check PASS。
- `timeout 600 make -j64 disk-image` exit 0、warning 0。zedBSD の source / toolchain の変更無し、共通 C source の runtime 回帰対象無し。host package 追加無し。

設計上の実装の補い: Layer / version の enumeration は、Phase 本文が要求する「後段が無い場合」の fallback を作るため I の表にも入れた（当初の I 10 個に2個追加）。外部の約束・依存・受け入れは変更無し。
証拠は `build/ws105-p003/`、永続の summary / manifest は `plan/history/ws105/q525/`。WSI の実装はまだ無く、Linux compositor / app / gdm / network / audio は後続。実機 GPU、古い backend の全ての組合せ、musl は未実施。push / GitHub publication は未実施。


実装 commit `5dcb10992baf2ae7b13e0d9f1af61b9b3a366d1f`。Finished UTC: 2026-10-01T06:35:02.871528+00:00。次の Phase は WS105 の既存完了までの自走指示で選定。push / GitHub 公開は未実施。
