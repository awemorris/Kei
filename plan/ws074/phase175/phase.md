<!-- awesome-plan project=zedbsd record=ws074-p175 -->

# ws074-p175: Origin Private File System

Status: planning（2026-10-02 ユーザー目標、Queueなし）
Disposition: normal
Parent: [WS074](../ws.md)
Primary Milestone: MG006（WS074を継承）
Prerequisites: [p172](../phase172/phase.md) whole-Phase clearedと実統合出力、[p174](../phase174/phase.md)の共有handle/権限契約の実出力。p174全体のclearanceだけで実APIが存在すると推定しない。

## 目的と計画範囲

[WHATWG File System Standard](https://fs.spec.whatwg.org/)の`navigator.storage.getDirectory()`を入口に、originごとに分離したprivateな保存領域とfile/directory handle、読書き・列挙・削除・永続化を実装する。利用者が選ぶ通常のhost filesystemへの権限とは区別する。固定仕様版と[WPT](https://github.com/web-platform-tests/wpt/tree/master/fs)の対象、originの定義、profile/再起動時の保存、quota、失敗/cleanup、workerや同期access handleの対象範囲を最初の設計Queueで棚卸しする。

2026-10-02の現source検索では`navigator.storage.getDirectory()`の入口は見つからない。p172が取り込むbranchの内容を再確認してから未実装範囲を確定する。

libbrowserがAPIとorigin分離を所有し、OSごとの永続保存の詳細は抽象interfaceへ閉じる。共通handle実装はp174の契約を使うが、OPFSのprivate領域が任意のhost pathへ流出しないことを確認する。機能群ごとに有限の実装Phaseへ分け、対応WPTと再起動/異originの隔離を検証する。

## 判定と再開

WSの到達目標は固定仕様/選定WPTに沿ったOPFSの実動作。p175の最初のclearanceは対象と保存契約、baseline、実装Phaseの分割までで、OPFS完了を意味しない。未対応/skipをPASSとして数えない。

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[browser component全文](../../standards/browser-component.md)を適用。p174の共有契約が実際に統合・確認されてから実行する。現時点でsource変更/試験/Queue実行なし。

Event ws074-browser-next-goals-20261002: ユーザーがOPFSをブラウザ専任枠の目標へ追加。p174との共有基盤と違いを明示。GitHub publication保留。
