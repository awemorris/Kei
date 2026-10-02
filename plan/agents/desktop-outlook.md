# Keilandデスクトップ高度化の作業一覧（2026-10-02）

この一覧は[Master](../master.md)と各WSの再開点をP9専任枠向けに投影したOutlook。順番は既存のdemo critical順位と実行可能な前提を考慮した候補で、Queue承認やWS優先順位の変更ではない。P9は機能/計測/規約の高度化を担当する。発見したバグはmainが[Bug Board](../known-bugs.md)へ照合・記録し、[P8](registry.md)へ回す。同じsource/実機を使う時はmainが編集/試験を直列化する。

| 対象 | 次の作業と状態 | 前提・担当境界 |
| --- | --- | --- |
| [WS099](../ws099/ws.md) デモのcompositor | [p014](../ws099/phase014/phase.md)の5330 i915 passthroughでC10を60分試験するscriptと結果。p012はUSB素起動/login/logout/shutdownのユーザー目視。最後に提案p018の全文規約と最終回帰 | p014はi915 lockを約80分占有、mainが日程調整。p012はユーザーの15分が必要。C6性能の最適化は優先を下げたまま。BUG-125などのwindow defectはP8。既に他WSで解決した電源断p013を再実装しない |
| [WS079](../ws079/ws.md) Notes/PDF Viewer | p016でQEMUのS8/S9と頁送り≤200msはcleared。5330のmouseとWindows QEMUのtouchの台本確認、後の実ペンL3 | 目視/実操作はユーザー。P9はimage/手順/ログを整える。結果がbugならP8へ |
| [WS090](../ws090/ws.md) libkeiui | 提案p016でTerminal drop p088の試験前提を切り分け、[p015](../ws090/phase015/phase.md)でTerminal/Notesのtouch scrollを共通部品へ。後に[p007](../ws090/phase007/phase.md)のSettings移行 | p015は既存の操作/画面を維持し10/10までに完了見込みの時だけ。p007はWS089 completion後。p016で本物の退行が見つかればticket化してP8へ。Files移行p009/p010はWS094 completionとユーザーの延期判断待ち |
| [WS089](../ws089/ws.md) Settings | p001〜p009/QEMU受け入れはclear。WS完了処理、共有試験の配置後にWS090 p007を解除。見た目のpolishは後ろ | 完了判定/共有planとtestsの移動はmainが調整。新たなbugはP8 |
| [WS094](../ws094/ws.md) desktop icons | [p011](../ws094/phase011/phase.md)のoverflow数と古い配置行の整理、[p012](../ws094/phase012/phase.md)の実機性能/操作、[p007](../ws094/phase007/phase.md)の最終規約/回帰 | p010はphase recordでclearedだが古い一覧/履歴に不一致があるためmainが証拠照合。overflowの表示方式はユーザーの「今のまま」を維持。p012は実機/ユーザーの時間が必要 |
| [WS100](../ws100/ws.md) 音量 | A1〜A6はclear。p006/A7の5330 speaker/headphoneはユーザーが耳で確認。音量曲線等のL3は結果を見て判断 | A7はユーザー判断でデモ必須ではない。実機で失敗したら既存/新bugをP8へ |
| [WS078](../ws078/ws.md) 名前の移行 | 残る注釈・logの名称とp005 | 多数の現役sourceと重なるため、他担当の大きな編集が落ち着いてからmainが範囲を切る |
| [WS102](../ws102/ws.md) keyboard / [WS081](../ws081/ws.md) touch | WS102の残りp022、速度p010/p011、IME連携p012。WS081はWindows QEMUでのtouch実測後にL3補間 | WS102はユーザーが優先を下げた。WS095 IMEは人間担当。touchの実測はユーザー入力が前提。bugはP8 |
| [WS110](../ws110/ws.md) 起動mode | 引数なしを通常session、`--testing`を試験modeにする案と関連scriptの整理 | ユーザーは「検討だけまずは」と指定した段階。実装Queue未承認。`--login`も本人確認/PIN交換案のみ |
| [WS113](../ws113/ws.md) 複数display | p001のi915/Vulkan Display/Settings契約→driver/標準Vulkan→compositor/Settings→窓の一括画面間移動→実i915検証 | ユーザーの「あとで実装」の計画。最初の完了対象はzedBSD i915、9Phase planned。Linux/FreeBSDの複数画面は今回の受け入れ外 |
| [WS114](../ws114/ws.md) GTK4互換 | Linux標準GTK4をKeilandで実測し[機能表](../ws114/gtk4-compat-matrix.md)をユーザーへ行別レビュー。その後、採用されたXDG-shell/portal経路だけ改善 | p001の実測前。採否はユーザー。続く[WS115](../ws115/ws.md) upstream GTK4移植→[WS116](../ws116/ws.md) Qt6範囲判断/移植→WS097/096独自実装の順。P9がWS114のcompositor部分を担う候補 |

最初のP9 Queue候補はWS099 p014の試験script/短時間試走/60分実機試験。i915設備が取れない場合は、別sourceで進められるWS090 p015かWS094 p011をmainが選ぶ。いずれもPhaseのscope/承認と回帰資源を確認してから投入する。P8の最初のbug候補は[BUG-125](../bugs/BUG-125.md)で、P9のWS099 sourceやC9 harnessと重なる時はmainが調整する。
