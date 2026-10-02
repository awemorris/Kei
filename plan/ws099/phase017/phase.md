<!-- awesome-plan project=zedbsd record=ws099-p017 -->
# ws099-p017: BUG-125 move/resizeの再現・試験同期の切り分け

Status: uncleared
Disposition: normal
Parent: [WS099](../ws.md)
Queue: q577 / q577-i01 / P8（uncleared履歴）、q583 / q583-i01 / B3（部分診断cleared）

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
Evidence/commands/versions/results: [初代診断](diagnosis.md)、[generation2回復](recovery.md)、[最終結果/再開条件](q577-result.md)、[全文規約レビュー](conformance.md)。q577-i01はuncleared。mainが共有Queue/WS/Bugへ投影する。
Resume: finite Queueの証拠と原因判定を確認して次attemptを選定する。

## Event

2026-10-02 / n3-start-ws099-p017: guideの提案を正式Phaseにし、BUG-125の試験同期診断だけをP8へ割当。compositor修正は別Queueへ。GitHub publication保留。

2026-10-02 / p8-q577-terminal-wrap: userの全subagent停止指示で06:47UTCに補完C9を安全な境界で終了。単独20/全C9×5のFAIL0は未達、whole-Phaseはuncleared。held request同期修正の部分成果を保持し、popupのmapped-but-invisible症状とmap前click症状、別p128 launch失敗を未修正として区別する。[結果/再開条件](q577-result.md)。製品source変更無し、owned2runtime/全QEMU/processは停止済み。共有Board/WS/Bugとremote publicationはmain所有、公開保留。新Queue未開始。

2026-10-02 / b3-q583-terminal-diagnosis: q583部分診断を完了、[結果](q583-result.md)と[適用規約review](q583-conformance.md)をB mainが3ed1834cへ統合。original5/handshake5の全部分PASS、残2症状は今回非再現。診断 overhead/非並列条件の限界を保持し、修正・whole基準達成と扱わない。whole Phase uncleared、BUG-125 trackingのまま。owned runtime停止。新しい弁別条件を具体化した別Queueまで追加反復しない。

2026-10-02 / b3-q589-dispatch: ユーザー継続Queue指示により[q589 partial scope](q589-approved-scope.md)を投入。q583のcapture前snapshot overheadを減らす有限診断、製品/共有試験変更なし。whole Phase基準とBUG dispositionは維持。B1停止後の資源grantで5回まで実測。
