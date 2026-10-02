# サブエージェント台帳

[運用契約](protocol.md)。2026-10-02のユーザー指示: `N_target=8`、`gpt-6.1-sol` / high。現在のruntime上限は子3（メインを含め4枠）。`N_effective`は実際の空き枠・利用制限・実行可能Queueに応じて調整する。P1〜P7の以前の担当は[Master](../master.md)に履歴として残す。次に割り当てるIDはP8。

| Agent / generation | Model / effort | WS | Worktree / branch | Current Queue | Ordered next Queues | State | Checkpoint / wrap-up / merge ACK |
| --- | --- | --- | --- | --- | --- | --- | --- |
| （製品実装担当なし） | — | — | — | — | — | idle | q576 finished。次Queueは未選定 |

この表は活動実体の記録。希望数だけで8人を「active」と記載しない。設計調査者3人は一時的な読取専用参加で、P8以降の実装担当IDやQueueを占有しない。agent/Queueが配属された時だけmainが行と個別laneを作り、終了/再開の世代とworktreeを保存する。GitHubへの共有は未同期。
