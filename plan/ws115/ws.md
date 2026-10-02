<!-- awesome-plan project=zedbsd record=ws115 -->

# WS115: upstream GTK4 を zedBSD の desktop package に移植する

<!-- awesome-plan-current:start -->
Status: planning
Primary Milestone: MG002
Related Milestones: MG006（GUI動作、compositor互換性）
Parent: [Master](../master.md)
Queue: none（q597-i01 cleared（P3）。共有 Queue の投影は main）
Resume point（2026-10-02 q597）: p001 cleared。[移植契約](port-contract.md)の §8 の判断（D-VER・L1・R1 など）を待ちながら、次は p004（meson の cross 契約と host 道具）。以前の記述: 依存 package の移植を含む Phase 表を作成（p004〜p010 を追加）。2026-10-02 user の順序では WS114 p007 → WS117 の後に移植へ進む。**ベータ1に zedBSD 上の GTK4 を入れるなら、依存 library の Phase（p001・p004〜p009。compositor を触らない）を WS117 と並行して始める判断がユーザーに要る**（下の「ベータ1の選択肢」）。到達点は WS117 の調査の後にユーザーが決める（2026-10-02 user「まず調査して…から決めます」）。
順序（2026-10-02 user（作業開始の指示））: WS114 p007 の後、WS117 を待たずに着手する。ベータ1 で素の GTK4 の移植を進め、移植できない所を記録する。
2026-10-02 user の判断: D-VER = **GTK 4.18.6**（「GTK 4.18.6でOKです。あとで4.24.1以降にアップデートする旨を記録します。」→ [F-066](../future-work.md)）。L1 = **libc に `sys/poll.h` を足す**（「sys/poll.hを足してください。」、main が `include/libc/sys/poll.h` を追加）。L2（libc に関数を足さない）・R1（Cairo → GL → Vulkan）・P1（portal・D-Bus を入れない）は推奨のまま、ユーザーの異論待ち。
2026-10-02 user:「portalはなしにしましょう。D-BusがないとGTK4が動かないということはないはずです。WindowsでもMacでも動きますよね。D-Busも実装しません。」 → P1（portal・D-Bus）は**入れない**で確定。xdg-desktop-portal と D-Bus の daemon は移植・実装しない。ファイルダイアログは GTK 組み込みの GtkFileChooserDialog。GtkApplication の session bus が無いときの挙動（一意性・起動の warning）は p010 で確かめ、必要なら build option や環境で抑える。
2026-10-02 user:「Keilandネイティブのfile chooserを呼び出すlibkeilandの関数呼び出しを、GTK4に追加してもOKです。」→ 新 [p011](phase011/phase.md)（p010 の後）。portal 無しで、GTK のファイルダイアログを Keiland の共有の file chooser で出す。
2026-10-02 user:「GtkApplication が、session bus が無いと警告を出す件は、該当コードを無効化するパッチをお願いします。」→ p010 の範囲に追加（glib/GTK の patch）。
2026-10-02 user（Vulkan 1.3 の調査 [vulkan-1.3-survey](vulkan-1.3-survey.md) の後）:「うーん、仕方ないです、CairoはGLでいきます。」→ R1 = **Cairo → GL**（p008 の epoxy の SONAME の patch で GL を有効に）。Vulkan renderer は libvulkan の header・instance 1.1 と i915 の compiler の作業が要るので後回し（[F-067](../future-work.md)）。
2026-10-02 user:「libcへの変更をあなたに明示的に許可します。」（`plan/ws115/proposed/libc-libintl-format-arg.diff`・`libc-cmsg-nxthdr.diff` の適用。P3 の権限は permission system に止められたため、P3 を再起動して適用する）。
<!-- awesome-plan-current:end -->

## Objective / scope

外部のGTK4ソースを公式tarball＋zedBSD patchとして `userland/packages/desktop/gtk4/` に追加し、zedBSD上でWaylandのGTK4アプリをbuild/実行する。移植時の実際の依存・OS API・描画・入力・Wayland/portal条件を記録し、後の完全な書き下ろし [WS097](../ws097/ws.md) の設計材料にする。GTK4ソースをbase/compositorへ取り込まない。既存[WS034のinventory](../ws034/package-inventory.md)を再検証して使う。手前のゴール（2026-10-02 user）は `userland/packages/desktop/gtk4` の実装、奥のゴールは独自実装の互換 gtk4（WS097）。

## 現状（2026-10-02 調査、計画担当）

- zedBSD の `userland/packages/` にあるのは zlib・expat・openssl・openssh・curl・ca-certificates・clang・libc++・remacs・noto-color-emoji だけ。**GTK4 の依存（meson のクロス契約、libffi・pcre2・glib、libpng・freetype・harfbuzz・fontconfig、pixman・cairo・fribidi・pango・gdk-pixbuf・libjpeg-turbo・libtiff・graphene・libepoxy・libxkbcommon・xkeyboard-config、wayland-protocols）は全て未移植**。WS034 の p025〜p028・p034・p038 は planning のまま未実行（tarball の版・hash・license・既知のクロス build 問題は [inventory](../ws034/package-inventory.md) §2.4〜§2.7 に調査済み）。
- 使えるもの: CMake の toolchain file と autoconf の cross cache（`userland/packages/external.mk`・`tools/gen-cross-toolchain.sh`）。meson の cross file は無い。host の meson 1.7.0・ninja・cmake 3.31。gperf は host に無い。
- zedBSD 独自の `libwayland-client.so`（queue・`prepare_read`・wrapper を実装、[README](../../userland/desktop/libwayland/README.md)）、EGL 1.5 と GLES（Vulkan の上、`userland/desktop/libegl`・`libglesv2`）、`libwayland-egl` がある。upstream の wayland-scanner の生成 code（`wl_proxy_marshal_flags` 等）との ABI 互換は未確認。
- libc: iconv・libintl・locale（newlocale/uselocale）・dlopen・shm_open・posix_spawn・pipe2・mkostemp・posix_fallocate・qsort_r はある。memfd_create・getifaddrs・eventfd・accept4 は無い（glib/GTK が要るかは p001 で確認）。

## Completion criteria（p001で対象app/操作を確定）

1. tarball版・出典・hash・license・patchを記録し、zedBSD target向けGTK4と必要依存がbuild/installできる。
2. zedBSD amd64 QEMU/Venusで選択したGTK4アプリがWayland上でwindowを表示し、p001で定めた代表操作を行える。Vulkan/ソフトウェアのどのrendererで成功したか分ける。
3. 必要だったOS API/第三者ライブラリ/Wayland protocol/portalと各patchの理由を引継ぎ表に残す。Linux標準GTK4との差分を示す。
4. 全変更sourceとpackage metadataの全文規約・provenance・build/guest回帰を検証する。

## ベータ1（fg019、2026-10-17）の選択肢（2026-10-02 計画担当の見積り、ユーザーが決める）

依存の Phase は直列（p004 → p005 → p006 → p007 → p008 → p002 → p010）で、1 Queue 3〜4h、合計およそ 36〜40h（10 Queue）。libc の不足・libtool の共有ライブラリ・meson の cross の問題で 1.5〜2 倍になり得る。

| 案 | ベータ1の到達線 | 条件 | 危険 |
| --- | --- | --- | --- |
| A（安全） | p001 の移植契約と、依存の一部（p004〜p006）まで。GTK4 本体はベータ1の後 | WS117 の後に開始（user の順序どおり） | ベータ1に GTK4 app は無い |
| **B（推奨）** | zedBSD 上で GTK4 の demo app（`gtk4-demo` か小さな試験 app）が **Cairo renderer・wl_shm** で window を出し、click/key/menu が通る | 依存の Phase（p001・p004〜p009）を WS117 と並行して 10/04 頃から 1 担当で直列に実行。GTK4 本体（p002・p010）は WS117 の p002 の採否の後 | 10/13〜10/15 着の見込みで余裕が小さい。GL/Vulkan renderer は後 |
| C | B に加え GL（zedBSD の libegl/libglesv2）か Vulkan（Venus）の renderer | B ＋ 2 担当目 | ベータ1の他の WS の枠を圧迫 |

推奨の GTK の版（D-VER、p001 で確定）: **4.18 系（Linux で実測した Debian13 の 4.18.6 と同じ系列）**。4.24.0（inventory の版）は glib ≥ 2.89.3 と meson ≥ 1.8 を要求し、host の meson 1.7 では足りない（meson の host 道具の build が要る）。

## Dependencies / ownership

[WS114](../ws114/ws.md)の採用範囲/実測と [WS117](../ws117/ws.md) の compositor 改良が runtime（p010）の前提。依存 library の build（p004〜p009）は compositor に依存しない。WS034 の p025（meson cross）・p026（glib）・p027（フォント系）・p028（描画系）・p034（libwayland 互換）・p038（Vulkan だけで GTK4 が動くかの調査）を本 WS の p004〜p009・p001 へ移管することを main に依頼する（WS034 の ws.md の更新は main。WS034 の他 package の目的と GTK3/Qt5 は WS034 に残す）。p029の旧GTK4実装枠はこのWSへ移した。Linuxでの標準GTK4の成功はzedBSD動作の証拠にしない。native menubarの後日案[F-045](../future-work.md)は通常GTK4移植の必須条件にしない。[Guardrail](../guardrail.md)、[C全文](../coding-style.md)、[方針](../standards/ws114-gtk-qt-learning.md)、[license audit](../tools/packages/audit-licenses.sh)。既存のinventory版は候補であり、実装時に公式入力と依存を再照合する。

**共有 path（main の割当が要る）**: `userland/packages/external.mk`・`userland/packages/tools/gen-cross-toolchain.sh`（meson の cross file、p004）、`userland/desktop/libwayland/`（全 Keiland app と libvulkan の WSI が使う、p009）。toolchain（`toolchain/`・`lang/clang`・`devel/libcxx`・共有 `build/llvm`）は変更しない。

## Phases

| ID | Purpose / goal | Status | Dependencies | 目安 |
| --- | --- | --- | --- | --- |
| [ws115-p001](phase001/phase.md) | 移植契約: 版（D-VER）・依存の一覧と版・libc の不足・host 道具・libwayland ABI・renderer・demo app・試験（WS034 p038 を含む） | cleared（q597 / P3、[移植契約](port-contract.md)） | WS114 の実測（済み）、WS034 inventory | 3〜4h |
| [ws115-p004](phase004/phase.md) | meson のクロス契約と host 道具（cross file・pkg-config・gperf・host の glib 道具と wayland-scanner）（旧 WS034 p025） | cleared（q600 / P3） | p001、共有 path の割当 | 3〜4h |
| [ws115-p005](phase005/phase.md) | libffi・pcre2・glib（旧 WS034 p026） | cleared（q602-i01、Q1 2026-10-02） | p004 | 4h |
| [ws115-p006](phase006/phase.md) | libpng・freetype・harfbuzz・fontconfig（旧 WS034 p027） | cleared（q605-i01、Q1 2026-10-02） | p005 | 3〜4h |
| [ws115-p007](phase007/phase.md) | pixman・cairo・fribidi・pango（旧 WS034 p028 の前半） | in-progress（q608-i01、受け入れの証拠あり。clearance は Q1） | p006 | 4h |
| [ws115-p008](phase008/phase.md) | gdk-pixbuf・libjpeg-turbo・libtiff・graphene・libepoxy・libxkbcommon・xkeyboard-config（旧 WS034 p028 の後半） | planning | p005（p007 と並行可） | 4h |
| [ws115-p009](phase009/phase.md) | libwayland-client の upstream ABI 互換・wayland-protocols・wayland-cursor（旧 WS034 p034） | planning | p001、共有 path の割当 | 3〜4h |
| [ws115-p002](phase002/phase.md) | GTK4 本体の package（`userland/packages/desktop/gtk4`）の build/install | planning | p007・p008・p009 | 4h |
| [ws115-p010](phase010/phase.md) | zedBSD QEMU で GTK4 demo app の起動と代表操作（Cairo → GL/Vulkan） | planning | p002、WS117 p003（compositor の改良） | 4h（＋debug） |
| [ws115-p003](phase003/phase.md) | 知見引継ぎ・最終全文規約と回帰 | planning | p010 | 3h |

Graph: WS114 + WS034 inventory → p001 → p004 → p005 → {p006 → p007, p008}; p001 → p009; {p007, p008, p009} → p002 → p010 → p003 → {WS097, WS116}。WS117 p003 → p010。p002 と p003 の scope は 2026-10-02 に改訂（下の Event）。

## Event

2026-10-02 / ws114-gtk-qt-port-plan-20261002: upstream GTK4移植をWS034 p029から別WSへ移管計画。独自実装WS097は保持。実装/guest試験なし、GitHub publication pending。

2026-10-02 / ws115-beta1-plan-20261002: 計画担当が依存 package の移植を Phase として追加（p004〜p010）。WS034 の p025〜p028・p034・p038 の移管を main に依頼（WS034 の記録の更新は main）。p002 の scope を「依存を除いた GTK4 本体の build/install」に、zedBSD 上の実行を新 p010 に分け、p003 の依存を p010 に改訂（p002・p003 に redesign の event）。p001 を planned（並行開始の判断待ち）。ベータ1の選択肢 A/B/C と推奨 B・D-VER の推奨 4.18 系を記録。実装・build・guest なし。

2026-10-02 / q597-p001-cleared: P3 が q597-i01 で p001 の移植契約（[port-contract.md](port-contract.md)）を作った。推奨は GTK 4.18.6 の組（glib 2.84.4・pango 1.56.4・fontconfig 2.17.1）。libc は `sys/poll.h` の header だけが要る。libwayland の不足は p009 の範囲として確定し、本家 libwayland と wayland-cursor は使わない。renderer は Cairo を最初にする。p004〜p010 の範囲と版は契約の §9 を正本とする（各 phase.md への反映は着手時）。ユーザーの判断（D-VER・L1・L2・R1・P1・S1）は未決。package・libc の source は変えていない。

2026-10-02 / q600-p004-cleared: P3 が q600-i01 で meson の cross 契約を作った（`userland/packages/tools/gen-meson-cross.sh`、external.mk の `ZEDBSD_EXTERNAL_MESON` と view と依存の閉包、host 道具の gperf 3.3）。試験 project と upstream の graphene を zedBSD target 向けに build し、ELF の検査を通した。zlib/expat に回帰は無い。sysroot に `sys/poll.h` が入り、glib の構成が pcre2 まで進むことを確かめた。次は p005。[p004](phase004/phase.md)。
