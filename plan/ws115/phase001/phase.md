<!-- awesome-plan project=zedbsd record=ws115-p001 -->

# ws115-p001: GTK4 target移植契約

Parent: [WS115](../ws.md)
Status: cleared（q597-i01、P3、2026-10-02。[移植契約](../port-contract.md)。Queue と共有記録への投影は Q1）
Disposition: normal
Primary Milestone: MG002（WSから継承）
Queue / attempts: q597-i01（P3、cleared）
Purpose / goal: GTK4 target移植契約
Prerequisites: WS114受け入れ/WS034必要成果
Investigation bound: timebox 3〜4h / 1 Queue。調査と文書だけ（package の追加・build はしない。tarball の取得と展開・meson の dry 構成は可）。

## Procedure / affected components

GTK4 tarball版/hash/依存/OS API/patch場所、zedBSD上の代表app/renderer/操作/試験を選定し、ユーザーの採用済み機能に合わせる。

## Clearance / verification

実装するtarget・依存・有限の試験・license境界が明確である。

## Standards / evidence / resume

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[scoped方針](../../standards/ws114-gtk-qt-learning.md)、[automation](../../standards/automation.md)を実装前に確認する。新コードは全文規約、外部tarballはprovenance・license・patch分離。具体コマンド/版/結果/commit/環境/成果物/skip/制限は未実施（計画のみ）。各PhaseはQueueの個別scope承認後に実行する。前提未達・判断待ちならattemptをunclearedとして証拠/再開条件を残す。

Event ws114-gtk-qt-port-plan-20261002: 2026-10-02 user指示から作成。planning/Queue none。GitHub publication pending。

## 詳細化（2026-10-02 計画担当）

### 範囲

1. **版（D-VER）**: GTK 4.18 系（推奨、Debian13 の 4.18.6 と同系列）か 4.24.0（inventory）。各版の meson・glib・pango・cairo・harfbuzz・graphene・wayland-protocols・libepoxy の下限を `meson.build` から読み、依存の版を確定する。公式 tarball の URL・size・SHA-256・license を確認。
2. **依存の一覧**: 必須/任意を分け、任意は無効にする方針（introspection・media・print・cloudproviders・sysprof・tracker・colord・X11・broadway・vulkan の要否）を決める。glib の file monitor の backend（zedBSD に inotify/kqueue が無い）、gio の network（getifaddrs が無い）、memfd_create・eventfd の要否を確認し、libc の不足の一覧と「patch で避ける／libc に足す（main の判断）」を分ける。
3. **host 道具**: meson の版、gperf（fontconfig）、同じ版の native の glib 道具（glib-compile-resources・glib-compile-schemas・glib-mkenums・gdbus-codegen）、wayland-scanner（host の 1.23 で足りるか）、glslc（Vulkan renderer）、python3 の module。
4. **libwayland の ABI**: upstream wayland-scanner が生成する code と GTK4 が呼ぶ `wl_*` の関数・`wl_proxy_marshal_flags`・`wl_display_*`・`wl_event_queue`・`wayland-cursor`・`wayland-egl` を、zedBSD の `userland/desktop/libwayland` の export と照合して不足の一覧を作る（p009 の scope）。
5. **renderer（WS034 p038 を含む）**: build に libepoxy が要るか、runtime で EGL 無しの Cairo renderer が wl_shm で動くか、zedBSD の libegl/libglesv2（Vulkan の上）で GL renderer、libvulkan（Venus）で Vulkan renderer が使えそうかの判断材料。ベータ1は Cairo を最初の目標にする案。
6. **demo app と試験**: `gtk4-demo`・`gtk4-widget-factory`、または小さな試験 app（window・button・entry・menu・dialog）。zedBSD QEMU での起動の手順（`boot-test.sh` で起動確認、操作は serial/SSH、画面は QMP PNG）。
7. **package の置き場所と Phase の分け方の確認**: p004〜p009・p002・p010 の scope を確定し、必要なら分ける。

### 受け入れ

上の 1〜7 が `plan/ws115/port-contract.md` にあり、各 package の版・hash・license・build 系・既知の問題・patch の方針・libc の不足・host 道具・libwayland の不足・renderer の方針・demo app と試験が決まっている。ユーザーの判断が要る点（D-VER、libc に関数を足すか、renderer の目標）を列挙する。

### 所有 path

`plan/ws115/`、自分の worktree の `build/`（tarball の展開は ignored な build の下）。

### 未決の判断

- D-VER（GTK の版）。
- WS117 と並行して始めるか（調査だけなので source の衝突は無い）。

## q597-i01 の結果（2026-10-02、P3）

承認: 2026-10-02 user「まずは素のGTK4を移植してください。移植できないところがないか、ノウハウを蓄積します。そのあとで独自実装を作ります。Qt6はこれらの作業のあとにします。」。Q1 が q597 として P3 に投入した。上の Status の「WS117 の後」はこの決定で置き換えた（WS117 を待たずに着手する）。時限 4h、base は main 73e259378。

成果: [port-contract.md](../port-contract.md) に範囲の 1〜7 をまとめた。
- 推奨の版の組は GTK 4.18.6・glib 2.84.4・pango 1.56.4・fontconfig 2.17.1（host の meson 1.7）。4.20/4.22/4.24.1 の要件を比べた。
- libc に関数を足す必要は無い。header の `sys/poll.h` が 1 つ要る（glib の dry 構成で確認）。
- host 道具: gperf だけを source から build する。
- libwayland の不足: `wl_log_set_handler_client`、wl_surface v5/v6、output と pointer の名前、enum、pkg-config。
- renderer は Cairo → GL（epoxy の patch）→ Vulkan（header の判断）。
- app は gtk4-demo と gtk4-widget-factory。試験の手順を決めた。
- ユーザーの判断が要る点: D-VER・L1・L2・R1・P1・S1（契約の §8）。

実施したこと:
- tarball の取得と SHA-256 の照合。
- GTK・glib・依存の meson.build の読解。
- zedBSD の libc.so・libwayland-client.so・libEGL の export と SONAME の照合。
- glib 2.84.4 の target 向け meson の dry 構成（`build/p3-q597/probe/`）。

未実施: package の build、GTK の構成、runtime。package・libc・libwayland の source は変えていない。判断の結論が出るまで p004 以降の範囲は契約の推奨のまま。判断が推奨と違えば p004〜p010 の範囲を直す。
