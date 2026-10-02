<!-- awesome-plan project=zedbsd record=ws117 -->

# WS117: Linux の本物の Qt6 の動作を調査し、素の Qt6 アプリが動くように compositor を改良する

<!-- awesome-plan-current:start -->
Status: planned
Primary Milestone: MG006
Related Milestones: MG002（WS115/WS116 の移植の前提）
Objectives: O2
Parent: [Master](../master.md)
Queue: none
Resume point（2026-10-02 計画詳細化）: [p001](phase001/phase.md)（Debian13 の標準 Qt6 6.8 の実測と機能表 Q01〜）が planned。開始条件は WS114 p007 cleared（user の順序）。p001 の後に p002 で行別採否をユーザーと決め、p003/p004 で compositor を改良する。
順序（2026-10-02 user（作業開始の指示））: WS115（GTK4 移植）と WS097（独自 GTK4）の後に着手する。
<!-- awesome-plan-current:end -->

## 目標（2026-10-02 ユーザー）

「本物のGTK4のzedBSDへの移植に進む前に、Linuxで本物のQt6の動作を調査しましょう。ここで、素のQt6アプリを動かせるように、コンポジタの改良を行います。そのあとで、素のGTK4と素のQt6を移植します。」

- Linux の Keiland（WS105）の上で、distro の標準 build の Qt6（Wayland QPA）の素のアプリを実測し、WS114 の GTK4 機能表と同じ形で動作/不足を表にする。
- 素の Qt6 アプリが動くために要る compositor（Keiland）の改良を行う。改良の範囲は機能表を見てユーザーと決める。
- 境界は [WS114–117 方針](../standards/ws114-gtk-qt-learning.md)、compositor の GPU/OS 境界は [Guardrail](../guardrail.md)。

## 完了の条件（WS acceptance）

1. 素の Qt6 アプリの代表（p001 で固定。候補は Qt Widgets の小さな app、`qml6` の Qt Quick、KF6 app 1 つ）について、Linux Keiland での実測の機能表（ソース実装と guest 実測を分ける、[機能表](qt6-compat-matrix.md)、p001 で作成）。
2. ユーザーが採用した行の compositor の改良を実装し、素の Qt6 アプリが Linux Keiland で起動・操作できる。zedBSD の compositor の振る舞いを壊さない（zedBSD target build warning 0 と `boot-test.sh`）。
3. 変えた source の全文規約・build・回帰。WS115（GTK4 移植）・WS116（Qt6 移植）へ知見を渡す。

## ベータ1（fg019、2026-10-17）までの到達目標（2026-10-02 計画、ユーザー確認待ち）

| # | 受け入れ（測れる形） | Phase |
| --- | --- | --- |
| Q-B1 | Debian13 の qt6-base/qt6-wayland（distro 版）で、代表 3 app の起動・window/menu/dialog・resize・入力・clipboard・装飾（SSD か CSD か）を全行に証拠か具体的な skip 理由付きで記録 | p001 |
| Q-B2 | 行別採否がユーザーにより記録される（WS114 p002 と同じ席を推奨） | p002 |
| Q-B3 | 採用された行（見込み: xdg-decoration で Qt が SSD を要求した時の Keiland 装飾、text-input-v3、xdg-activation、primary selection、cursor-shape 等）で、代表 app が Qt の既定の環境変数のまま（`QT_QPA_PLATFORM=wayland` 以外の回避策無し）起動・操作できる | p003・p004 |
| Q-B4 | 引継ぎ表、全文規約、Linux build と zedBSD target build の回帰 | p005・p006 |

ベータ1の最低線は Q-B1・Q-B2 と、採用行のうち起動を妨げる不足の修正（p003）。p004 は採否次第で取消。

## Dependencies / standards

WS114 p007 cleared（user の順序、同じ decoration/seat/toplevel の source を使う）。WS105 の Linux compositor と guest 道具（[plan/tools/keiland-linux](../tools/keiland-linux/README.md)）は context。WS114 の機能表と同じ形式。[Guardrail](../guardrail.md)、[C全文規約](../coding-style.md)、[automation](../standards/automation.md)、[WS114–117 方針](../standards/ws114-gtk-qt-learning.md)。Qt6 を guest に導入する行為と code の変更は Queue の承認後。Linux distro の Qt6 の成功は zedBSD 上の動作の証拠にしない。

**source の衝突**: 改良の対象は `userland/desktop/wayland/` の protocol.c・toplevel.c・seat.c・popup.c・decoration.c・text-input.c・data.c 等で、WS114 p003・WS095 p005（compose.c・protocol.c・display.c・shell.c・input-method.c）・WS099・WS094・WS113・WS127 の compositor の Queue と同じ file を触る。main が同時実行を避けるか merge 順を決める。

## Phases

| ID | 目的 | Status | 依存 | 目安 |
| --- | --- | --- | --- | --- |
| [ws117-p001](phase001/phase.md) | Qt6 の代表 app の選定、guest 実測、機能表 Q01〜 | planned | WS114 p007 cleared（順序） | 3〜4h |
| [ws117-p002](phase002/phase.md) | 行別採否をユーザーと確定 | planning（p001 の表待ち） | p001 | user 30分 |
| [ws117-p003](phase003/phase.md) | 採用行の compositor 改良 その1（起動・window・装飾・入力の不足） | planning（採否待ち） | p002、WS114 p003 と直列 | 3〜4h |
| [ws117-p004](phase004/phase.md) | 採用行の compositor 改良 その2（任意の protocol） | planning（採否待ち、不要なら取消） | p002、p003 | 3〜4h |
| [ws117-p005](phase005/phase.md) | 素の Qt6 の再検証と WS115/WS116 への引継ぎ | planning | p003/p004 | 2〜3h |
| [ws117-p006](phase006/phase.md) | 全変更の全文規約と最終回帰 | planning | p005 | 2〜3h |

Graph: WS114 p007 → p001 → p002 → p003 → p004 → p005 → p006 → {WS115 p002 以降の runtime 前提, WS116 p001}。p003 と WS114 p003 は同じ file を触るので直列。

## Event

2026-10-02 / ws117-create: user の順序の更新で新設（main）。

2026-10-02 / ws117-beta1-plan-20261002: 計画担当が p001〜p006 の Phase file を作成し、ベータ1の到達目標と受け入れを記録。p001 を planned（開始条件 WS114 p007 cleared）、他は採否待ちの planning。実装・guest 実行なし。Queue/Master 投影は main。
