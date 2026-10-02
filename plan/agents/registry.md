# サブエージェント台帳

[運用契約](protocol.md)。2026-10-02のユーザー指示: `N_target=8`、`gpt-6.1-sol` / high。現在のruntime上限は子3（メインを含め4枠）。`N_effective`は実際の空き枠・利用制限・実行可能Queueに応じて調整する。P1〜P7の以前の担当は[Master](../master.md)に履歴として残す。最新指示「N=3でしばらく実行を続けてください」によりN_target/N_effective=3、generation 1を開始する。

| Agent / generation | Model / effort | WS | Worktree / branch | Current Queue | Ordered next Queues | State | Checkpoint / wrap-up / merge ACK |
| --- | --- | --- | --- | --- | --- | --- | --- |
| P8（バグ修正専任、generation 1） | `gpt-6.1-sol` / high | [Bug Board](../known-bugs.md)の未解決項目を個別のhandling WS/Phaseで担当 | `/home/awe/zedBSD-worktrees/p8` / `codex/p8` | [q577](P8/queue.md) | [BUG-125](../bugs/BUG-125.md)を最初の候補、次はデモ影響と再現性でmainが選定 | running / generation 1 | fb8732b93 → mainb9afcdbe8、20単独/C9 5回の有限検証中 |
| P9（Keilandデスクトップ高度化専任、generation 1） | `gpt-6.1-sol` / high | [desktop outlook](desktop-outlook.md)のWS099/090/094/113/114等を順に担当 | `/home/awe/zedBSD-worktrees/p9` / `codex/p9` | [q578](P9/queue.md) | [WS099 p014](../ws099/phase014/phase.md)など、依存と実機/ユーザー時間に応じてmainが選定 | running / generation 1 | 7d82da63a → mainc503330b8、freshshort14round/187秒PASS、60分継続中 |
| P10（ブラウザ専任、generation 1） | `gpt-6.1-sol` / high | [WS074](../ws074/ws.md) 固定 | `/home/awe/zedBSD-worktrees/p10` / `codex/p10` | [q579](P10/queue.md) | [p172](../ws074/phase172/phase.md) → [p100](../ws074/phase100/phase.md) → [p174](../ws074/phase174/phase.md) → [p175](../ws074/phase175/phase.md) → [p173](../ws074/phase173/phase.md) → [p176](../ws074/phase176/phase.md) を候補として検討 | running / generation 1 | 95835ef85 → maindebb5bd23、source移植/ABI/Acid2exact/Acid3score100、全文manual補正中 |

この表は活動実体と明示的な専任予約の記録。reservedはactive agent/Queueを意味しない。希望数だけで8人を「active」と記載しない。設計調査者3人は一時的な読取専用参加で、P8以降の実装担当IDやQueueを占有しない。P8/P9/P10はgeneration1稼働中。mainが個別laneで終了/再開の世代とworktreeを保存する。GitHubへの共有は未同期。

2026-10-02 / n3-execution-start: latest userがN=3継続実行を承認。runtime IDはspawn後に追記。main checkout所有Q1、publication保留、pushなし。

Runtime: P8=`/root/p8_bugs`、P9=`/root/p9_desktop`、P10=`/root/p10_browser`。全員GPT-6.1 Sol High、起動時Queue受領済み。base41aac4fc7、3独立worktree、共有toolchain symlink read-only。

P8 capacity incident: GPT-6.1 Sol High一時capacityでturn失敗。mainが未追跡5filesをplan/ws099/temp/p8-recoveryへ回収し同agentへ再投入。同modelで復帰、N_effective=3維持。runtime/成果を保持し重複試験なし。
