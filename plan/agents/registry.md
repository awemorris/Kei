# サブエージェント台帳

[運用契約](protocol.md)。2026-10-02のユーザー指示: `N_target=8`、`gpt-6.1-sol` / high。現在のruntime上限は子3（メインを含め4枠）。`N_effective`は実際の空き枠・利用制限・実行可能Queueに応じて調整する。P1〜P7の以前の担当は[Master](../master.md)に履歴として残す。P8〜P10を専任枠として予約し、実体化/Queue開始は未実施。

| Agent / generation | Model / effort | WS | Worktree / branch | Current Queue | Ordered next Queues | State | Checkpoint / wrap-up / merge ACK |
| --- | --- | --- | --- | --- | --- | --- | --- |
| P8（バグ修正専任枠、世代未開始） | `gpt-6.1-sol` / high | [Bug Board](../known-bugs.md)の未解決項目を個別のhandling WS/Phaseで担当 | Queue承認時に作成 | なし | [BUG-125](../bugs/BUG-125.md)を最初の候補、次はデモ影響と再現性でmainが選定 | reserved / 未起動 | 2026-10-02 ユーザー指示。既存のscheduled owner/ユーザー判断を維持。バグ一覧は実装Queueではない。 |
| P9（Keilandデスクトップ高度化専任枠、世代未開始） | `gpt-6.1-sol` / high | [desktop outlook](desktop-outlook.md)のWS099/090/094/113/114等を順に担当 | Queue承認時に作成 | なし | [WS099 p014](../ws099/phase014/phase.md)など、依存と実機/ユーザー時間に応じてmainが選定 | reserved / 未起動 | 2026-10-02 ユーザー指示。発見したdesktopのバグはmainがBug Boardへ登録しP8へ渡す。 |
| P10（ブラウザ専任枠、世代未開始） | `gpt-6.1-sol` / high | [WS074](../ws074/ws.md) 固定 | Queue承認時に作成 | なし | [p172](../ws074/phase172/phase.md) → [p100](../ws074/phase100/phase.md) → [p174](../ws074/phase174/phase.md) → [p175](../ws074/phase175/phase.md) → [p173](../ws074/phase173/phase.md) → [p176](../ws074/phase176/phase.md) を候補として検討 | reserved / 未起動 | 2026-10-02 ユーザー追加目標。p101 CSS2も保持。commit/merge/Phase clearanceごとにmainが次Queueを投入する。 |

この表は活動実体と明示的な専任予約の記録。reservedはactive agent/Queueを意味しない。希望数だけで8人を「active」と記載しない。設計調査者3人は一時的な読取専用参加で、P8以降の実装担当IDやQueueを占有しない。P8/P9/P10はユーザーの3枠固定指示を記録した予約で、実際のagent/Queueが配属された時だけmainが個別laneを作り、終了/再開の世代とworktreeを保存する。GitHubへの共有は未同期。
