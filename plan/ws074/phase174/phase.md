<!-- awesome-plan project=zedbsd record=ws074-p174 -->

# ws074-p174: File System Access API

Status: planning（2026-10-02 ユーザー目標、Queueなし）
Disposition: normal
Parent: [WS074](../ws.md)
Primary Milestone: MG006（WS074を継承）
Prerequisites: [p172](../phase172/phase.md) whole-Phase clearedと実統合出力、[p100](../phase100/phase.md) cleared。p175と共通のhandle/権限基盤を先に設計する。

## 目的と計画範囲

Webページが利用者の選択したローカルファイル/ディレクトリへアクセスする[WICG File System Access API](https://wicg.github.io/file-system-access/)をlibbrowserに追加する。対象候補は`showOpenFilePicker()`、`showSaveFilePicker()`、`showDirectoryPicker()`とfile/directory handleの読書き・列挙。実際の固定仕様版、対象WPT、権限の寿命、ユーザー操作とUI、site origin、sandbox、永続化、取消/失敗は最初の設計Queueで確定する。

2026-10-02の現source検索では上記picker APIの入口は見つからない。p172が取り込むbranchの内容を再確認してから未実装範囲を確定する。

ブラウザshellは選択UIとOSのfile dialog/権限の提示を所有し、Waylandに依存しないlibbrowserへ抽象interfaceで結果を渡す。エンジンから任意のhost pathを直接開かない。非対話のtest harnessはfixtureと権限を明示して使う。標準仕様とWPTの証拠で、APIを機能群ごとの有限Phaseへ分ける。OPFSの保存領域とorigin分離は[p175](../phase175/phase.md)が所有する。

## 判定と再開

WSの到達目標は、固定仕様に対する選定WPTと、開く・保存する・directoryを扱う対話の実動作を証拠で確認すること。p174の最初のclearanceは仕様/対象WPT/権限・UI契約/分割Phaseの確定であり、APIの実装完了を意味しない。実装Phaseはbaselineと依存を見て追加する。未対応のAPI、失敗、skipを隠さない。

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[browser component全文](../../standards/browser-component.md)を適用。p172取込とp100 clearance前には実行しない。現時点でsource変更/試験/Queue実行なし。

Event ws074-browser-next-goals-20261002: ユーザーがFile System Access APIをブラウザ専任枠の目標へ追加。WS074とp175/p173の順へ投影。GitHub publication保留。
