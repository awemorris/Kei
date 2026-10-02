<!-- awesome-plan project=zedbsd record=ws074-p176 -->

# ws074-p176: JavaScript Test262 の全面計測と段階的改善

Status: planning（2026-10-02 ユーザー目標、Queueなし）
Disposition: normal
Parent: [WS074](../ws.md)
Primary Milestone: MG006（WS074を継承）
Prerequisites: [p172](../phase172/phase.md) whole-Phase cleared/実統合出力。専任枠ではp100→p174→p175→p173の後にQueueを配列するが、技術的な必須依存はp172とJS runnerの実出力。

## 目的と計画範囲

[TC39 Test262](https://github.com/tc39/test262)をJavaScriptエンジンの主要な互換性指標として使う。既存のWS074 runner/固定版/分母と、p172のbranchが持つ変更を照合し、固定commit・全suiteのmanifest・test harness・run modeを再確定する。現行の`intl402`・`staging`等の除外は理由を再検証し、未実施/skipをPASSにしない。構文、runtime、browser embeddingの結果を分ける。

最初の有限Queueでは全suiteのbaseline（PASS/FAIL/ERROR/timeout/unsupportedの分母と一覧）を採り、失敗clusterと依存から小さな実装Phaseを設計する。JS/Wasm共通engineの既存設計とlibbrowser公開境界を保つ。ユーザーは今回Test262の到達率を指定していないため、全suiteの測定・継続改善を目標とし、数値の最終基準はbaseline後に決める。既存のWS074 design M4（test262 ≥80%）を勝手に置換しない。

## 判定と再開

p176のclearanceは固定suiteの再現可能なbaseline、runnerのcoverage、失敗群別の後続Phase設計。Test262全件PASSを意味しない。各改善のclearanceとWSの最終受け入れは別途証拠で判定する。

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[browser component全文](../../standards/browser-component.md)を適用。現時点でsource変更/試験/Queue実行なし。

Event ws074-browser-next-goals-20261002: ユーザーがJavaScript: Test262を専任枠の目標へ追加。達成率の数値は指定されていない。GitHub publication保留。
