<!-- awesome-plan project=zedbsd record=ws114 -->

# WS114: Linux Keiland 上の標準 GTK4 互換性を調査・改善する

<!-- awesome-plan-current:start -->
Status: incomplete
Primary Milestone: MG006
Related Milestones: MG002（後続の移植知見）
Parent: [Master](../master.md)
Queue: none（q592-i01 cleared（P3）/ q587-i01 uncleared / q581-i01 cleared / q580-i01 unclearedの履歴を保持。共有Queue投影はmain担当）
Resume point（2026-10-02 q592）: **p007 cleared**（[q592 結果](phase007/q592-result.md)）。CSD の完成（B1/B2）は QEMU と Linux guest で達成。並行して p002 の行別採否をユーザーに提示する（[推奨案](phase002/phase.md)、WS117 p002 と同じ席で決めると速い）。p007 cleared が WS117 p001 の開始条件（2026-10-02 user の順序）。p003〜p006 は p002 の採否待ち。
2026-10-02 user:「portalはなしにしましょう。D-BusがないとGTK4が動かないということはないはずです。WindowsでもMacでも動きますよね。D-Busも実装しません。」 → portal（ws114-p004 など）は取り消し、D-Bus は実装しない。
2026-10-03 user:「org_kde_kwin_server_decorationに対応しましょう。どちらのプロトコルも使わず、自分で飾りを描くアプリは、無視します。」「…全画面が多いと思います。」→ 新 [p008](phase008/phase.md)（宣言の無い窓を既定で SSD、CSD の宣言で CSD、全画面は SSD 無し）。P2 の compositor の線で p023 の後に投入。
<!-- awesome-plan-current:end -->

## ベータ1（fg019、2026-10-17）までの到達目標（2026-10-02 計画、ユーザー確認待ち）

2026-10-02 user「GTK4は、Linuxでの本物のGTK4によるCSDの動作を完成させましょう。」を最優先の到達点にする。

| # | 受け入れ（測れる形） | Phase |
| --- | --- | --- |
| B1 | 最終 source の Linux compositor を guest に導入し SHA を照合。Debian13 標準 GTK4 4.18.6 の GL/Cairo/Vulkan 各 renderer で、SSD 二重表示 0、CSD の move/resize/maximize/fullscreen/restore、menu/tooltip/modal、別 client の空 entry への Unicode paste の完全一致が PNG/protocol 証拠で通る | p007 |
| B2 | native Terminal/Files/Textedit の SSD（tabs・controls・max/dock/restore/close）に回帰無し。最終 source の zedBSD target build warning 0 と `boot-test.sh` の PNG | p007 |
| B3 | G01〜G19（G05 以外）の採否がユーザーにより記録される | p002 |
| B4 | 採用された基礎行（推奨: G04 restore 時の buffer 増加、G08 grab 後の wheel/hover、G10 の Kei IME 日本語入力）が標準 GTK4 で再現しなくなる | p003 |
| B5 | WS115/WS117/WS097 への引継ぎ表（protocol・buffer・入力・依存/サービス・回避策）と全変更の全文規約・最終回帰 | p005・p006 |

ベータ1の最低線は B1・B2（CSD の完成）。B3〜B5 はユーザーの採否と残り日数で決める（p004 portal は推奨: ベータ1 では取消または保留）。

## Objective / scope

LinuxのKeiland compositorでディストリビューションの標準ビルドGTK4を起動・操作し、必要なWayland/XDG-shell/desktop-portalとの連携を観測する。[機能表](gtk4-compat-matrix.md)の行ごとにユーザーが採用した範囲だけをcompositor/sessionに実装し、再検証する。先にGTK4と現実のcompositorの接点を学び、その知見をzedBSDのupstream移植 [WS115](../ws115/ws.md) と後の独自実装 [WS097](../ws097/ws.md)へ渡す。GNOME全機能の再現は目標ではない。

基礎検証: window/menu/dialog、resize、描画、keyboard/pointer、text clipboard。portalは実アプリの要求とユーザー判断で個別に選ぶ。portal frontend/backendはsession D-Bus側のサービスとして扱い、XDG-shell protocolと混同しない。既存backendの再利用も調査する。Linux compositor変更は選択結果が固まるまで設計しない。

## WS acceptance（レビュー後に確定）

1. 標準GTK4の版、guest、起動条件、各行の動作・失敗・未検証が再現可能な証拠で記録され、[機能表](gtk4-compat-matrix.md)の採否がユーザーにより確定する。
2. 採用されたXDG-shellとportal経路についてLinux Keilandで実操作が通り、代替/未採用の範囲を明記する。実装するportalの種類はレビュー後に確定する。
3. upstream GTK4移植と独自実装に引き渡すprotocol、buffer、入力、依存/サービス、失敗・回避策を文書化する。
4. 全コード変更の最終全文規約レビュー、対象Linux buildと標準GTK4 guest回帰を行い、未実施の確認を明記する。

## Dependencies / standards

WS105 Linux compositorはcontext、WS034 p038のzedBSD Vulkan/EGL横断調査は別目的。WS114 → WS115 → WS116の学習順。WS097/WS096は書き下ろしのまま後で行う。[Guardrail](../guardrail.md)、[C全文規約](../coding-style.md)、[automation](../standards/automation.md)、[WS114–116方針](../standards/ws114-gtk-qt-learning.md)。変更前に同じ資料と実ソースを再確認。GTK4をhost/guestに導入する行為とcode変更はQueue選定後。

## Phases

| ID | Purpose / goal | Status | Dependencies | 目安 |
| --- | --- | --- | --- | --- |
| [ws114-p001](phase001/phase.md) | 標準Linux GTK4 baselineと機能表の実測 | cleared（q581 / 残測定と全行skip理由） | WS105実行環境（context） | — |
| [ws114-p002](phase002/phase.md) | 行ごとの採否をユーザーと確定 | planned（ユーザーの判断の席。推奨案を記載） | p001の実測 | user 30分 |
| [ws114-p003](phase003/phase.md) | 選択されたXDG-shell/compositor修正 | planning（p002 の採否待ち） | p002の採用範囲、p007 cleared | 3〜4h |
| [ws114-p004](phase004/phase.md) | 選択されたportal/session統合 | planning（推奨: ベータ1では取消/保留） | p002の採用範囲。不要なら取消判断を記録 | 0〜4h |
| [ws114-p005](phase005/phase.md) | 標準GTK4の再検証と知見の引継ぎ | planning | p003/p004/p007の採用出力 | 2〜3h |
| [ws114-p006](phase006/phase.md) | 全変更の全文規約と最終回帰 | planning | p005実測/最終source | 2〜3h |
| [ws114-p007](phase007/phase.md) | G05 CSD/明示SSDの装飾モードとGTK4確認 | cleared（q592 / P3。q587 は uncleared の履歴） | p001実測・G05ユーザー指示 | 3h |

Graph: WS105 context → p001 → p002 → {p003,p004} → p005 → p006 → WS115。追加: p001 + G05ユーザー指示 → p007 → {p003, p005, WS117 p001}。選択しない行のPhaseは現状のまま自動clearせず、採否に応じ取消と依存/WS受け入れを改訂する。実装は新Queueの承認が必要。

## Event

2026-10-02 / ws114-gtk-qt-port-plan-20261002: userのLinux標準GTK4先行、XDG-shell/portal機能表のレビュー後に選択実装、zedBSD移植と書き下ろしへの学習順を記録。現在はsource調査だけ、guest標準GTK4未実行。GitHub publicationはlocal outbox pending。

2026-10-02 / desktop-next-gtk4: user指定でP9の次作業をWS114 p001→行別採否レビューに変更。[q580](../agents/P9/queue.md)は3時間のbaseline実測のみ。GTK4移植そのものはWS114選択改善→WS115、Qt6はその後。採否前の機能実装/portal backend追加は未許可。

2026-10-02 / q580-wrap-uncleared: GTK4 4.18.6のwindow/menu/dialog、同一client clipboard、FileDialog、maximize/fullscreen等を実測し、全19行に証拠またはskip理由を保存。未測定項目が残るためp001/q580はuncleared。専用QEMU/SSHを停止しoverlayを保全。B1がp001継続候補を所有するが、採否p002やsource修正は開始しない。

2026-10-02 / q581-baseline-cleared: p001の残主要操作を測定、全19行の再現証拠/具体skipを保存し調査基準clear。WS acceptanceの採否/改善/引継ぎ/全文最終回帰は未達、WS114はincomplete。q580 uncleared履歴は保持。[q581結果](phase001/q581-result.md)。追加CSD指示のsource実装は新Phase/Queue投入後。

2026-10-02 / ws114-csd-user-selection-20261002: userがG05の装飾モード実装・GTK4確認を個別採用。[p007](phase007/phase.md)を追加しq587へ投入。p002は他行の判断を継続、p003はG05を重複実装せず、p005はp007出力を再検証/引継ぎに含める。WS acceptanceとp006最終conformanceは未達。

2026-10-02 / A-B-checkpoint-start-projection: B df66db5eをA435a62126へ統合しp001/q581調査clearを投影。G05個別採用のp007/q587はuser開始報告でin-progress、他機能採否p002とWS全体受け入れは未達のまま。

2026-10-02 / q587-user-wrap-uncleared: p007のCSD/明示SSDとaccepted xdg move/resize matching releaseを実装し、wire・GTK4 GL/Cairo/Vulkan・native Terminal/Filesの部分証拠を保存。userの全agent終了指示で正常停止し、最終source runtime導入、別client clipboard空target一致、Textedit controls、最終zedBSD install/boot PNG等を未達としてuncleared。p005の引継ぎに利用可能な部分成果でありp007出力のclearanceを供給しない。WS114 incomplete、p002他行採否/p006最終conformanceを維持。[結果](phase007/q587-result.md) / [再開](phase007/q587-resume.md)。mainがQueue/共有記録とremote統合を引継ぐ。

2026-10-02 / ws114-beta1-plan-20261002: 計画担当が fg019（ベータ1）向けに詳細化。最低線を p007 の CSD 完成（B1/B2）とし、p007 の新 attempt を残り 5 点に限定して Queue 投入可と記録。p002 に行別の推奨案（提案であり採否ではない）を追記し planned に、p003 は p007 cleared も依存に追加、p004 は portal をベータ1で取消/保留する推奨を記録。p007 cleared を WS117 p001 の開始条件として投影。実装・guest 実行なし。Queue/Master/WS117 投影は main。

2026-10-02 / q592-p007-cleared: P3 が q592-i01 で p007 の残り 5 点を最終 source で確かめた。Linux guest への導入と SHA 照合、GTK4 の GL/Cairo/Vulkan、空 entry への別 client の paste の完全一致、Textedit の全 control、SSD の phantom release 0、Terminal/Files、OS/GPU 境界、zedBSD target build の warning 0、boot-test の PNG。製品 source の変更なし。p007 cleared。B1/B2 は達成。WS acceptance（p002 の採否、p003〜p006）は未達のまま。Queue・Master・WS117 への投影は Q1。

2026-10-03 / p008: Q1 判定で cleared（P2 の提案を受け入れ。Linux GTK4 の CSD・二重無し、zedBSD の C 基準 15/15、boot-test）。
