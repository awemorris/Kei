<!-- awesome-plan project=zedbsd record=ws115-p009 -->

# ws115-p009: libwayland-client の upstream ABI 互換・wayland-protocols・wayland-cursor

Parent: [WS115](../ws.md)
Status: cleared（2026-10-03 Q1。q613-i01、ad993eb6a まで main に統合。interface の版の引き上げは compositor と一緒に後の Phase）
Disposition: normal
Primary Milestone: MG002（WSから継承）
Queue / attempts: q613-i01（P3、継続 dispatch、時限 4h、base main 396756c61）
Purpose / goal: upstream wayland-scanner の生成 code を使う GTK4（と Qt6）が zedBSD の libwayland-client で動くようにする。
Prerequisites: p001 の不足の一覧、共有 path（`userland/desktop/libwayland`）の main の割当
Investigation bound: timebox 4h
Origin: WS034 p034（移管を main に依頼）

## 範囲

- p001 で見つけた不足の関数（`wl_proxy_marshal_flags` 等）・`wayland-client-core.h` の header・`.pc` を zedBSD の独自 libwayland に足す（独立実装を保ち upstream の source を取り込まない、WS034 の 2026-09-23 決定）。wayland-cursor は独自実装か upstream の package か p001 の判断。
- wayland-protocols は data だけの package（`userland/packages/desktop/wayland-protocols`）。
- 既存の Keiland app・libvulkan の WSI・libwayland-egl に回帰を出さない。

## 受け入れ

upstream wayland-scanner で生成した xdg-shell の client code を zedBSD の libwayland でリンク・実行する小さな試験が guest の Keiland に window を出す。既存の native app（Terminal・Files）と Vulkan の demo に回帰無し、`boot-test.sh` の PNG。変更 C の全文規約。

## 所有 path

`userland/desktop/libwayland/`（共有、main の割当が要る）、`userland/packages/desktop/wayland-protocols/`、`plan/ws115/`

## Standards / evidence / resume

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[scoped方針](../../standards/ws114-gtk-qt-learning.md)、[automation](../../standards/automation.md)、[license audit](../../tools/packages/audit-licenses.sh) を実装前に確認する。外部 package は公式 tarball を取得・検証して patch し、source tree へ取り込まない（AGENTS.md）。新コードは全文規約。toolchain（`toolchain/`・`lang/clang`・`devel/libcxx`・共有 `build/llvm` 等）は変更・build しない。共有 `build/` は読取専用、自分の worktree の `build/` を使う。全体の `make check` は禁止。具体コマンド/版/結果/commit/環境/成果物/skip/制限は未実施（計画のみ）。各PhaseはQueueの個別scope承認後に実行する。前提未達・判断待ちならattemptをunclearedとして証拠/再開条件を残す。

Event ws115-beta1-plan-20261002: 2026-10-02 計画担当が依存 package の移植 Phase として作成。版・option は p001 の移植契約で確定する。planning/Queue none。GitHub publication pending。

## q613-i01 の結果（2026-10-03、P3）

承認: Q1 の継続 dispatch（q613、時限 4h）。base は main 396756c61（前回統合済み 581021c3e）。所有 path に `userland/desktop/keiland/wayland/`（libwayland-client の公開 header）を Q1 が加えた（2026-10-02、方針は追加だけ）。compositor の server 側（`userland/desktop/wayland/`、P2）は変えていない。

### commit

- 7f6d5f4dd:
  - libwayland の ABI の追加（client.c・protocol.c・event.c・wire.c・data-device-protocol.c・internal.h・Makefile・pkgconfig/）
  - 公開 header（wayland-client-core.h・wayland-client-protocol.h・wayland-util.h）
  - `userland/packages/desktop/wayland-protocols`（Makefile・patch 0001）
  - p008 を cleared、p009 を in-progress にした。
- 本 commit: `tests/xdg-probe/`・`xdg-probe.sh`・`build-desktop-image.sh`・`tests/wl-log/`・`wl-log.sh`、`evidence/q613/`、この記録、ws.md、port-contract §5。

### libwayland-client に足した物（port-contract §5 の表）

| 項目 | 実装 |
| --- | --- |
| `wl_log_set_handler_client` | `wayland-util.h` に `wl_log_func_t`、`wayland-client-core.h` に宣言。handler は atomic に保持する。既定は NULL で、今までどおり何も出さない。server の `wl_display.error` で upstream と同じ行（`interface@id: error code: message`、消えた object は `[destroyed object]`）を作る。handler は connection の mutex を外してから `wl_display_read_events` で呼ぶ（README の「callback は mutex の外」の契約を守る）。 |
| `wl_surface_listener` の v6 の 2 member | 末尾に `preferred_buffer_scale`・`preferred_buffer_transform`。event の表は 4 件にし、event.c に型付きの case 2・3 を足した。event の版は wire.c で proxy の版と照合するので、v6 未満の surface では呼ばれない。既存の 2 member の listener の binary も読み越さない。 |
| `wl_surface_offset`・`WL_SURFACE_*_SINCE_VERSION` | request の表に opcode 10 の `offset`（"5ii"）、関数 `wl_surface_offset`。macro は全 request・event。 |
| `WL_POINTER_AXIS_VALUE120_SINCE_VERSION`（と RELATIVE_DIRECTION） | macro だけ（8・9）。 |
| `wl_output_destroy`・`WL_OUTPUT_*_SINCE_VERSION`・`enum wl_output_transform`・`wl_output_subpixel`・`wl_output_mode` | 関数（client 側の destroy）と macro。enum は upstream 1.23.1 の `wayland.xml` から生成し、前の macro（`WL_OUTPUT_MODE_*`・`WL_OUTPUT_TRANSFORM_NORMAL`・`WL_OUTPUT_SUBPIXEL_UNKNOWN`）を同じ値の enum の定数に置き換えた。 |
| `enum wl_shm_format` | upstream の 123 値。前の macro `WL_SHM_FORMAT_ARGB8888`（0）・`XRGB8888`（1）は同じ値の enum の定数になった（`#ifdef` で使う client は無いことを grep で確かめた）。 |
| `wl_data_device_manager_get_version` | 関数。 |
| `wayland-client.pc`（1.23.1）・`wayland-egl.pc`（18.1.0） | `userland/desktop/libwayland/pkgconfig/`。`make wayland-client` で `build/packages/wayland-client/stage`（library 2 つと .pc）を作る。GTK は `ZEDBSD_EXT_..._DEPENDS` に `wayland-client` を入れる。zedbsd-pkg-config で 1.23.1・18.1.0 が読めることを確かめた。 |

- interface の version: `wl_surface_interface` の表は v6 まで記述する。版は 4 のまま（compositor の wl_compositor v4）。`wl_registry_bind` が `version > interface->version` を拒むので、上げると、`wl_compositor_interface.version` で bind する client が compositor の広告を超えて頼むことになる。compositor が v5/v6 を広告するときに一緒に上げる。
- **wayland-protocols 1.49**（`userland/packages/desktop/wayland-protocols`、data だけで image には入れない）: patch 0001 で enum header（`--strict` で作る）を wayland-scanner ≥ 1.24 のときだけ作る。1.49 の XML は `frozen` 属性を使い、host の scanner 1.23.1 の DTD がそれを知らないため。stage は XML 58 個と `wayland-protocols.pc`（Version 1.49）。GTK の build がこれらの XML を scanner に通すとき同じ問題が出るかは p002 で確かめる。

### 試験

- build:
  - `make sysroot-amd64`: exit 0。
  - `make -j disk-image`: exit 0。1115 の compile で、libwayland・libkeiland・libvulkan・libegl・Terminal・Files などの全 client を新しい header で作り直した。compiler の warning は NoctLang の既知の 1 件だけ。
  - `libwayland-client.so` が `wl_log_set_handler_client`・`wl_surface_offset`・`wl_output_destroy`・`wl_data_device_manager_get_version` を export する。
- host の試験:
  - `plan/ws035/tests/p075/run-host.sh`（generic な event の配送と server が作る object）: PASS（CLIENT DONE failures=0、SERVER DONE children_destroyed=4）。
  - `plan/ws073/tests/wayland-dispatch-once.sh`（全 source を host の gcc の -Werror と ASan/UBSan で）: PASS 2 回。
  - 新しい `plan/ws115/tests/wl-log.sh`: socketpair の偽の server が `wl_display.error` を送る。handler が `wl_display@1: error 3: boom` を 1 回受け、dispatch は -1、error 71（EPROTO）。通常と ASan/UBSan（leak 検出あり）の両方で PASS。
  - `plan/ws014/phase006/tests/run-wayland-client.sh` は移設前の path（`libc/include`）を指していて古いので、走らせていない。
- **受け入れの試験（QEMU の Venus の desktop guest、`zdesktop-guest.sh`、renderer は main の build の物を読み取りで使った）**:
  - `plan/ws115/tests/xdg-probe.sh`: host の upstream wayland-scanner 1.23.1 が wayland-protocols 1.49 の xdg-shell.xml から client-header と private-code を作る。zedBSD の sysroot の header と `-lwayland-client` で link した。xdg の interface は program の中で local（`d`）で、`xdg_` の export は無い。
  - image: `build-desktop-image.sh`（criteria の image ＋ probe）。
  - guest で compositor（`/bin/wayland`）を起動し、`xdg-probe 25` を動かした。出力: data device manager version 3、wl_surface version 4、xdg_wm_base version 4、toplevel bounds 1280x800（v4 の configure_bounds が upstream の生成 code の listener に届いた）、`wl_surface.offset skipped (version 4 < 5)`、window shown 320x200、`xdg-probe: PASS`、終了した。
  - **[evidence/q613/xdg-probe.png](../evidence/q613/xdg-probe.png)**: compositor の画面の中央に 320×200 の probe の色（#2a7fd4）の window が出ていることを目で確かめた。compositor の log に error の行は無い。
- **回帰（同じ guest、全 client を作り直した image）**:
  - wltest（Vulkan WSI）: 120 frame、exit 0。compositor が import した。
  - Terminal と Files: 両方の process が生きていて、log は正常（ZTERM START、ZFILES READY）。[evidence/q613/terminal-files.png](../evidence/q613/terminal-files.png) で両方の window が描かれていることを目で確かめた。
  - seat-probe・subsurface-probe・data-probe: exit 0、DONE。
  - 最後に compositor は生きていて、`ZWL (FAILED|GPU_ERROR|VULKAN_ERROR|PROTOCOL)` の行は無い。
- `plan/tools/boot-test.sh build/p3-q592/zedbsd/hdd-image.img`（新しい libwayland と作り直した client を含む通常の image）: PASS。[evidence/q613/boot.png](../evidence/q613/boot.png) に login prompt が出ていることを目で確かめた。
- 既存の binary を作り直さずに動かす試験はしていない（全 client を同じ tree で作り直す方を選んだ。Q1 の指示の 2 つ目の選択肢）。
- guest の port: guest.py は 127.0.0.1 の空き port を乱数で選ぶ（SSH 59053、gdb 36193）。10300〜10399 の割り当ては、共有の道具を変えないと使えない。
- 実機: 未実施。

### license

- wayland-protocols 1.49: MIT（COPYING）。build の data だけで、image には入らない。
- `userland/desktop/libwayland`・`keiland/wayland` の追加は Zlib。enum の名前と値は upstream の protocol の事実で、API-PROVENANCE.md の pinned 1.23.1 の範囲にある。

### 受け入れの判定（P3 の評価。確定は Q1）

- upstream wayland-scanner の xdg-shell の client が zedBSD の libwayland で link・実行され、guest の Keiland に window を出した: 満たす（QEMU。PNG）。
- 既存の native app（Terminal・Files）と Vulkan の demo（wltest）に回帰が無い: 満たす（QEMU。PNG と log）。
- boot-test の PNG: 満たす（QEMU）。
- 変更した C の全文規約: 新しい関数は規約の形で書いた（宣言の一行・関数の注釈・段落の注釈・成功の return）。既存の file の形に合わせた所（event.c の case の並び）もある。
- 実機: 未実施。

### 残り・申し送り

- compositor が wl_compositor v5/v6・xdg_wm_base v5 以降を広告するときの作業: interface の version を上げる、xdg の型付きの表に v5 の event を足す（P2・後の Phase）。
- GTK の build で wayland-protocols 1.49 の XML が scanner 1.23 の非 strict の生成で通るかは p002 で確かめる。
- `run-wayland-client.sh` の古い path（main の道具）。
