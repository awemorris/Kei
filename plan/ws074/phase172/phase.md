<!-- awesome-plan project=zedbsd record=ws074-p172 -->

# ws074-p172: origin/browser2 を libbrowser 配置へ取り込む

Status: cleared
Disposition: normal
Parent: [WS074](../ws.md)
Primary Milestone: MG006（WS074から継承）
Queue / attempts: browser3 q598 / q598-i01（finished / cleared、209/209全文review・input caller修正・whole最終検証）；q597 / q597-i01（finished / uncleared、review204/209・残5全文review/whole未達、ユーザー再起動停止）；q596 / q596-i01（finished / uncleared、170/209・残39・whole未達）；q595 / q595-i01（finished / cleared、ten-file partial、whole uncleared）；q594 / q594-i01（finished / cleared、five-file partial、whole uncleared）；q590/q584/q579（finished / uncleared）
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
Standards: [Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[browser component全文](../../standards/browser-component.md)、[automation](../../standards/automation.md)。WS107の不変移動style例外は完了時に失効し、このPhaseへ流用しない。`make check`禁止、HAL API/toolchain変更は対象外。q590最新user指示によりbrowser3へWIP commit/normal pushを実施。旧q579/q584のno-push履歴は保持。
Planning-time commands: 計画時は`git fetch`、`git merge-base`、`git diff --name-status`とWS107 inventoryのread-only照合のみ。その後のq579/q584/q590統合・修正・検証は下のdated eventsと各checkpointに保存。
Current outcome/evidence: q598-i01 finished/cleared. [checkpoint98](import/checkpoint98/README.md) fixes 209/209 final hashes and full-standard review; checkpoint96 carries manifest569 (209 applied/356 archive/4 excluded/0 unresolved), host plain/ASan 98 regression groups and 81 goldens each, Acid2 exact120000 pixels, ABI/source/include/client. Post-main final target warm build warning0, final boot/login and Venus p014 PASS after the recorded cold-start retry. Shared Agent A Queue ID/projection collision and GitHub Issue/Project publication remain pending; these are synchronization limits, not missing browser output.
Resume: whole p172 is cleared. WS074 remains incomplete; [p100](../phase100/phase.md) Acid3 exact-pixel work may be selected only through a new finite approved Queue with fixed assets and bounds. Agent A must reconcile the browser3 q594–q598 shared ID collisions/projections before publishing them. No Acid3/p100 execution occurred here.

## Event

2026-10-02 / browser3-direct-continuation: current user assigns browser continuation directly without subagents and authorizes WIP commits/pushes to browser3. q590-i01 resumes the remaining112 complete C/header reviews after q584. [Exact scope](browser3/q590-approved-scope.md), base origin/main261903952; all209 prior hashes match, A1 stopped. Existing whole-Phase criteria and downstream gate remain. Timebox09:41:27–12:41:27 UTC, review checkpoints record source/validation precisely; Issues/Project publication deferred.

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

2026-10-02 / q590-browser3-checkpoint15: [internal amendment](browser3/q590-amendment-01.md) records reproduced retired-child CSS cache allocation after the real geometry callback; the narrow detached guard fixes it, with pre-fix10/12 and final12/12 viewport checks. [checkpoint15](import/checkpoint15/README.md) completes3 full C/component reviews:100/209 reviewed,109 C/header pending; own warning0 host build,188 actual native checks, implementation object text equal and disclosed style-context semantic difference. No foreign Phase/design/acceptance changes; q590/p172 remain in-progress, downstream gate closed, Issue/Project publication deferred.

2026-10-02 / q590-browser3-checkpoint16: [checkpoint16](import/checkpoint16/README.md) finishes4 native fixture full C/component reviews, including root cleanup on failure, explicit status checking and binding/post-GC guards. Reviewed104/209, remaining105 C/header; four warning0 own fixture compiles/links and44/44 actual native checks, identical script/assertion string literals. No production changes or new acceptance claim; q590/p172 in-progress, gate closed, GitHub publication deferred.

2026-10-02 / q590-browser3-checkpoint17: [checkpoint17](import/checkpoint17/README.md) finishes click/timer complete full C/component review, preserving guarded activation/GC/finite virtual timer contracts. Reviewed106/209, remaining103 C/header; own warning0 build, native171 checks, actual click-page57 PASS/0 FAIL and14 runtime+8 diagnostic settle checks. String literals unchanged; differing object texts disclosed. q590/p172 in-progress, gate closed, remote Issue publication deferred.

2026-10-02 / q590-browser3-checkpoint18: [amendment02](browser3/q590-amendment-02.md) records a real data-copy-failure leak, baseline LeakSanitizer512 bytes/8 allocations exit23, corrected actual resource105/105 exit0/no leak. [checkpoint18](import/checkpoint18/README.md) finishes link/SVG text/resource fixture full review:109/209, remaining100 C/header, warning0 build, SVG42/viewport12/link22 checks. Full C§14 permits the12 checker goto findings, each one forward shared cleanup jump; manual receipt retained. No foreign Phase/criteria change; q590/p172 in-progress, downstream gate closed, Issue publication deferred.

2026-10-02 / q590-browser3-checkpoint19: [checkpoint19](import/checkpoint19/README.md) completes builtin bootstrap/helper and native class/private access full review. Reviewed111/209, remaining98 C/header; installer19-order/string literals preserved, own warning0 build, VM factory20/heap31 and exact class17/builtins30 JS lines. Negative snprintf now receives necessary EIO safeguard; normal output unchanged. Three permitted shared-cleanup goto findings manually classified; q590/p172 in-progress and gate closed.

2026-10-02 / q590-browser3-checkpoint20: [checkpoint20](import/checkpoint20/README.md) completes weak DOM subscription/adoption and native form control full review. Reviewed113/209, remaining96 C/header; original ownership/partial-error contracts preserved, own warning0 build and147 native checks including actual clone GC/ENOMEM. C literal/type-table equality, differing object texts/assert line metadata and style0 recorded. q590/p172 in-progress, gate closed, Issue publication deferred.

2026-10-02 / q590-browser3-checkpoint21: [checkpoint21](import/checkpoint21/README.md) finishes DOM HTML serialization/Symbol full reviews:115/209, remaining94. Exact iteration47/markup22 lines, native77 and warning0 own build; Symbol text identical, serializer difference disclosed. q590/p172 in-progress, gate closed, Issue publication deferred.

2026-10-02 / q590-browser3-checkpoint22: [checkpoint22](import/checkpoint22/README.md) finishes3 XML/attribute native fixture full reviews:118/209, remaining91. Individual allocation/status guards, complete failure cleanup and checked post-GC observations preserve existing cases; native73/warning0 scoped compiles/style0. q590/p172 in-progress, downstream gate closed, Issue publication deferred.

2026-10-02 / q590-browser3-checkpoint23: [checkpoint23](import/checkpoint23/README.md) finishes Page geometry full review:119/209, remaining90. Exact DOM43 lines, child-style30/native160 and own warning0 build; five permitted forward-cleanup goto findings manually classified, original geometry/retirement behavior preserved. q590/p172 in-progress, gate closed, Issue publication deferred.

2026-10-02 / q590-browser3-checkpoint24: [checkpoint24](import/checkpoint24/README.md) finishes child resource task full review:120/209, remaining89. Private drain extraction/immediate ownership guards preserve scan failure cleanup, origin/MIME/strict XML/events; snprintf bounds checked. Actual HTTP/local loader52/native148/page14 and own warning0 build;28 permitted cleanup goto findings classified. Internal redesign stays within p172; q590/p172 in-progress, gate closed, Issue publication deferred.

2026-10-02 / q590-browser3-checkpoint25: [amendment03](browser3/q590-amendment-03.md) records reproduced quoted-parameter/leading-subtype MIME defects, baseline76/80 exit1 and final80/80. [checkpoint25](import/checkpoint25/README.md) completes data URL full review:121/209, remaining88, previously reviewed supporting fixture revalidated without double counting. Actual resource105/LSAN80/warning0 build;14 permitted cleanup goto findings classified. First WIP050031f14 source/evidence push preceded inventory completion; this follow-up corrects fixture classification and supplies missing projections/receipts, all209 hashes reconcile. q590/p172 in-progress, unchanged foreign criteria/dependencies/gate, Issue publication deferred.

2026-10-02 / q590-browser3-checkpoint26: [evidence](import/checkpoint26/README.md), NodeIterator full review, native46/page60/own warning0/style0; original cursor/prototype/weak ownership preserved; reviewed122/209, remaining87 C/header. q590/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-02 / q590-browser3-checkpoint27: [evidence](import/checkpoint27/README.md), Full traversal/removal fixture reviews, native46/scoped warning0/style0; unchanged GC/adoption corpus; reviewed124/209, remaining85 C/header. q590/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-02 / q590-browser3-checkpoint28: [evidence](import/checkpoint28/README.md), Media full review/reproduced six wrong matches fixed, native39/page38/own warning0/style0; supporting fixture revalidated; reviewed125/209, remaining84 C/header. q590/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-02 / q590-browser3-checkpoint29: [evidence](import/checkpoint29/README.md), JSON full review,116 exact JS lines/LSAN9/own warning0;48 permitted cleanup findings, ordinary error guards; reviewed126/209, remaining83 C/header. q590/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-02 / q590-browser3-checkpoint30: [evidence](import/checkpoint30/README.md), CSS model/mutation fixture full reviews, native56/scoped warning0;14 permitted cleanup findings; original corpus unchanged; reviewed128/209, remaining81 C/header. q590/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-02 / q590-browser3-checkpoint31: [evidence](import/checkpoint31/README.md), Select/table full review, native140/page213/own warning0/style0; constant registries and native contracts preserved; reviewed130/209, remaining79 C/header. q590/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-02 / q590-browser3-checkpoint32: [evidence](import/checkpoint32/README.md), Full select/option/table collection fixture reviews, native123/scoped warning0;19 permitted cleanup findings, original GC corpus unchanged.; reviewed133/209, remaining76 C/header. q590/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-02 / q590-browser3-checkpoint33: [evidence](import/checkpoint33/README.md), Realm full review, native82/JS79 exact lines/own warning0/style0; primary/managed ownership and microtask behavior preserved.; reviewed134/209, remaining75 C/header. q590/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-02 / q590-browser3-checkpoint34: [evidence](import/checkpoint34/README.md), Click fixture full review, actual native10/scoped warning0/style0; unchanged no-stack-root activation/cancellation corpus.; reviewed135/209, remaining74 C/header. q590/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-02 / q590-browser3-terminal: 2026-10-02T12:33:42+00:00, q590-i01/whole p172 uncleared、archive（別セッションの記録、main には入れない）を保存・read-back検証。全文135/209、残74、全209hash一致、必要whole検証未達が理由。全owned process終了、次Queue無し、WS incomplete。WIP/push許可はlatest userのbrowser3指示、Issue/Project公開保留。再開はterminal証拠と有限再選定。

2026-10-02 / q594-direct-start: latest user「続けてください。」authorizes same p172 continuation. [Exact five-file slice](browser3/q594/selection.json)/[scope](browser3/q594/approved-scope.md),90min, all209 current hashes verified. Item clearance is scoped; whole p172 still requires remaining full review/final gates. No interface/dependency/design/foreign Phase changes. Post-next-WIP cleanup is user-authorized maintenance.

2026-10-02 / q594-browser3-checkpoint35: [evidence](import/checkpoint35/README.md), XML binding/parser/stream fixture full reviews, native51/scoped warning0;24 permitted cleanup findings; original native scripts and GC contracts unchanged.; reviewed138/209, remaining71 C/header. q594/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-02 / q594-browser3-checkpoint36: [evidence](import/checkpoint36/README.md), CSS rule model and Range deletion full reviews, final own warning0 build/native67;18 permitted shared-cleanup jumps; original CSS sources/native scripts and GC root exclusion preserved.; reviewed140/209, remaining69 C/header. q594/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-02 / q594-browser3-terminal: [terminal/verified archive](browser3/q594/README.md). q594 finished/item cleared for exact five-file partial scope, whole p172 uncleared/WS incomplete. Full review135→140/209,69 C/header remain, all209 hashes match, scoped118 native checks/warning0. Requested441-file/~107MiB cleanup completed after next WIP. New Queue/downstream unselected, next unreserved q595/q591–q593 reservations retained; WIP/normal browser3 push, Issue/Project publication deferred.

2026-10-02 / q595-autonomous-start: current userしばらく自走 instruction applied to [finite ten-file selection](browser3/q595/selection.json), at most3h. All209 source hashes matched q594 terminal. No subagents, whole criteria/downstream dependencies unchanged, shared toolchain untouched, WIP/browser3 push, Issue/Project deferred.

2026-10-02 / q595-browser3-checkpoint37: [evidence](import/checkpoint37/README.md), Range boundary/data full reviews; native89+100/scoped warning0;36 permitted cleanup jumps; original native GC/root exclusion and mutation scripts preserved.; reviewed142/209, remaining67 C/header. q595/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-02 / q595-browser3-checkpoint38: [evidence](import/checkpoint38/README.md), Range clone/extract full reviews; native15+14/scoped warning0;53 permitted cleanup jumps; original6000/1800 participant corpus and allocation-GC root exclusions preserved.; reviewed144/209, remaining65 C/header. q595/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-02 / q595-browser3-checkpoint39: [evidence](import/checkpoint39/README.md), Range insert/mutation/surround full reviews; native92+36+16/scoped warning0;43 permitted cleanup jumps; original direct native conversion/host callback/1800-node GC and weak tokens preserved.; reviewed147/209, remaining62 C/header. q595/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-02 / q595-browser3-checkpoint40: [evidence](import/checkpoint40/README.md), Collection native/form full reviews plus actual verified unpublished-state and argument-conversion GC repairs; native37+13 and affected12+25+10+65+56=218, own private build/scoped warning0;33 permitted forward cleanup jumps; original caller-root exclusions/corpora and all production strings/23 constant tables preserved.; reviewed150/209, remaining59 C/header. q595/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-02 / q595-collection-ownership-amendment01: verified missing callee roots during unpublished-wrapper allocation and four user conversion families repaired within selected collection binding/fixtures. Original failures and exact snapshots retained; final218 actual native checks/warning0 and retirement reclamation verified. Internal design only, foreign scope/interfaces/dependencies unchanged, whole p172 uncleared/WS incomplete and59 remaining full reviews/final gates. Issue/WS remote event delivery deferred. [design/evidence](browser3/q595/amendment01.md).

2026-10-02 / q595-browser3-terminal: [verified terminal/archive](browser3/q595/README.md), exact ten-file partial cleared/Queue finished. Review140→150/209,59 C/header remain/all209 current hashes verified, actual native580/warning0. Two reproduced collection GC ownership defects repaired; whole p172 uncleared/WS incomplete/final gates and downstream unchanged. All WIP/browser3 normal push, main only/own invoked builds and fixtures returned, Issues/Project deferred. Next Queue unselected, next free q596/q591–q593 reservations retained.

2026-10-03 JST / q596-autonomous-acid3-start: current user「では、その目標に向かって自走をお願いします。」selects [existing p172 finite continuation](browser3/q596/selection.json), at most3h/remaining59 full reviews and original whole final checks, from all209 verified current hashes. Goal remains Acid3 score100/100/zero differing pixels; p100 gate and fg017/WS relative priority retained, no new product choice. Main only/all WIP/browser3 push, remote Issues/Project deferred.

2026-10-03 JST / q596-function-ownership-amendment01: [internal design/fix](browser3/q596/amendment01.md); actual factory/reentrant-GC failures reproduced and input/unpublished/suspended-state ownership revised in the selected files. Scope/interfaces/dependencies/whole criteria unchanged; final checkpoint41 verification pending, p172 in-progress/WS incomplete.

2026-10-03 JST / q596-browser3-checkpoint41: [evidence](import/checkpoint41/README.md), VM function/factory full reviews; actual parent/code/closure and reentrant-native GC repairs, default factory56/interp45/realms30/ownership27/heap31 plus retired-realm PASS, fixed JS14/14, own warning0;83 permitted forward cleanup findings and original production strings/tables preserved.; reviewed152/209, remaining57 C/header. q596/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-03 JST / q596-browser3-checkpoint42: [evidence](import/checkpoint42/README.md), Realm/context ownership fixture full reviews; native30+27/scoped warning0/style0; immediate checked execution before teardown/source release, output brand guards and preserved actual GC/root exclusions/corpora.; reviewed154/209, remaining55 C/header. q596/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-03 JST / q596-browser3-checkpoint43: [evidence](import/checkpoint43/README.md), Form-owner/native clone/table mutation/row fixture full reviews; native56+26+42+75=199/scoped warning0, 37 permitted cleanup jumps, original actual GC/root exclusions/native scripts/6000/512 cloning corpus and C strings retained.; reviewed158/209, remaining51 C/header. q596/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-03 JST / q596-browser3-checkpoint44: [evidence](import/checkpoint44/README.md), Insertion/mutation-record/style-sheet/submit full reviews; native29+16+21+13=79 plus prior table42+75=117/scoped warning0; 85 allowed single forward cleanup jumps; original native GC/root exclusions/scripts/C strings retained; prior split-call format claim corrected and revalidated.; reviewed162/209, remaining47 C/header. q596/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-03 JST / q596-browser3-checkpoint45: [evidence](import/checkpoint45/README.md), Native SVG length409/text347 full reviews; actual60+42=102 native checks/scoped warning0; 34 permitted forward cleanup jumps; original scalar/text/child XML corpora, precise GC/sole native roots and C strings retained.; reviewed164/209, remaining45 C/header. q596/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-03 JST / q596-browser3-checkpoint46: [evidence](import/checkpoint46/README.md), XML model518/projector541 full reviews; actual236+54=290 native checks/scoped warning0; 20 permitted forward cleanup jumps; original XML syntax/bounds/pinned resources/native graphs, actual30000 wide corpus/precise GC roots and C strings retained.; reviewed166/209, remaining43 C/header. q596/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-03 JST / q596-browser3-checkpoint47: [evidence](import/checkpoint47/README.md), Native property617/shared-heap probe337 full reviews; actual65+8=73 native checks/scoped warning0; 0 permitted forward cleanup jumps; original native property policies/errors/GC and8 realm identity cases/C strings retained.; reviewed168/209, remaining41 C/header. q596/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-03 JST / q596-browser3-checkpoint48: [evidence](import/checkpoint48/README.md), Actual HTTP/local child-loading584 full review; native52/scoped warning0; 25 permitted forward cleanup jumps; original asynchronous/delayed/cancellation/timer/native GC/8MiB pressure/root interval corpus and C strings retained.; reviewed169/209, remaining40 C/header. q596/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-03 JST / q596-browser3-checkpoint49: [evidence](import/checkpoint49/README.md), Frame lifetime1190 full review; original148 native checks/scoped warning0; 79 permitted forward cleanup jumps; all29 native saved graph families/capped failure/owner counters/precise GC root intervals and C strings retained.; reviewed170/209, remaining39 C/header. q596/p172 in-progress, WS incomplete, downstream gates unchanged, Issue publication deferred.

2026-10-03 JST / q596-browser3-terminal: [terminal/verified archive](browser3/q596/README.md). q596/q596-i01 finished/uncleared at the unchanged 3h deadline, p172 uncleared and WS074 incomplete. Checkpoints41–49 finished20 full reviews, now170/209 with39 production C/header pending. Checkpoint50 captured the real vm_iter_rest GC crash and a scoped repair with warning0 compile and unchanged actual probe exit0; spread.c full review and one style issue remain. Whole p172 manifest/ABI/client/native/ASan/target/boot gates remain unverified, and the whole host builder exit is unknown. p100 remains planned/unselected; last fixed Acid3 comparison is37.04% agreement,302208/480000 differing pixels. Next q597 is only a candidate, q591–q593 reservations retained; no subagents, Issues/Project publication deferred.

2026-10-03 JST / q597-autonomous-start: current userの既存Acid3自走指示により、q596 archive read-back後、[39 source/whole original p172 exact selection](browser3/q597/selection.json)を有限3hで開始。全209 current hashesとclean HEADを確認、他executor無し。p100はwhole clearance/実出力待ち、WS incomplete、A/B予約と後続依存は維持。main単独、WIP/browser3 push、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint51: [evidence](import/checkpoint51/README.md), VM spread.c全文review完了。実GCで修正前の引数展開NaN/オブジェクトspread失敗/配列spread SIGSEGVを確認し、同じ3経路とrest/protocolの計5probeは修正後exit0。full host build warning0、JS14/14、関連host56+100+31+37、styleは許容forward cleanup54のみ。他の38 C/headerとwhole manifest/ABI/client/ASan/target/bootは未達。review171/209、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint52: [evidence](import/checkpoint52/README.md), JS Function builtin628行全文review。修正前の実GCでapply引数喪失→NaN、bind未公開関数回収→SIGSEGVを再現し、修正後同じ2probe exit0/期待値42・live=1。full host build warning0、JS14/14、host56+100+31、styleは許容forward cleanup30のみ。他の37 C/headerとwhole gatesは未達。review172/209、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint53: [evidence](import/checkpoint53/README.md), DOM mixin631行全文review。検索文字列/中間・結果配列とappend/prepend未挿入Textの実GC所有を修正、full host build warning0、DOM23/23、native insertion29/mutation16/tree exit0、styleは許容forward cleanup19のみ。残36 C/headerとwhole gates未達。review173/209、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint54: [evidence](import/checkpoint54/README.md), native table row655行全文review、source変更なし。scoped compile warning0/style0、元のhost-row75/75・mutation42/42・collection25/25。残35 C/headerとwhole gates未達。review174/209、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint55: [evidence](import/checkpoint55/README.md), CharacterData/Text742行全文review、source変更なし。scoped compile warning0/style0、host Text20/tree exit0/mutation16/range-data100/range-insert92。残34 C/headerとwhole gates未達。review175/209、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint56: [evidence](import/checkpoint56/README.md), HTML reflection803行全文review/属性setter GC所有と宣言順修正。full host build warning0、HTML probe40/40/object URL44/44/DOM23/23/host-form29/url exit0、style許容forward cleanup5のみ。残33 C/headerとwhole gates未達。review176/209、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint57: [evidence](import/checkpoint57/README.md), HTML input原本814行・修正後896行全文review。属性setterの一時文字列/atom GC所有と宣言順を修正、full host build warning0、入力probe94/94・controls100/100・DOM23/23・関連native130/130、styleは許容forward cleanup7のみ。残32 C/headerとwhole gates未達。review177/209、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint58: [evidence](import/checkpoint58/README.md), page sheets853行全文review。非同期URL解決失敗時の部分sheet破棄を修正、full host build warning0/style0、専用page75/75・HTTP async19/19・DOM23/23・native33/33。OOM注入は未実施。残31 C/headerとwhole gates未達。review178/209、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint59: [evidence](import/checkpoint59/README.md), iframe binding890行全文review。table初期化に不要なforward宣言を規約位置へ移動、source意味変更なし。full host build warning0/style0、専用page43/43・関連native242+retirement・DOM23/23。残30 C/headerとwhole gates未達。review179/209、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint60: [evidence](import/checkpoint60/README.md), page lifecycle/dump1038行全文review。dumpの全fallible appendを即時check、失敗layoutの部分tree破棄と前回成功layout復元を修正。full host build warning0、sheets DOM/style golden完全一致、native relayout21/position22、DOM23/23、style許容forward cleanup4のみ。OOM注入未実施。残29 C/headerとwhole gates未達。review180/209、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint61: [evidence](import/checkpoint61/README.md), browser shell main906行全文review。dump/--run console/PPMの出力失敗を非zero終了へ修正、normal4経路と/dev/full失敗3経路確認。full host build warning0/style0、DOM/style golden一致、DOM23/23・JS14/14。残28 C/headerとwhole gates未達。review181/209、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint62: [partial evidence](import/checkpoint62/README.md), DOM nodeのdoctype生成中3文字列rootと属性容量overflowを修正。full host build warning0、native204/204・DOM23/23、style許容forward cleanup2のみ。ただしbind/implementation.cとhtml/modes.cの連続文字列変換中所有が未解決で、dom/node.c全文reviewは未計上、181/209・残28を維持。次は呼び出し元を追跡・修正し再検証。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint63: [evidence](import/checkpoint63/README.md), DOM node全文reviewを呼び出し元のGC所有修正後に完了。bind/implementation.cの既レビュー箇所を再検証、html/modes.cのDOCTYPE経路を修正し同ファイル全文reviewは保留。全209hash一致、review182/209・残27。full host build warning0、XML probe62/62・native204/204・DOM23/23、style許容forward cleanup26のみ。html5lib corpus未配置、強制GC再現は未主張。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint64: [evidence](import/checkpoint64/README.md), CSS tokenizer1104行全文review。CRLFのbad-string source offset誤りを修正前後の実token比較で確認し、lookahead加算overflowを防止。全209hash一致、review183/209・残26。full host build warning0/style0、native CSS116/116・DOM23/23・values style golden完全一致。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint65: [evidence](import/checkpoint65/README.md), TreeWalker全文review・NodeFilter namespace/生成state/callback値/受理nodeの一時GC所有を修正。全209hash一致、review184/209・残25。full host build warning0、traversal実GC9/9・page70 PASS・DOM23/23、style許容forward cleanup7のみ。修正前crash再現は未主張。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint66: [evidence](import/checkpoint66/README.md), layout box tree全文review・極端な長さ/NaNのlayout unit変換を定義済み範囲へ制限。全209hash一致、review185/209・残24。full host build warning0/style0、UBSan変換5値、native relayout21/position22/table75・DOM23/23・style golden4件完全一致。下流geometry算術は別の制限。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint67: [evidence](import/checkpoint67/README.md), computed style binding全文review・公開前stateのGC所有を修正。全209hash一致、review186/209・残23。full host build warning0/style0、frame lifetime148/148・関連page68 PASS・DOM23/23。修正前GC再現は未主張。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint68: [evidence](import/checkpoint68/README.md), page script/event loop全文review・定数表位置と外部script失敗診断の確保結果を修正。全209hash一致、review187/209・残22。full host build warning0/style0、resource105/105・settle14+8・write3経路・child loading52/52・DOM23/23。resource runner初回binary指定誤りは修正後結果のみ採用。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint69: [evidence](import/checkpoint69/README.md), Window/realm全文review・定数表に必要な宣言だけ前置き、postMessageのoptions/origin/event/callbackをtimer登録までGC rootで保持。全209hash一致、review188/209・残21。full host build warning0、styleは許容のforward cleanup8件のみ。frame lifetime148/148・realm30/30・DOM23/23・settle14+8・postMessage origin5経路（4受信/1拒否）PASS。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint70: [evidence](import/checkpoint70/README.md), CSSOM1203行全文review。source変更不要、全209hash一致、review189/209・残20。GCC scoped warning0/style0、cp69 whole host build warning0。native CSSOM12/12・mutation12/12・rule model44/44、font指定のpage22/22・23/23、DOM23/23。font省略の初回pageは15 FAIL、必要fontで再実行後の結果のみ採用。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint71: [evidence](import/checkpoint71/README.md), DOM query/classList/dataset全文review・一時selector文字列、結果配列、token/map/accessorのGC rootを公開まで保持。全209hash一致、review190/209・残19。full host warning0、styleは許容のforward cleanup25件のみ。DOM23/23・child style30/30・3000要素/400 class add/remove cycles/dataset probe PASS。強制GC位置の検証なし。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint72: [evidence](import/checkpoint72/README.md), strict UTF8/XML parser1524行全文review。source変更不要、全209hash一致、review191/209・残18。GCC scoped warning0/style0、cp71 whole host build warning0。XML model236/236・DOM54/54・binding34/34・node32/32・document GC16/16。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint73: [evidence](import/checkpoint73/README.md), VM object/array全文review・length非確保判定、symbol/accessor生成、named property/reshape/array shrinkのGC rootを修正。全209hash一致、review192/209・残17。full host warning0、styleは許容のforward cleanup3件のみ。object100/0・array length4/4・VM factory GC56/56・JS14/14・DOM23/23。OOM fault injection未実施。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint74: [evidence](import/checkpoint74/README.md), HTML tokenizer2481行全文review。source変更不要、全209hash一致、review193/209・残16。GCC scoped warning0/style0、cp73 whole host warning0。9caseのwhole/UTF16単位stream一致、token/error明示検査pass。html5lib corpusはlocalに存在せず未実施。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint75: [evidence](import/checkpoint75/README.md), HTML insertion modes2119行全文review。source変更不要、全209hash一致、review194/209・残15。GCC scoped warning0/styleは許容cleanup5件、cp73 whole host warning0。tree focused5/5（doctype/head/body/table foster parenting/template）。html5lib corpusなしで未実施。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint76: [evidence](import/checkpoint76/README.md), HTML parser全文review。detached elementのattribute/template割当中GC rootを追加、C literal/ABI維持、全209hash一致、review195/209・残14。full host/scoped warning0、styleは許容cleanup3件。native parser8/8・stream9/9・table row75/75・DOM23/23・tree3/3。強制GC割当窓未実施。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint77: [evidence](import/checkpoint77/README.md), VM property access全文review。literal accessor/descriptor accessor/string wrapperのGC rootを追加、C literal/ABI維持、全209hash一致、review196/209・残13。full host/scoped warning0/style0。object100/0・VM factory GC56/56・JS14/14・DOM23/23。強制GC割当窓未実施。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint78: [evidence](import/checkpoint78/README.md), Element binding全文review。hidden/script.async一時stringのGC rootを追加、C literal/ABI維持、全209hash一致、review197/209・残12。full host/scoped warning0、styleは許容cleanup4件。DOM23/23・form29/0・image geometry10/10。強制GC割当窓未実施。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint79: [evidence](import/checkpoint79/README.md), Node binding全文review。共有node-array appendでcaller arrayとowning wrapperのGC rootを追加、C literal/ABI維持、全209hash一致、review198/209・残11。full host/scoped warning0/style0。DOM23/23・insertion29/29・clone26/0・collection GC37/37・XML binding34/34。強制GC割当窓未実施。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint80: [evidence](import/checkpoint80/README.md), Document binding全文review。Cookie無host時の未初期化statusを修正し、namespace/nameと未公開node、streamから除去したchildのGC rootを追加。既存literal維持、error pathの"complete"のみ1個追加、ABI維持、全209hash一致、review199/209・残10。full host/scoped warning0、styleは許容forward cleanup11のみ。DOM23/23、namespace12/12、stream9/9、parser8/8、XML document16/16、XML binding34/34、collection GC37/37。強制GC割当窓未実施。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint81: [evidence](import/checkpoint81/README.md), inline style binding全文review。一時宣言配列とname/value・未公開accessorのGC rootを追加し全成功/失敗経路で解放。既存literal/ABI維持、全209hash一致、review200/209・残9。full host/scoped warning0、styleは許容forward cleanup35のみ。CSSOM12/12・mutation12/12・rule44/44・style source21/21・DOM23/23・collection GC37/37。強制GC割当窓未実施。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint82: [evidence](import/checkpoint82/README.md), VM interpreter全文review。arguments object/key/accessor、rest iterator、super array引数をGC保護。generator resume失敗値とloose equalityの未初期化読取りを修正。既存literal/ABI維持、全209hash一致、review201/209・残8。初回host buildのmaybe-uninitializedを修正後、full/scoped warning0、styleは許容forward cleanup13のみ。object100/0・VM factory56/56・array4/4・JS14/14・DOM23/23。強制GC割当窓未実施。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint83: [evidence](import/checkpoint83/README.md), Object builtin全文review。native vector/descriptor、converted wrapper、未公開結果、tag一時文字列をGC保護。既存literal内容/ABI維持、全209hash一致、review202/209・残7。full/scoped warning0、styleは許容forward cleanup28のみ。object100/0・native properties65/65・VM factory56/56・JS14/14・DOM23/23。全getter窓の強制GC未実施。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint84: [evidence](import/checkpoint84/README.md), CSS parser全文review。未閉鎖属性/関数selectorと属性flag後の余剰tokenを拒否、nth数値範囲を検証、@supportsのENOMEMを伝播。DOM query異常系5件追加。既存literal内容/ABI維持（s flag literal追加）、全209hash一致、review203/209・残6。full/scoped warning0、style0、CSSOM12/12・rule model44/44・mutation12/12・JS14/14・DOM23/23。ENOMEM注入未実施。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint85: [evidence](import/checkpoint85/README.md), Event binding全文review。新規Event/EventTarget状態とwrapper、constructor option取得、dispatch listener snapshotのGC root寿命を補強し、getter成功/失敗経路を明示。bind/input.cの隣接Keyboard/Wheel callerに未root箇所を発見したが、q597対象外のため未修正。全209hash一致、review204/209・残5。full/scoped warning0、styleは許可されたcleanup goto15件のみ、click10/10・JS14/14・DOM23/23。全経路の強制GC未試験、whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 JST / q597-browser3-checkpoint86: [evidence](import/checkpoint86/README.md), Array helper部分review。array_hasのerror時未初期化出力、sort/joinのGC root寿命、scratch割当失敗を修正。全209hash一致、review204/209・残5（Arrayは部分review）。full/scoped warning0、styleは許可されたcleanup goto5件のみ、JS14/14。全callback強制GC・ENOMEM注入未実施。whole gates未達、q597/p172 in-progress、WS incomplete、p100未選定、Issue/Project公開保留。

2026-10-03 / q597-i01-terminal-uncleared: q597 archive（別セッションの記録、main には入れない）。ユーザーhost再起動指示で期限前に停止。39選定中34全文review、204/209・残5（Array部分）。checkpoint87で全209hash/whole host build warning0・JS14/14・DOM23/23・click10/10を検証。残5全文review、Arrayの追加GC root、対象外bind/input.c caller問題、元のprovenance/API/client/native/ASan/target/boot全体gateが未達。p172 uncleared、p100 blocked/unselected。再開はcheckpoint87と新しい有限Queue承認後。GitHub Issue/Project投稿保留。

2026-10-03 / q598-start: user再開指示、q597 archive/readback、全209hash/clean branch/remote一致を確認。[scope](browser3/q598/approved-scope.md)、[selection](browser3/q598/selection.json)で残5全文reviewと隣接bind/input.cのKeyboard/Wheel caller root修正および元のwhole gateを3hに選定。p172 in-progress、WS incomplete。p100未選定、GitHub Issue/Project投稿保留。

2026-10-03 JST / q598-checkpoint88: [evidence](import/checkpoint88/README.md), bind/input.cのKeyboardEvent/WheelEvent wrapperを追加getter中にGC rootで保持。209 inventoryの対象外なのでreview countは204/209・残5のまま、全209hash一致。whole/scoped warning0、JS14/14・DOM23/23、styleは許可cleanup goto7のみ。強制GC getter全経路未試験、whole gate未達、q598/p172 in-progress、WS incomplete、p100未選定。

2026-10-03 JST / q598-checkpoint89: [evidence](import/checkpoint89/README.md), Array constructor/prototype、新規copy配列、pop/shift戻り値、reduce accumulatorの一時GC rootを追加。全209hash一致、Arrayは部分reviewで204/209・残5のまま。whole/scoped warning0、JS14/14・DOM23/23、styleは許可cleanup goto29のみ。全再入経路の強制GC未実施、whole gate未達、q598/p172 in-progress、WS incomplete、p100未選定。

2026-10-03 JST / q598-checkpoint90: [evidence](import/checkpoint90/README.md), Array helperのreceiver/value、flat再帰mapped値、reverse一時値をGC rootで保護。全209hash一致、Arrayは部分reviewで204/209・残5のまま。whole/scoped warning0、JS14/14・DOM23/23、styleは許可cleanup goto33のみ。primitive wrapper等の残reviewと全経路強制GCは未実施、whole gate未達、q598/p172 in-progress、WS incomplete、p100未選定。

2026-10-03 JST / q598-checkpoint91: [evidence](import/checkpoint91/README.md), js/builtin_array.c 2858行の全文C/component reviewと再入GC root補強を完了。全209hash一致、review205/209・残4。GCC14.2 whole/scoped warning0、JS14/14・DOM23/23、focused Array5例、styleは許可cleanup goto47のみ。全callback強制GC/ENOMEM注入は未実施。whole gate未達、q598/p172 in-progress、WS incomplete、p100未選定。

2026-10-03 JST / q598-checkpoint92: [evidence](import/checkpoint92/README.md), bind/environment.c 3123行の全文C/component reviewを完了。MutationObserverのjob enqueue失敗時にrecordsを失わず再試行可能に修正。全209hash一致、review206/209・残3。GCC14.2 whole/scoped warning0、JS14/14・DOM23/23、style指摘0。enqueue ENOMEM注入/全callback強制GCは未実施。whole gate未達、q598/p172 in-progress、WS incomplete、p100未選定。

2026-10-03 JST / q598-checkpoint93: [evidence](import/checkpoint93/README.md), bind/range.c 3219行の全文C/component reviewを完了、source修正不要。全209hash一致、review207/209・残2 CSS。GCC14.2 whole/scoped warning0、Range native8本373検査・JS14/14・DOM23/23、style指摘0。全allocation失敗注入は未実施。whole gate未達、q598/p172 in-progress、WS incomplete、p100未選定。

2026-10-03 JST / q598-checkpoint94: [evidence](import/checkpoint94/README.md), css/values.c 4533行の全文C/component reviewを完了。色指定末尾の余分なtoken拒否と出力capacity検査を実装。全209hash一致、review208/209・残cascade.c。GCC14.2 whole/scoped warning0、CSS host5本128検査・JS14/14・DOM23/23、無効色4例期待通り、style指摘0。capacity故障注入未実施。whole gate未達、q598/p172 in-progress、WS incomplete、p100未選定。

2026-10-03 JST / q598-checkpoint95: [evidence](import/checkpoint95/README.md), css/cascade.c 4227行の全文C/component reviewを完了。メディア条件の上限超過を黙って切り捨てずE2BIGに修正。全209hash一致、review209/209・残0。GCC14.2 whole/scoped warning0、CSS host128/128・JS14/14・DOM23/23、style指摘0。元のwhole final gatesは未達、q598/p172 in-progress、WS incomplete、p100未選定。

2026-10-03 JST / q598-checkpoint96: [evidence](import/checkpoint96/README.md), 209/209最終hash・追加input.c全文確認。browser2 manifest569件はapplied209/archive356/excluded4/unresolved0、原Phase71件は履歴として保持。host plain/ASan各98回帰+81golden・Acid2 120000画素一致、独立client CPU/GPU描画、39exports/167source/377include・禁則依存0。target初回image/boot PNG合格、browser source警告0・外部package警告233行。最終warm buildのimage書込/最終boot/guest shellを残し、q598/p172 in-progress、WS incomplete、p100未選定。

2026-10-03 JST / q598-checkpoint97: [evidence](import/checkpoint97/README.md), unchanged-source target warm rebuild warning0/image check OK、同一imageの初回bootはQMP screendump timeout・bounded retryでlogin PNG合格。Venus guest p014は描画/GPU比較/スクロール/ Ctrl+Q/ERROR0合格、titlebar drag/close未達（3番目windowのdecoration mode1残留）。209/209 hash一致、guest停止。browser shellのsame-size configure後再commit欠落が推定原因。q598対象外source修正のscope判断待ち、p172 in-progress、p100未選定。

2026-10-03 JST / q598-amendment01: user「origin/mainをマージすれば継続できると思います。」に従い、[origin/main 483531ebe integration](browser3/q598/amendment01.md)をnative shell gateの前提として追加。WS114 p008のSSD既定を取り込む。q594–q598の共有Queue ID衝突は[両版保存](../../agents/browser3/main-merge-483531ebe/README.md)しA側への意味的同期待ち。p172未clear、p100未選定。

2026-10-03 JST / q598-terminal-cleared: [final evidence](import/checkpoint98/README.md)、local Queue archive（別セッションの記録、main には入れない）。209/209全文規約review/hash、browser2 manifest569の未解決0、host plain/ASan各98回帰+81golden・Acid2 exact、ABI/独立client、origin/mainのSSD変更を統合したtarget warning0 warm build/最終boot login PNG・Venus p014再試行PASS。初回cold p014の固定5秒待ち失敗は限界として保持。p172 whole cleared、WS074 incomplete。p100は前提を満たすが未選定・未実行。shared Queue q594–q598 ID衝突とIssue/Project publicationはA側の投影待ち。
