# P9 Queue q578
Status: active
Attempt: q578-i01 / in-progress
Owner: Q1 main（canonical記録） / P9 generation 1（isolated executor）
Approval: current user / 2026-10-02 chat「では、N=3でしばらく実行を続けてください」、直前の専任3枠と最初の候補に基づく。
Started UTC: 2026-10-02T04:50:43.686853+00:00
Timebox: 最大3時間 / 1Phase
Phase: [phase](../../ws099/phase014/phase.md)
Snapshot: [q578-approved-phase.md](q578-approved-phase.md) / SHA256 `6b7a2e619fe882e5e9a3feee6dc6445206c81680cbd800374fe958a6ee6008d8`
Exact scope: C10 hardware試験script、3分試走、60分soak。host占有は所有lock確認後。compositor修正なし。
Dependencies: approved Phaseにある実出力を開始時に検証。未検証依存は実行せずmainへ返す。
Worktree: /home/awe/zedBSD-worktrees/p9
Branch: codex/p9
Checks/criteria: snapshotのwhole-Phase基準を保持。部分commit/Queue結果とPhase clearanceを区別。
Ordered next Queues: 未投入。mainが結果/依存確認後に明示dispatch。
Merge requests / ACK: none
Outcome: 実行準備、未検証。
Sync: local-only records pending publication（configured github、公開保留）。push禁止、全commit -m WIP。

Preflight: fixture host solaris10-man（chaos）SSH可、他QEMU/owner/lock無し、GPU既にvfio-pci。mainが所有lockを取得した専用fixtureの使用を許可。rebind/reboot/他VM停止はしない。source freshnessを確認してimageを選ぶ。

MR P9-q578-01: requested4bdd9224f/base41aac4fc7。main review: sh-n/diff-check PASS、exact owner/QEMU PID cleanup、有限elapsed、fresh receipt、error/restart判定を確認。試験script統合checkpointのみ、短試走/60分/10窓/実open-closeの画面確認は未実施。古いimageで現行clearanceを主張しない。
