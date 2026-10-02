<!-- awesome-plan project=zedbsd record=ws074-p100 -->

# ws074-p100: Acid3 100/100・pixel完全一致

Status: planned（2026-10-02 目標強化、Queueなし）
Disposition: normal
Parent: [WS074](../ws.md)
Primary Milestone: MG006（WS074を継承）
Prerequisites: [p099](../phase099/phase.md) cleared、[p172](../phase172/phase.md) whole-Phase clearedと実際に統合・検証された出力。branch側の同じ論理IDの記録はp172で意味的に照合する。

## 目的と範囲

固定した歴史的Acid3を現行libbrowser/browserで動かし、画面上の100/100と固定参照画像とのpixel完全一致を両方満たす。2026-09-30に計画された100/100・Uncaught/crash/timeout 0に、2026-10-02ユーザーがpixel完全一致とfail 0を追加した。

実行Queueの選定前に、p172が取り込むbranchのp100成果/証拠を照合し、Acid3資産のcommit、runner、viewport、font、参照画像、色空間、比較方式、反復回数と時間の上限を固定する。既存のAcid2とWS107独立componentの回帰を保つ。一般的なCSS/layout/DOM/描画修正を失敗の原因ごとに有限のQueueへ分ける。参照画像や試験を出力に合わせて改変しない。

## Clearance

- 固定したAcid3が表示する得点100/100、参照との画素差0、試験のfail/Uncaught/crash/timeout 0。試験環境・入力・出力画像・比較結果を保存する。
- 該当するbrowser/libbrowserとWS107の回帰、build、sourceの全文規約を確認する。未実施/skipをPASSとして数えない。
- p172のwhole-Phase clearance前に実装を始めない。後続の[p173](../phase173/phase.md)とp101は本Phaseのverified出力を前提にする。

## 制約・再開

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[browser component全文](../../standards/browser-component.md)を適用。WS107の内容不変移動style例外は流用しない。着手時の設計/試験条件をphaseへ確定し、exact-scope Queue承認を得る。現時点は計画のみ、source変更/試験/Queue実行なし。

Event ws074-dedicated-interop2025-20261002: ユーザーがAcid3のpixel単位100%・fail 0を指定。WS074のp100を強化。WS/branch側p100との照合はp172へ接続。GitHub publication保留。
