# Agent B3 Queue q589

Status: active
Attempt: q589-i01 / pending
Owner: Agent B / B3 bug executor
Approval: current user / 2026-10-02 continuous same-agent Queue指示。既存BUG-125の次の有限切り分け、製品修正なし。
Timebox: 最大45分、5 partial runs / 各240秒
Phase: [ws099-p017](../../ws099/phase017/phase.md)
Snapshot / exact partial scope: [q589 approved scope](../../ws099/phase017/q589-approved-scope.md) / SHA256 `16005006ee6e1868d2ad67dc1c1328f27d27d1e43f02dd012fa31ad7877464c7`
Dependencies: q583統合3ed1834cとq577元FAIL/image、B1 guest終了→main QEMU grant（外部資源）。
Graph: q583観測 → q589; q587 guest停止 → q589 guest。
Criteria: snapshotのPartial clearance criteria。whole p017 uncleared/BUG trackingを保持。
Worktree: `/home/awe/zedBSD-worktrees/b3` / `codex/b3-bug125`
Merge requests / ACK: 未提出。
Previous: [q583 finished partial diagnosis](q583-queue.md)
Upcoming Work Outlook: 非再現なら次の弁別条件をmainへ。負荷/製品修正は未投入。
Sync: WIP/no push、GitHub shared projectionはA所有。同じagentを維持して後続dispatchを受ける。
