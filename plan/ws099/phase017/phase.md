<!-- awesome-plan project=zedbsd record=ws099-p017 -->
# ws099-p017: BUG-125 move/resizeの再現・試験同期の切り分け

Status: in-progress
Disposition: normal
Parent: [WS099](../ws.md)
Queue: q577 / q577-i01 / P8

## 承認と有限範囲

2026-10-02 current user「では、N=3でしばらく実行を続けてください」。直前にレビュー対象として記録したP8の最初のBUG-125候補を実行する。上限3時間。既存[guide](../guide.md)の提案p017を具体化する。
対象: `plan/ws035/tests/zdesktop-p076.sh` とこのPhaseの試験/証拠、[BUG-125](../../bugs/BUG-125.md)の関連証拠。compositorは読取のみ。共有Board/WS表はmainが更新する。

## 手順・完了条件

1. 現行image/試験で症状を再現し、settledのgeometryと画面のgeometryを比べる。必要な一時的`--log-frames`でsettled→composeの時刻を確認する。serial/console logで判定しない。
2. 原因が描画待ちのみと証明できた場合だけ、実settled後のpixel待ちを最大0.5秒×6に限定して直す。期待geometry/実resize不具合を待ち直しで隠さない。
3. 試験側の修正なら単独p076 20回とC9 5回でFAIL 0。source全変更を適用全文規則でレビューする。sh構文確認、実geometry/期待pixelのnegative条件を維持する。実行不能・本物のcompositor defect・時限超過ならuncleared、再現/限界と修正Phase案を保存する。
4. BUG-125をresolvedにするのは原因を直して上記検証した場合だけ。部分診断/commitは全Phase clearanceではない。

Dependencies: WS099 p003/p007の現行C9、WS035 p076試験、Venus QEMU image。結果の実在を開始時に照合する。
Standards: [Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、AGENTS.md、[automation](../../standards/automation.md)。make check、HAL API/toolchain変更禁止。共有toolchain read-only、担当別runtime/port/socket。
Evidence/commands/versions/results: 未実施。mainが統合checkpointと結果を追記する。
Resume: finite Queueの証拠と原因判定を確認して次attemptを選定する。

## Event

2026-10-02 / n3-start-ws099-p017: guideの提案を正式Phaseにし、BUG-125の試験同期診断だけをP8へ割当。compositor修正は別Queueへ。GitHub publication保留。
