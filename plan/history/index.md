## 2026-10-02 並列実行開始（結果は未確定）

P8 q577、P9 q578、P10 q579をlatest userのN=3継続指示で開始。[Queue index](../queue.md)。既存q576結果は保持。merge/終了証拠は各laneへ追記。GitHub publication保留。

<!-- awesome-plan project=zedbsd record=past-log -->

# Past Log

Last finished Queue: [q576](queue-q576.md)（WS111 p002 cleared）

## 最新: q576 /WS111 completed

L1〜L3: 両OS shell/native build/install、FreeBSD/opt script0755、LinuxGDM direct entry unchanged、全source review PASS。[結果](/home/awe/zedBSD-claude1/plan/history/ws111/q576/result.md)。--loginは本人確認/PIN交換案のみ、WS110/testing未実装。

WIP commit、今回のlauncher git push/実機pullは既存指定環境の承認を利用。Issue/Project公開保留。WS110/testing/--loginは検討のみ。

## Queue history（直近30、古い順）

| Queue | Outcome |
| --- | --- |
| [q547](queue-q547.md) | WS108 p003 uncleared |
| [q548](queue-q548.md) | WS108 p003 cleared |
| [q549](queue-q549.md) | WS108 p004 cleared |
| [q550](queue-q550.md) | WS109 p001 uncleared / driver license・起動方法・realdevice回答待ち |

| [q551](queue-q551.md) | WS109 p001 item cleared /Phase cleared |
| [q552](queue-q552.md) | WS109 p002 item uncleared /Phase uncleared |
| [q553](queue-q553.md) | WS109 p002 item cleared /Phase uncleared |
| [q554](queue-q554.md) | WS109 p004 item cleared /Phase uncleared |
| [q555](queue-q555.md) | WS109 p004 item cleared /Phase uncleared |
| [q556](queue-q556.md) | WS109 p002 item cleared /Phase uncleared |
| [q557](queue-q557.md) | WS109 p003 item cleared /Phase uncleared |
| [q558](queue-q558.md) | WS109 p003 item cleared /Phase uncleared |
| [q559](queue-q559.md) | WS109 p003 item cleared /Phase uncleared |
| [q560](queue-q560.md) | WS109 p002 item cleared /Phase uncleared |
| [q561](queue-q561.md) | WS109 p002 item cleared /Phase uncleared |
| [q562](queue-q562.md) | WS109 p002 item uncleared /Phase uncleared |
| [q563](queue-q563.md) | WS109 p002 item uncleared /Phase uncleared |
| [q564](queue-q564.md) | WS109 p002 item cleared /Phase uncleared |
| [q565](queue-q565.md) | WS109 p002 item cleared /Phase cleared |
| [q566](queue-q566.md) | WS109 p005 item cleared /Phase uncleared |
| [q567](queue-q567.md) | WS109 p003 item uncleared /Phase uncleared |
| [q568](queue-q568.md) | WS109 p003 item uncleared /Phase uncleared |
| [q569](queue-q569.md) | WS109 p003 item cleared /Phase uncleared |
| [q570](queue-q570.md) | WS109 p003 item cleared /Phase cleared |
| [q571](queue-q571.md) | WS109 p004 item cleared /Phase cleared |
| [q572](queue-q572.md) | WS109 p005 item cleared /Phase cleared |
| [q573](queue-q573.md) | WS109 p006 cleared |
| [q574](queue-q574.md) | WS109 p007 cleared |
| [q575](queue-q575.md) | WS111 p001 cleared |
| [q576](queue-q576.md) | WS111 p002 cleared |

前回全文は[保存済みindex](ws109/q551/previous-past-log.md)。以前の全summary/判断/bugリンクは[through q548](past-log-through-q548.md)、[through q537](past-log-through-q537.md)、[through q522](past-log-through-q522.md)。各承認scope/結果はQueue archiveを参照。

2026-10-02 /ws111-completed-20261002: p001/p002に加えWS L1〜L3を照合、completed。userのGDM直接entry/pw本人確認→PIN案を保持。--login実装は検討のみ。

## 計画の追記 / 2026-10-02

Event ws112-package-plan-20261002: userの「あとで実装」に従い[WS112](../ws112/ws.md)（5OS package作成/CI全5種類release）をplannedで追加。RPi OS arm64、ad hoc生成可、CI runtime不要、FreeBSD source-only。
実装・build・Queue実行は無し。q576 finishedと旧WS108/WS111の完了証拠は保持。新実装候補はOutlookのみ、Issue/Projectとrepository pushは未実施。

2026-10-02 / ws112-rpi-build-only-20261002: 追加user決定: RPiはbuildが通ればOK。GPU/GUI確認を要求しないことをWS112とaffected Phase/方針へ反映、計画のみ。

## 計画追記 / 2026-10-02（WS113）

Event ws113-multidisplay-plan-20261002: [WS113](../ws113/ws.md)をMG006のplannedとして追加。i915→標準Vulkan Display通知、Settings→libkeiland→compositor拡張、全拡張/全mirror、配置drag、拡張時の窓単一出力/pointer越境切替を保存。zedBSD i915を完了の対象とし、9Phaseを計画。実装/Queue/実機試験なし。q576とWS089の過去証拠を保持。Issue/Project公開とpushは未実施。

## 計画追記 / 2026-10-02（WS074 branch gate）

Event ws074-browser2-gate-20261002: [WS074 p172](../ws074/phase172/phase.md)をorigin/browser2取込のblocking Phaseとして追加。WS107移動表による旧engine→libbrowser対応とbranch側Phase/test/bugの意味的照合を計画。全未実行browser Phaseはp172全体clearまで実行不可。branch tip e53ef03b8をread-only fetchで観測、merge/patch実行なし。q576とp099の歴史的結果は保持、Issue/Project公開保留。

## 計画追記 / 2026-10-02（GTK4/Qt6 upstream学習）

Event ws114-gtk-qt-port-projections-20261002: [WS114](../ws114/ws.md) Linux標準GTK4の[19項目表](../ws114/gtk4-compat-matrix.md)と行別採否→選択したXDG-shell/portal改善、[WS115](../ws115/ws.md) zedBSD upstream GTK4移植、[WS116](../ws116/ws.md) GTK4学習後のQt6範囲判断/移植を計画。WS034 p029/p030は未実行のまま移管。WS096/097完全書き下ろしは保持し、upstream知見を後で渡す。GTK4 guest実測/機能採否/Qt6範囲は未了。q576 finished/Queueなし、source変更なし。Issue/Project publicationとpushは未実施。

## 運用設計追記 / 2026-10-02（サブエージェント別Queue）

Event subagent-queues-projections-20261002: userの希望N=8/GPT-6.1 Sol High、mainのQueue配布/merge、agent別QueueとWS affinity、commit可能地点での頻繁なmerge要求、積んだ次Queueによる継続、通常/urgentラップアップを[運用契約](../agents/protocol.md)に記録。[台帳](../agents/registry.md)は実装担当なし。現runtime上限で子は同時最大3。固定版Awesome Planの単一executor/Queue既定にはzedBSD固有の最新指示を適用。3人の短時間読取専用調査を使い、product source/Queueは変更なし。q576 finished、GitHub publicationとpushは未実施。

## 計画追記 / 2026-10-02（WS074ブラウザ専任）

Event ws074-dedicated-interop2025-20261002: userが前回3人案のP10を[WS074](../ws074/ws.md)専任に固定。p172 branch統合後の[p100](../ws074/phase100/phase.md) Acid3 100/100・pixel完全一致・fail 0、[p173](../ws074/phase173/phase.md)でInterop 2025公式focus area対象WPTの固定版/baselineを定めて全件PASSへ進む。p101 CSS2全件目標とfg010/全体demo順位は保持。P10はreserved/未起動、q576 finished、実装/Queue/merge/pushなし。Issue/Project publication保留。

## 計画追記 / 2026-10-02（3専任枠とブラウザ次目標）

Event three-dedicated-lanes-and-browser-goals-20261002: userが[P8バグ修正、P9デスクトップ高度化、P10ブラウザ](../agents/registry.md)を固定。P9の発見したbugはmainが[Bug Board](../known-bugs.md)へ登録しP8へ渡す。[desktop作業一覧](../agents/desktop-outlook.md)を作成。P10の新目標は[p174 File System Access](../ws074/phase174/phase.md)→[p175 OPFS](../ws074/phase175/phase.md)→[p173 Interop 2025 100%](../ws074/phase173/phase.md)→[p176 Test262](../ws074/phase176/phase.md)。既存p172/p100 gateとp101 CSS2は保持、Test262最終率は未指定。3枠ともreserved/未起動、q576 finished、実装/Queue/merge/pushなし。Issue/Project publication保留。
