<!-- awesome-plan project=zedbsd record=q534 -->

# Queue

Status: finished
Active Queue: なし
Last finished Queue: q534

## q534

- Purpose: gdm と logind（seat-logind・最小の D-Bus・pause と resume・`keiland.desktop`）
- Focus: fg012 / WS105 の既存 Linux の受け入れを満たす。
- Timebox: 60 分。既知の build・必須検証は最大 150 分、未知の問題の調査は 30 分で結果と再開条件を記録する。
- Approval: current user, 2026-10-01「ws105の完了をゴールにして、自走をお願いします。」。既存 WS105 p001〜p011 の範囲を依存順の 1 Phase Queue で実装・検証・記録する。commit は WIP、push / GitHub 公開は行わない。
- Exact approved scope: [ws105-p009](ws105/q534/phase.md) 全範囲、開始前 [snapshot](ws105/q534/scope.md) SHA256 `822d2dee79a62353c5ff88ba55f44f350e667e5f2b5c829616dff2f3aae26cde`。
- Executor: Codex Q1。前 Queue finished、並行 executor なしを確認。
- Applicable rules: AGENTS.md、Guardrail、coding-style.md 全文、WS105 D1〜D25、design.md の指定節。
- Prerequisites: Phase の依存と既存出力を実装・証拠で確認。material scope / criteria の変更・取消なし。
- Started UTC: 2026-10-01T10:24:36.395480+00:00

| Attempt | Phase | Status | Dependency | Selection reason |
| --- | --- | --- | --- | --- |
| q534-i01 | [ws105-p009](ws105/q534/phase.md) | uncleared | p008（context） | WS105 の既存依存順 |

Dependency graph: Phase の依存（context）→ q534-i01/ws105-p009。他の Phase は次の Queue で選定する。

## Upcoming Work Outlook

WS105 の依存を満たす次の Phase。新しい product / risk 判断はユーザーの決定が前提。既存 WS105 の完了までの自走指示で選定・実行する。
GitHub への publication は deferred、outbox に保持する。

## Outcome

ws105-p009 **uncleared**。

uncleared（q534-i01、未達の前提で停止）。sourceはWIP commitに保存。D-Bus AUTH EXTERNAL / UNIX_FD / Hello、bounded frame / 16FD / signal queue / method deadline5s、logind control/device/pause/resume、backend dispatch、common os_paused / old poll snapshot guard、gdm session install / guest setupを実装。gcc14.2 / clang19.1.7 build warning0、26ELF、source-sync、331header、4Linux Cのstyle-check0 PASS。

gdm専用guestを作成し、psはuser kei / XDG session、logには正しいescaped session pathを確認。しかし最初のdisplay acquireでresult=-3、frames0 error5となり自動loginが繰り返されたためgdmを停止。試験用LD_PRELOAD observerでlibraryのDRM_IOCTL_SET_MASTER（0x641e）がerrno13/EACCESと確認した。Linux drm_auth.c の drm_master_check_perm はlogindが開いた共有fdのSET_MASTERに現在processの権限を要求する。現在のkms.cは渡されたmaster fdにも無条件SET_MASTERを行い、root専用のp005試験では発見できなかった。未知の前提を発見したためこのattemptを終了し、修正をp005で選定する。p009のVT / app / LogOut / rootdirect / target回帰は未実施、clear免除なし。

証拠 original build/ws105-p009/{gcc2,clang,elf,sources,headers,session-check,drm-observe,stop-loop}.log / session-first.png（画面が出ない、受け入れFAIL）。gdm停止、Linux専用guestは修正確認のため稼働、hostにpackage追加0、host gdmは触れていない。試験observerはguest overlayだけで本番には入れない。再開条件: p005がlogindから渡されたmaster fdをSET_MASTER不要で扱えることを検証、root direct / CRTC復元の回帰PASS。その後p009同じ基準で再試行。GitHub未公開、pushなし。


実装 commit `3400a098a66f8ae10875ca458b95d00aff01ac6d`。Finished UTC: 2026-10-01T10:42:07.310406+00:00。次の Phase は WS105 の既存完了までの自走指示で選定。push / GitHub 公開は未実施。
