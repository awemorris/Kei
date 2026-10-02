<!-- awesome-plan project=zedbsd record=past-log -->

# Past Log

最新統合 / 2026-10-02: リモートB0018bc6c8をmain35a6c8634へ統合。全6担当終了、active Queueなし。[q587](queue-q587.md)は実装/部分検証uncleared、[q588](queue-q588.md)はsource/host/build部分cleared・whole未達、[q589](queue-q589.md)は準備のみuncleared。過去の待機/停止未確認の記述は当時の履歴。最終runtime/boot等を未実施のまま保存、GitHub計画publication pending。


最新: [q584](queue-q584.md) / 2026-10-02 07:12–08:53 UTC。通常終了でuncleared、browser全文97/209・残112件。A1最終成果10e844f23へ統合、209hashes確認。A1/A2/A3すべて終了、未達条件と再開証拠を保存。B停止は最終報告未確認。GitHub publication保留。


2026-10-02 / current execution: userの全agent区切り終了指示でA1/A2/A3を回収・終了済み。q581/q582/q583部分cleared、q585/q586契約調査uncleared。B q587/q588/q589の停止は未確認。[Queue](../queue.md)、[registry](../agents/registry.md)を参照。B lane記録991fc890をA c87341a78へ統合。旧wrap-upのuncleared履歴とq578 clearedは保持。

Last finished Queue: [q584](queue-q584.md)（browser残全文review / uncleared）

## 最新: q586と全agent wrap-up（2026-10-02）

[Archive](queue-q586.md)に承認/90min結果/契約・source能力20行/技術採択・次候補・未実施を保存。D-ATOMIC未決でp001 uncleared、全後続planned。A3成果2ad9c951 → root83b111c63統合・clean/process無し・終了確認。userが全担当を安全なcommit地点で切り上げるよう指示しA1/A2回収中、新Queue無し。

B最新4b655803 → A8f807c73fから[q582 cleared](queue-q582.md)、q588/p007 source partial開始、q589/低overhead準備・guest資源待ち、q587/入力release amendment01を投影。元scope/timebox/criteriaとsnapshot hashを確認。q593/B2背景資産・3 OS収録は予約のみ。B停止receipt未受領、GitHub publication pending、WIP/no push。


## 最新: q585 / native package契約調査（2026-10-02）

[Archive](queue-q585.md)に承認lane/Phase snapshotと結果を保存。5OS入力・metadata署名・native環境・形式/依存・共通payload/source/CI成果物と後続commandを具体化。D2 RPi方式は委任技術判断で採用、Fedora/Arch boot適用D1 user回答待ちでq585-i01/p001 uncleared。実image/guest/build/package/runtime/CI/remoteは未実施。same-session待機、q591/p002は候補のみ。

q581/p001 GTK調査clearとq583部分診断clearをB df66db5e→A435a62126から投影し、q587/p007装飾を開始。whole p017/BUG-125は未解決、q588 B2最終規約・q589 B3追加切り分けを予約。A1 checkpoint11 reviewed88/209、A3契約を統合・ACK、後続gate保持。全WIP、pushなし、GitHub publication pending。


## 最新: q577/q579/q580 wrap-up（2026-10-02）

ユーザーの2セッション移行前の停止指示によりP8/P9/P10を通常wrap-upした。[q577](queue-q577.md) BUG-125、[q579](queue-q579.md) browser2統合、[q580](queue-q580.md) GTK4 baselineはいずれも部分成果・未達基準・再開条件を保存してuncleared。q578 clearedは維持。全worker成果をmainへ統合し、owned runtimeを停止した。A/B分担は新Queueを自動承認しない。

## q578 / C10 hardware cleared（2026-10-02）

[Archive](queue-q578.md)、[結果](../ws099/phase014/q578-result.md)。i915 passthroughで実3602秒/278周/errors0/restarts0、final live Terminal/disk receipt/cleanup確認。WS099はincomplete、USB素実機は範囲外。補助framebuffer boot timeoutは別記。P9利用上限終了後mainが判定。q577/q579はmain継続中、3children停止後、ユーザー利用上限回復確認でgeneration2再起動/N_effective3、全WIP/pushなし/GitHub公開保留。

## 2026-10-02 並列実行開始（結果は未確定）

P8 q577、P9 q578、P10 q579をlatest userのN=3継続指示で開始。[Queue index](../queue.md)。既存q576結果は保持。merge/終了証拠は各laneへ追記。GitHub publication保留。

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
| [q578](queue-q578.md) | WS099 p014 cleared / C10 passthrough60分 |
| [q577](queue-q577.md) | WS099 p017 uncleared / BUG-125有限診断 |
| [q579](queue-q579.md) | WS074 p172 uncleared / browser2統合と最終review残 |
| [q580](queue-q580.md) | WS114 p001 uncleared / GTK4部分実測 |

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

2026-10-02 / initial-A-checkpoints: q584 checkpoint06/main c2743455c、q585 survey/main 5acb47a9c、q586 source-audit/main 8021bc210をレビュー統合・各ACK。全3Queueは実行中、clearanceなし。B next q587をuser依頼で予約し、q581〜q583の現行scopeは保持。

2026-10-02 / B-checkpoint-df66db5e: [q581](queue-q581.md)調査cleared、[q583](queue-q583.md)部分診断cleared/whole p017 unclearedをA435a62126へ統合。q587/p007装飾の開始を投影。B次ID q588/q589予約、same-session連続投入のuser指示をprotocolへ保存。GitHub publication保留、pushなし。
