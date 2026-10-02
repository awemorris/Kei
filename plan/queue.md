<!-- awesome-plan project=zedbsd record=queue -->

# Queue / all-agent index

Active Queues: q577 (P8), q578 (P9), q579 (P10)
Status: active
Main executor / plan writer: Q1
Approval: current user / 2026-10-02「では、N=3でしばらく実行を続けてください」。既定専任3枠の最初のscopeを各lane/snapshotに固定。各上限3時間、N_effective=3。
Last finished Queue: [q576](history/queue-q576.md)（archive存在確認済み）

| Queue / attempt | Agent | Phase | Exact scope | State | Approval / checkpoint |
| --- | --- | --- | --- | --- | --- |
| q577 / q577-i01 | P8 | [phase](ws099/phase017/phase.md) | BUG-125の再現/同期診断、証明された試験raceのみ修正。compositor読取のみ。 | active / in-progress | [lane](agents/P8/queue.md) |
| q578 / q578-i01 | P9 | [phase](ws099/phase014/phase.md) | C10 hardware試験script、3分試走、60分soak。host占有は所有lock確認後。compositor修正なし。 | active / in-progress | [lane](agents/P9/queue.md) |
| q579 / q579-i01 | P10 | [phase](ws074/phase172/phase.md) | pinned origin/browser2のmanifest/path対応/競合分類とブラウザ差分統合。ABI/所有境界維持。後続Phase開始不可。 | active / in-progress | [lane](agents/P10/queue.md) |

Dependency graph: C9実出力 → q577; c5-hw/hdmi-h4-hw → q578; WS107実source/p099/browser2 SHA → q579 → 後続browser（外部context、未投入）。3Queue間のsource依存なし。hardwareはP9のみ、QEMU runtimeは担当別。

## Upcoming Work Outlook

WS111 launcherはcompleted。WS110/testingと本人確認--loginは検討のみ。
[P8 bug](agents/registry.md): [Bug Board](known-bugs.md)の未解決項目を個別のhandling WS/Phaseで消化。最初の候補は[BUG-125](bugs/BUG-125.md)（デモC9のresize不安定）。既存の保留/実機/owner条件を保持、Queue未選定。
[P9 desktop](agents/desktop-outlook.md): WS099/090/094等の高度化と実機/規約検証を専任で担当。最初の候補は[WS099 p014](ws099/phase014/phase.md)のC10実機1時間。見つけたbugはmainがBoardへ登録しP8へ。Queue未選定。
[P10 browser / WS074 p172](ws074/phase172/phase.md): origin/browser2取込が全後続browser Phaseのblocking gate。取込後の候補は[p100](ws074/phase100/phase.md) Acid3 pixel完全一致 → [p174](ws074/phase174/phase.md) File System Access → [p175](ws074/phase175/phase.md) OPFS → [p173](ws074/phase173/phase.md) Interop 2025 100% → [p176](ws074/phase176/phase.md) Test262。p101 CSS2全件も保持。全員reserved/未起動、Queue未選定。
[WS112 p001](ws112/phase001/phase.md): 5OS package/CI配布の契約・入力・形式調査。RPi arm64/buildのみ、CI runtime不要、FreeBSD source-only。planned/未順位、実装Queue未選定。
[WS113 p001](ws113/phase001/phase.md): zedBSD i915複数displayとVulkan通知/Settingsの契約・実機fixture。全拡張/全mirror、pointer越境で窓一括移動。planned/未順位、実装Queue未選定。
[WS114 p001](ws114/phase001/phase.md): Linux標準GTK4の実測と[機能表](ws114/gtk4-compat-matrix.md)の証拠化。続く採否レビュー後にWS114改善→WS115 upstream GTK4→WS116 Qt6範囲判断/移植→WS097/096書き下ろし。未順位/Queue未選定。


2026-10-02 / n3-execution-start: reserved3枠を実行に移す。共有計画の上書き・browser2全branch merge・pushなし。成果は小さなWIP commitからmainがレビュー/統合。
