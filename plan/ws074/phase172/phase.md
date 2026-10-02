<!-- awesome-plan project=zedbsd record=ws074-p172 -->

# ws074-p172: origin/browser2 を libbrowser 配置へ取り込む

Status: uncleared
Disposition: normal
Parent: [WS074](../ws.md)
Primary Milestone: MG006（WS074から継承）
Queue / attempts: q584 / q584-i01 / A1（finished / uncleared、2026-10-02 user通常停止）；q579 / q579-i01 / P10（finished / uncleared）
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
Planning-time commands: 計画時は`git fetch`、`git merge-base`、`git diff --name-status`とWS107 inventoryのread-only照合のみ。その後のq579/q584統合・修正・検証は下のdated eventsと各checkpointに保存。
Current outcome/evidence: q584はユーザー通常停止指示でuncleared。最終[checkpoint14](import/checkpoint14/README.md)に全209current hashes、全文review97完了、残112 C/header、warning0 plain build/全168object text同一/実native117checksと所有process終了を保存。既往target/ABI/ASan成果を保持し、whole基準は未達。
Resume: mainの最終WIP統合/共有projectionを確認し、残112の全文review・必要規約修正・影響範囲検証を新たな有限p172 Queueへ明示的に選定/承認する。whole clearと実統合出力確認まで後続browser Phaseを開始しない。

## Event

2026-10-02 / ws074-browser2-gate-20261002: current userがbranch統合をWS074のblocking Phaseに指定。Phase172を計画し、WS074の全未実行browser作業の前提に追加。GitHub Issue/Projectはpublication保留。

2026-10-02 / ws074-dedicated-interop2025-20261002: userがブラウザ専任P10とp172後のAcid3 pixel完全一致・Interop 2025を追加。p172のimport scope/clearanceは維持。後続p100/p173の新条件は取込だけで達成とみなさず、統合出力を確認してから別Queueで測る。GitHub comment publication保留。

2026-10-02 / ws074-browser-next-goals-20261002: userがp174 File System Access、p175 OPFS、p176 Test262を後続目標に追加し、p173 Interop 2025の全対象PASS 100%を明示。p172のimport criteriaは変更しない。新Phaseもp172 whole-Phase clearanceと統合済みsourceの確認まで開始不可。GitHub publication保留。

2026-10-02 / n3-start-P10: current user「では、N=3でしばらく実行を続けてください」により最初の有限Queue q579を承認・開始。上限3時間、既存whole-Phase基準を保持。部分commitはclearanceではない。GitHub publication保留。

2026-10-02 / c-table-forward-order-20261002: full/manual review identified §2 vars-before-forward order conflicts with ANSI C callback-table initializers (bind/xml.c and svg-length.c). main asked user, who approved only necessary function prototypes before constant tables. Full C §2/Guardrail/automation updated; irrelevant prototypes remain normal block. This resolves the local policy question without weakening other conformance/whole-clear criteria or authorizing downstream work. Other manual review remains in-progress. GitHub decision comment pending publication.

2026-10-02 / q579-wrap-uncleared: userの全agent停止指示で通常wrap-up。branch差分の現行libbrowser配置への統合、ABI/境界、plain/ASan buildと既存回帰、target boot/native p014までの証拠は保存した。[checkpoint05](import/checkpoint05/README.md)時点で全209対象のmanual reviewは8完了、C/header 141とその他60が未完了。whole-Phaseはuncleared、後続browser gateは閉じたまま。再開は残りreviewを有限Queueへ再選定する。

2026-10-02 / q584-start: 最新userのAgent A N=3開始指示により残reviewとin-scope規約修正を最大3hのq584/A1へ選定。q579原結果を保存し、whole-Phase受け入れと後続gateを維持。指定model/highで独立worktreeから開始。

2026-10-02 / q584-A1-checkpoint06: A1 completed full C/component review of four small implementations and verified the eight previous reviewed hashes. [checkpoint06](import/checkpoint06/README.md) retains final hashes, warning0 plain build, compiler-specific identical object text and implementation tokens modulo two error-code names. Reviewed12/209; remaining137 C/header+60other. q584-i01 and p172 remain in-progress; no downstream work or whole-Phase clearance. GitHub publication pending under main ownership.

2026-10-02 / q584-A1-checkpoint07: Four more complete C/component reviews finished; [checkpoint07](import/checkpoint07/README.md) fixes SVG/PI/XML projection/HTML input hashes and warning0 compiler-specific identical object text. Reviewed16/209; remaining133 C/header+60other. q584-i01/p172 remain in-progress; downstream gate closed and remote publication pending.

2026-10-02 / q584-A1-checkpoint08: All60 non-C import entries have complete applicable manual review and exact applied-source hash reconciliation; [checkpoint08](import/checkpoint08/README.md) retains runner syntax, engine registry and seven missing direct native page receipts (260 PASS lines). Reviewed76/209; remaining133 C/header. q584-i01/p172 remain in-progress, downstream gate closed, remote publication pending.

2026-10-02 / q584-A1-checkpoint09: Four private headers now have complete C/component review; [checkpoint09](import/checkpoint09/README.md) retains unchanged types/field order/prototypes, warning0 rebuild and all168 production object-text equality. Reviewed80/209; remaining129 C/header. q584-i01/p172 remain in-progress; downstream gate closed and remote publication pending.

2026-10-02 / q584-A1-checkpoint10: Four small native fixtures have complete full C/component review; [checkpoint10](import/checkpoint10/README.md) retains executable-token equality, warning0 build/compiles, three identical object texts plus disclosed assert line metadata, and four PASS runtimes. Reviewed84/209; remaining125 C/header. q584-i01/p172 remain in-progress; downstream gate closed and remote publication pending.

2026-10-02 / q584-fixture-failure-safeguards: main確認で既存q584の必要なin-scope規約修正にはfixtureの失敗検知も含まれ、全修正のtoken一致は承認条件ではない。host-namespaceのfprintf戻り値を保存して負値なら即時return（既存failures加算を重複しない）、host-image-zeroのatom生成を別変数でNULL確認し既存construction failureへ返す。production OOM/仕様は変更せず、影響する2fixtureのcompile/runと変化したerror pathを記録する。新Queueや後続browser開始の許可ではない。

2026-10-02 / q584-A1-checkpoint11: Four more fixtures have full C/component review; main clarified necessary fixture failure handling is in q584 scope. [checkpoint11](import/checkpoint11/README.md) records namespace report checking and explicit image atom NULL refusals, warning0 final build/compiles, four PASS native runs and the retained/corrected initial call-layout build failure. Reviewed88/209; remaining121 C/header. q584-i01/p172 remain in-progress; downstream gate closed and remote publication pending.

2026-10-02 / q584-A1-checkpoint12: Three complete sizing/geometry/cloning implementations reviewed; [checkpoint12](import/checkpoint12/README.md) fixes hashes, exact token/object equality, warning0 build/compiles and affected native46 observations including real clone ENOMEM/collection. Reviewed91/209; remaining118 C/header. q584-i01/p172 remain in-progress; gate closed and remote publication pending.

2026-10-02 / q584-A1-checkpoint13: SubmitEvent implementation now has complete full C/component review; [checkpoint13](import/checkpoint13/README.md) verifies necessary2-only callback declarations, unchanged initializer bodies, exact final object text, warning0 targeted compile/link and native13/13 lifetime tests. Reviewed92/209; remaining117 C/header. q584-i01/p172 remain in-progress; gate closed and remote publication pending.

2026-10-02 / q584-A1-wrap-uncleared: Latest user requests all agents to end at a safe checkpoint, superseding same-session continuation and p172-completion-only wrap-up. [checkpoint14](import/checkpoint14/README.md) finishes five private header reviews with unchanged declaration tokens/field order, all168 exact production object texts, warning0 build/final-header compiles and actual117 native checks. Reviewed97/209; remaining112 C/header, zero other files. q584-i01 ends uncleared before10:12 UTC; whole p172 is uncleared because remaining full review/final conformance is absent. No new Phase/Queue, no owned running process, no intentionally uncommitted source. Resume requires merged final hashes and explicit finite p172 reselection; WS/Queue/history/main projections and GitHub publication are main-owner reconciliation.
