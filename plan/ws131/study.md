# WS131 p001 検討: libkeiland を GUI toolkit 兼 desktop の抽象化層にできるか

2026-10-03、読み取りの調査（source・build・QEMU は無し）。対象の tree は main `bf3abb3a5`。行数は `wc -l` の目安。

## 0. 結論（先に）

- **実現できる。** 窓の土台はすでに半分以上が共有されている（libkeiui の `kui_window` を Text Editor・Image Viewer・PDF Viewer・Notes・Terminal の 5 つが使う）。
  残る重複は「窓の周りの desktop の部品の結線」（titlebar・System Menu・context menu・glass・action の queue・main loop・touch・独自の registry）と、
  `kui_window` に移っていない Files・Settings・browser の shell の自前の窓（libkeiui の `present.c` のほぼ写し）である。
- **推奨は案 B（層を分ける）**: libkeiland を「app の骨組み」（接続・registry を一つに・event loop・窓・入力・clipboard・DnD・IME・menu・titlebar・glass・desktop の設定）
  の公開 API にし、libkeiui は widget と描画（canvas・text・theme・scroll・ui・部品・file chooser）に専念する。依存の向きは今と同じ libkeiui → libkeiland。
  app から見ると「libkeiland が toolkit（窓と desktop）、libkeiui がその上の部品」になる。
- 吸収する案 A は、zdesktop・xserver・probe が link する libkeiland に widget・font・画像の依存を入れ、WS090 の決定 J1 を逆にする割に、得るものが名前の一本化だけなので推奨しない。

## 1. 棚卸し

### 1.1 app ごとの分類

記号: **K** = libkeiland、**U** = libkeiui、**自** = app の自前、— = 無し。

| 項目 | Files | Settings | Text Editor | Image Viewer | PDF Viewer | Notes | Terminal | browser（shell） |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Wayland の接続・registry・xdg-shell | 自（`window.c` 1340。toplevel と `--desktop` の desktop surface の 2 通り） | 自（`window.c` 1350、Files の写し: 差 528 行） | U `kui_window_open` | U | U | U ＋ 自の registry（tablet） | U ＋ 自の registry（data device・primary） | 自（`window.c` 1176） |
| 描画の present | 自 Vulkan WSI（`present.c` 1243。libkeiui の `present.c` と差 142 行） | 自 Vulkan WSI（1218、Files と差 141 行） | U `KUI_PRESENT_VULKAN` | U `NONE` ＋ 自 Vulkan（`present.c` 1702、画像の GPU 表示） | U `VULKAN` | U `NONE` ＋ 自 Vulkan（stroke） | U `NONE` ＋ 自 Vulkan（文字） | 自 Vulkan（`present.c` 648、libbrowser の描画を載せる） |
| canvas・text（CPU） | 自（`canvas.c` 1396・`text.c` 741、libkeiui と差 183・197 行） | Files の `canvas.c`・`text.c`・`icons.c` を source のまま compile | 自 `canvas.c` 365・`text.c` 786 ＋ U（handle・dialog・chip） | 自 283・295 | 自 350・277 | GPU | GPU | libbrowser |
| titlebar（keiland_titlebar） | K ＋ 自の結線 453（drop target・progress・breadcrumb） | K ＋ 結線 358（検索） | K ＋ 結線 313（find field） | K ＋ 結線 369 | K ＋ 結線 321 | K（`window.c` 内） | K（`tabs.c`、tab） | K ＋ 結線 349（URL field） |
| glass（keiland_glass） | K ＋ 結線 152 | K ＋ 162（Files と差 56） | K ＋ 112 | K ＋ 191 | — | — | — | — |
| System Menu・context menu | K ＋ 自 `menu.c` 897（context あり） | K ＋ 371 | K ＋ 540（context あり） | K ＋ 550（context あり） | K ＋ 365 | K ＋ 316 | K ＋ 413 | — |
| 入力 pointer・key・touch | 自（wl_pointer/keyboard/touch の listener） | 自 | U の event の queue | U | U | U（pointer を筆の入力へ変換） | U（key を bytes へ） | 自 |
| key の repeat | 自 | 自 | U（BUG-111 の順序） | U | U | しない | U | 自 |
| touch の scroll・gesture | K gesture/scroller ＋ 自 `touch.c` 666 | U `kui_scroll_bar` のみ | U `kui_scroll`・`kui_text_touch` | K ＋ 自 783 | K ＋ 自 774（Image Viewer と差 329） | K ＋ 自 1216 | K ＋ 自 611 | K ＋ 自 601 |
| clipboard・primary | 自 `clip.c` 205 | — | U | — | — | — | 自 `clipboard.c` 808・`primary.c` 330 | — |
| DnD | 自 `dnd.c` 909（source と drop） | — | — | — | — | — | 自（clipboard.c 内、drop のみ） | — |
| IME（text-input-v3） | — （field は自前の US の表） | — | U `kui_window_text_input` | — | — | — | — | — |
| file chooser | （app 自身が file manager） | — | U | U | U | U | — | — |
| frame の駆動 | 自 loop | 自 loop | 自 loop（約 150 行: repeat_wait→dispatch→repeat→take→app の queue→refresh→present） | 同型 | 同型 | 同型（window.c） | `dispatch_fds`（pty） | 自 |
| 設定の読み込み | — | K `keiland_preferences`（書く側） | — | — | — | — | — | — |
| 最近の file | K `keiland_recent` | — | K | K | K | K | — | — |
| desktop surface | K `keiland_desktop`（`--desktop`） | — | — | — | — | — | — | — |

その他の利用者: keiland-ime は libkeiui の canvas・text だけを使い、`keiland_ime_status_v1` を libkeiland を通さず直接 bind している（keiland.h の冒頭の方針からは外れている。WS131 の範囲外だが、抽象化層にするなら後で揃える対象）。

### 1.2 重複の量（目安）

| 重複 | 量 | 共有の先 |
| --- | --- | --- |
| 自前の窓と present（Files・Settings・browser shell） | 約 6,400 行（Files 2,583・Settings 2,568・browser 1,824）。うち Files・Settings の present は libkeiui の写しで差 140 行ほど | `kui_window`（今）／案 B の `keiland_window` |
| titlebar・glass・menu の結線（7 app） | 約 6,600 行（titlebar 2,500・glass 730・menu 3,450）。どれも「静的な表 → model を作る → 状態の差分だけ送る → 選ばれた action を app の queue に積む」の同じ形 | 表を受ける宣言的な API |
| app の action の queue と main loop | app ごとに 150〜300 行 ×6 | `keiland_app` の loop |
| touch の結線（gesture・scroller・16 ms の tick・時刻の補正） | 約 4,650 行（6 app）。半分ほどは app 固有の意味（swipe・pinch・ペンと palm） | libkeiui の `kui_scroll`（WS090 p015 の案）と、touch の view の部品 |
| CPU の canvas・text の写し | 約 4,500 行（Files・Text Editor・Image Viewer・PDF Viewer。中身は少しずつ違う） | `kui_canvas`・`kui_text`（WS090 p007/p009） |
| 独自の registry（Notes の tablet、Terminal の data device・primary、libkeiland の各 object） | libkeiland の `titlebar.c`・`menu.c`・`glass.c`・`edit.c`・`keyboard-inset.c`・`desktop.c` が**それぞれ** registry と roundtrip を作る。Text Editor は起動で 5〜6 回 | 一つの registry を共有 |

### 1.3 差（独自の拡張・癖）

- Files: 窓の役割が 2 つ（xdg toplevel と `keiland_desktop` の desktop surface）。DnD の source と titlebar の drop target、progress。
- Image Viewer・Notes・Terminal: 自前の Vulkan で描く（`KUI_PRESENT_NONE`）。toolkit は VkSurface か wl_surface を渡す逃げ道が要る。
- Notes: tablet（`zwp_tablet_v2`）を自前の registry で bind。key を repeat しない。pointer の全 motion を筆の sample として使う。
- Terminal: pty の fd と一緒に待つ（`kui_window_dispatch_fds`）。clipboard と primary を自前に持つ（自分の選択を自分で paste する時の詰まりの回避）。
- Text Editor だけが IME・keyboard inset・編集の操作（`keiland_edit`）・libkeiui の部品を全て使う。最も「toolkit の app」に近い。
- browser: libbrowser は Wayland を使えない（Guardrail）。shell だけが窓を持つ。System Menu は無い。
- app は desktop の設定（theme・文字の大きさなど）を読まない。`kui_theme_default()` は固定。

## 2. 今の責務

| | libkeiland（`keiland.h`、`keiland_`） | libkeiui（`keiui.h`、`kui_`） |
| --- | --- | --- |
| 役（header の冒頭） | zdesktop の独自拡張の wrapper と、desktop から OS への唯一の入口 | Kei の見た目の部品と描画、窓 |
| 中身 | System Menu・context menu・titlebar（sheet を含む）・glass・desktop surface・keyboard inset・編集の操作・recent・preferences・network・audio・touch の motion・scroller・gesture | canvas・text（emoji）・icons・theme・scroll・scroll bar・ui（hit と focus）・text touch・**window（xdg toplevel、Vulkan/shm present、input の queue、repeat、clipboard、primary、text-input、edit、inset）**・部品・file chooser |
| 版 | `KEILAND_VERSION 21`（追加のみ。16 で file chooser を除いた前例あり） | `KUI_VERSION 12` |
| 利用者 | zdesktop（audio・motion・glyph）、xserver、probe、全 app、libkeiui | app 5 つ ＋ ime |
| OS の module | `zedbsd/`・`linux/`・`freebsd/`（audio・network link）、`wpa/`（Linux/FreeBSD の WiFi） | 無し（Wayland・Vulkan だけ） |
| link（Linux） | `libwayland-client.so -lm`、soname `libkeiland.so`（版の無い soname、exports.map で `keiland_*` だけ出す） | `libkeiland.so libwayland-client.so libtruetype.so libpng-compat.so libvulkan.so.1`、soname `libkeiui.so` |
| build | `Makefile`（zedBSD）・`Makefile.linux`（`keiland-linux.mk` が include）・`Makefile.freebsd`（`keiland-freebsd.mk`） | 同じ 3 本 |

WS090 の J1 は「libkeiland は広げず新しい libkeiui を作る。libkeiland は zdesktop・xserver・probe も link するので、Vulkan と窓を持つ部品を入れると要らない依存を負わせる。層は libkeiui → libkeiland の一方向」と決めた。
ただし実際は libkeiui の窓が libkeiland の `keiland_edit`・`keyboard_inset`・titlebar の sheet を中で使い、menu・titlebar・glass は「app のもの」として accessor（`kui_window_display`・`_toplevel`・`_surface`）で app に渡している。この境目が app ごとの結線の重複を生んでいる。

## 3. 案と比較

### 案 A: libkeiui を libkeiland に吸収して一つにする

- 形: `libkeiland.so` 一つ。`kui_*` を残すか `keiland_*` に改名。libkeiui.so は無くすか、互換の薄い library にする。
- 移行: build の統合 → 改名（する場合は全 app の機械的な置換）→ 以後は案 B と同じ骨組みを足す。
- ABI: libkeiui.so を消すと既存の binary が動かない（soname に版が無いので互換の shim が要る）。改名は全 symbol の非互換。
- Linux/FreeBSD: libkeiland の link に libtruetype・libpng-compat・libz-compat・libvulkan が加わり、zdesktop・xserver・probe が全部を負う。
- 利点: library と header が一つ、名前の通りの「libkeiland が toolkit」。
- 危険: J1 を逆にする。compositor が client の widget・font・chooser を含む library を load する。改名の churn がベータ1（10/17）の作業と衝突する。部品の host 試験が大きな library 全体に依存する。

### 案 B（推奨）: 層を分け、libkeiland を toolkit の土台にする

- libkeiland = 窓・desktop・OS の抽象化と app の骨組み。libkeiui = widget と描画（今の canvas〜file chooser。窓の実装は libkeiland へ移る）。
- 形（案、名前と細部は p002 の設計で決める）:

```c
/* 接続と registry を一つ持つ。zdesktop の拡張が無い compositor では各機能が無いだけ。 */
struct keiland_app *keiland_app_open(const struct keiland_app_options *options);	/* display、app_id、名前 */
void keiland_app_close(struct keiland_app *app);
int keiland_app_dispatch(struct keiland_app *app, int timeout_ms);			/* repeat と timer を含めて待つ */
int keiland_app_watch_fd(struct keiland_app *app, int fd, unsigned events, keiland_fd_fn callback, void *data);	/* pty など */
int keiland_app_take(struct keiland_app *app, struct keiland_event *event);		/* 窓・menu・titlebar・chooser の入力を一つの queue で */

/* 窓: 役（toplevel・desktop surface・sheet）、見せ方（VULKAN・SHM・NONE）、全画面、glass。 */
struct keiland_window *keiland_window_create(struct keiland_app *app, const struct keiland_window_options *options);
int keiland_window_present(struct keiland_window *window, const uint32_t *pixels, size_t stride);
int keiland_window_vulkan_surface(struct keiland_window *window, VkInstance instance, VkSurfaceKHR *surface);	/* NONE の app 用 */

/* 宣言的な desktop の部品: app は表を渡し、選ばれたものは KEILAND_EVENT_ACTION（action の番号）で返る。 */
int keiland_window_set_menu(struct keiland_window *window, const struct keiland_menu_entry *entries, size_t count);
int keiland_window_set_controls(struct keiland_window *window, const struct keiland_control_entry *entries, size_t count);
int keiland_window_set_action_state(struct keiland_window *window, uint32_t action, unsigned state);	/* enabled・checked。差分だけ送る */
int keiland_window_popup(struct keiland_window *window, const struct keiland_menu_entry *entries, size_t count, int x, int y);
int keiland_window_set_glass(struct keiland_window *window, const struct keiland_glass_panel *panels, unsigned count);

/* clipboard・primary・DnD（受ける型と出す型）・IME・keyboard inset・編集の操作 */
int keiland_window_copy(...); size_t keiland_window_paste(...); int keiland_window_accept_drops(...); int keiland_window_start_drag(...);
void keiland_window_text_input(struct keiland_window *window, int enabled); void keiland_window_text_cursor(...);

/* desktop の設定（theme・文字の大きさ）を読み、変わったら KEILAND_EVENT_PREFERENCES */
```

  - 入力の受け方は今の app の全てが使う **pull 型（dispatch → take）を主**にする。callback 型の `keiland_app_run` は後から足せる補助にする（今の loop を壊さないため）。
  - 今の各 app の `struct menu_item {id, parent, type, label, action, role, modifiers, keysym}` は全 app で同じ形なので、そのまま公開の `keiland_menu_entry` にできる。titlebar の control も同様。
  - 一つの registry を `keiland_app` が持ち、titlebar・menu・glass・edit・inset・desktop・data device・primary・text-input・tablet を一度の roundtrip で bind する。
    既存の `keiland_*_create(display, …)` は互換と compositor 以外の利用者のために残す。
- 移行の順: (1) libkeiland に app・window を足し、実装は libkeiui の `window.c`・`present*.c`・`clipboard.c`・`primary.c`・`text-input.c`・`edit.c`（約 4,950 行）を移す。`kui_window_*` は libkeiui に**薄い wrapper として残し**、既存の 5 app は無変更で動く。(2) 宣言的な menu・titlebar・glass・action の queue。(3) app を一つずつ新 API へ。(4) DnD・primary・tablet・desktop surface を足して Terminal・Notes・Files の自前の registry を除く。(5) 全 app の移行後に `kui_window_*` を除く（KUI_VERSION を上げ、KEILAND 16 の前例と同じ手順）。
- ABI: 追加のみ（KEILAND_VERSION 22〜）。soname に版が無いので、除く時だけ全 app を同時に作り直す。`exports.map` に `keiland_app_*`・`keiland_window_*` を足す。
- Linux/FreeBSD: 窓の層は Wayland と Vulkan だけで OS に固有の code を持たないので、共通の source（`libkeiland/*.c`）に置き、3 本の Makefile と exports.map に足す。libkeiland の link に `libvulkan.so.1`（と Vulkan の header）が加わる。zdesktop と xserver は既に libvulkan を link しているので実害は小さい。OS の境界の check（`plan/tools/keiland-os-boundary/check.sh`）は変わらない。
- 利点: 部品の library は app 以外に負担をかけない。GTK4/Qt6 の互換（WS097/096）の backend（GDK の backend・Qt の QPA）が必要とするのは窓・入力・clipboard・menu・titlebar であって Kei の widget ではないので、案 B の libkeiland はそのまま土台になる（Qt は自分で widget を描き、Kei の見た目にするなら libkeiui を style として使える）。
- 危険: 窓の input の扱い（BUG-111 の repeat の順序、BUG-112、text-input の順序）を移す時の退行。compositor の link が少し増える（libvulkan の NEEDED、使わない code）。

### 案 C: 今の .so の構成のまま、骨組みを libkeiui に足す

- `kui_app` と宣言的な menu・titlebar を libkeiui に置く。libkeiland は今のまま（desktop の拡張と OS）。
- 最も churn が少なく、技術的には案 B と同じ重複が消える。ただし「libkeiland を toolkit に」というユーザーの意図とは名前の上で逆になる。
- 「libkeiland（窓と desktop）＋ libkeiui（部品）を合わせて Keiland の toolkit と呼ぶ」と決めるなら、案 C を選んでよい。

### 案 C2（将来の選択肢）: libkeiland を二つに割る

- libkeiland = app 向け toolkit（案 A 相当を含んでよい）、`libkeiland-system`（仮）= compositor・xserver が使う OS の service・motion・glyph。
- J1 の懸念を消したうえで一本化できるが、zdesktop・xserver の link と soname の変更、OS の module の置き場の移動が要り、WS104/105/109 の境界の再確認が要る。ベータ1 の後に必要が出たら検討する。

### 比較

| | A 吸収 | B 層を分ける（推奨） | C libkeiui に骨組み | C2 割る |
| --- | --- | --- | --- | --- |
| ユーザーの意図（libkeiland が toolkit） | ◎ | ○（窓と desktop が libkeiland） | △（名前の定義しだい） | ◎ |
| compositor・xserver の依存 | 増える（font・widget・png） | 少し増える（libvulkan、窓の code） | 不変 | 減る |
| 既存の app への影響 | 改名なら全部 | wrapper で無変更、順に移す | 無変更、順に移す | link の変更 |
| ABI | 非互換か shim | 追加のみ | 追加のみ | soname の変更 |
| WS090 J1 | 逆にする | 窓だけ libkeiland へ（部品は分離のまま） | 保つ | 置き換える |
| GTK4/Qt6 互換の土台 | ○ | ◎ | ○ | ◎ |
| 量 | 大 | 中 | 中 | 大 |

## 4. 関係と危険

- **WS090 の残りの Phase**: p007（Settings）・p009/p010（Files）・p015（scroll を `kui_scroll` へ）・p012（規約）。案 B では p010 の「窓」と p007 の窓の部分を WS131 の新 API に向け直すのが合理的（二度の移行を避ける）。描画の層（canvas・text）と scroll の移行（p009・p015）は libkeiui の仕事のまま WS090 に残る。WS090 の Phase 表の改訂は main の作業。
- **並行の app の作業**: ベータ1（10/17）に向け WS127（Files）・WS089（Settings）・WS128（他の標準 app）・WS120（音楽、新規）が並行する。同じ app の `main.c`・`menu.c`・`titlebar.c` を触るので衝突する。app の移行の Phase はその app の WS と順番を決める必要がある（ユーザーか main の判断）。
- **GTK4/Qt6**: WS115〜117 の本物の GTK4/Qt6 は標準の Wayland を直接使い、libkeiland には乗らない（乗るとしても Keiland の titlebar・menu を使う任意の plugin）。WS097/096 の互換の書き下ろしは案 B の `keiland_app`/`keiland_window` の上に乗せる設計にでき、その時に要る機能（複数の窓、popup、子の窓、cursor の形、scale）が API の要件になる。
- **Wayland の隠し方**: `KUI_PRESENT_NONE` の app（Image Viewer・Notes・Terminal）と browser の shell は自前の Vulkan を持つ。`keiland_window_vulkan_surface` で wl の型を見せずに済むが、隠しきれない所（独自の protocol を要する将来の機能）は accessor を残す。
- **入力の退行**: repeat（BUG-111）、chooser の開閉（BUG-112）、IME の順序、touch の時刻の補正（`TOUCH_TIME_BEHIND`）。窓の実装を移す Phase は、Text Editor の既存の QEMU の試験（開く・編集・保存・touch・clipboard・PRIMARY・IME）を前後で流す。
- **HAL・toolchain**: 触らない。新しい外部依存も無い。

## 5. 推奨と移行の Phase の案（各 2〜4 時間）

| Phase | 内容 | 検証 | 依存 |
| --- | --- | --- | --- |
| p002 | API の設計（`keiland_app`・`keiland_window`・`keiland_event`・宣言的な menu/control の表・設定の通知・DnD の範囲）、`kui_window` の wrapper 化の手順、WS090 の Phase との境目。design-reviewer の review | 設計の review | ユーザーの判断（案） |
| p003 | libkeiland に app と window を足し、libkeiui の window・present・clipboard・primary・text-input・edit を移す。`kui_window_*` は wrapper。registry を一つに。3 本の Makefile と exports.map。KEILAND_VERSION 22 | build（zedBSD・`make keiland-linux`・FreeBSD）warning 0、既存の 5 app を無変更で起動、Text Editor の QEMU の回帰、boot-test | p002 |
| p004 | 宣言的な menu・context menu・titlebar・glass と action の queue（`set_action_state` の差分送り） | host 試験（表 → model の変換、差分）と probe | p003 |
| p005 | **最初の移行: Text Editor**（menu.c・titlebar.c・glass.c・te_window の queue・main loop を新 API へ。最も toolkit に近く、IME・chooser・部品を全て使うので API の確かめに向く） | QEMU の Text Editor の試験一式、行数の減り | p004 |
| p006 | PDF Viewer・Image Viewer（Image Viewer は `vulkan_surface`） | 各 app の guest 試験、touch-guest | p005 |
| p007 | DnD・primary・tablet・`dispatch_fds` 相当を libkeiland へ。Terminal・Notes を移し、自前の registry を除く | Terminal・Notes の guest 試験、demo-s8-s9 | p005 |
| p008 | Settings（自前の窓と present を除く。WS089・WS090 p007 と順を合わせる） | Settings の host・guest 試験 | p005、WS089 との調整 |
| p009 | Files（toplevel と desktop surface の 2 役、DnD の source、drop target。WS127・WS094・WS090 p010 と順を合わせる） | Files の guest 試験、desktop の試験 | p007、WS127/WS094 との調整 |
| p010（任意） | browser の shell の窓を新 API へ（libbrowser は触らない） | browser の起動と描画 | p006 |
| p011 | `kui_window_*` の wrapper を除く（KUI_VERSION を上げる）、keiland-ime の status の protocol を libkeiland へ（任意） | 全 app の build と起動 | 全 app の移行 |
| p012 | 規約の全文との照合と回帰（code を作る WS の必須の Phase） | coding-style.md 全文、各 OS の build、boot-test | 全て |

新しい app（WS120 の音楽など）は p004 の後から新 API で書けば移行が要らない。

## 6. ユーザーの判断が要る点

1. **案の選択**: B（推奨。libkeiland = 窓と desktop と OS、libkeiui = 部品）か、A（一つの library に吸収、J1 を逆にする）か、C（名前の定義だけ変え骨組みは libkeiui）か。
2. **時期**: ベータ1（10/17）の標準 app の作業（WS127・WS089・WS128・WS120）と並行するか、p002〜p004（libkeiland の側だけ、app は無変更）を先に進め、app の移行は各 app の WS と順を決めてから行うか。
3. **WS090 との分担**: Files・Settings の窓の移行を WS090 p007/p010 から WS131 へ移してよいか（WS090 は描画と scroll と部品に絞る）。
4. **app から Wayland を隠す度合い**: 完全に隠す（将来の別の backend を見込む）か、自前の Vulkan や独自 protocol のための accessor を残すか（推奨: 残す、ただし新 API の app は使わない）。
5. **desktop の設定の反映**: theme・文字の大きさなどを app が読み、変更に追従する機能を WS131 の範囲に入れるか（今はどの app も読まない）。
6. **GTK4/Qt6 互換（WS097/096）**: 将来その backend を `keiland_app`/`keiland_window` の上に作る前提で API を設計してよいか（複数の窓・popup・cursor・scale を最初から要件にするか）。
