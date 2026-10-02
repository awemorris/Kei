<!-- awesome-plan project=zedbsd record=q583 -->

# q583 / B3 / 2026-10-02

Status: finished
Attempt: q583-i01 / cleared（部分scope）

## Exact scope / approval

Approval: current user / 2026-10-02「では、N=3で作業を開始してください。」。既存B3のBUG-125引継ぎ候補を症状別の有限診断として選定。

Exact scope: q577の証拠を保持し、p076の二症状を独立に有限観察する。(1) submenu configure/map済みだが橙pixelが見えない場合のframe/capture時刻、(2) far-menu map前に次clickが入る場合のinput/map順。元のstages1–4と既存`--log-frames`、B3所有の独立timeline helperを使い、各症状を最大5回/各guest実行4分/guest計10実行まで観察する。far-menu診断ではfresh map surface/count/configure/focusを最大0.5秒×6でpollし、transport deadline10秒で打ち切る。最初の失敗を保存し、後続captureでPASSへ書き換えない。期待geometry・accepted request・negative条件を維持する。共有p076 harness、compositor製品sourceは読取のみ。p128/C2は別症状として報告し、このattemptに修正を混ぜない。

Preserved approval SHA256: `a111d019d78868c493edd8c4f8542a7a67d85648fa2b33a1a5fda905bf5141c3`。正本lane: [保存記録](../agents/B3/queue.md)。snapshotは元Phase directoryをrelative-link基準にする。

## Outcome / evidence / limits

二症状各5回の部分診断clear/非再現。whole p017 uncleared、BUG-125 reproduced/tracking。原FAIL保持。

[詳細結果](../ws099/phase017/q583-result.md)。B checkpoint df66db5e → A435a62126、source/チェック/原assetsをreview、195診断+73GTK assetsのhashと代表画像を確認。未実施関門は原結果を保持。owned runtime停止済み。GitHub Issues/Project保留、WIP commit、pushなし。
