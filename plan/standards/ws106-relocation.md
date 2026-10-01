# WS106: 純粋な配置変更の規約の例外（全文）

Authority: 2026-10-01 current user「WS106 は移動に限定し、既存スタイルの維持を認める」。
AGENTS.md の near-final 全文規約レビューで既存1300 style-check候補を扱うため、具体的scopeの例外を合意。
[移動前の指摘候補](../ws106/style-before.txt)、[全file/hash/mode](../ws106/survey.json)。

- Scope: WS106 の対象30 package＋直下13 filesと、その既存source/header/assetの移動・参照修正。
- 既存C実装を同じ内容で移す。既存の宣言/制御/コメント等の旧styleはこのWSの変更により一括修正しない。
- 許容するC/header差分は、実sourceの所在を保つincludeとpath/commentだけ。関数/処理/評価順/所有/lifetimeを変えない。
- 新しいC実装/意味を変える修正への例外ではない。そうした変更が必要なら全文を適用し、Queue範囲を見直す。
- 全文reviewは省略しない。全moveのhash/mode、全diff、include先、既存behaviorの維持、build/install/bootと例外外の標準適合を確認。
- formatterの一括適用はせず、path-only変更の範囲をreviewする。既存style-check候補をnormalize後に比較し、新しい指摘が増えていないことを確認。
- Expiry/review: WS106 の完了時に今回の適用を終了する。別WS/後日のimplementation refactorのstyleを緩める根拠にはしない。

C全文coding-style.md、Guardrail、automationの索引から参照。この例外は移動の目的と無関係な既存style修正による振る舞いの変化を避けるためのユーザー決定。
