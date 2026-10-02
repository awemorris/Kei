# P8 Queue q577
Status: active
Attempt: q577-i01 / in-progress
Owner: Q1 main（canonical記録） / P8 generation 1（isolated executor）
Approval: current user / 2026-10-02 chat「では、N=3でしばらく実行を続けてください」、直前の専任3枠と最初の候補に基づく。
Started UTC: 2026-10-02T04:50:43.686853+00:00
Timebox: 最大3時間 / 1Phase
Phase: [phase](../../ws099/phase017/phase.md)
Snapshot: [q577-approved-phase.md](q577-approved-phase.md) / SHA256 `5c94068a5f16af1fdf5ab9fad00d2fb5f353ce9ef14fbab46a3d008aa8006775`
Exact scope: BUG-125の再現/同期診断、証明された試験raceのみ修正。compositor読取のみ。
Dependencies: approved Phaseにある実出力を開始時に検証。未検証依存は実行せずmainへ返す。
Worktree: /home/awe/zedBSD-worktrees/p8
Branch: codex/p8
Checks/criteria: snapshotのwhole-Phase基準を保持。部分commit/Queue結果とPhase clearanceを区別。
Ordered next Queues: 未投入。mainが結果/依存確認後に明示dispatch。
Merge requests / ACK: none
Outcome: 実行準備、未検証。
Sync: local-only records pending publication（configured github、公開保留）。push禁止、全commit -m WIP。

Preflight: 専用loopback SSH55747/gdb37895、image SHA256992e83f498cda2a1d506bb6ad7975b3c4592069fcb6493eca707e0d84403ac37。boot-test PASS、login PNG main目視/ユーザー提示済み。過去q538にRESIZE refused reason=no-pressがあり、試験のimplicit grab同期を診断中。

MR P8-q577-01: requested966a19d39ac82ccdd8a88bf29ebcf8f65084f4b0/base41aac4fc7、diagnosis/boot PNG/baseline1証拠のみ。main log/source比較・sh-n/diff-check review PASS。単独1回PASS、過去no-press症状の説明、provenance限界を保持。修理/全Phase clearanceなし。mainがBOOT PNG目視しuserへ提示済み。

MR P8-q577-02: requested8f28d9e02d8516f5210acdb71d19c8c65cf7ab26、lastACK966a19d39。main review原log/refused-no-press/expectedMISSING比較、prototype注入無効・短縮条件と全p076を区別する訂正、diff-check PASS。証拠checkpointのみ。held new-requestcountで1回geometryPASS、負例/20単独/5C9は未達。
