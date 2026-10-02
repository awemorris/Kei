# P9 Queue q580
Status: finished
Attempt: q580-i01 / uncleared
Owner: Q1 canonical / P9 generation2 executor
Approval: current user / 2026-10-02 chat GTK4をdesktop次作業へ指定、利用上限回復後再起動許可。既存WS114 p001実測scopeを保持。
Started UTC: 2026-10-02T06:30:07.702259+00:00
Timebox: 最大3時間 / 1Phase
Phase: [ws114-p001](../../ws114/phase001/phase.md)
Snapshot: [approved phase](q580-approved-phase.md) / SHA256 `2204afc8e4ba60ff500ee5fac2fe02717fb44b21fb76cef0c28687bf801baec7`
Exact scope: Debian13標準GTK4/gtk4-demoをLinux Keilandで実測。G01–G11基本window/入力/clipboard/renderer経路、G12–G19の観測と未試験理由。機能表にstdout/stderr/QMP PNG/版/再現手順。compositor/GTKソース変更なし、portal実装なし、採否決定なし。
Dependencies: WS105/WS108 guestとLinux compositor実出力を確認。専用clone/overlay/runtimeのみ、既存guest停止/共有build消去なし。
Criteria: 全19行に測定またはskip理由とreview材料を保存。実行不能ならuncleared/再開条件。部分成果をwhole-clearにしない。
Worktree: /home/awe/zedBSD-worktrees/p9 / codex/p9
Ordered next Queues: なし。p002採否レビューが後続実装のgate。
Last Queue: [q578 finished/cleared](../../history/queue-q578.md)
Merge requests / ACK: generation2 baseは起動時main HEAD
Sync: GitHub publication保留、pushなし、all commit -m WIP

Outcome: [terminal result](../../ws114/phase001/q580-result.md)。GTK4 4.18.6の部分実測と19行表を保存。move/resize、cross-client clipboard、wheel/touch、D&D/PRIMARY、renderer/scale/IME等が未測定でPhaseはuncleared。専用QEMU/SSHは停止、overlayは再開用に保全。
