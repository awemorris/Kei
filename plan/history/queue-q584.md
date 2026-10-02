# q584 / A1 browser normal wrap-up

Status: finished
Attempt: q584-i01 / uncleared
Phase: [ws074-p172](../ws074/phase172/phase.md) / uncleared
Period: 2026-10-02 07:12–08:53 UTC（最大3h、ユーザー通常終了指示で終了）

最終成果 `2f65567505edc8969e78cc2e4c4e87e63ff0c9b4` は Agent A `10e844f23` に統合。A1終了、tree clean、owned process無し。
[checkpoint14](../ws074/phase172/import/checkpoint14/README.md)に全209current hashes、全文review97件、残112 C/header、warning0 build、168 object .text一致、native117 checksを保存。rootは209 hashesと差分空白検査を確認。boot/ASan等の既存証拠は以前のcheckpoint参照、新規guest実行なし。whole Phase/WS acceptance未達、後続browser gate維持。

再開は残112件を有限p172 Queueに明示選定し、hash/規約/前提を再確認。q590はID予約のみ。GitHub publication pending、pushなし。

## Original approved lane and checkpoint history

```markdown
# Agent A1 Queue q584

Status: active
Attempt: q584-i01 / in-progress
Owner: Agent A canonical writer / A1 browser executor
Approval: current user / 2026-10-02「エージェントA、あなたもN=3で作業を開始してください」。直前のA/B担当分担とp172再開条件を適用。
Started UTC: 2026-10-02T07:12:00Z
Timebox: 最大3時間、1 Phaseの残review/検証。
Phase: [ws074-p172](../../ws074/phase172/phase.md)
Snapshot: [approved phase](approved-phase.md) / SHA256 `37f6fcd13e5b754faefa561c9e9499d6508879a82ceb44ef9d987ff212ac7156`
Exact scope: q579の統合済み209対象について、残る141 C/header・60その他の全文規約/component review、必要なin-scope規約修正と影響範囲の有限検証。既存必要宣言先行例外を使用。whole-Phase基準は維持、後続browser Phase開始不可。
Dependencies: q579 checkpoint05/main統合source、p099/WS107実成果を開始時に照合。
Worktree: /home/awe/zedBSD-worktrees/a1 / codex/a1-browser
Next Queue: q590 ID予約のみ。p172 whole-clear前に後続browser Phaseを開始しない。
Merge requests / ACK: A1-001 requested → integrated `4da5eb2e5e7ad96feb42a85b50b568cd2b0fbc65` → main `c2743455c` / ACK delivered。source diff/ownership/VM_THROWN契約と8 source hashes照合、worker warning0 build/object一致証拠。review12/209、whole gate閉。
Sync: GitHub Issues/Project publication保留。commitはWIP、pushなし。

MR A1-002 requested: last ACK `4da5eb2e5e7ad96feb42a85b50b568cd2b0fbc65` → `334cf75604d2a7259c3137689091fb3e554ffd76`。4 source全文review/checkpoint07、main review pending。agent独立review継続、未統合を依存解除に用いない。

2026-10-02 / MR ACK reconciliation: A1-002 334cf756 → e75cd6557、A1-003 7c8a7e9c → b3823176a、A1-004 443bb5fb/A1-005 78e9b0a → 6b49f3074、A1-006 029ba933 → 5c0817c60 integrated/ACK delivered。checkpoint11 reviewed88/209、remaining121 C/header、whole gate closed。root source hashes/style-check0/diff-checkと差分意味・worker runtime evidence確認。raw generated disassemblyの空白は除外/bytes維持。以前のrequested/pending行は当時履歴として保持。

2026-10-02 / wrap-up-request: userの「Phase完了後」指示を、後の「すべてのエージェントを終了に向かわせます。きりのいいところで作業をきりあげてもらいます。」で置換。新対象/Queueを開始せず現作業を安全なWIP checkpointへ保存して終了する。A1-007305a6cd9 → 6bf7efebf、A1-00812d34303 → 52c3ee3d7 ACK、review92/209。whole p172未達を完了扱いにしない、最終outcome/MRは回収待ち。
```

## Exact approved Phase snapshot

SHA256: `37f6fcd13e5b754faefa561c9e9499d6508879a82ceb44ef9d987ff212ac7156`

```markdown
<!-- awesome-plan project=zedbsd record=ws074-p172 -->

# ws074-p172: origin/browser2 を libbrowser 配置へ取り込む

Status: in-progress
Disposition: normal
Parent: [WS074](../ws.md)
Primary Milestone: MG006（WS074から継承）
Queue / attempts: q584 / q584-i01 / A1（active）；q579 / q579-i01 / P10（finished / uncleared）
Blocking: このPhaseが**whole-Phase cleared**になり、統合されたブラウザの実出力を確認するまで、WS074の後続のsource・runner・互換性作業は開始不可。p099以前の歴史的cleared状態には遡及適用しない。2026-10-02追加の[p100](../phase100/phase.md) pixel完全一致・[p173](../phase173/phase.md) Interop 2025もこのgateの後。

## Goal and exact scope

GitHubの`origin/browser2`が持つブラウザ変更を、WS107完了後の現行`userland/desktop/libbrowser/`（engine）と`userland/desktop/browser/`（shell）の境界へ移して統合し、元のbranchの機能/試験資産を壊さず後続Phaseが作業できるsourceを作る。ユーザーは2026-10-02にこの取込をWS074のblocking Phaseとして追加するよう指示した。今回の指示は計画追加であり、取り込み作業の実行Queue/merge/pushの許可ではない。

観測（2026-10-02、read-only fetch後）: `origin/browser2` tip `e53ef03b80113aec959deb67f828cba21d68d4be`、共通祖先`493b6eea90c45b3c1f393c0c62a0f7882b43621c`、HEADとbranchは分岐。祖先→branchに569変更ファイル、そのうちbrowser配下92 entry（engine旧パス91は現mainに同パス無し）。branchにはWS074 phase100およびphase102〜171、testsとplanの大量の追加がある。WS107の移動表は179ファイル、うちengine165をlibbrowserへ移しshell14をbrowserへ残した。branch側のPhase IDが171まであるため本Phaseは172。tip・差分は実行前に再取得して照合する。

対象: branchのbrowserに関係するsource、生成器・データ・テスト・build登録、WS074の関連計画/証拠と必要なbug ticketの意味的統合。branch側の無関係な`plan/master.md`/Queue/履歴や他OSの変更は機械的に上書きしない。WS107後のpublic ABI、shell→libbrowser.so依存、Waylandなしのengine、callback契約と現行の改善を保つ。WS074 p100/p101等の元のacceptanceを、importの都合で勝手にclearしない。

## Procedure / affected components

1. Queue選定時に`origin/browser2`のtip/共通祖先/変更一覧とローカル未commit差分を再確認し、承認されたcommit snapshotを固定する。既存の[WS107移動表](../../history/ws107/inventory.json)の`before`→`after`を旧engine sourceの機械的path対応に使い、shellの14件はbrowser側に残す。新規ファイルは[component boundary](../../standards/browser-component.md)の所有規則で分類する。パス/sha/適用成否の一覧を残す。
2. 共通祖先からbranchのブラウザ差分を得て、対応後の現行pathに適用する。branch全体のmerge/cherry-pickを盲目的に行わず、WS107の移動とその後の品質修正/公開ABIへ衝突した箇所だけ内容を比較して解決する。ユーザーの言う規則的なファイル移動を使い、全ファイルを個別に会話contextへ読み込むことを前提にしない。
3. 新sourceのMakefile登録/生成器/ヘッダー/テスト参照をlibbrowser所有へ合わせる。shellはpublic APIのみ、libbrowserはWaylandから独立、標準Vulkanを利用可能。WS107のcallback契約・ABI/所有・ライセンスを保つ。branchの新API/動作変更が既存設計に衝突する場合は証拠を記録し、意思決定が要ればunclearedにして問う。
4. branchのWS074 Phase/テスト/bug証拠をIDを維持して意味的に照合する。特にbranchのphase100・phase102〜171の結果を現行WS074へ根拠なしにコピーせず、成果物・依存・accepted結果を検証してから反映する。Master/Queue/Past Logを過去のbranch状態で置換しない。重複IDや矛盾を保存して再開条件を示す。
5. 所有境界・build/source一覧・独立public client・ブラウザshell・固定のhost/guest/focused regressionsを実行し、branch側の追加機能が残ることとWS107/p099の既知結果への後退を確かめる。全変更sourceを[C全文](../../coding-style.md)と[component全文](../../standards/browser-component.md)で最終reviewし、コマンド/バージョン/例外/未実施を残す。

## Clearance criteria

- pinned `origin/browser2`のブラウザに関係する変更と必要な計画/試験証拠について、移動表+新規所有分類の全件を`applied / redundant / superseded / unresolved`へ照合し、unresolvedが無い。無関係なbranch差分を取り込んだと偽らない。
- current libbrowser/browser所有とpublic API/ABI、Waylandなしのlibrary、Vulkan利用、shell動作が維持される。build/source登録に漏れが無い。WS107の試験と該当のブラウザ回帰が通る。失敗を単にbranchのものとして放置しない。
- branchの関連Phase/bug/testの出典と検証済み結果を保存する。p099のAcid2等の旧clearanceを維持できる証拠を取る。p100/p101のような後続Phaseは、取り込みだけで自動clearにしない。
- 全in-scope変更をC全文/境界全文でreviewし、format/lint/該当build/test/bootをPhaseで定めた範囲で実施。残る標準違反や欠けた実API/branch変更はuncleared。whole-Phase cleared/実出力確認前に後続browser作業を再開しない。

## Dependencies, bounds and evidence

Prerequisites: WS107 completedの実source/API移動成果、WS074 p099 cleared、`origin/browser2`の到達可能なcommit。これは既存成果の照合条件であり、新しいQueueの許可ではない。
Investigation bound: まず変更manifest/移動対応と競合分類を最大3時間の有限attemptで実施。scopeが大きくtimeboxを超える、branch tipがmaterialに変わる、またはABI/仕様の判断が要るときはattemptをunclearedで記録し、設計/Queueを再選定する。段階的適用をしてもPhase全体は上の全件受け入れまでclearにしない。
Standards: [Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[browser component全文](../../standards/browser-component.md)、[automation](../../standards/automation.md)。WS107の不変移動style例外は完了時に失効し、このPhaseへ流用しない。`make check`禁止、HAL API/toolchain変更は対象外。pushしない。
Commands/results/commit/environment/artifacts/skipped checks: 計画時の`git fetch`、`git merge-base`、`git diff --name-status`とWS107 inventoryのread-only照合のみ。source変更、patch適用、build/回帰/bootは未実施。
Resume: 本Phaseのscope/branch SHAと最新状態を再確認し、1 Phase Queueへ明示的に選定/承認してから実行する。

## Event

2026-10-02 / ws074-browser2-gate-20261002: current userがbranch統合をWS074のblocking Phaseに指定。Phase172を計画し、WS074の全未実行browser作業の前提に追加。GitHub Issue/Projectはpublication保留。

2026-10-02 / ws074-dedicated-interop2025-20261002: userがブラウザ専任P10とp172後のAcid3 pixel完全一致・Interop 2025を追加。p172のimport scope/clearanceは維持。後続p100/p173の新条件は取込だけで達成とみなさず、統合出力を確認してから別Queueで測る。GitHub comment publication保留。

2026-10-02 / ws074-browser-next-goals-20261002: userがp174 File System Access、p175 OPFS、p176 Test262を後続目標に追加し、p173 Interop 2025の全対象PASS 100%を明示。p172のimport criteriaは変更しない。新Phaseもp172 whole-Phase clearanceと統合済みsourceの確認まで開始不可。GitHub publication保留。

2026-10-02 / n3-start-P10: current user「では、N=3でしばらく実行を続けてください」により最初の有限Queue q579を承認・開始。上限3時間、既存whole-Phase基準を保持。部分commitはclearanceではない。GitHub publication保留。

2026-10-02 / c-table-forward-order-20261002: full/manual review identified §2 vars-before-forward order conflicts with ANSI C callback-table initializers (bind/xml.c and svg-length.c). main asked user, who approved only necessary function prototypes before constant tables. Full C §2/Guardrail/automation updated; irrelevant prototypes remain normal block. This resolves the local policy question without weakening other conformance/whole-clear criteria or authorizing downstream work. Other manual review remains in-progress. GitHub decision comment pending publication.

2026-10-02 / q579-wrap-uncleared: userの全agent停止指示で通常wrap-up。branch差分の現行libbrowser配置への統合、ABI/境界、plain/ASan buildと既存回帰、target boot/native p014までの証拠は保存した。[checkpoint05](import/checkpoint05/README.md)時点で全209対象のmanual reviewは8完了、C/header 141とその他60が未完了。whole-Phaseはuncleared、後続browser gateは閉じたまま。再開は残りreviewを有限Queueへ再選定する。

```
