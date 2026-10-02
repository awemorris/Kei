# サブエージェント台帳

[運用契約](protocol.md)。指定modelは`gpt-6.1-sol` / high。runtime上限は各メインsessionにつき子3（メインを含め4枠）。旧P8/P9/P10 generation2はwrap-up済み。2026-10-02最新user指示でAはN=3を実起動、BもN=3投入済みとのuser報告とcommit991fc890を確認。A/B各3laneを下表で管理する。

| Agent / generation | Model / effort | WS | Worktree / branch | Current Queue | Ordered next Queues | State | Checkpoint / wrap-up / merge ACK |
| --- | --- | --- | --- | --- | --- | --- | --- |
| A1 browser / generation1 | `gpt-6.1-sol` / high | WS074 | `/home/awe/zedBSD-worktrees/a1` / `codex/a1-browser` | [q584](A1/queue.md) | q590 ID予約 / whole p172-clear前は後続禁止 | running | runtime `/root/a1_browser`、base0e68854ac、A1-001〜006 integrated、last029ba933 → 5c0817c60 ACK、review88/209 |
| A2 packages / generation1 | `gpt-6.1-sol` / high | WS112 p001 | `/home/awe/zedBSD-worktrees/a2` / `codex/a2-packages` | [q585 finished](A2/queue.md) | q591 ID予約 / p002候補のみ | waiting in same session | runtime `/root/a2_packages`、base0e68854ac、A2-001〜006 integrated、D1返答待ちでq585/p001 uncleared、guest/build未実施 |
| A3 display / generation1 | `gpt-6.1-sol` / high | WS113 p001 | `/home/awe/zedBSD-worktrees/a3` / `codex/a3-display` | [q586](A3/queue.md) | q592 ID予約 / exact scope未投入 | running | runtime `/root/a3_display`、base0e68854ac、A3-001〜004 df2f36ff → ec870f856 ACK、D-ATOMIC返答待ち・調査継続 |
| B1 GTK / generation1 | `gpt-6.1-sol` / high | WS114 p007 | `/home/awe/zedBSD-worktrees/b1` / `codex/b1-ws114` | [q587](B1/queue.md) | 残行はp002採否後 | running（B/user開始報告） | q581 cleared、df66db5e → A435a62126、p007開始投影 |
| B2 desktop / generation1 | `gpt-6.1-sol` / high | WS094 p011 | `/home/awe/zedBSD-worktrees/b2` / `codex/b2-ws094` | [q582](B2/queue.md) | q588 ID予約 / WS094最終全文規約レビュー | running（B報告） | source/build checkpoint df66db5e → A435a62126、guest/boot未完 |
| B3 bugs / generation1 | `gpt-6.1-sol` / high | WS099 p017 / BUG-125 | `/home/awe/zedBSD-worktrees/b3` / `codex/b3-bug125` | [q583 finished](B3/queue.md) | q589 ID予約 / BUG-125追加切り分け | waiting in same session（B/user報告） | 部分item cleared、whole p017 uncleared。df66db5e → A435a62126 |
| P8（バグ修正専任、generation 2） | `gpt-6.1-sol` / high | [Bug Board](../known-bugs.md)の未解決項目を個別のhandling WS/Phaseで担当 | `/home/awe/zedBSD-worktrees/p8` / `codex/p8` | [q577](P8/queue.md) | BUG-125はAgent B B3の再開候補 | stopped / wrapped | final23065bf05 → main c1487ae3f。q577 uncleared、runtime cleanup済み |
| P9（Keilandデスクトップ高度化専任、generation 2） | `gpt-6.1-sol` / high | [desktop outlook](desktop-outlook.md)のWS099/090/094/113/114等を順に担当 | `/home/awe/zedBSD-worktrees/p9` / `codex/p9` | [q580](P9/queue.md) | WS114 p001はAgent B B1の再開候補 | stopped / wrapped | final8556185a3 → main ff0522115。q580 uncleared、QEMU/SSH cleanup済み |
| P10（ブラウザ専任、generation 2） | `gpt-6.1-sol` / high | [WS074](../ws074/ws.md) 固定 | `/home/awe/zedBSD-worktrees/p10` / `codex/p10` | [q579](P10/queue.md) | p172はAgent A A1の再開候補 | stopped / wrapped | final db6a5b336 → main2f37ca98e。q579 uncleared、owned processなし |

この表は活動実体と明示的な専任予約の記録。reservedはactive agent/Queueを意味しない。希望数だけで8人を「active」と記載しない。設計調査者3人は一時的な読取専用参加で、P8以降の実装担当IDやQueueを占有しない。generation1は利用上限で停止、ユーザー回復確認によりgeneration2へ。mainが個別laneで終了/再開の世代とworktreeを保存する。GitHubへの共有は未同期。

2026-10-02 / n3-execution-start: latest userがN=3継続実行を承認。runtime IDはspawn後に追記。main checkout所有Q1、publication保留、pushなし。

Runtime: P8=`/root/p8_bugs`、P9=`/root/p9_desktop`、P10=`/root/p10_browser`。全員GPT-6.1 Sol High、起動時Queue受領済み。base41aac4fc7、3独立worktree、共有toolchain symlink read-only。

P8 capacity incident: GPT-6.1 Sol High一時capacityでturn失敗。mainが未追跡5filesをplan/ws099/temp/p8-recoveryへ回収し同agentへ再投入。同modelで復帰、N_effective=3維持。runtime/成果を保持し重複試験なし。

2026-10-02 / n3-hard-usage-limit: 05:27 UTC頃、3人とも同modelの利用上限で終了（runtime再確認でchild稼働0）。現在N_effective=0、N_target=3/専任予約/model指定は維持。mainが既存有限Queueの検証と未commit成果保全を引継ぐ。p8/p9 tracked patch無し、p10 tracked patch571633bytesとuntracked7成果を各WS tempへ保全。別modelへの変更なし。再開時は同指定model/新generationで最新canonical Queue/ACKから回復する。

2026-10-02 / n3-generation2: current userが利用上限回復/再起動を許可。mainのintegration HEADへ各clean worktreeをfast-forward後、同model/highの3枠へ再dispatch。P9=q580 WS114 p001、P8=q577残範囲、P10=q579残全文manual。前attempt deadlineを延長しない。

Generation2 runtime: P8=/root/p8_bugs_g2、P9=/root/p9_desktop_g2、P10=/root/p10_browser_g2。3人とも指定model/highで稼働確認、main25729c88aから開始。q577/q579期限07:50:43UTC、q580上限3h。

2026-10-02 / two-session-wrap: userの指示で3agentを通常wrap-upし、全commitをmainへ統合・ACK。q577/q579/q580はterminal uncleared、q578 cleared。runtime/owned QEMU/SSH/build processなし。A/B各3laneへの再配属は[Master](../master.md)とsession文書を正本とし、新Queue開始までN_effective=0。
