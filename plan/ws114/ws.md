<!-- awesome-plan project=zedbsd record=ws114 -->

# WS114: Linux Keiland 上の標準 GTK4 互換性を調査・改善する

Status: incomplete
Primary Milestone: MG006
Related Milestones: MG002（後続の移植知見）
Parent: [Master](../master.md)
Queue: なし（last: q580-i01 uncleared）
Resume point: q580の部分実測と[19行機能表](gtk4-compat-matrix.md)を保存。未測定のmove/resize、cross-client clipboard、入力、D&D/PRIMARY、renderer/scale/IME等を有限なp001継続で測り、p001をclearしてからユーザーの行別採否へ進む。

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

| ID | Purpose / goal | Status | Dependencies |
| --- | --- | --- | --- |
| [ws114-p001](phase001/phase.md) | 標準Linux GTK4 baselineと機能表の実測 | uncleared（q580 finished / 部分実測） | WS105実行環境（context） |
| [ws114-p002](phase002/phase.md) | 行ごとの採否をユーザーと確定 | planning | p001の実測 |
| [ws114-p003](phase003/phase.md) | 選択されたXDG-shell/compositor修正 | planning | p002の採用範囲 |
| [ws114-p004](phase004/phase.md) | 選択されたportal/session統合 | planning | p002の採用範囲。不要なら取消判断を記録 |
| [ws114-p005](phase005/phase.md) | 標準GTK4の再検証と知見の引継ぎ | planning | p003/p004の採用出力 |
| [ws114-p006](phase006/phase.md) | 全変更の全文規約と最終回帰 | planning | p005実測/最終source |

Graph: WS105 context → p001 → p002 → {p003,p004} → p005 → p006 → WS115。選択しない行のPhaseは現状のまま自動clearせず、採否に応じ取消と依存/WS受け入れを改訂する。実装は新Queueの承認が必要。

## Event

2026-10-02 / ws114-gtk-qt-port-plan-20261002: userのLinux標準GTK4先行、XDG-shell/portal機能表のレビュー後に選択実装、zedBSD移植と書き下ろしへの学習順を記録。現在はsource調査だけ、guest標準GTK4未実行。GitHub publicationはlocal outbox pending。

2026-10-02 / desktop-next-gtk4: user指定でP9の次作業をWS114 p001→行別採否レビューに変更。[q580](../agents/P9/queue.md)は3時間のbaseline実測のみ。GTK4移植そのものはWS114選択改善→WS115、Qt6はその後。採否前の機能実装/portal backend追加は未許可。

2026-10-02 / q580-wrap-uncleared: GTK4 4.18.6のwindow/menu/dialog、同一client clipboard、FileDialog、maximize/fullscreen等を実測し、全19行に証拠またはskip理由を保存。未測定項目が残るためp001/q580はuncleared。専用QEMU/SSHを停止しoverlayを保全。B1がp001継続候補を所有するが、採否p002やsource修正は開始しない。
