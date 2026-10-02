# Agent B1 Queue q587

Status: active
Attempt: q587-i01 / pending
Owner: Agent B / B1 GTK executor
Approval: current user / 2026-10-02、[Phaseに保存した追加CSD指示](../../ws114/phase007/phase.md)。具体実装と確認の追加実行を承認。Agent Aがq587を予約。
Timebox: 最大3時間 / 1 Phase
Phase: [ws114-p007](../../ws114/phase007/phase.md)
Snapshot: [approved phase](q587-approved-phase.md) / SHA256 `6de8672526c942a4211b12369846a116e1049f6a38e6461cfc7a068e789a339f`
Exact scope / criteria: 上記snapshotのScope/designとClearance/verification。SSD未要求またはCSD要求でKeilandが装飾しないmodeを実装し、標準GTK4・native SSD回帰を専用runtimeで確認する。
Dependencies: p001/q581 clearedの実測と停止済み資産、G05ユーザー採用指示。QEMU実測はB2専用guest終了後のmain割当に依存（外部資源、q587の実装権限ではない）。
Graph: q581実測 + G05 user decision → q587; B2 guest停止 → q587 guest検証。
Worktree: `/home/awe/zedBSD-worktrees/b1` / `codex/b1-ws114`
Merge requests / ACK: 未提出。
Previous: [q581 finished/clearedのlane snapshot](q581-queue.md)。過去attemptを改変しない。
Upcoming Work Outlook: 残る機能行はp002判断後に個別選定。p005/p006は依存未達で未投入。
Sync: GitHub publication保留。WIP commitのみ、pushなし。Agent Aが共有投影を所有。
