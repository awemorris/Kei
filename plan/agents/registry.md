# サブエージェント台帳

[運用契約](protocol.md)。固定名 P1〜P8（Claude Code の Agent tool のサブエージェント）。2026-10-02 ユーザー指定で N=4。過去の同名担当の記録は [history](../history/index.md) と git の履歴にある。

| Agent / generation | Agent type | WS | Worktree / branch | Current Queue | Ordered next Queues | State | Checkpoint / merge ACK |
| --- | --- | --- | --- | --- | --- | --- | --- |
| P1 / generation2（2026-10-02 権限の引き継ぎのため再起動、generation1 は通常 wrap-up） | phase-runner（high） | WS005（WiFi）・WS118 | `/home/awe/zedBSD-worktrees/p1` / `agent/p1` | [q590](P1/queue.md) | — | running | — |
| P2 / generation1 | phase-runner（high） | WS099（BUG-125） | `/home/awe/zedBSD-worktrees/p2` / `agent/p2` | [q591](P2/queue.md) | lane の Next | running | — |
| P3 / generation2（2026-10-02 権限の引き継ぎのため再起動） | phase-runner（high） | WS115 | `/home/awe/zedBSD-worktrees/p3` / `agent/p3` | [q592](P3/queue.md) | lane の Next | running | — |
| P4 / generation1 | phase-runner（high） | WS095 → WS127/WS089 | `/home/awe/zedBSD-worktrees/p4` / `agent/p4` | [q593](P4/queue.md) | lane の Next | running | — |
| P5〜P8 | — | 未配属 | — | — | — | N=4 の間は起動しない | — |

担当の WS と最初の Queue は、ベータ1（fg019）の内容をユーザーと決めてから割り当てる。

2026-10-02 23:50 / pace: user「では、N=3から2にコントロールする推奨案にします。」（週間の利用枠を 2026-10-04 日 17:00 までに使い切る）。P4 は q606（ws129-p010）の後に止める → N=3（P1・P2・P3）。2026-10-03 土 10:00 ごろ N=2（P2・P3）、P1 は 5320 の実機の作業の間だけ一時的に動かす。土曜の昼に使用率を見て調整。
