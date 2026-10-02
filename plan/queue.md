<!-- awesome-plan project=zedbsd record=queue -->

# Queue / all-agent index

Active Queues: q584/A1 browser、q585/A2 package契約調査、q586/A3 display契約調査。B側実行はB-owned laneから後で投影。
Status: active
Main executor / plan writer: Q1
Approval: current user / 2026-10-02「では、N=3でしばらく実行を続けてください」。既定専任3枠の最初のscopeを各lane/snapshotに固定。各上限3時間、開始時N_effective=3、利用上限後0/main引継ぎ。
Last finished Queue: [q580](history/queue-q580.md)

| Queue / attempt | Agent | Phase | Exact scope | State | Approval / checkpoint |
| --- | --- | --- | --- | --- | --- |
| q584 / q584-i01 | A1 | [p172](ws074/phase172/phase.md) | 統合済みbrowserの残全文reviewとin-scope修正・有限検証、3h | active / in-progress | [lane](agents/A1/queue.md) |
| q585 / q585-i01 | A2 | [p001](ws112/phase001/phase.md) | 5OS packageの入力/形式/native環境/CI契約調査のみ、60min | active / in-progress | [lane](agents/A2/queue.md) |
| q586 / q586-i01 | A3 | [p001](ws113/phase001/phase.md) | Vulkan hotplug/出力/Settings/窓所属/実機fixture設計のみ、90min | active / in-progress | [lane](agents/A3/queue.md) |
| q577 / q577-i01 | P8 | [phase](ws099/phase017/phase.md) | BUG-125の再現/同期診断、証明された試験raceのみ修正。compositor読取のみ。 | finished / uncleared | [archive](history/queue-q577.md) |
| q578 / q578-i01 | P9 | [phase](ws099/phase014/phase.md) | C10 hardware試験script、3分試走、60分soak。host占有は所有lock確認後。compositor修正なし。 | finished / cleared | [lane](agents/P9/queue.md) |
| q580 / q580-i01 | P9 | [phase](ws114/phase001/phase.md) | Debian13標準GTK4実測/19行機能表、compositor/GTK source変更なし、3時間 | finished / uncleared | [archive](history/queue-q580.md) |
| q579 / q579-i01 | P10 | [phase](ws074/phase172/phase.md) | pinned origin/browser2のmanifest/path対応/競合分類とブラウザ差分統合。ABI/所有境界維持。後続Phase開始不可。 | finished / uncleared | [archive](history/queue-q579.md) |

Dependency graph: C9実出力 → q577; c5-hw/hdmi-h4-hw → q578; WS107実source/p099/browser2 SHA → q579 → 後続browser（外部context、未投入）。3Queue間のsource依存なし。hardwareはP9のみ、QEMU runtimeは担当別。

## Upcoming Work Outlook

2026-10-02 A N=3再開: 最新user開始指示を既定A/B分担の最初の有限Phaseへ適用。q584はq579統合source→残review、q585/q586はcontext成果→契約調査で相互のsource依存なし。q581〜q583はB用に予約。全posterior Phase未投入、push/Issue公開保留。

WS111 launcherはcompleted。WS110/testingと本人確認--loginは検討のみ。
[P8 bug](agents/registry.md): [Bug Board](known-bugs.md)の未解決項目を個別のhandling WS/Phaseで消化。最初の候補は[BUG-125](bugs/BUG-125.md)（デモC9のresize不安定）。既存の保留/実機/owner条件を保持、Queue未選定。
[P9 desktop](agents/desktop-outlook.md): WS099/090/094等の高度化と実機/規約検証を専任で担当。最初の候補は[WS099 p014](ws099/phase014/phase.md)のC10実機1時間。見つけたbugはmainがBoardへ登録しP8へ。Queue未選定。
[P10 browser / WS074 p172](ws074/phase172/phase.md): origin/browser2取込が全後続browser Phaseのblocking gate。取込後の候補は[p100](ws074/phase100/phase.md) Acid3 pixel完全一致 → [p174](ws074/phase174/phase.md) File System Access → [p175](ws074/phase175/phase.md) OPFS → [p173](ws074/phase173/phase.md) Interop 2025 100% → [p176](ws074/phase176/phase.md) Test262。p101 CSS2全件も保持。全員reserved/未起動、Queue未選定。
[WS112 p001](ws112/phase001/phase.md): 5OS package/CI配布の契約・入力・形式調査。RPi arm64/buildのみ、CI runtime不要、FreeBSD source-only。planned/未順位、実装Queue未選定。
[WS113 p001](ws113/phase001/phase.md): zedBSD i915複数displayとVulkan通知/Settingsの契約・実機fixture。全拡張/全mirror、pointer越境で窓一括移動。planned/未順位、実装Queue未選定。
[WS114 p001](ws114/phase001/phase.md): Linux標準GTK4の実測と[機能表](ws114/gtk4-compat-matrix.md)の証拠化。続く採否レビュー後にWS114改善→WS115 upstream GTK4→WS116 Qt6範囲判断/移植→WS097/096書き下ろし。未順位/Queue未選定。


2026-10-02 / n3-execution-start: reserved3枠を実行に移す。共有計画の上書き・browser2全branch merge・pushなし。成果は小さなWIP commitからmainがレビュー/統合。

Preflight 2026-10-02: browser2 fetch確認 tip e53ef03b80113aec959deb67f828cba21d68d4be/common493b6eea90c45b3c1f393c0c62a0f7882b43621c不変。toolchain4trees writable dirs0。P9 solaris10-man SSH可、他QEMU/lockなし、GPU既存vfio-pci。P8既存image/renderer実在、geometry差の診断を優先。P10 manifest/classification開始。

2026-10-02 / q578-finished: C10 hardware60分cleared、[archive](history/queue-q578.md)保存確認。3children利用上限終了、mainがq577/q579の既存範囲を引継ぎ。後続Queueは未投入。

2026-10-02 / n3-restart-gtk4: userが利用上限回復と再起動許可。3枠同指定modelでgeneration2へ。q577/q579は元の3時間deadline維持、q580はdesktop次作業の新しい有限baseline Queue。全共有記録/mergeはQ1、pushなし。

2026-10-02 / n3-wrap-for-two-sessions: userの停止・回収指示によりP8/P9/P10を通常wrap-up。q577/q579/q580はいずれも部分成果・再開条件・cleanupを保存してunclearedで終了し、実行中Queueは0。A/B分担は計画上の所有であり、新しいQueueの承認ではない。
