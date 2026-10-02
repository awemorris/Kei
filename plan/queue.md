<!-- awesome-plan project=zedbsd record=queue -->

# Queue

Active Queue: なし
Last finished Queue: q576
Status: finished
Cycle: q576
Approval: current user /2026-10-02 chat「起動コマンドが長いので、/opt/keiland/bin/keiland-desktop でシェルスクリプトにまとめておいてもらえますか。LinuxでもFreeBSDでも使えるように。LinuxではGDMも引き続き対応。」＋correction「GDMはスクリプトを通さない方がいいです」。共通launcher実装/install/verificationを承認、GDMは既存wayland直接起動を保つ。--loginは「追加するのはどうでしょうか」の検討のみ。WS110 --testing/main.c変更は未承認。WIP commit、git push/実機pullは既存の開発host→FreeBSD検証承認を本launcherの同じ環境導入に適用、force pushなし。Issue/Project公開は保留。
Timebox: 最大30分 /1Phase
Focus: WS111 console launcher。既存fg010とdemo順保持。
Snapshot: [Phase](history/ws111/q576/approved-phase.md)、SHA256 f03786ed2f36a6d569747b6df384dfba48c54d0825f1b2b1ba3ab341bd8d5223

| Attempt | Phase | Exact scope | Status | Dependencies |
| --- | --- | --- | --- | --- |
| q576-i01 | [ws111p002](ws111/phase002/phase.md) | 最終shell/make/docs全文規約確認、Linux/FreeBSD実sh/env/args/execとnative script build/install/mode/byte、GDM direct entryを検証。 | cleared | p001 actual launcher/build/install outputs |

Dependency graph: WS105/WS109 context → p001 → p002。WS110/--login implementationは本Queue外。
Started UTC: 2026-10-02T02:47:22.972372+00:00

## Upcoming Work Outlook

WS111 launcherはcompleted。WS110/testingと本人確認--loginは検討のみ。
[WS074 p172](ws074/phase172/phase.md): origin/browser2取込。WS107移動表を利用してlibbrowser配置へ対応、全後続browser Phaseのblocking gate。planned/Queue未選定。p100→p101は取込後に証拠を再評価。
[WS112 p001](ws112/phase001/phase.md): 5OS package/CI配布の契約・入力・形式調査。RPi arm64/buildのみ、CI runtime不要、FreeBSD source-only。planned/未順位、実装Queue未選定。
[WS113 p001](ws113/phase001/phase.md): zedBSD i915複数displayとVulkan通知/Settingsの契約・実機fixture。全拡張/全mirror、pointer越境で窓一括移動。planned/未順位、実装Queue未選定。

Outcome: q576-i01 /Phase cleared。L1〜L3: 両OS shell/native build/install、FreeBSD/opt script0755、LinuxGDM direct entry unchanged、全source review PASS。[結果](/home/awe/zedBSD-claude1/plan/history/ws111/q576/result.md)。--loginは本人確認/PIN交換案のみ、WS110/testing未実装。
Finished UTC: 2026-10-02T02:51:00.579101+00:00

WS111 completion: L1〜L3 verified、p001/p002 cleared、completed。次Queue無し。--login/WS110 testingは未承認の実装scopeとして別候補。

## Planning follow-up / 2026-10-02

Event ws112-package-plan-20261002: [WS112](ws112/ws.md)と[候補p001](ws112/phase001/phase.md)をUpcoming Work Outlookへ追加。
5OS package/CI releaseの計画のみ、RPi arm64、CI runtime不要、FreeBSD source-only。未順位・Queue未選定。
q576の承認/attempt/outcomeは変更しない。Active Queueなし、次Queueを作らない。既存demo順位/WS106保留を保持。

2026-10-02 / ws112-rpi-build-only-20261002: OutlookのWS112 RPi候補はbuild/deb生成で受け入れ、GPU/GUI関門なし。Active Queueなし、q576履歴不変。

2026-10-02 / ws113-multidisplay-plan-20261002: WS113は後日候補のみ。q576 finished/Active Queueなし、既存承認とdemo順位は変更しない。

2026-10-02 / ws074-browser2-gate-20261002: WS074 p172をOutlookへ追加。q576 finished/Active Queueなし。追加は計画のみでbranch実装取込を開始しない。
