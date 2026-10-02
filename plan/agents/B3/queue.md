# Agent B3 Queue q583

Status: finished
Attempt: q583-i01 / cleared（部分診断のみ）
Owner: Agent B / B3 bug executor
Approval: current user / 2026-10-02「では、N=3で作業を開始してください。」。既存B3のBUG-125引継ぎ候補を症状別の有限診断として選定。
Timebox: 最大90分 / ws099-p017の部分診断scope
Phase: [ws099-p017](../../ws099/phase017/phase.md)
Snapshot: [approved phase](approved-phase.md) / SHA256 `a111d019d78868c493edd8c4f8542a7a67d85648fa2b33a1a5fda905bf5141c3`
Exact scope: q577の証拠を保持し、p076の二症状を独立に有限観察する。(1) submenu configure/map済みだが橙pixelが見えない場合のframe/capture時刻、(2) far-menu map前に次clickが入る場合のinput/map順。元のstages1–4と既存`--log-frames`、B3所有の独立timeline helperを使い、各症状を最大5回/各guest実行4分/guest計10実行まで観察する。far-menu診断ではfresh map surface/count/configure/focusを最大0.5秒×6でpollし、transport deadline10秒で打ち切る。最初の失敗を保存し、後続captureでPASSへ書き換えない。期待geometry・accepted request・negative条件を維持する。共有p076 harness、compositor製品sourceは読取のみ。p128/C2は別症状として報告し、このattemptに修正を混ぜない。
Dependencies: q577統合済み同期patch、元FAIL/PNG、Venus imageと明示rendererを確認。B3専用runtime/portを使う。
Criteria: 二症状それぞれの再現条件/時刻/画像による分類、または再現不能/期限到達を証拠とともに記録する。新helperのsyntax/規約と有限試験結果を確認する。この部分itemがclearしてもwhole p017の20回単独/5回C9基準は満たさず、Phaseはuncleared、BUG-125はtrackingのまま。共有harness修正は原因判定後の別Queue。
Worktree: `/home/awe/zedBSD-worktrees/b3` / `codex/b3-bug125`
Next Queue: 未投入。compositor修正は別Queue。
Merge requests / ACK: MR B3-q583-01（base ea55c973、提出2d33d3e7、B統合c2a8ba9f、ACK）。timeline helper/host negative/syntax/準備記録。guest未実施、whole Phase uncleared。B1 guest停止後にtiming観察を実行。
Sync: GitHub publication保留。WIP commitのみ、pushなし。Agent Aが共有投影を所有。

MR B3-q583-02: 提出6a00fba6、B統合3ed1834c、ACK。原条件5/handshake5の部分PASS、2症状非再現。195assetsのhashをmainで照合、代表PNGを目視。whole p017 unclearedとBUG-125 reproduced/trackingを保持。[結果](../../ws099/phase017/q583-result.md)、[conformance](../../ws099/phase017/q583-conformance.md)。owned guest/renderer停止済み。追加反復は未投入。
