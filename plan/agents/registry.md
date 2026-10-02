# サブエージェント台帳

[運用契約](protocol.md)。固定名 P1〜P8（Claude Code の Agent tool のサブエージェント）。2026-10-02 ユーザー指定で N=2 から開始。過去の同名担当の記録は [history](../history/index.md) と git の履歴にある。

| Agent / generation | Agent type | WS | Worktree / branch | Current Queue | Ordered next Queues | State | Checkpoint / merge ACK |
| --- | --- | --- | --- | --- | --- | --- | --- |
| P1 / generation1 | phase-runner（high） | WS004（BUG-134 AX211） | `/home/awe/zedBSD-worktrees/p1` / `agent/p1` | [q590](P1/queue.md) | — | running | — |
| P2 / — | — | 未配属 | — | — | — | not started | — |
| P3〜P8 | — | 未配属 | — | — | — | N=2 の間は起動しない | — |

担当の WS と最初の Queue は、ベータ1（fg019）の内容をユーザーと決めてから割り当てる。
