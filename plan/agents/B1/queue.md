# Agent B1 Queue q581

Status: finished
Attempt: q581-i01 / cleared
Owner: Agent B / B1 GTK executor
Approval: current user / 2026-10-02「では、N=3で作業を開始してください。」。既存B1担当とWS114 p001残測定候補を選定。
Timebox: 最大3時間 / 1 Phaseの残測定
Phase: [ws114-p001](../../ws114/phase001/phase.md)
Snapshot: [approved phase](approved-phase.md) / SHA256 `635c60678bd8d56d172f5f43fc5069866ea0f1f9dc515a3799972b4737715144`
Exact scope: q580で未測定の標準Debian13 GTK4 4.18.6 baselineを、保存済みKeiland package/sourceに固定した専用guestで有限実測する。interactive move/有効なresize、cross-client Unicode clipboard、wheel/focus/cursor、tooltip/repositionを優先し、Cairo/Vulkan rendererは実行可能性と結果を分けて調べる。G01–G19各行へ実測証拠または具体的skip理由、版、手順、QMP PNG、Wayland/application/D-Bus結果を記録する。source修正、p002の採否、p003/p004の実装、guest外の共有build変更は含めない。
Dependencies: q580の保存overlay/backing/keyとWS105/WS108 Linux compositor実成果を起動前に確認する。B1専用runtime/SSH2249を使い、他guestは停止しない。
Criteria: 行別reviewに必要な再現可能な結果/未測定理由と正常停止証拠。whole p001基準を満たさなければattempt unclearedにして残件を保存する。
Worktree: `/home/awe/zedBSD-worktrees/b1` / `codex/b1-ws114`
Next Queue: 未投入。p002のユーザー採否は別。
Merge requests / ACK: MR B1-q581-01（base ea55c973、提出21ce88c0、B統合9046f6fa、ACK）。[結果](../../ws114/phase001/q581-result.md)をreview。G01–G19に実測/具体skip、move/resize・別client clipboard・scroll/tooltip・software Cairo/Vulkanを確認しguest正常停止。p001の調査基準を満たしてcleared。GTK4全機能の成功やbug修正ではなく、WS114はincomplete。ユーザー追加CSD実装は別Phase/Queue。
Sync: GitHub publication保留。WIP commitのみ、pushなし。Agent Aが共有投影を所有。
