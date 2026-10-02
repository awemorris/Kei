<!-- awesome-plan project=zedbsd record=queue -->

# Queue / all-agent index

Active Queues: q582/B2 desktop、q584/A1 browser、q586/A3 display契約調査、q587/B1 GTK4装飾。q581とq583はfinished/cleared（q583は部分診断のみ）、q585/A2はfinished/uncleared。
Status: active
Main executor / plan writer: Q1
Approval: 各lane/snapshotがexact scopeの正本。Aは2026-10-02 user「エージェントA、あなたもN=3で作業を開始してください」、BはuserのB開始指示とcommit991fc890のlaneを確認。A/B各3枠、timeboxはlaneごと。旧P8/P9/P10の承認・終了履歴を現行Queueへ流用しない。
Last finished Queue: [q585](history/queue-q585.md)（D1返答待ち / 契約調査uncleared）

| Queue / attempt | Agent | Phase | Exact scope | State | Approval / checkpoint |
| --- | --- | --- | --- | --- | --- |
| q581 / q581-i01 | B1 | [p001](ws114/phase001/phase.md) | GTK4標準baseline残実測のみ、最大3h | finished / cleared | [archive](history/queue-q581.md)、B df66db5e → A435a62126 |
| q582 / q582-i01 | B2 | [p011](ws094/phase011/phase.md) | Files desktop overflow log・listing成功時pruneと限定回帰、最大3h | active / in-progress（B報告） | [lane](agents/B2/queue.md)、commit991fc890 |
| q583 / q583-i01 | B3 | [p017](ws099/phase017/phase.md) | BUG-125の2popup症状を各最大5runで時刻/画素分類、90min。部分scope | finished / cleared（whole Phase uncleared） | [archive](history/queue-q583.md)、B df66db5e → A435a62126 |
| q587 / q587-i01 | B1 | [p007](ws114/phase007/phase.md) | CSD/SSD modeのconfigure/ack/commitと標準GTK4/native SSD回帰、3h | active / in-progress（user開始共有） | [lane](agents/B1/queue.md)、snapshot6de86725 |
| q584 / q584-i01 | A1 | [p172](ws074/phase172/phase.md) | 統合済みbrowserの残全文reviewとin-scope修正・有限検証、3h | active / in-progress | [lane](agents/A1/queue.md)、A1-006 → 5c0817c60 ACK / reviewed88/209 |
| q585 / q585-i01 | A2 | [p001](ws112/phase001/phase.md) | 5OS packageの入力/形式/native環境/CI契約調査のみ、60min | finished / uncleared | [archive](history/queue-q585.md)、D1 user返答待ち |
| q586 / q586-i01 | A3 | [p001](ws113/phase001/phase.md) | Vulkan hotplug/出力/Settings/窓所属/実機fixture設計のみ、90min | active / in-progress | [lane](agents/A3/queue.md)、A3-004 → ec870f856 ACK |
| q577 / q577-i01 | P8 | [phase](ws099/phase017/phase.md) | BUG-125の再現/同期診断、証明された試験raceのみ修正。compositor読取のみ。 | finished / uncleared | [archive](history/queue-q577.md) |
| q578 / q578-i01 | P9 | [phase](ws099/phase014/phase.md) | C10 hardware試験script、3分試走、60分soak。host占有は所有lock確認後。compositor修正なし。 | finished / cleared | [lane](agents/P9/queue.md) |
| q580 / q580-i01 | P9 | [phase](ws114/phase001/phase.md) | Debian13標準GTK4実測/19行機能表、compositor/GTK source変更なし、3時間 | finished / uncleared | [archive](history/queue-q580.md) |
| q579 / q579-i01 | P10 | [phase](ws074/phase172/phase.md) | pinned origin/browser2のmanifest/path対応/競合分類とブラウザ差分統合。ABI/所有境界維持。後続Phase開始不可。 | finished / uncleared | [archive](history/queue-q579.md) |

Dependency graph: C9実出力 → q577; c5-hw/hdmi-h4-hw → q578; WS107実source/p099/browser2 SHA → q579 → 後続browser（外部context、未投入）。旧3Queue間のsource依存なし。旧q578 hardwareはP9のみ。現6Queue依存は下のA/B再開記録を正本とし、QEMU/runtime所有は各laneで分離。

## Upcoming Work Outlook

2026-10-02 A/B N=3再開: 最新user開始指示を既定A/B分担の最初の有限Phaseへ適用。q579統合source→q584→後続browser（未投入）、WS108/111 context→q585、WS075/089/103 context→q586。q580 overlay→q581、WS094 p010→q582、q577同期patch/原FAIL→q583。6Queue間に実行source依存なし。q583は部分診断clearとwhole p017 unclearedを区別。q585/q586は文書調査のみでB sourceと競合なし。全後続Phase未投入、push/Issue公開保留。

WS111 launcherはcompleted。WS110/testingと本人確認--loginは検討のみ。
[P8 bug](agents/registry.md): [Bug Board](known-bugs.md)の未解決項目を個別のhandling WS/Phaseで消化。最初の候補は[BUG-125](bugs/BUG-125.md)（デモC9のresize不安定）。既存の保留/実機/owner条件を保持、Queue未選定。
[P9 desktop](agents/desktop-outlook.md): WS099/090/094等の高度化と実機/規約検証を専任で担当。最初の候補は[WS099 p014](ws099/phase014/phase.md)のC10実機1時間。見つけたbugはmainがBoardへ登録しP8へ。Queue未選定。
[P10 browser / WS074 p172](ws074/phase172/phase.md): origin/browser2取込が全後続browser Phaseのblocking gate。取込後の候補は[p100](ws074/phase100/phase.md) Acid3 pixel完全一致 → [p174](ws074/phase174/phase.md) File System Access → [p175](ws074/phase175/phase.md) OPFS → [p173](ws074/phase173/phase.md) Interop 2025 100% → [p176](ws074/phase176/phase.md) Test262。p101 CSS2全件も保持。A1はq584/p172 reviewを実行中。後続Phaseは未投入。
[WS112 p001](ws112/phase001/phase.md): 5OS package/CI配布の契約・入力・形式調査。RPi arm64/buildのみ、CI runtime不要、FreeBSD source-only。q585 finished/uncleared、D1 boot適用回答待ち、q591/p002候補のみ。
[WS113 p001](ws113/phase001/phase.md): zedBSD i915複数displayとVulkan通知/Settingsの契約・実機fixture。全拡張/全mirror、pointer越境で窓一括移動。q586調査中、製品実装Queueは未投入。
[WS114 p001](ws114/phase001/phase.md): Linux標準GTK4の実測と[機能表](ws114/gtk4-compat-matrix.md)の証拠化。B1 q581でbaseline調査cleared、q587/p007装飾モード実装・GTK4確認を開始。他機能の採否レビュー、WS115 upstream GTK4→WS116 Qt6範囲判断/移植→WS097/096書き下ろしは後続未投入。


2026-10-02 / n3-execution-start: reserved3枠を実行に移す。共有計画の上書き・browser2全branch merge・pushなし。成果は小さなWIP commitからmainがレビュー/統合。

Preflight 2026-10-02: browser2 fetch確認 tip e53ef03b80113aec959deb67f828cba21d68d4be/common493b6eea90c45b3c1f393c0c62a0f7882b43621c不変。toolchain4trees writable dirs0。P9 solaris10-man SSH可、他QEMU/lockなし、GPU既存vfio-pci。P8既存image/renderer実在、geometry差の診断を優先。P10 manifest/classification開始。

2026-10-02 / q578-finished: C10 hardware60分cleared、[archive](history/queue-q578.md)保存確認。3children利用上限終了、mainがq577/q579の既存範囲を引継ぎ。後続Queueは未投入。

2026-10-02 / n3-restart-gtk4: userが利用上限回復と再起動許可。3枠同指定modelでgeneration2へ。q577/q579は元の3時間deadline維持、q580はdesktop次作業の新しい有限baseline Queue。全共有記録/mergeはQ1、pushなし。

2026-10-02 / n3-wrap-for-two-sessions: userの停止・回収指示によりP8/P9/P10を通常wrap-up。q577/q579/q580はいずれも部分成果・再開条件・cleanupを保存してunclearedで終了し、実行中Queueは0。A/B分担は計画上の所有であり、新しいQueueの承認ではない。

2026-10-02 / next-ID-reservation: **q587はB1のq581後続、WS114装飾モード実装・GTK4確認用に予約**。userの共有指示を記録、Bのexact lane/snapshotは未到着。予約はactive membershipではない。次の未予約IDはq588。[協調記録](agents/two-session-coordination.md)。

2026-10-02 / B-df66db5e-projected: B checkpoint df66db5eをmain435a62126へ統合、195 BUG診断assets+73 GTK assetsのhash照合、代表PNG目視、Files source diff/style-check0・helper syntaxを確認。q581/p001調査clear、q583部分item clear/whole p017 uncleared、user共有によりq587/p007開始投入を投影。B2 q582 guest/boot関門は未達のまま。次ID q588/B2最終規約、q589/B3追加切り分け予約、A q590〜q592確保。全次Queueは個別lane/snapshotと依存で実行を確認する。

2026-10-02 / A-checkpoints-terminal-q585: A1 reviewed88/209とA3契約/foreign Phasesを統合・ACK。A2 q585契約調査を有限上限内に終了、D1未解決でuncleared、[archive](history/queue-q585.md)検証済み。workerは同sessionで待機、q590〜q592はID予約/候補のみ。GitHub publication保留、pushなし。

2026-10-02 / q593-reserved-B2: userが既存白樺・湖背景と見つかれば直線的抽象版の共通source収録を追加指示。q588後続WS099 p019のID **q593** をB2へ予約、exact Phase/scope/assets/3 OS verificationと依存はBから受領後に投影。予約はactive membershipではない。次の未予約IDq594。[協調記録](agents/two-session-coordination.md)。
