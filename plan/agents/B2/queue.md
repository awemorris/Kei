# Agent B2 Queue q582

Status: active
Attempt: q582-i01 / in-progress
Owner: Agent B / B2 desktop executor
Approval: current user / 2026-10-02「では、N=3で作業を開始してください。」。既存B2担当の[WS094 p011](../../ws094/phase011/phase.md)を選定。
Timebox: 最大3時間 / 1 Phase
Phase: [ws094-p011](../../ws094/phase011/phase.md)
Snapshot: [approved phase](approved-phase.md) / SHA256 `e3bca2fe809cf8af60f705454a43d43b677153221f8486922d8bd304686a0f77`
Exact scope: Files desktop gridで描かないoverflow項目数を既存ready logへ加え、存在しない名前の保存行をlisting成功時にmemoryから除く。表示方式は変えない。`desktop-layout.c`、`ui-desktop.c`、`files.h`とWS094専用host/guest試験だけを編集する。Phaseのhost/guest/build/style/boot確認を行う。compositor、Settings、libkeiland、WS081試験、HAL/toolchainは編集しない。
Dependencies: p010 clearedの実出力を確認する。独立BUILDとguest/runtimeを使い、共有toolchainとAのWS113 sourceは読取のみ。
Criteria: Phase記載のlog/prune動作とhost/guest回帰、warning0 build、全文規約、boot-test。未実施や失敗はattempt uncleared/再開条件へ。
Worktree: `/home/awe/zedBSD-worktrees/b2` / `codex/b2-ws094`
Next Queue: 未投入。
Merge requests / ACK: なし
Sync: GitHub publication保留。WIP commitのみ、pushなし。Agent Aが共有投影を所有。
