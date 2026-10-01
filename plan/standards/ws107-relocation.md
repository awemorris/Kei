# WS107 relocation の限定規約例外（全文）

Authority: 2026-10-02 current user「移動部分の既存スタイル維持を認める」。提示したscopeはWS107の内容不変の165file移動部分のみ。品質修正・新規テストと既知style候補14件にはC全文を適用。

- 内容不変のengine source/private header/table/shader移動とlocator修正に限り、既存implementation styleを保つ。
- viewの意味変更をした関数と新helper/新規試験、handler/loaderの14候補修正にはC全文を適用し、edited scopeをformat/manual reviewする。
- 全move/hash/mode、全diff、ABI/include/link/data/所有/入力/失敗処理を最終conformanceでレビューする。formatterだけを適合根拠にしない。
- 例外を使って別の実装/semantic rewriteを追加しない。未変更関数の一括reformatはしない。
- Expiry: WS107完了時に今回の適用終了。次のWS/後のrefactorへの恒久例外ではない。

[移動台帳](../history/ws107/inventory.json)、[既存style14候補](../history/ws107/style-before.txt)、[設計と有限修正](../ws107/design.md)。

2026-10-02 / ws107-completed: 今回の適用終了。結果は[conformance](../history/ws107/conformance.md)。今後のsemantic編集を免除しない。
