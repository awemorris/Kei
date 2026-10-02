<!-- awesome-plan project=zedbsd record=ws117-p001 -->

# ws117-p001: Linux Keiland 上の標準 Qt6 の実測と機能表

Parent: [WS117](../ws.md)
Status: planned
Disposition: normal
Primary Milestone: MG006（WSから継承）
Queue / attempts: none
Purpose / goal: Debian13 の distro 版 Qt6（qt6-base・qt6-wayland、6.8 系）の素のアプリを Linux Keiland で動かし、WS114 の機能表と同じ形で動作・不足を表にする。
Investigation bound: timebox 3〜4h / 1 Queue。表の全行に証拠か具体的な skip 理由を残せば clear。不足の修正はしない（p003 以降）。
Prerequisites: WS114 p007 cleared（2026-10-02 user の順序「GTK4のCSDの動作を完成させ…そのあと…Linuxで本物のQt6の動作を調査」）。WS105 の guest 道具。

## 範囲

- 代表 app を 3 つ固定する（候補、実行時に決めて理由を残す）: (a) Qt Widgets の小さな試験 app（menu・dialog・QLineEdit・QTextEdit・QComboBox の popup・tooltip・drag。qt6-base-dev で guest 内 build、source は `plan/ws117/tests/`）、(b) `qml6`（Qt Quick、RHI の OpenGL/Vulkan/software を切替えて各 1 回）、(c) KF6 の小さな app 1 つ（例: kcalc。distro の package）。
- 起動条件: `QT_QPA_PLATFORM=wayland`、`XDG_RUNTIME_DIR`/`WAYLAND_DISPLAY`。`WAYLAND_DEBUG=1` の protocol trace、`QT_LOGGING_RULES` で qt.qpa.wayland の log。
- 表の行（Q01〜、WS114 の G 行と対応を付ける）: 接続と global bind、xdg-shell configure/ack、popup/positioner（menu・combo・tooltip）、move/resize/maximize/fullscreen/min-max、**装飾（Qt は xdg-decoration があれば SSD を要求、無ければ bradient の CSD。p007 の新しい mode で二重装飾が無いか）**、wl_shm と EGL の buffer、renderer（Widgets raster・Quick の RHI）、pointer/keyboard/cursor-shape/scroll、clipboard と primary selection と D&D、text-input-v3（Qt 6.7+）と IME、output/scale/fractional scale、xdg-activation、Qt 独自の extension（qt-shell・qt-windowmanager 等、bind の有無だけ）、portal（FileDialog の通常経路と portal の有無）、theme/font/a11y。
- 版・ELF・package の一覧を記録する。

## 受け入れ

1. 代表 3 app の版と起動条件、全行の実測（PNG・protocol trace・操作の結果）か具体的な skip 理由が `plan/ws117/qt6-compat-matrix.md`（本 Phase で作成）にある。ソース調査と guest 実測を分け、未検証を合格としない。
2. 起動を妨げる不足（blocking）と品質の不足と任意の機能を区別し、p002 で使う「対応提案」の欄を埋める。
3. guest・compositor を正常停止し、停止の証拠を保存する。

## 検証

guest の QMP PNG と WAYLAND_DEBUG trace。host の画面・入力 device は使わない。compositor は main の現行 source を P 担当の worktree で build（`make -f userland/desktop/keiland-linux.mk`）、SHA を記録。

## 所有 path

`plan/ws117/`（phase・tests・evidence・qt6-compat-matrix.md）、自分の worktree の `build/`。製品 source は変更しない。

## 未決の判断

- WS114 p007 の clear を待たずに並行して始めてよいか（調査だけで source を変えないため、並行でも衝突は無い）。user の順序を守るなら待つ。

## Standards / resource

[Guardrail](../../guardrail.md)、[C全文規約](../../coding-style.md)、[automation](../../standards/automation.md)、[WS114–117 方針](../../standards/ws114-gtk-qt-learning.md) を変更前に実内容で読む。新しい C は全文規約。Linux guest は loopback SSH と QMP PNG（AGENTS.md の WS105/WS108 の例外）、serial/console log を判定に使わない。zedBSD の起動は `plan/tools/boot-test.sh` だけ。共有 toolchain/sysroot と main の `build/` は読取専用。QEMU と SSH port は main が割り当てる。全体の `make check` は禁止。具体コマンド/版/結果/commit/環境/成果物/skip/制限は実行時に記録する（今は計画のみ）。

2026-10-02 / ws117-beta1-plan-20261002: 計画担当が作成。planned、Queue 未投入。
