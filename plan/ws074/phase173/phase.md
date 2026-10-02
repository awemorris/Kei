<!-- awesome-plan project=zedbsd record=ws074-p173 -->

# ws074-p173: Interop 2025 WPTの対象固定とbaseline

Status: planned（2026-10-02 追加、Queueなし）
Disposition: normal
Parent: [WS074](../ws.md)
Primary Milestone: MG006（WS074を継承）
Prerequisites: [p172](../phase172/phase.md) whole-Phase cleared/実統合出力、[p100](../phase100/phase.md) cleared。

## 目的

ブラウザをWPT Interop 2025の公式focus area対象テストで検証し、全対象PASS 100%を目指す。その前段として公式の選定範囲、固定WPT commit、対象manifest、実行環境、runner能力、baselineを確定し、後続の小さな改善Phaseへ分ける。ユーザーの2026-10-02指示は目標と専任枠の指定であり、本Phaseの実行Queue承認ではない。

参照: [Interop 2025公式説明](https://github.com/web-platform-tests/interop/blob/main/2025/README.md)（focus areasとinvestigation effortsを別に記載）、[WPT](https://github.com/web-platform-tests/wpt)。focus areaに含まれるtestharness/reftest等を対象とし、調査項目は得点対象と混同しない。対象の選定規則と分母は実行時に公式manifest/labelsと固定commitから再現可能にする。

## 最初の有限な作業とclearance

1. p172で取り込まれたrunner/branch側WS074記録を確認し、公式Interop 2025 focus areaのtest selectionを固定commitで列挙する。各testの種別・期待・重複・環境要件をmanifestへ記録する。
2. 現行browser/libbrowserで走る範囲を測る。未対応のrunner機能、未実装web機能、実際のFAIL、crash、timeout、skipを分ける。実行不能をPASSとして扱わない。
3. 分母・PASS数・各focus areaの率と失敗clusterを再現可能なreportへまとめ、依存/コストを考慮して改善Phaseを設計する。対象を都合で除外しない。WPT資産はWS074の既存方針どおり固定取得し、ライセンスを確認する。

本Phaseのclearanceは**対象固定・runner/baseline・後続Phase設計**の完了を意味する。Interop 2025全件PASSの達成を意味しない。最終目標は固定した全対象でPASS 100%、fail/error/crash/timeout/未説明skip 0とし、後続の改善と最終回帰の証拠がそろった時にWSレベルで判定する。対象の変更や実行不能の扱いに製品/範囲判断が必要なら証拠と選択肢を記録し、勝手に基準を緩めない。専任P10ではp174 File System Accessとp175 OPFSを先に配列するが、本Phaseに不要な技術的依存を追加しない。

## 制約・再開

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[browser component全文](../../standards/browser-component.md)を適用。公開ABIとWaylandなしのlibbrowserを保つ。最初のattemptはmanifest/runner/baselineに限定し、時間上限と検証コマンドはQueue選定時に確定する。p100の実出力が統合・確認されるまで実行しない。現時点は計画のみ、source変更/試験/Queue実行なし。

Event ws074-dedicated-interop2025-20261002: ユーザーが専任ブラウザ担当によるInterop 2025クリアを目標に追加。p173を計画し、p172/p100を前提にした。WSのp101 CSS2全件目標は保持。GitHub publication保留。

Event ws074-browser-next-goals-20261002: ユーザーがInterop 2025 100%を明示し、File System Access/OPFSを先に積むよう指定。全対象PASS 100%/未説明skip 0を数値目標とし、p174/p175を専任laneの先の候補にした。p172/p100の技術的前提と本Phaseのbaseline clearance条件は維持。GitHub publication保留。
