<!-- awesome-plan project=zedbsd record=ws131-design -->

# WS131 移行計画: libkeiland-backend の分離と libkeiui の libkeiland への吸収

[WS131](ws.md) の移行計画の正本。初版は ws131-p002 / q628-i01、改訂（第 2 版）は q629-i01（2026-10-03、P3）。**設計だけで、source は変えていない。**
事実は main `dad53b125`（初版は `c5c59904e`、WS131 の対象の source は同じ）を読んで測った。行数は `wc -l`、名前の数は header からの機械的な抽出。

第 2 版で取り込んだもの:

| 入力 | 中身 | 反映した所 |
| --- | --- | --- |
| 2026-10-03 user（D2 と進め方） | 「GPUバッファ管理もbackendに移したいです。ただし、backend化は一気に実装せず、1つずつ移行することで、正確に、確実に作業した方がいい」 | §3.5（GPU の buffer の protocol を backend へ、protocol の host の interface）、§7（backend を 1 領域ずつの Phase に） |
| 2026-10-03 user（名前の規則、最新） | macro・enum の定数は `KL_`、関数・型・変数は `kl_`（`kui_*` も今の `keiland_*` も）、`paths.h` の `KEILAND_BINDIR`・`KEILAND_LIBEXECDIR` などはそのまま、公開の header は最終的に `keiland.h` 一つ、compositor の `ZWL_`・`zwl_` は `KWL_`・`kwl_`、全て段階的 | §5（名前の段）、[rename-map.md](rename-map.md)・[rename-map-kwl.md](rename-map-kwl.md) |
| 2026-10-03 user（ABI） | 「まだベータ版ですらないので、ABIは変更して問題ないです。ABIバージョンも上げなくていいです。」 | §5.5（版の段階を計画から外す。旧名の互換は段階的な移行の間の source の互換だけ） |
| design-reviewer の review（Q1 の要約 `plan/ws131/reviews/2026-10-03-design-review.md`、22 項目） | 退行・取りこぼし・前提の誤り・事実の誤り | §11 の対応表に全項目 |

---

## 0. 要約

### 0.1 できあがりの構成

| 部品 | 使う者 | 中身 | 置き場 |
| --- | --- | --- | --- |
| **libkeiland-backend** | compositor だけ | 電源・WiFi/network・音声・PnP、compositor の画面と入力の OS の部分（seat・logind/seatd・VT・sessiond との受け渡し・evdev・表示の取得）、**GPU の buffer の protocol**（zedBSD の `keiland_gpu_buffer_v1`、Linux・FreeBSD の `zwp_linux_dmabuf_v1`）、client の資格の確認 | 共通の interface と共有の仕組み: `userland/desktop/libkeiland-backend/`、実装: `libkeiland-backend-zedbsd/`・`-linux/`・`-freebsd/` |
| **libkeiland** | 標準 app（と xserver・probe） | desktop の拡張の wrapper（menu・titlebar・glass・desktop surface・inset・edit・recent・motion・scroller・gesture）＋ 吸収した libkeiui ＋ 拡張の client（`kl_system_*`: 設定・WiFi・音量・電源・PnP）＋ app の骨組み（`kl_app`）。OS の code を持たない。公開の名前は全て `kl_`・`KL_`、header は `keiland.h` 一つ | `userland/desktop/libkeiland/`（libkeiui は `libkeiland/ui/`） |
| **compositor**（zdesktop） | — | OS に依存しない共通の source だけ。wl_buffer の寿命と OS に依らない画像の型は compositor が持ち、backend に小さな「protocol の host」の interface を渡す。新しい拡張 `kl_system_manager_v1` で app に機能を出し、`desktop.conf` を一手に記録する。内部の名前は `kwl_`・`KWL_`。touch IME の UI のために libkeiland の描画の層を link してよい（D4、Wayland の client の部分は使わない） | `userland/desktop/wayland/` |

経路の例（ユーザーの確認 1）: Settings → libkeiland の `kl_system_network_request()` → 拡張の `join` の要求 → compositor → `kl_backend_network_request()` → networkd（zedBSD）／ wpa_supplicant（Linux・FreeBSD）。

### 0.2 Phase の一覧（23 個、合計の目安 90〜125 h。実行の順は表の順: p025 は p020 の後・p023 の前）

backend は 1 領域ずつ移し、各 Phase の終わりに 3 OS の build と回帰を確かめてから次へ進む（ユーザー「1つずつ移行」）。名前の改名も段に分ける。

| Phase | 線 | 目的（一行） | 終わった時に成り立つこと | 依存 | 目安 |
| --- | --- | --- | --- | --- | --- |
| [p003](phase003/phase.md) | backend | 土台と **network** | 3 OS の backend の tree と build、network の OS の code が backend に、system bar の WiFi が backend を使う | p002 の承認、P1 の `network-zedbsd.c` の merge | 4〜5 h |
| [p004](phase004/phase.md) | backend | **音声** | audio の OS の code が backend に、system bar の音量が backend を使う | p003 | 3〜4 h |
| [p005](phase005/phase.md) | backend | **電源** | 電源の状態と操作の領域（zedBSD の greeter の POWER、他は unsupported。Linux の logind は D-Bus が移る p006 で有効に） | p004 | 3〜4 h |
| [p006](phase006/phase.md) | backend | **seat・session** | logind・直の seat・seatd・VT・sessiond の受け渡し（READY/GO/RELEASED/LOGOUT/QUIT・AUTH・UNLOCK）が backend に、callback の約束 | p005 | 4〜5 h |
| [p007](phase007/phase.md) | backend | **入力** | evdev の走査・device の読み・lease が backend に | p006 | 3〜4 h |
| [p008](phase008/phase.md) | backend | **表示** | 表示の node と取得・解放が backend に | p007 | 3〜4 h |
| [p009](phase009/phase.md) | backend | **GPU の buffer** と境界の確定 | GPU の buffer の protocol が backend に（protocol の host の interface）、compositor の OS の dir が 0、配置の Guardrail と checker を確定 | p008 | 4〜5 h |
| [p010](phase010/phase.md) | 拡張 | 拡張の protocol と設定の記録 | `kl_system_manager_v1`・`kl_system_*`、compositor の store と worker の thread。**毎秒の監視はまだ残す** | p004（p005 の後なら電源も） | 4〜5 h |
| [p011](phase011/phase.md) | 拡張 | Settings を拡張へ、監視の除去、libkeiland の OS を 0 に | Settings が `kl_system_*`、毎秒の `stat` の監視を除く（BUG-125 の原因）、旧 network・audio・preferences の API と道具の向け直し、書き手の Guardrail | p010、WS089・P1 の区切り | 4〜5 h |
| [p012](phase012/phase.md) | toolkit | libkeiui を libkeiland へ**移す**（名前は変えない） | `libkeiland/ui/`、`libkeiui.so` が無い、Files・config・package・header の install の取りこぼし無し | p003 の merge、ベータ1 の app の区切り | 4 h |
| [p013](phase013/phase.md) | 名前 | 旧 libkeiui の名前を `kl_`・`KL_` に | `kui_`→`kl_`（341）、`keiui.h` は互換の macro、`keiland-ui.h` | p012 | 3〜4 h |
| [p014](phase014/phase.md) | 名前 | 旧 libkeiland の名前を `kl_`・`KL_` に | `keiland_`→`kl_`・`KEILAND_`→`KL_`（266、paths.h を除く）、互換の macro | p013、p011 | 3〜4 h |
| [p015](phase015/phase.md) | toolkit | app の骨組みの API | `kl_app`、宣言的な menu・titlebar・glass | p014・p010 | 4〜5 h |
| [p016](phase016/phase.md) | app | Text Editor | 新 API・新名、自前の結線が無い | p015 | 3〜4 h |
| [p017](phase017/phase.md) | app | PDF Viewer・Image Viewer | 同 | p016 | 3〜4 h |
| [p018](phase018/phase.md) | app | Terminal・Notes | DnD・primary・tablet・fd の監視が libkeiland に | p016 | 4〜5 h |
| [p019](phase019/phase.md) | app | Settings の窓 | 自前の窓と present が無い | p011・p016 | 4 h |
| [p020](phase020/phase.md) | app | Files | toplevel と desktop surface、DnD の source | p018 | 4〜5 h |
| [p021](phase021/phase.md) | 名前 | compositor の内部の名前を `kwl_`・`KWL_` に | `zwl_`→`kwl_`（494）・`ZWL_`→`KWL_`（271）、`zwl*.h`→`kwl*.h`。log の文字列は変えない | p009、p011 | 3〜4 h |
| [p022](phase022/phase.md) | 名前 | compositor の log の接頭辞を `KWL ` に | `"ZWL "` を読む試験 201 本を同時に直す | p021 | 3〜4 h |
| [p025](phase025/phase.md) | app | browser の shell の窓（D7） | shell の自前の窓（`window.c` 1,176 行）と present を `kl_app` と `kl_window_vulkan_surface` に、URL の field を宣言的な titlebar に。libbrowser は触らない。WS074 との衝突は開始の前に Q1 がユーザーに確認 | p020 | 4〜5 h |
| [p023](phase023/phase.md) | 終わり | 互換の除去・header の一本化・PnP | 旧名の互換と `keiui.h`・`keiland-ui.h` を除き `keiland.h` 一つ。WS132 があれば PnP | p016〜p020・p025、p022 | 3〜4 h |
| [p024](phase024/phase.md) | 終わり | 全文規約と 3 OS の回帰、WS の完了 | 全 checker・全回帰 | 全て | 4〜6 h |

初版（14 Phase、約 50 h）からの変更: backend を 7 領域の 7 Phase に、名前の改名を 4 段（p013・p014・p021・p022）に、libkeiui の吸収を「移す」と「改名」に割った（review 15）。browser の shell の窓の移行は D7 の決定（2026-10-03 user「D7,browserのshellもlibkeilandで書きましょう。」）で p025 として足した（ID は新規、順は p020 の後・p023 の前）。見積もりは review 15 の幅に合わせた。

### 0.3 ユーザーの判断（詳細は §9）

決まったこと（2026-10-03 user、p002 第 2 版のレビュー）: D1・D3・D5・D6・D10〜D13・D15・D16 は推奨のとおり承認。D4・D7・D8・D9 は下の表の決定。それ以前の決定: D2（GPU の buffer も backend へ）、ABI は変えてよく版も上げない、名前の規則（`KL_`・`kl_`・`KWL_`・`kwl_`）、1 領域ずつの移行、**D14（Guardrail の範囲、§9）**: Guardrail の規則は desktop の OS の抽象化に限る。Terminal の pty は Terminal に OS の依存を残し macro の block で切り替える、X server は Wayland の規格（evdev）の key code の定数を自分の header に持ち `<uapi/input.h>` を include しない、Files の xattr は app 側の OS の依存として残し情報の panel は xattr の名前だけの簡単な表示にする。

| # | 問い | 決定 |
| --- | --- | --- |
| D1 | backend を静的な内部 library にするか | 静的（install しない） |
| D3 | evdev の macro の選択を backend の header へ | 移す |
| D4 | compositor と libkeiland | touch IME の UI のために link してよい（不要なら link しない）。循環を作らない条件は §9 |
| D5 | 拡張の認可 | registry で同じ uid の client にだけ見せる（WS113 p005 と同じ） |
| D6 | 動作中の手での `desktop.conf` の編集 | 監視しない（次の起動で効く） |
| D7 | browser の shell の窓の移行 | **入れる**（2026-10-03 user）: p025、p020 の後・p023 の前。libbrowser は触らない |
| D8 | 実行の時期 | 単独走行（N=1）で p003〜p024 を番号の順に流す。p011 の後に Q1 がユーザーに進み具合を報告する区切り。開始はユーザーの承認と P2 の終了の後に Q1 が指示 |
| D9 | WS090 の残り | WS131 の完了の後に扱う。WS131 の間は WS090 を動かさない。Settings と Files の窓は p019・p020 のまま |
| D10 | 移動の例外（機械的な改名と path の修正を含む） | 認める |
| D11 | FreeBSD の実行の証拠 | build と監査は必須、起動は passthrough なしで動く範囲 |
| D12 | 電源の操作の範囲 | zedBSD の session は unsupported（sessiond の拡張は別の WS）、Linux は logind |
| D13 | WS113 の表示の設定を同じ manager に | 載せる |
| D15 | Wayland の protocol の名前（`keiland_*_v1`）も `kl_*_v1` にするか | する（p021 で wire の名前と生成の定数を同時に）。新しい拡張は `kl_system_*_v1` で作る |
| D16 | app・libbrowser の include guard と、picture・artwork の内部の `keiland_` の名前 | library・compositor・共有の source は `kl_`・`KL_` に、libbrowser と app の include guard は対象外 |

---

## 1. 前提

- ユーザーの決定（全文は [ws.md](ws.md)）: libkeiland を libkeiland-backend と libkeiland に分解。app は OS の抽象化を直接持たない（確認 1）。compositor の OS ごとの画面・入力の処理も backend の OS の tree へ（確認 2）。libkeiui は libkeiland に吸収（確認 3）。設定の記録は compositor が行い、毎秒の stat を無くす。移行計画はユーザーがレビューする。
- 範囲外: HAL・toolchain・kernel（WS132 の `/dev/system` は WS132）、sessiond の protocol の拡張（D12）、libbrowser、外部 package、GTK4/Qt6（§6.4 の要件だけ）。

---

## 2. 棚卸し（実測）

### 2.1 libkeiland の file（C の source と header 22・13,018 行）

| file | 行 | 役割 | 移行先 |
| --- | --- | --- | --- |
| `version.c` | 25 | `keiland_version()` | 残る（`kl_version`） |
| `menu.c` | 1128 | System Menu・context menu | 残る |
| `titlebar.c` | 988 | titlebar（sheet を含む） | 残る |
| `glass.c` | 310 | glass の panel | 残る |
| `desktop.c` | 387 | desktop surface | 残る |
| `keyboard-inset.c` | 254 | keyboard の inset | 残る |
| `edit.c` | 281 | 編集の操作 | 残る |
| `recent.c` | 463 | 最近の file | 残る |
| `motion.c` | 1316 | touch の motion（compositor も使う） | 残る。compositor は libkeiland の link で使う（D4） |
| `scroll.c`・`gesture.c` | 806・716 | scroller・gesture | 残る |
| `preferences.c` | 922 | `desktop.conf` の読み書き | 書式は compositor の store へ（p010）、公開 API は p011 で除く |
| `zedbsd/network-zedbsd.c`・`network-link-zedbsd.c` | 1138・376 | networkd の client、interface・DNS（ioctl 3） | backend-zedbsd（p003） |
| `zedbsd/audio-zedbsd.c` | 530 | audiod の client | backend-zedbsd（p004） |
| `linux/network-link-linux.c`・`audio-linux.c` | 315・577 | interface・DNS（ioctl 3）、ALSA（ioctl 10） | backend-linux（p003・p004） |
| `freebsd/network-link-freebsd.c`・`audio-freebsd.c` | 340・459 | interface・DNS（ioctl 3）、OSS（ioctl 7） | backend-freebsd（p003・p004） |
| `wpa/network-wpa.c`・`network-config-wpa.c`・`network-wpa.h` | 1282・362・43 | wpa_supplicant（Linux・FreeBSD） | `libkeiland-backend/wpa/`（p003） |

zedBSD の build は `userland/base/net/protocol.c`・`wifi-conf.c`・`wifi-store.c` を source のまま compile する（`libkeiland/Makefile:19-22`）。共通 7,596 行・OS 5,422 行。

### 2.2 libkeiland の公開 API（`keiland.h` 1,343 行、関数 111、`KEILAND_VERSION 21`（`keiland.h:49`））

| 領域 | 関数 | 今の利用者（呼ぶ所の数） | 移行後 |
| --- | --- | --- | --- |
| version | 1 | — | `kl_version`（版は上げない） |
| System Menu・window menu・context menu | 18＋3＋1 | Settings 11・Files 14・Text Editor 13・Image Viewer 12・PDF Viewer 11・Notes 11・Terminal 11・menu-probe 9 | 残る（p015 で宣言的な API を上に） |
| titlebar | 17 | 全 app・xserver 2・libkeiui 5・probe | 残る |
| recent・glass・desktop surface | 3・4・3 | app・libkeiui・kuidemo | 残る |
| network | 11 | **compositor 8**（`wayland/network.c`）・Settings 11 | 実装は backend（p003）、Settings は `kl_system_*`、旧 API の除去（p011） |
| audio | 8 | **compositor 5**（`volume.c`）・Settings 7 | 実装は backend（p004）、以下同じ |
| motion | 12 | **compositor 9**（`touch.c`） | 残る。compositor は link で使う（D4） |
| scroller・gesture | 9＋9 | 6 app・browser・libkeiui | 残る |
| preferences | 7 | **compositor 6**（`preferences.c`・`volume.c`）・Settings 7 | 書式は compositor へ（p010）、除去（p011） |
| keyboard inset・edit | 2＋3 | libkeiui（窓） | 残る |

公開の名前（関数・struct・typedef・macro・enum の定数）は 266（[rename-map.md](rename-map.md)、`paths.h` の macro を除く）。

### 2.3 libkeiui の file（C の source と header 29・16,236 行、ほかに shader 2 と `shaders/regenerate.py`、`KUI_VERSION 12`）

| file | 行 | 役割 |
| --- | --- | --- |
| `window.c`・`window.h`・`present.c`・`present-shm.c`・`clipboard.c`・`primary.c`・`text-input.c`・`edit.c`・`input.c` | 2097・317・1213・222・504・324・345・246・66 | 窓（xdg toplevel・input の queue・repeat・present・clipboard・PRIMARY・IME） |
| `canvas.c`・`text.c`・`icons.c`・`icons-line.c`・`theme.c` | 1397・814・366・421・71 | 描画の層 |
| `scroll.c`・`scroll-bar.c`・`text-touch.c`・`ui.c` | 656・406・371・1679 | scroll view・入力の層 |
| `widgets.c`・`field.c`・`list.c`・`cards.c` | 433・433・399・407 | 部品 |
| `chooser.c`・`chooser-model.c`・`chooser-view.c`・`chooser.h` | 485・1232・986・169 | file chooser |
| `version.c`・`internal.h`・`shaders.h` | 23・69・85 | 版・内部・SPIR-V |
| （source として compile）`userland/desktop/picture/color-glyph.c` | — | 色付きの glyph（compositor も compile） |

公開の名前は 341（関数 151・struct 25・enum 1・enum の定数 48・macro 113・typedef 3）。内部の非 static の関数は `keiui_` で始まる 55 個。

### 2.4 libkeiui の利用者（link と source）

| 利用者 | 形 |
| --- | --- |
| Text Editor・Image Viewer・PDF Viewer・Notes・Terminal・keiland-ime・kuidemo | `libkeiui.so` を link（zedBSD `platform/amd64/vmunix.mk`、Linux・FreeBSD の `Makefile.linux`・`.freebsd`） |
| **Files** | link しないが、`files/ui-scrollbar.c:22` が `<keiui.h>` を include し、Files の 3 本の Makefile（`Makefile:14`・`Makefile.linux:20`・`Makefile.freebsd:20`）が `libkeiui/scroll-bar.c` を source として compile（review 2） |
| package の選択 | `config/ci/config-amd64.mk:37` と試験の config 4 本（`plan/ws035/tests/config-amd64-zdesktop.mk`・`config-amd64-userland.mk`・`plan/ws081/tests/config-amd64-demo-win.mk`・`plan/ws090/tests/config-amd64-textinput.mk`）の `ZEDBSD_USER_PROGRAMS` が package 名 `libkeiui` を書く（review 2） |
| header の install | FreeBSD の `keiland-freebsd.mk:184-187` の公開の header の表に `keiland.h keiui.h`（review 2） |
| host の試験 | `keiui.h` か `libkeiui/` の path を使う 13 file: `plan/tools/files/host-build.sh`・`plan/tools/keiui/host-chooser.sh`・`plan/tools/textedit/host-core.sh`・`plan/ws127/tests/scroll-bar-test.c`・`.sh`・`plan/ws102/tests/host-inset.sh`・`plan/ws090/tests/host-widgets.sh`・`.c`・`host-input.sh`・`.c`・`host-draw.sh`・`.c`、`plan/tools/keiland-os-boundary/check.sh`（review 19） |

link の依存（zedBSD、`vmunix.mk:886-896`）: libtruetype・libkeiland・libwayland-client・libvulkan・libpng-compat・libz-compat。libkeiland.so は zedBSD では既に libwayland-client と libtruetype に依存（`vmunix.mk:1004-1011`）、Linux では libwayland-client だけ（`libkeiland/Makefile.linux:20`）。

### 2.5 compositor の OS の module（23 file・6,591 行 ＋ 境界の header 166 行）

| file | 行 | 役割 | compositor の内部への依存 | 移す Phase |
| --- | --- | --- | --- | --- |
| `zedbsd/handoff-zedbsd.c` | 308 | sessiond との READY/GO/RELEASED/LOGOUT/QUIT と、lock 中の UNLOCK の答え（`:285-298`） | `zwl.h`、`zwl_request_stop`・`zwl_lock_answer`・`zwl_compose_output_close`・`zwl_clipboard_history_clear`、`server->auth_fd`・`control_fd`・`greeter`・`handed_over`・`locked`・`logout_ms` | p006 |
| `zedbsd/input-zedbsd.c` | 315 | evdev の node の走査（ioctl 7） | `zwl_input_probe`、`server->inputs`・`input_scan_time` | p007 |
| `zedbsd/os-zedbsd.c` | 132 | 表示の取得・poll | `zwl-os.h` | p008 |
| `zedbsd/gpu-zedbsd.c`・`gpu-zedbsd.h` | 140・39 | kernel の画像の記述（`<uapi/gpu.h>`）の解読 | `zwl-gpu.h` | p009 |
| `zedbsd/gpu-buffer-zedbsd.c` | 525 | `keiland_gpu_buffer_v1` の protocol と Vulkan の import | `zwl_create`・`zwl_find`・`zwl_take_fd`・`zwl_object_destroy`・`zwl_import_adopt`・`zwl_import_set_alpha`、`compose->device`、`surface->acquire`・`acquire_count` | p009 |
| `linux/os-linux.c` | 433 | VT（ioctl 6）・seat の選択・表示の取得 | `compose.h`・`zwl-os.h`、`server->compose`・`failed`・`socket_given`・`socket_path` | p006（VT・seat）・p008（表示） |
| `linux/seat-logind-linux.c`・`seat-direct-linux.c`・`seat-linux.h`・`dbus-linux.c`・`dbus-linux.h` | 793・139・50・1111・66 | logind（PauseDevice・`PauseDeviceComplete` `:705`・`ResumeDevice` `:603`）・直の seat・D-Bus | `zwl_input_close`・`zwl_compose_quiesce`・`zwl_compose_output_close`、`server->dirty`・`inputs`・`os_paused`・`windowed` | p006 |
| `linux/input-seat-linux.c` | 81 | evdev の lease | `evdev/seat.h` | p007 |
| `linux/sync-linux.c` | 38 | dma-buf の同期の export（ioctl 1） | `dmabuf/sync.h` | p009 |
| `freebsd/os-freebsd.c` | 209 | seatd・表示の取得 | 同上 | p006・p008 |
| `freebsd/seat-freebsd.c`・`.h` | 534・25 | libseat | `zwl_input_close`・`zwl_compose_quiesce`・`zwl_compose_output_close`、`server->…` 7 | p006 |
| `freebsd/sync-freebsd.c` | 56 | dma-buf の同期（ioctl 1） | `dmabuf/sync.h` | p009 |
| `evdev/input-evdev.c`・`seat.h` | 349・21 | Linux・FreeBSD の evdev の走査（ioctl 8） | `zwl_input_probe` | p007 |
| `dmabuf/gpu-dmabuf.c`・`sync.h` | 1147・15 | `zwp_linux_dmabuf_v1` と import | `zwl_create`・`zwl_create_server`・`zwl_emit`・`zwl_error`・`zwl_error_code`・`zwl_take_fd`・`zwl_object_destroy`・`zwl_import_adopt`・`zwl_import_set_alpha`、`compose->device`・`physical`・`instance`、`object->gpu_private`、`surface->acquire` | p009 |
| `session/handoff-session.c` | 65 | sessiond の無い session | `zwl_compose_output_close` | p006 |
| `zwl-os.h`・`zwl-input.h`・`zwl-gpu.h`・`zwl-evdev.h` | 45・46・55・20 | compositor ⇔ OS の境界 | — | `keiland-backend.h` へ段階的に |

compositor の共通の source の中で OS に触れる所:

| 所 | 根拠 | 扱い |
| --- | --- | --- |
| evdev の header の macro の選択 | `wayland/zwl-evdev.h:12-18` | backend の header へ（p007、D3） |
| greeter の sessiond の行（`AUTH`・`POWER`）と lock 画面の `UNLOCK` | `wayland/greeter.c:1054`・`:1104`・`:255-275` | backend-zedbsd の session と電源へ（p005・p006） |
| 設定の毎秒の監視（thread で `stat`） | `wayland/preferences.c:7-23`・`:36`・`:237-360` | p011 で除く（review 1） |
| system bar の network・音量が libkeiland の OS の API、event loop の中で store の読み書き | `wayland/network.c:212`・`:1164`・`:1283`、`wayland/volume.c:136`・`:703-706` | backend へ（p003・p004）、disk と ioctl の待ちは worker へ（p010、review 7） |
| 入力の 2 秒ごとの走査 | `wayland/main.c:654-655`、`zwl.h:68` | backend へ（p007） |

### 2.6 試験と道具（移行で直す物）

| 種類 | file | 直す Phase |
| --- | --- | --- |
| 旧 network・audio・preferences の API を呼ぶ道具と試験（review 6） | `plan/tools/keiland-linux/network-probe.c`・`audio-probe.c`・`lib-smoke.c`、`plan/ws089/tests/host-network.c`・`host-slot.c`・`host-preferences.c`、`plan/ws100/tests/host-audio.c` | p003・p004（path）、p011（API の向け直し） |
| 移す source の path を compile する host の試験 | `plan/tools/files/host-build.sh`・`plan/tools/textedit/host-core.sh`・`plan/ws089/tests/host-build.sh`・`host-preferences.sh`・`plan/ws081/tests/host-scroll.c`・`run-motion.sh`・`host-motion.c`・`plan/ws102/tests/host-inset.sh`・`plan/ws100/tests/host-audio.sh`・`plan/ws090/tests/host-widgets.sh`・`host-input.sh` | その source を移す Phase |
| compositor の OS の dir を参照する道具 | `plan/tools/gpu-boundary/gpu-zedbsd-host.c`・`run-gpu-zedbsd-host.sh`・`v1-check.sh`、`plan/tools/keiland-linux/dbus-wire.c`、`plan/tools/keiland-freebsd/dmabuf-export-rejected.c` | p006・p009 |
| 手で書いた `desktop.conf` を数秒で適用させる試験（review 1） | `plan/ws089/tests/settings-p007.sh` | p011（拡張で変える形に書き換え） |
| compositor の log の `"ZWL "` を読む試験 | sh・py で 201 本（`plan/history` を除く）。compositor の 44 file が 63 種類の tag を出す | p022 |
| build の照合 | `plan/tools/keiland-linux/makefile-sync.sh:13-30`（`Makefile` と `Makefile.linux` の対）、`elf-check.sh:17`（`libkeiui.so` の名前） | p003・p012 |

---

## 3. libkeiland-backend の設計

### 3.1 置き場

```text
userland/desktop/libkeiland-backend/            共通: interface と、複数の OS が共有する仕組み
  keiland-backend.h                             interface（compositor と 3 つの実装が include。公開の header の dir には置かない）
  keiland-backend-evdev.h                       evdev の型と code の選択（今の wayland/zwl-evdev.h、唯一の macro の block）
  wpa/ evdev/ session/                          Linux・FreeBSD の共有の仕組み
  unsupported/                                  未実装の領域が ENOTSUP を返す共通の実装
userland/desktop/libkeiland-backend-zedbsd/     sources.mk（zedBSD の build の fragment）と *-zedbsd.c
userland/desktop/libkeiland-backend-linux/      Makefile.linux と *-linux.c（logind・D-Bus・ALSA・VT・dma-buf の protocol と同期）
userland/desktop/libkeiland-backend-freebsd/    Makefile.freebsd と *-freebsd.c（libseat・OSS・dma-buf）
```

zedBSD の fragment の名前は `Makefile` にしない。top の `Makefile:262-266` が `userland/*/*/Makefile` を package として自動に include するため（review 21）。file の名前は `<役割>-<OS>.c`・`<役割>-<仕組み>.c` を保ち、移動は `git mv`。dma-buf の protocol（今の `dmabuf/gpu-dmabuf.c`）は Linux と FreeBSD が共有するので `libkeiland-backend/dmabuf/` に置き、Linux・FreeBSD の system（Mesa）の Vulkan の header で compile する。zedBSD の Vulkan の header（`include/libc/vulkan/`）に `VK_EXT_image_drm_format_modifier` が無いことは、dma-buf が zedBSD で compile されないので問題にならない。

### 3.2 形: 静的な内部 library（D1）

| | 静的（推奨） | 共有 `libkeiland-backend.so` |
| --- | --- | --- |
| install・package | 無し（WS108・WS112 の file の一覧は `libkeiui.so` が消える以外に変わらない） | `/lib`・`$(PREFIX)/lib` に足し、deb・rpm・Arch の一覧を変える |
| 境界の強制 | checker（include の禁止、Linux の `libkeiland-backend.a` の未定義の symbol に `zwl_`・`kwl_` が無い） | linker の `-z defs` でも |
| zedBSD | compositor の package の source の一覧に `sources.mk` の変数（`picture/color-glyph.c` を複数の binary が compile するのと同じ形） | `vmunix.mk` に link の規則 |
| Linux・FreeBSD | `KEILAND_LINUX_STATIC`（`keiland-linux.mk:79-88`）で `.a`、`KEILAND_LINUX_PROGRAM`（`:57`）の link に渡す | `KEILAND_LINUX_LIBRARY` |

backend の source は compositor の header（`userland/desktop/wayland/`）も `<keiland.h>` も include しない。compositor の状態は callback と protocol の host の interface でだけ動かす。

### 3.3 interface（領域ごと、案）

最終の宣言は、その領域を移す Phase が 3 OS の実装と一緒に確定する。どの OS でも未実装の領域は `ENOTSUP` を返し、compositor はその機能を出さない（拡張の `capabilities`）。名前は最初から `kl_backend_`・`KL_BACKEND_`。

```c
/* userland/desktop/libkeiland-backend/keiland-backend.h（案） */
#define KL_BACKEND_INTERFACE	1U			/* compile 時の照合だけ（ABI の版ではない） */

struct kl_backend;

/* backend が compositor に知らせること（p003 で枠、各領域の Phase が field を足す）。§3.4 の約束に従う。 */
struct kl_backend_host {
	void *data;
	void (*session_paused)(void *data);					/* p006 */
	void (*session_resumed)(void *data);					/* p006 */
	void (*session_stop)(void *data, unsigned reason);			/* p006: sessiond の QUIT、logind の session の終わり */
	void (*session_answer)(void *data, unsigned request, int error);	/* p006: AUTH・UNLOCK・POWER の答え（request で区別） */
	void (*input_found)(void *data, int descriptor, const char *path, const struct kl_backend_input_caps *caps);	/* p007 */
	void (*input_revoked)(void *data, int descriptor);			/* p006・p007 */
	void (*device_changed)(void *data, const struct kl_backend_device_event *event);	/* p023（WS132） */
};

/* 中心（p003） */
int kl_backend_open(const struct kl_backend_options *options, const struct kl_backend_host *host, struct kl_backend **backend);
void kl_backend_close(struct kl_backend *backend);
size_t kl_backend_poll_count(const struct kl_backend *backend);
void kl_backend_poll_fill(struct kl_backend *backend, struct pollfd *descriptors);
void kl_backend_poll_done(struct kl_backend *backend, const struct pollfd *descriptors);
void kl_backend_tick(struct kl_backend *backend, uint64_t now_ms);

/* network（p003）: 今の keiland_network_* を改名した pull 型。要求は一度に一つ（他は EBUSY、keiland.h の今の約束） */
/* kl_backend_network_open・close・update・get_state・get_scan・request・get_request・get_links・get_dns・save_key・get_saved */

/* audio（p004）: open・close・fd・update・get_state・set_volume・feedback・available */

/* power（p005） */
int kl_backend_power_get_state(struct kl_backend *backend, struct kl_backend_power_state *state);
int kl_backend_power_action(struct kl_backend *backend, unsigned action);	/* POWEROFF・REBOOT・SUSPEND。答えは host->session_answer */

/* session（p006）: sessiond（zedBSD）、logind・seatd・VT（Linux・FreeBSD） */
int kl_backend_session_ready(struct kl_backend *backend);			/* READY → GO（期限つき） */
void kl_backend_session_released(struct kl_backend *backend);		/* 表示を返した後の RELEASED */
int kl_backend_session_logout(struct kl_backend *backend);
int kl_backend_session_authenticate(struct kl_backend *backend, const char *user, const char *password);	/* greeter */
int kl_backend_session_unlock(struct kl_backend *backend, const char *password);			/* lock 画面（review 5） */
int kl_backend_peer_uid(int socket_descriptor, uid_t *uid);	/* p010 で使う。zedBSD は getpeereid（src/libc/openbsd.c:261） */

/* input（p007）: evdev の型（keiland-backend-evdev.h） */
void kl_backend_input_scan(struct kl_backend *backend);
int kl_backend_input_absinfo(int descriptor, uint32_t axis, struct input_absinfo *info);
int kl_backend_input_name(int descriptor, char *name, size_t size);
int kl_backend_input_id(int descriptor, struct input_id *id);
ssize_t kl_backend_input_read(int descriptor, struct input_event *events, size_t capacity);
void kl_backend_input_close(struct kl_backend *backend, int descriptor);

/* display（p008）: backend は Vulkan の API だけを使う */
int kl_backend_display_node(struct kl_backend *backend, int *descriptor, const char **path);
VkResult kl_backend_display_acquire(struct kl_backend *backend, const struct kl_backend_vulkan *vulkan, VkPhysicalDevice physical, VkDisplayKHR display);
void kl_backend_display_release(struct kl_backend *backend, const struct kl_backend_vulkan *vulkan, VkPhysicalDevice physical, VkDisplayKHR display);

/* GPU の buffer（p009）: §3.5 */
```

### 3.4 callback の約束（review 5・16）

| 約束 | 内容 |
| --- | --- |
| 再入しない | callback の中から `kl_backend_*` を呼ばない（呼ぶ時は compositor が flag を立てて event loop の次の回に行う）。backend は callback の前に自分の状態を確定させる |
| 順序 | 呼ぶのは `kl_backend_poll_done`・`kl_backend_tick`・`kl_backend_input_scan` の中だけ（compositor の event loop の thread） |
| pause | logind の `PauseDevice` は `session_paused` の戻りの後に `PauseDeviceComplete`（`seat-logind-linux.c:705`）を backend が送る。compositor は callback の中で出力を止め入力を閉じ終える |
| resume | `ResumeDevice` は新しい fd を持つ（`:603`）。backend は lease を差し替え、`session_resumed` の後に `input_found` で新しい fd を渡し直す。compositor は古い fd を使わない |
| 答えの多重化 | sessiond への要求（AUTH・UNLOCK・POWER・LOGOUT）は**一度に一つ**。答えを待つ間の次の要求は `EBUSY`。答えは `session_answer(request, error)` で要求の種類を付けて返す。今の「lock 中の行は UNLOCK の答え」（`handoff-zedbsd.c:294-298`）は backend の中の待ちの状態で区別する |
| 失敗 | 戻り値の errno。host の callback は失敗を返さない |

### 3.5 GPU の buffer（p009、D2 の決定）

compositor が持つ物: wl_buffer の寿命、OS に依らない画像の型（VkImage・memory・大きさ・format・alpha）、surface の acquire の fence の列、Vulkan の device。
backend が持つ物: GPU の buffer の protocol の全て（global の名前と版、要求の解読、params の検証、import の Vulkan の呼び出し、kernel の記述の解読（zedBSD）、dma-buf の modifier と同期の export（Linux・FreeBSD））。

逆向きの依存は compositor が渡す **protocol の host の interface** で解く。今の 2 つの module が compositor から使う物（§2.5 の右の列の実測）から作った:

```c
/* compositor が backend に渡す（p009）。resource は compositor の wire の object の不透明な handle。 */
struct kl_backend_protocol_host {
	void *data;
	struct kl_backend_resource *(*resource_create)(void *data, struct kl_backend_resource *parent, uint32_t id, unsigned role, uint32_t version);	/* zwl_create */
	struct kl_backend_resource *(*resource_create_server)(void *data, struct kl_backend_resource *parent, unsigned role, uint32_t version);	/* zwl_create_server */
	struct kl_backend_resource *(*resource_find)(void *data, struct kl_backend_resource *any, uint32_t id);	/* zwl_find（同じ client の surface・buffer） */
	void (*resource_destroy)(void *data, struct kl_backend_resource *resource);				/* zwl_object_destroy */
	void **(*resource_private)(struct kl_backend_resource *resource);					/* gpu_private */
	int (*emit)(void *data, struct kl_backend_resource *resource, uint32_t opcode, const void *payload, size_t size);	/* zwl_emit */
	int (*post_error)(void *data, struct kl_backend_resource *resource, uint32_t code, const char *reason);	/* zwl_error・zwl_error_code */
	int (*take_fd)(void *data, struct kl_backend_resource *resource);					/* zwl_take_fd */
	VkResult (*buffer_register)(void *data, struct kl_backend_resource *buffer, VkImage image, VkDeviceMemory memory, uint32_t width, uint32_t height, VkFormat format, uint32_t alpha);	/* zwl_import_adopt・set_alpha */
	int (*surface_fence)(void *data, struct kl_backend_resource *surface, int fd, uint64_t value);		/* surface の acquire の列へ（今の zwl_gpu_commit の中の操作） */
	const struct kl_backend_vulkan *(*vulkan)(void *data);						/* instance・physical・device・proc addr */
};

/* backend が出す（今の zwl-gpu.h の置き換え） */
const char *kl_backend_gpu_global_interface(void);	/* "keiland_gpu_buffer_v1" か "zwp_linux_dmabuf_v1" */
uint32_t kl_backend_gpu_global_version(void);
int kl_backend_gpu_bind(struct kl_backend *backend, const struct kl_backend_protocol_host *host, struct kl_backend_resource *factory);
int kl_backend_gpu_request(struct kl_backend *backend, const struct kl_backend_protocol_host *host, struct kl_backend_resource *resource, uint32_t opcode, const unsigned char *bytes, size_t size);
void kl_backend_gpu_commit(struct kl_backend *backend, const struct kl_backend_protocol_host *host, struct kl_backend_resource *surface, struct kl_backend_resource *buffer);
void kl_backend_gpu_resource_free(struct kl_backend *backend, struct kl_backend_resource *resource);
uint32_t kl_backend_gpu_instance_extensions(const char **names, uint32_t capacity);
uint32_t kl_backend_gpu_device_extensions(VkPhysicalDevice physical, const char **names, uint32_t capacity);
VkExternalFenceHandleTypeFlagBits kl_backend_gpu_frame_fence_type(void);
```

- compositor の `protocol.c` の global の表（`:47-73`、今の 3 番の `ZWL_FACTORY`）は名前と版を backend に問い、bind・要求・commit・object の解放（今の `protocol.c:188`・`:632`・`:1560`、`objects.c:571`）を backend へ渡す。
- `zwl_buffer_layout`（zedBSD の wire の layout）は backend-zedbsd の中に閉じる（C3 の意味は保つ）。
- protocol の host は p009 で作り、拡張（p010）は compositor の中の code なので使わない。WS132・WS113 の protocol も compositor の中に置く。

### 3.6 build と install

| | zedBSD | Linux | FreeBSD |
| --- | --- | --- | --- |
| backend | `libkeiland-backend-zedbsd/sources.mk` が `KL_BACKEND_SOURCES` を定義し、`wayland/Makefile` が明示に include | `libkeiland-backend-linux/Makefile.linux` が `.a`、`KEILAND_LINUX_PACKAGES` の compositor の前 | 同じく `Makefile.freebsd` と `keiland-freebsd.mk` |
| compositor | `wayland/Makefile` の `KEILAND_ZEDBSD_SOURCES`（`:6-9`）を領域ごとに減らす。link（`vmunix.mk:1047-1060`）の `-l:libkeiland.so` は D4 で残してよい（使う物が描画の層と motion だけになったことを p011 で B2 が確かめる） | `wayland/Makefile.linux:57-65` の OS の source を領域ごとに減らす | 同（`:53-58`、libseat は残る） |
| install | backend は install しない | 同 | 同 |
| OS ごとの data | `wayland/linux/apps.conf.in`・`keiland.desktop`・`freebsd/apps.conf.in` は install の data。`wayland/data/` へ移す（p009） | | |
| 移行の終わり | compositor の 3 つの Makefile の source の一覧が同じになる（GPU の buffer も backend なので例外なし） | | |

### 3.7 Guardrail の改訂案（Q1 が `plan/guardrail.md` に適用）

段階的に適用する（review 13）:

| 時 | 適用する文 |
| --- | --- |
| p003 の merge | 「Keiland の OS の境界の改訂予定（2026-10-03）」に「移行中: backend へ移した領域は下の新しい配置の規則に従う。移していない領域は今の規則のまま（WS131 の Phase の表）」を足す |
| p009 の merge（配置の規則の確定） | 下の「配置」の段落で「Keiland の OS の境界」を置き換え、「compositor は libvulkan だけ」に path の改訂を足す |
| p011 の merge（書き手の規則） | 下の「app と設定」の段落を足す |

> **配置**（2026-10-03 ユーザーの決定、WS131）: desktop の OS の抽象化（電源・network・音声・PnP・設定・seat・入力・表示・GPU の buffer）の OS に固有の code は libkeiland-backend にだけ置く。interface と複数の OS が共有する仕組みは `userland/desktop/libkeiland-backend/`、OS ごとの実装は `libkeiland-backend-zedbsd/`・`-linux/`・`-freebsd/`。libkeiland-backend を使うのは compositor だけ。compositor（`userland/desktop/wayland/`）と libkeiland は OS の header（`<uapi/…>`・`<linux/…>`・`<dev/…>`）・`"userland/base/…"`・`ioctl()`・OS の macro の block を持たない。macro の block は `libkeiland-backend/keiland-backend-evdev.h` の evdev の header の選択だけ。backend の OS の tree は自分の OS の header だけを include し、compositor の内部と `<keiland.h>` を include しない。**app の自分の機能のための OS の依存はこの規則の対象外**（D14、2026-10-03 user）: Terminal の pty（`TIOCSWINSZ` は 3 OS で同じ、`openpty` の header が `<pty.h>`／FreeBSD の `<libutil.h>` で違うので macro の block で切り替える）と Files の xattr（`tags.c`・`info.c`・`task.c`、FreeBSD の extattr の読み替えは Files の中）。X server は対象外にせず、Wayland の規格（evdev）の key code の定数を自分の header に持って `<uapi/input.h>` の include を無くす。対象外の一覧は checker の許可の表に置く。
> **app と設定**: app は WiFi・network・音量・電源・PnP・desktop の設定を libkeiland の `kl_system_*`（拡張 `kl_system_manager_v1`）でだけ扱う。`desktop.conf` は compositor だけが書く。compositor が libkeiland を link する時は描画の層と motion だけを使い、Wayland の client の部分を使わない（D4）。

「compositor は libvulkan だけ」: 規則の中身（GPU の UAPI の直の ioctl の禁止）は変えず、「OS 固有の部分は `libkeiland-backend-zedbsd/` に閉じる。確かめは `v1-check.sh`（対象の path を改訂）」に直す。

### 3.8 境界の checker の改訂案（領域の Phase ごとに足し、p009 で確定、p011・p023 で強める）

| check | 今（`check.sh`） | 改訂 |
| --- | --- | --- |
| C1 共通の source の OS の include | libkeiland と wayland の OS の dir を除く、evdev の 1 行の例外（`:11-31`） | wayland の全体・libkeiland の全体。evdev の例外は backend の header へ。app は D14 の許可の表の物を除いて対象 |
| C2 ioctl | evdev の 5 つの例外（`:35-43`） | wayland・libkeiland で 0。app は許可の表（Terminal の `TIOCSWINSZ` など） |
| C3 zedBSD の wire の layout | `zwl_buffer_layout` が `wayland/zedbsd` の外に無い | `<uapi/gpu.h>`・`gpu_image_descriptor`・`zwl_buffer_layout` が `libkeiland-backend-zedbsd/` の外に無い |
| L1〜L3 | OS の macro、OS の tree の include | backend の tree に合わせて path を改訂 |
| B1（新） | — | backend が compositor の header・`<keiland.h>` を include しない、`libkeiland-backend.a` の未定義の symbol に `zwl_`・`kwl_` が無い |
| B2（新、p011 から） | — | compositor の未定義の symbol のうち libkeiland の物が D4 の許可の表（`ui/` の描画の層と部品・motion・scroller・gesture・`kl_version`）の中だけ（3 OS の `nm -u`）。libkeiland の Wayland の client の部分（`kl_system_*`・`kl_app_*`・`kl_window_*`・protocol の wrapper）が 0 |
| B3（新） | — | app と libkeiland が `keiland-backend.h` を include せず backend を link しない |
| B4（新） | — | `nm -D libkeiland.so` の集合が公開の header の関数の集合と一致（exports.map は header から生成、§5.4。review 18） |
| B5（新、p023 から FAIL） | — | 旧名（`kui_`・`KUI_`・`keiland_`・`KEILAND_`（paths.h を除く）・`keiui.h`）の使用が 0 |
| makefile-sync | `Makefile` と `Makefile.linux` の対 | backend の tree は対象外、代わりに 3 つの compositor の Makefile の source の一覧の一致 |
| v1-check | `backend=$dir/zedbsd/gpu-zedbsd.c`（`:14-16`） | backend の path、毒入りの header の compile の対象に compositor の全体 |

`plan/tools/keiland-os-boundary/`・`gpu-boundary/` の変更は Q1 の委任が要る（review 12、各 phase.md に記載）。

---

## 4. compositor の拡張の protocol（p010・p011）

### 4.1 共通の約束（WS113 の表示の設定も載せる案、D13）

1. **snapshot**: object を作ると今の状態を event の列で送り、最後に `done(serial)`。client は `done` までの途中を app に見せない。変化も「変わった item ＋ `done`」。
2. **要求と結果**: 要求は client の `request_id` を持ち、compositor は必ず一度 `result(request_id, applied, saved)` を返す。`applied` は適用の結果、`saved` は記録の結果（WS113 p005 の「applied と saved を別の結果」と揃える、review 10）。値は protocol の列挙で errno の数値を送らない。
3. **error の列挙**: `0 ok`・`1 denied`・`2 unsupported`・`3 busy`・`4 invalid`・`5 unavailable`・`6 failed`・`7 not_saved`。
4. **一度に一つ**（review 8）: network の要求は client と compositor の system bar を合わせて一度に一つ（backend の今の約束、`keiland.h:689-698`）。他は `busy`。鍵の保存 → profile の通知（`KEILAND_NETWORK_REQUEST_PROFILES`、`keiland.h:627`）→ join の 3 段は `save_key` の中で compositor が行い、protocol には出さない。
5. **認可**（D5、review 9）: compositor は client の uid を `kl_backend_peer_uid()` で得て、compositor と同じ uid の client にだけ registry に global を見せる（今の `zwl_ime_global_visible` の仕組み、`protocol.c:433`・`:613`）。socket は 0700 の `XDG_RUNTIME_DIR` にあるので、これは主な防御ではなく、uid の違う client の誤った接続を止める多重の防御。zedBSD は `getpeereid`（`include/libc/unistd.h:124`、`src/libc/openbsd.c:261`）がある（networkd は `SO_PEERCRED` を使う、`userland/base/networkd/main.c:2963`）。WiFi の鍵は log に出さない。
6. **Keiland 以外の compositor**（review 22）: global が無ければ `kl_system_open()` は `ENOTSUP`。Settings は該当の頁を「この desktop では使えない」と表示する。

### 4.2 message の表（案、D15 を採れば名前は `kl_system_*_v1`）

**`kl_system_manager_v1`**（global、version 1、compositor の global の表の 25 番）: request `destroy`・`get_settings(new_id)`・`get_network(new_id)`・`get_audio(new_id)`・`get_power(new_id)`・`get_devices(new_id)`（version 2 で WS113 の `get_displays`）、event `capabilities(bits)`。

| object | request（全て `request_id` を持つ、`destroy` を除く） | event |
| --- | --- | --- |
| `kl_system_settings_v1` | `set(key, value)`・`unset(key)` | `value(key, value)`・`removed(key)`・`done(serial)`・`result` |
| `kl_system_network_v1` | `scan`・`join(ssid)`・`disconnect`・`set_wifi(on)`・`save_key(ssid, key)`（8〜63 文字、compositor の uid の利用者の store、`keiland.h:705-713`）・`query_details` | `state(…)`・`access_point(ssid, rssi, secured)`・`link(…)`・`dns(address)`・`saved(ssid)`・`done(serial, changed)`・`result` |
| `kl_system_audio_v1` | `set_volume(left, right, muted)`・`feedback` | `state(service, reachable, device, rate, channels, left, right, muted)`・`done`・`result` |
| `kl_system_power_v1` | `action(poweroff｜reboot｜suspend)` | `state(source, percent, charging, actions)`・`done`・`result` |
| `kl_system_devices_v1` | `eject(id)` | `device(id, kind, state, name, location)`・`done`・`result`（p023 まで枠だけ） |

- settings の key の検査: 書式（`keiland.h:1082-1086`）と、型のある既知の key の範囲（`wayland/preferences.c:38-50`: `window.opacity` 85〜100・`pointer.speed` 25〜300・`keyboard.repeat.rate` 5〜60・`keyboard.repeat.delay` 150〜1000・`pointer.natural`・`sound.muted` 0/1・`sound.volume` 0〜100・`wallpaper`）。未知の key は書式が正しければ保存。
- 電源（D12、review 4）: zedBSD の sessiond は session の control の socket で `UNLOCK` と `LOGOUT` だけを受け（`sessiond/session.c:309-324`）、`POWER` は greeter の socket だけ（`sessiond/greeter.c:517`）。よって zedBSD の session の中の電源の操作は `unsupported`（`actions` に出さない）。sessiond に session の `POWER` を足すのは WS131 の外（別の WS、ユーザーの判断）。log out は電源ではなく session の領域（`kl_backend_session_logout`）で、拡張に出すかは p010 で決める。
- protocol の `wl_interface` の表は libkeiland の中で static にし、外に出さない（review 17）。

### 4.3 compositor の側: 設定の記録と stat の除去

| 段 | 内容 |
| --- | --- |
| p010 | `wayland/settings-store.c`（`libkeiland/preferences.c` の書式・lock・読み直し・rename を移す）。拡張の `set`・`unset` と system bar の音量を store で記録。**毎秒の監視（`preferences.c:237-360`）は残す**: Settings が p011 まで `desktop.conf` を直接書くので、監視を除くと Settings の変更が compositor の再起動まで効かなくなる（review 1）。二つの書き手は今と同じ「lock・読み直し・変えた key だけ・rename」（`keiland.h:1082-1084`）で互いの変更を失わない（今も `volume.c:703-706` と Settings が書いている） |
| p011 | Settings を拡張へ移すのと**同じ Phase で**監視の thread と `PREFERENCES_CHECK_MS` を除く。`settings-p007.sh` を「拡張で変えて数秒で適用」に書き換える |
| 読み | 起動時に一度だけ（最初の frame の前） |
| 書き | event loop で disk を待たない（BUG-125 の教訓、`preferences.c:16-20`）。**worker の thread**（review 7）が store の書き、network の `save_key`・`get_saved`・`query_details`（`wayland/network.c:1164`・`:1283` の今の同期の読み書きを含む）を受ける。同じ key の連続の変更は 250 ms でまとめる。終了・log out・電源断の前に flush して待つ（review 10） |
| 手での編集 | 監視しない（D6）。次の起動で読まれる |
| WS113 | `displays.conf` も同じ store と worker（WS113 p005 の契約） |

### 4.4 libkeiland の client の API（`kl_system_*`、p010）

```c
struct kl_system *kl_system_open(struct wl_display *display);		/* global が無ければ NULL・ENOTSUP。p015 の後は kl_app_system(app) */
void kl_system_close(struct kl_system *system);
int kl_system_dispatch(struct kl_system *system, unsigned *changed);	/* 待たない */
unsigned kl_system_capabilities(const struct kl_system *system);
int kl_system_take_result(struct kl_system *system, uint32_t *request_id, int *applied, int *saved);
int kl_system_settings_get(const struct kl_system *system, const char *key, char *value, size_t size);
int kl_system_settings_get_int(const struct kl_system *system, const char *key, int fallback, int minimum, int maximum);
int kl_system_settings_set(struct kl_system *system, const char *key, const char *value, uint32_t *request_id);
int kl_system_settings_unset(struct kl_system *system, const char *key, uint32_t *request_id);
/* network: get_state・get_scan・request・save_key・query_details・get_links・get_dns・get_saved（data の struct は今の公開の物を kl_ の名前で） */
/* audio: get_state・set_volume・feedback。power: get_state・action。devices: get・eject */
```

### 4.5 他の WS との共有

- WS113 p005: version 2 の `get_displays` と `kl_system_display_*`（D13）。WS131 p010 の merge の後に始める。
- WS132: kernel の `/dev/system` → backend-zedbsd の device 領域 → `kl_system_devices_v1`（p023）。Linux は udev の netlink、FreeBSD は devd（WS132 の範囲 4）。自動 mount の daemon（WS132 の範囲 2）は WS132 の物で、backend はその daemon の通知も受けられる形にする（review 22）。

---

## 5. 名前と libkeiui の吸収

### 5.1 名前の規則（2026-10-03 user、最終）と段

| 対象 | 今 | 最終 | 段 |
| --- | --- | --- | --- |
| 旧 libkeiui の関数・型・変数 | `kui_*`（純粋な名前 178） | `kl_*` | p013（`keiui.h` は互換の macro） |
| 旧 libkeiui の macro・enum の定数 | `KUI_*`（163） | `KL_*` | p013 |
| libkeiland の関数・型・変数 | `keiland_*` | `kl_*` | p014（互換の macro） |
| libkeiland の macro・enum の定数 | `KEILAND_*`（keiland.h） | `KL_*` | p014 |
| install の path の macro | `KEILAND_BINDIR`・`KEILAND_DATADIR`・`KEILAND_LIBEXECDIR`・`KEILAND_SYSCONFDIR`（`paths.h`）と make の `KEILAND_PREFIX` | そのまま | — |
| 新しく作る名前 | — | 最初から `kl_backend_*`・`kl_system_*`・`kl_app_*`・`KL_*` | p003〜 |
| compositor の内部 | `zwl_*`（494）・`ZWL_*`（271）、`zwl.h`・`zwl-*.h` | `kwl_*`・`KWL_*`、`kwl.h` | p021（[rename-map-kwl.md](rename-map-kwl.md)） |
| compositor の log の接頭辞 | `"ZWL "`（63 種類） | `"KWL "` | p022（試験 201 本と一緒） |
| Wayland の protocol の名前 | `keiland_*_v1`・`KEILAND_*_V1_*`（libwayland の protocol の header に 74） | `kl_*_v1`・`KL_*_V1_*`（D15） | p021（compositor・libkeiland・libwayland の header・ime を同時に） |
| 共有の source の内部 | `keiland_picture_*`・`keiland_color_glyph`・`keiland_mark_*` | `kl_*`（D16） | p014 |
| app・libbrowser の include guard | `KEILAND_FILES_H`・`KEILAND_BROWSER_*_H` など | そのまま（D16） | — |
| 公開の header | `keiland.h`・`keiui.h` | `keiland.h` 一つ | p013 で `keiland-ui.h`、p023 で `keiland.h` に統合 |

[rename-map.md](rename-map.md) は `keiland.h`（266）と `keiui.h`（341）の 607 の名前の最終の対応で、解決の要る衝突は 0。

| 衝突の扱い | 内容 |
| --- | --- |
| 一本化 | `KUI_EDIT_*` 12・`KUI_KEYBOARD_INSET_*` 3 と同名の `KEILAND_*` は値が同じ → `KL_*` 一つ |
| 除く | `kui_version`・`KUI_VERSION`（`kl_version`・`KL_VERSION` 一つ、版は上げない） |
| 別名 | `kui_edit_fn` → `kl_window_edit_fn`、`kui_keyboard_inset_fn` → `kl_window_keyboard_inset_fn`（`keiland_edit_fn`・`keiland_keyboard_inset_fn` と型が違う） |
| 再利用 | `kui_file_chooser*` → `kl_file_chooser*`（旧 `keiland_file_chooser_*` は KEILAND_VERSION 12〜15、`keiland.h:1066-1070`） |
| 近い名前（衝突ではない） | `kl_scroll_*`（旧 `kui_scroll`）と `kl_scroller_*`、`kl_window_*`（旧 `kui_window`）と `kl_window_menu_*` |

既存の tree に `kl_`・`KL_`・`kwl_`・`KWL_` の名前は無い（2026-10-03 の grep、Q1 も確認）。`kui_` で始まる field・変数の名前は app・ime・kuidemo・files で 0 件。

### 5.2 互換の方式（source の互換だけ、ABI の互換は持たない）

- ユーザーの決定で ABI は変えてよく版も上げないので、旧名の互換は**段階的な移行の間の source の互換**だけ。旧名の symbol は library に残さない。全ての利用者は tree の中で同じ build で作り直す。
- p013: `keiui.h` を `#include <keiland.h>` と `#define kui_X kl_X`・`#define KUI_X KL_X` の列（rename-map から生成）にする。p014: `keiland.h` の終わりに旧 `keiland_*`・`KEILAND_*` の互換の macro の block（`KL_COMPAT` が定義されていない時だけ、p023 で除く）。
- macro の互換は旧名を局所の名前に使う code を壊しうる。各段の全 build で確かめる。
- Linux・FreeBSD の source からの install に残る古い `libkeiui.so` は install の規則で消す（p012）。deb・rpm は file の一覧から消える。

### 5.3 libkeiui を移す（p012、名前は変えない）

- `git mv userland/desktop/libkeiui/* userland/desktop/libkeiland/ui/`（`edit.c`・`scroll.c`・`version.c` が libkeiland と同名なので subdirectory。`version.c` は p013 で除く）。
- 取りこぼしの無いように直す所（review 2）: Files の `ui-scrollbar.c:22` の include と 3 本の Makefile の `libkeiui/scroll-bar.c`、package 名 `libkeiui` の 5 か所（§2.4）、`vmunix.mk` の libkeiui.so の規則と 7 つの app の link、FreeBSD の公開の header の表（`keiland-freebsd.mk:184-187`）、`keiland-linux.mk:109`、`elf-check.sh:17`、§2.4 の host の試験 13 file の path。
- link: libkeiland.so に libpng-compat・libz-compat・libvulkan（と Linux では libtruetype）が加わる。xserver・probe・wlshm もこれらを load する（libkeiui.so の大きさは 173,656 byte、2026-10-02 の build）。compositor も libkeiland を link するので、これらを load する（D4、compositor は既に libtruetype・libpng-compat・libz-compat・libvulkan を link している、`vmunix.mk:1047-1060`）。

### 5.4 exports.map

公開の header の関数の一覧から生成する（glob にしない）。`picture/color-glyph.c` などの共有の source の内部の名前と、protocol の `wl_interface` の表を出さない。checker B4 が `nm -D` と header を照合する（review 18）。

### 5.5 ABI と版

版の段階は持たない（ユーザーの決定）。`KL_VERSION` は 21 のまま、`kl_version()` は残す。soname は今の通り版なし。

### 5.6 compositor の改名（p021・p022）

- p021: `zwl_`→`kwl_`・`ZWL_`→`KWL_`（[rename-map-kwl.md](rename-map-kwl.md)、backend へ移った境界の名前は既に `kl_backend_`）、`zwl.h`→`kwl.h`。compositor の source を compile する道具（`plan/tools/gpu-boundary/`・`plan/tools/titlebar/`・`plan/ws035/tests/`・`plan/ws102/tests/`・`plan/tools/keiland-freebsd/` に 1 つずつ）も直す。log の文字列は変えない。D15 を採れば protocol の名前もここで。
- p022: log の接頭辞 `"ZWL "` を `"KWL "` に、それを読む試験 201 本を同じ commit で直す。前後で同じ試験を流して結果が同じことを確かめる。

---

## 6. app の骨組みの API（p015）

- 吸収で一つの libkeiland の中の層になる: `ui/` の描画・部品と、その上の `kl_app`・`kl_window`。p013 で旧 `kui_window_*` は `kl_window_*`。p015 は `kl_app` を足し、窓を app の下に置く。既存の `kl_window_open()` は暗黙の app を作る簡便の形として残す。
- desktop の OS の機能は `kl_app_system(app)` の `struct kl_system`。

```c
struct kl_app *kl_app_open(const struct kl_app_options *options);		/* registry は一つ、bind は一度の roundtrip */
void kl_app_close(struct kl_app *app);
int kl_app_dispatch(struct kl_app *app, int timeout_ms);
int kl_app_watch_fd(struct kl_app *app, int fd, unsigned events);
int kl_app_take(struct kl_app *app, struct kl_app_event *event);
struct kl_system *kl_app_system(struct kl_app *app);
struct kl_window *kl_app_window_create(struct kl_app *app, const struct kl_window_options *options);
int kl_window_set_menu(struct kl_window *window, const struct kl_menu_entry *entries, size_t count);
int kl_window_set_controls(struct kl_window *window, const struct kl_control_entry *entries, size_t count);
int kl_window_set_action_state(struct kl_window *window, uint32_t action, unsigned state);
int kl_window_popup_menu(struct kl_window *window, const struct kl_menu_entry *entries, size_t count, int x, int y);
int kl_window_set_glass(struct kl_window *window, const struct kl_glass_panel *panels, unsigned count);
#if defined(VK_VERSION_1_0)	/* Vulkan の header を先に include した利用者だけが見る（review 20） */
int kl_window_vulkan_surface(struct kl_window *window, VkInstance instance, VkSurfaceKHR *surface);
#endif
```

- 入力は pull 型（dispatch → take）を主にする。`struct kl_menu_entry` は今の各 app の `struct menu_item` と同じ形。
- accessor（旧 `kui_window_display`・`_surface`・`_seat`、`keiui.h:811-817`）は残す。
- WS097/096 のために、複数の窓・popup・子の窓・cursor・scale を API の形で塞がない。

---

## 7. Phase の計画

### 7.1 順序（文の表）

| 線 | 順 | 説明 |
| --- | --- | --- |
| backend | p003 → p004 → p005 → p006 → p007 → p008 → p009 | 1 領域ずつ。各 Phase の受け入れに 3 OS の build と回帰。前の Phase の cleared と main への統合を確かめてから次 |
| 拡張 | p004 → p010 → p011 | p010 は network（p003）と音声（p004）と電源（p005）を要る。D8 の決定で番号の順に流すので、p010 の時には p003〜p009 が済んでいる |
| toolkit・名前 | p003 の merge → p012 → p013 → p014（p011 も要る）→ p015（p010 も要る）→ p016 → {p017, p018, p019（p011 も要る）}、p018 → p020 | p014 は p011 の後（旧 network・audio・preferences を除いた後に改名して手間を減らす） |
| compositor の名前 | p009・p011 → p021 → p022 | backend へ移す前に改名すると移す file を二度触る |
| browser | p020 → p025 → p023 | D7。WS074 との衝突は開始の前に Q1 がユーザーに確認 |
| 終わり | p016〜p020・p025・p022 → p023 → p024 | 番号の順ではなく表の順（p025 は p020 の後・p021 の前でも後でもよいが、p023 の前） |

### 7.2 実行の時期と前提（D8 の決定）

- **実行の順**: p003〜p020 → p025 → p021〜p024 の順に一つずつ流す（全ての依存はこの順で満たされる、§7.1）。
- **体制（2026-10-03 user で更新）**: WiFi の残り（P1）と WS131（P3）を並行（N=2）。P1 の作業と重なる path（`libkeiland/zedbsd/network-zedbsd.c`・`network-link-zedbsd.c`・`libkeiland/linux|freebsd|wpa/`・`wayland/network.c`・`wayland/Makefile*`）は p003 の間 P3 に予約（Q1）、統合の後に Q1 が P1 に新しい path を伝える。p011（Settings）は P1 の WiFi の後。
- **区切り**: p011 の cleared の後に、Q1 がユーザーに進み具合（backend の 7 領域、拡張、毎秒の監視の除去、BUG-125 の p076 の 20 回）を報告する。続けるかはその時のユーザーの指示に従う。
- **開始**: ユーザーの承認と P2（WS099）の終了の後に Q1 が指示する。

前提の確認（2026-10-03、main `7b8b2217a` を取り込んだ worktree で）:

| 前提 | 状態 |
| --- | --- |
| P1 の `libkeiland/zedbsd/network-zedbsd.c` の変更が main にある（p003） | **満たされた**。P1 の q627 は main に統合済み（`48237bb2a`）。q627 の統合は `network-zedbsd.c` に触れておらず、この file の最後の変更は `6efb4f2bb`（2026-10-01）。P1 の worktree（`agent/p1`、HEAD `c6bba607b`）に libkeiland・`settings/network.c`・`wayland/network.c` の未 commit の変更は無い。p003 の開始の時に Q1 がもう一度確かめる |
| P2 の BUG-125 の作業の merge（p010・p011） | P2 の終了の後に開始するので、開始の時に満たされる |
| WS090（D9） | WS131 の間は動かさない。WS131 の完了の後に残り（Files の描画の層・scroll・規約）を扱う |
| WS113 | WS113 p005 は WS131 p010 の後。N=1 の間は WS113 も動かない |
| WS132 | p023 の時に WS132 の成果が無ければ、PnP は残件として WS132 へ |
| ベータ1（10/17）の標準 app | ベータ1 の日程との関係は Q1 がユーザーと決める |
| WS074（browser、Codex が作業中） | p025 の開始の前に Q1 がユーザーに衝突を確かめる |
| WS105・WS109・WS112 | 各 Phase で `make keiland-linux`・`makefile-sync.sh`・`elf-check.sh`。package の一覧の変化（`libkeiui.so` の消滅、p012）を Q1 に伝える |

### 7.3 回帰の範囲

記号: ● 必須、○ 変えた所に関わる時。zedBSD は [zedbsd-commands.md](../tools/keiland-linux/zedbsd-commands.md)、Linux は [keiland-linux/README.md](../tools/keiland-linux/README.md)、FreeBSD は [keiland-freebsd/README.md](../tools/keiland-freebsd/README.md)。全ての Phase で 3 OS の build（zedBSD warning 0、`make keiland-linux` の gcc と clang、FreeBSD の native build と `native-build-audit.py`）と `check.sh` を流す（表では省く）。

| Phase | zedBSD | Linux | FreeBSD（D11） |
| --- | --- | --- | --- |
| p003 network | boot-test、C1・C2・C9、`settings-regress.sh`、system bar の WiFi の手順 | guest の `network-probe`、`wifi-setup.sh` の hwsim、compositor の PNG | 起動できれば system bar の PNG |
| p004 音声 | boot-test、`host-audio.sh`・`volume-p004.sh`・`volume-p005.sh`、`settings-regress.sh` | `audio-probe` | ○ |
| p005 電源 | boot-test、greeter の電源の button（`zdesktop-p101.sh` の image の greeter で） | unsupported の確認（logind は p006） | unsupported の確認 |
| p006 seat・session | boot-test、C1・C2・C9、`zdesktop-p101.sh`（login・log out）・`zdesktop-p102.sh`〜`p104.sh`（lock・unlock） | logind と直の seat、VT の切り替えの後の復帰、`seat-fd.c`・`dbus-wire`、logind の Reboot（使い捨ての guest） | ○ |
| p007 入力 | C1・C2・C9（pointer・key・touch・pen）、`demo-s8-s9.sh` | QMP の入力 | ○ |
| p008 表示 | boot-test、C1・C2・C9 | KMS の確認（`display-probe.c`） | ○ |
| p009 GPU の buffer | boot-test、C1・C2・C9、GPU の境界（§6、forge・fence、v1-check） | `dmabuf-probe`・`dmabuf-forge`・`wsi-check.sh`・acquire-fence | `dmabuf-export-rejected.c`・`wsi-window` |
| p010 拡張 | 新しい host の試験、拡張の probe の guest、`settings-regress.sh`（旧経路）、volume | guest の probe | ○ |
| p011 Settings | `settings-regress.sh`・書き換えた `settings-p007.sh`、volume、C9 の p076（BUG-125）を単独で 20 回 | Settings の起動、向け直した `network-probe`・`audio-probe`・`lib-smoke` | ○ |
| p012・p013・p014 | ws090 の host 試験・host-chooser・`textinput-p013.sh`・`viewers-p008.sh`・`demo-s8-s9.sh`・Files の `files-regress.sh`・boot-test | 全 app の起動の PNG・`elf-check.sh` | ○ |
| p015 | 新しい host の試験、kuidemo、Text Editor の試験 | kuidemo | ○ |
| p016〜p020 | その app の既存の試験（各 phase.md） | その app の PNG | ○ |
| p021・p022 | C1・C2・C9、GPU の境界、boot-test。p022 は log を読む 201 本のうち C 基準と各 WS の代表を前後で | compositor の PNG | ○ |
| p023・p024 | 全て | 全て | 全て |

QEMU の証拠と実機の証拠を分ける。WS131 は実機を受け入れの条件にしない。QEMU の console・serial の log で判定しない。

### 7.4 rollback（review 14）

- 基本は**前へ直す**（forward fix）。統合の後に他の WS の変更が同じ file に乗ると revert できないため。
- revert（`git revert -m 1` で統合の commit を戻す）は、その Phase の後に同じ file への他の変更が無い時だけ。後ろの Phase から順に戻す。
- data の移行は無い（`desktop.conf` の書式は変えない）。p012 を戻すと Linux・FreeBSD は再 install が要る。

---

## 8. 他の WS への影響（Q1 が各 WS に反映）

| WS | 影響 |
| --- | --- |
| WS090 | libkeiui が無くなる。p007（Settings の窓）・p010（Files の窓）は WS131 p019・p020 へ。p009（Files の描画の層）・p015（scroll）・p012（規約）は **WS131 の完了の後に** `kl_` の名前で扱う。WS131 の間は WS090 を動かさない（D9 の決定） |
| WS104・WS105・WS109 | 完了した WS の境界の規則と道具を WS131 が改訂（WS の記録は変えない） |
| WS113 | p005 は `kl_system_manager_v1` の `get_displays`（D13）、WS131 p010 の後 |
| WS132 | device 領域と `kl_system_devices_v1` が受け口（p023） |
| WS112・WS108 | `libkeiui.so` が消える（p012）。backend は install しない |
| WS099 | BUG-125 の原因の除去は p011 |
| WS097・WS096 | §6 の要件 |
| WS131 自身 | ws.md の題「libkeiland を GUI toolkit 兼 desktop 機能の抽象化層にする」は分解の決定の後の中身と合わない。「libkeiland-backend の分離と libkeiui の吸収」への改題を Q1 に依頼（review 22） |

---

## 9. ユーザーの判断（2026-10-03 のレビューで D1・D3・D5・D6・D10〜D13・D15・D16 は推奨のとおり承認、D4・D7・D8・D9 は下の決定）

| # | 問い | 選択肢 | 推奨と理由 | 止まる Phase |
| --- | --- | --- | --- | --- |
| D1 | backend の形 | 静的な内部 library ／ 共有 `.so` | 静的: install・package の変更が無い（§3.2） | p003 |
| D3 | evdev の型と code | macro の選択を backend の header へ ／ 独自の code の表 | 前者: 合意済みの例外の場所が移るだけ | p007 |
| D4 | compositor と libkeiland | **決定（2026-10-03 user）**: compositor は touch IME の UI のために libkeiland を link してよく、使わなくなれば link しない（2026-10-03 user）。循環を作らない条件: (a) compositor は libkeiland の Wayland の client の部分（`kl_system_*`・`kl_app_*`・`kl_window_*`・file chooser・protocol の wrapper の menu・titlebar・glass・desktop・keyboard inset・edit・recent）を使わない（compositor は server で、使えば自分に client として接続する形になる）。使ってよいのは `ui/` の描画の層と部品（canvas・text・icons・theme・widgets・field・list・cards・scroll）と motion・scroller・gesture・`kl_version` だけ。(b) libkeiland は compositor・backend の header と symbol を使わない。(c) 拡張の protocol の定義（opcode・型の定数）は `userland/desktop/keiland/` の共有の header に置き、compositor は libkeiland の拡張の client の code を呼ばない。(d) `desktop.conf` の store は compositor の中だけにあり、libkeiland に書き手を置かない。checker B2 が (a) を 3 OS の `nm -u` で確かめる | — | p010・p011 |
| D5 | 拡張の認可 | registry で同じ uid ／ 許可した program | 同じ uid（多重の防御、§4.1） | p010 |
| D6 | 手での `desktop.conf` の編集 | 監視しない ／ 再読み込みの要求 | 監視しない | p011 |
| D7 | browser の shell の窓の移行 | **決定（2026-10-03 user「D7,browserのshellもlibkeilandで書きましょう。」）**: 入れる。p025（p020 の後・p023 の前）。libbrowser は触らない。WS074（Codex が作業中）との衝突は p025 の開始の前に Q1 がユーザーに確かめる | — | p025 |
| D8 | 実行の時期 | **決定**: 単独走行（N=1）で p003〜p024 を番号の順に、p011 の後に Q1 の報告の区切り（§7.2） | — | — |
| D9 | WS090 の残り | **決定**: WS131 の完了の後に扱い、WS131 の間は WS090 を動かさない。Settings と Files の窓は p019・p020 | — | — |
| D10 | 移動の例外 | WS106・WS107 と同じ範囲を限った例外 ／ 全て全文規約に直す | 例外: 移す既存の code は約 2.8 万行（libkeiui 16,236・libkeiland の OS 5,422・compositor の OS 6,591）。**「変えない移動」に機械的な改名（rename-map の適用）と path・include の修正を含める**と明記する（review 11）。変えた関数と新しい code は全文規約。正本は `plan/standards/`（Q1） | p003・p012 |
| D11 | FreeBSD の実行の証拠 | build と監査は必須、起動は passthrough なしで ／ passthrough を許す | 前者（2026-10-03 の Guardrail） | p003〜p009 |
| D12 | 電源の操作 | zedBSD の session は unsupported、sessiond の拡張は別の WS ／ WS131 で sessiond を拡張 | 前者: sessiond は session の socket で POWER を受けない（§4.2）。Linux は logind、FreeBSD は unsupported | p005 |
| D13 | WS113 の表示の設定 | 同じ manager の version 2 ／ 独自の global | 同じ manager | WS113 p005 |
| D14 | Guardrail の範囲 | **決定済み（2026-10-03 user）**: desktop の OS の抽象化に限る。Terminal の pty は Terminal に残し macro の block（`terminal/main.c:37`・`:42`）、X server は key code の定数を自分の header に（`xserver/keymap.c` の `<uapi/input.h>` を除く、p009）、Files の xattr は app 側に残し情報の panel は名前だけ（p020） | — | p009・p018・p020 |
| D15 | protocol の名前 | `kl_*_v1` に変える（p021） ／ `keiland_*_v1` のまま | 変える: 「Keiland 関連はすべて KL_」に揃う。全ての client は tree の中で、ABI は変えてよい | p010（新しい拡張の名前）・p021 |
| D16 | 共有の source の内部の名前と include guard | library・compositor・共有の source は `kl_`、libbrowser と app の include guard は対象外 ／ 全て | 前者: libbrowser は別の component の規約（WS107）で、include guard は利用者に見えない | p014 |

---

## 10. 未確認と限界

- 設計は source を読んだだけで、build・試験・QEMU は流していない。
- interface（§3.3・§3.5）は案。各領域の Phase が 3 OS の実装と一緒に確定する。protocol の host は今の 2 つの module の実測から作ったが、p009 の最初に `protocol.c`・`objects.c`・`import.c` の呼び出しを読み直して足りない操作を足す。
- zedBSD の peer の uid は `getpeereid` がある（§4.1）。zedBSD の compositor の socket で動くかは p010 で確かめる。
- FreeBSD の guest で passthrough なしに compositor を起動できるかは未確認（D11）。
- 見積もりは review 15 の幅（85〜120 h）。backend の各領域の Phase は 3 OS の回帰の時間が支配的。

---

## 11. design-reviewer の review への対応（Q1 の要約の番号）

| # | 指摘 | 対応 |
| --- | --- | --- |
| 1 | p006（旧）と p007（旧）の間の退行 | 監視の除去を p011（Settings の移行と同じ Phase）へ。`settings-p007.sh` の書き換えを p011 の範囲に（§4.3） |
| 2 | Files・config・FreeBSD の header の取りこぼし | §2.4・§5.3、p012 の範囲と受け入れ |
| 3 | Guardrail が app を縛りすぎる | D14（ユーザーの決定）、§3.7 の配置の段落に対象外を明記、checker の許可の表（Terminal の pty、Files の xattr）、X server の key code の定数は p009 |
| 4 | D12 の前提の誤り | §4.2 の電源、D12 を改訂（zedBSD の session は unsupported） |
| 5 | UNLOCK と答えの多重化 | `kl_backend_session_unlock`、§3.4 の答えの多重化、p006 の回帰に `zdesktop-p102.sh`〜`p104.sh` |
| 6 | 旧 API を除くと壊れる道具 | §2.6、p011 の範囲 |
| 7 | event loop の disk と ioctl の待ち | §4.3 の worker の thread（p010） |
| 8 | network の要求の多重化と PROFILES | §4.1 の 4 |
| 9 | 認可の実効性と zedBSD の peer | §4.1 の 5、D5 |
| 10 | 書き込みの flush と applied/saved | §4.1 の 2、§4.3 |
| 11 | D10 の範囲 | D10 に明記 |
| 12 | 所有 path | 各 phase.md に「Q1 の委任が要る」 |
| 13 | Guardrail の適用の時期 | §3.7 の段階 |
| 14 | rollback | §7.4 |
| 15 | 見積もりと p008（旧）の分割 | §0.2（85〜120 h）、吸収を p012（移す）と p013（改名）に |
| 16 | callback の約束 | §3.4 |
| 17 | protocol の名前と interface の表 | `kl_system_*_v1`（D15）、表は static（§4.2） |
| 18 | B4 と改名表の矛盾 | B4 を header との照合に（§3.8・§5.4） |
| 19 | keiui.h を消すと壊れる試験 | §2.4 の 13 file、p023 の範囲 |
| 20 | 公開の header の Vulkan の型 | `VK_VERSION_1_0` の条件で宣言（§6） |
| 21 | zedBSD の fragment の名前 | `sources.mk`（§3.1） |
| 22 | その他 | probe の試し方は p010 の phase.md、Keiland 以外の compositor は §4.1 の 6、WS132 の範囲 2 は §4.5、ws.md の題は §8、実行者は各 phase.md で「Q1 が割り当てる（high）」 |
| 事実 | 誤りの訂正 | `KEILAND_VERSION` は `keiland.h:49`、chooser の版は `keiland.h:1066-1070`、`KEILAND_LINUX_PROGRAM` は `keiland-linux.mk:57`、libkeiland.so は zedBSD で既に libtruetype に依存、shader は 2 と `regenerate.py`、§2.4 の Files の分類、§4.1 の peer の uid、D12、rename-map の行番号（comment を除いて数え直した） |
