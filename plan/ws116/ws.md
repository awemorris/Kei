<!-- awesome-plan project=zedbsd record=ws116 -->

# WS116: upstream Qt6 の移植範囲を検討し、zedBSD packageへ移植する

<!-- awesome-plan-current:start -->
Status: planning
Primary Milestone: MG002
Related Milestones: MG006（GUI動作、compositor互換性）
Parent: [Master](../master.md)
Queue: none / 計画のみ
Resume point（2026-10-02 計画詳細化）: p001 の入力を WS117（Linux の Qt6 の機能表と改良、2026-10-02 user の順序）に改訂し、host 道具（p004）と qtwayland・実行（p005）の Phase を追加。p001 は WS117 p002（Qt6 の行別採否）の後に、module と代表 app と到達点をユーザーと決める。ベータ1に入れるかはユーザーの判断（下の選択肢）。
<!-- awesome-plan-current:end -->

## Objective / scope

GTK4の先行移植を学んでから、Qt6 upstream のどのmodule・依存・Wayland QPA・機能をzedBSDへ移植するか決め、`userland/packages/desktop/qt6/` に公式tarball＋zedBSD patchを置いてbuild/実行する。結果を後のQt6完全書き下ろし [WS096](../ws096/ws.md)へ渡す。既存[WS034のinventory](../ws034/package-inventory.md)のqtbase/qtwaylandは候補であり、確定したscopeではない。

## Completion criteria（p001で具体化）

1. GTK4移植で判明した課題とQt6固有の要求を比較し、userがmodule/代表app/描画/Wayland/portal/受け入れ範囲を決める。
2. 決定したQt6 upstream packageの出典/hash/license/patch/依存を記録し、zedBSD targetでbuild/install、QEMU/Venusで代表appを起動/操作する。
3. Linux/GTK4と共通の対応、Qt6固有のOS API/QPA/Wayland/renderer条件を独自実装WS096へ引き渡す。全変更の全文規約、provenance、target build/実行を最終検証する。

## 現状と依存の見通し（2026-10-02 計画担当）

- zedBSD の packages に Qt の依存は無い（WS115 の現状の表を参照）。ただし qtbase は zlib・pcre2・freetype・harfbuzz・libpng・libjpeg・double-conversion・md4c を**同梱の版で build できる**（`-qt-zlib` 等の相当の CMake option）。fontconfig・ICU・glib は無効にできる（font は Qt の font database、`QT_QPA_FONTDIR`）。Widgets は raster＋wl_shm で EGL 無しに描ける。Qt Quick は RHI（OpenGL/Vulkan/software）。
- 必要なもの: C++17/20 の clang＋libc++（ある）、CMake の toolchain file（`external.mk` にある）、**host の同じ版の Qt の道具（moc・rcc・uic・qtwaylandscanner）**、libxkbcommon と libwayland-client（WS115 p008・p009 と共有）。
- GTK4 より依存が少なく、WS115 の p008（libxkbcommon）と p009（libwayland 互換）ができれば、それ以外は Qt の同梱で進められる見込み。

## ベータ1（fg019、2026-10-17）の選択肢（ユーザーが決める）

| 案 | ベータ1の到達線 | 条件 |
| --- | --- | --- |
| A（推奨） | WS117 の Linux 調査と改良まで。zedBSD への Qt6 の移植はベータ1の後（user の順序: GTK4 の移植の後に Qt6） | — |
| B | zedBSD で Qt Widgets の小さな app が raster＋wl_shm で window を出す（qtbase の Core/Gui/Widgets ＋ qtwayland、依存は同梱） | 2 担当目を 10/07 頃から。WS115 p008（libxkbcommon）・p009 の成果が先に要る。GTK4（WS115 案 B）と同時に追うと他の fg019 の WS の枠が減る |

## Dependencies / standards

[WS117](../ws117/ws.md) の Linux Qt6 の機能表と採否（p001 の入力、2026-10-02 user の順序）、[WS115](../ws115/ws.md) の移植の知見と共有の依存（p004 の CMake/host 道具の契約、p008 の libxkbcommon、p009 の libwayland 互換）。旧[WS034](../ws034/ws.md) p030は移管先をここにする。native menubarの後日案[F-045](../future-work.md)はp001で必要性をレビューする。userが検討する前にQt6 codeを実装しない。[Guardrail](../guardrail.md)、[C全文](../coding-style.md)、[方針](../standards/ws114-gtk-qt-learning.md)。toolchain は変更しない。

## Phases

| ID | Purpose / goal | Status | Dependencies | 目安 |
| --- | --- | --- | --- | --- |
| [ws116-p001](phase001/phase.md) | Qt6 upstream の module・版・代表 app・同梱依存・到達点のレビュー | planning（WS117 p002 待ち） | WS117 p002（Linux の採否）、WS115 p001（共通の契約） | 3h ＋ user |
| [ws116-p004](phase004/phase.md) | host の Qt の道具（moc・rcc・uic・qtwaylandscanner 等）の build | planning | p001 | 3〜4h |
| [ws116-p002](phase002/phase.md) | qtbase（決定した module）の cross build/install | planning | p001・p004、WS115 p008（libxkbcommon） | 4h |
| [ws116-p005](phase005/phase.md) | qtwayland の cross build と zedBSD QEMU での代表 app の起動・操作 | planning | p002、WS115 p009、WS117 p003 | 4h（＋debug） |
| [ws116-p003](phase003/phase.md) | 独自実装へ引継ぎ・最終全文規約/回帰 | planning | p005 | 3h |

Graph: {WS117 p002, WS115 p001} → p001 → p004 → p002 → p005 → p003 → WS096。WS115 p008 → p002、WS115 p009 + WS117 p003 → p005。p002 の scope とp003 の依存は 2026-10-02 に改訂。

## Event

2026-10-02 / ws114-gtk-qt-port-plan-20261002: GTK4先行移植後にQt6範囲を検討するuser方針を記録。WS096書き下ろしは保持。実装/試験なし、GitHub publication pending。

2026-10-02 / ws116-beta1-plan-20261002: 計画担当が 2026-10-02 user の順序の更新（WS117 で Linux の Qt6 を先に調査）に合わせ、p001 の入力を WS117 p002 に改訂。host 道具の p004、qtwayland と zedBSD 実行の p005 を追加し、p002 を qtbase の cross build に限定、p003 の依存を p005 に改訂（各 Phase に event）。ベータ1の選択肢 A（推奨）/B を記録。実装・build なし。
