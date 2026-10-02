# サブエージェント台帳

[運用契約](protocol.md)。2026-10-02のユーザー指示: `N_target=8`、`gpt-6.1-sol` / high。現在のruntime上限は子3（メインを含め4枠）。`N_effective`は実際の空き枠・利用制限・実行可能Queueに応じて調整する。P1〜P7の以前の担当は[Master](../master.md)に履歴として残す。次に割り当てるIDはP8。

| Agent / generation | Model / effort | WS | Worktree / branch | Current Queue | Ordered next Queues | State | Checkpoint / wrap-up / merge ACK |
| --- | --- | --- | --- | --- | --- | --- | --- |
| （製品実装担当なし） | — | — | — | — | — | idle | q576 finished。次Queueは未選定 |
| P10（ブラウザ専任枠、世代未開始） | `gpt-6.1-sol` / high | [WS074](../ws074/ws.md) 固定 | Queue承認時に作成 | なし | [p172](../ws074/phase172/phase.md) → p100 → [p173](../ws074/phase173/phase.md) を候補として検討 | reserved / 未起動 | 2026-10-02 ユーザー指示。P8・P9の割当は前回のレビュー案のまま未確定。commit/merge/Phase clearanceごとにmainが次Queueを投入する。 |

この表は活動実体と明示的な専任予約の記録。reservedはactive agent/Queueを意味しない。希望数だけで8人を「active」と記載しない。設計調査者3人は一時的な読取専用参加で、P8以降の実装担当IDやQueueを占有しない。P10はユーザーが前回の3人案からブラウザ専任を指定したため先に予約した。agent/Queueが実際に配属された時だけmainが個別laneを作り、終了/再開の世代とworktreeを保存する。GitHubへの共有は未同期。
