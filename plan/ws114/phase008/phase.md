<!-- awesome-plan project=zedbsd record=ws114-p008 -->
# ws114-p008: org_kde_kwin_server_decoration に対応し、宣言の無い窓を既定で SSD にする

Status: cleared（2026-10-03 Q1 判定（user「Q1の判断で閉じられるものは閉じてください。」）: P2 の提案を受け入れ。Linux GTK4 の CSD・二重無し、zedBSD の C 基準 15/15、boot-test）。元の記載: in-progress（q618-i01 は 2026-10-03 04:25 に終了。P2 は cleared を提案、判定は Q1）
Disposition: normal
Parent: [WS114](../ws.md)

## ユーザーの決定（2026-10-03）

「org_kde_kwin_server_decorationに対応しましょう。どちらのプロトコルも使わず、自分で飾りを描くアプリは、無視します。」
補足:「どちらのプロトコルも使わず、自分で飾りを描くアプリは、全画面が多いと思います。」

背景: GTK4 は xdg-decoration を使わず、KDE の `org_kde_kwin_server_decoration` だけで CSD/SSD を宣言する（GTK 4.18.6 の `gdk/wayland/gdktoplevel-wayland.c:1272-1279`）。Keiland はこれを提供していないので、GTK4 は何も宣言しない。今の compositor は「SSD を明示に要求した窓だけ SSD」（WS114 p007 の G05）。

## 範囲

1. compositor（`userland/desktop/wayland/`）に `org_kde_kwin_server_decoration_manager`（default_mode の event、create、request_mode）を実装し、`decoration.c` の xdg-decoration・keiland_titlebar と一つの決め方にまとめる。
2. 既定を変える: どの protocol でも宣言の無い toplevel は SSD。xdg-decoration の `client_side` か KDE の `request_mode(CLIENT)` で CSD を宣言した窓は CSD。全画面（fullscreen）の窓には SSD を付けない（宣言の無い自前の装飾の app は全画面が多い、ユーザーの補足）。どちらの protocol も使わず自分で飾りを描く窓の二重は対象外（ユーザーの決定）。
3. 確かめ: GTK4（Linux の本物と zedBSD の WS115）が KDE の protocol で CSD を宣言して二重にならない、宣言の無い単純な client（wlshm 等）にタイトルバーが付き全画面では付かない、Keiland の app（keiland_titlebar）、X11（zterm）、Qt6（WS117 で）。C 基準（C2・C3・C4・C8・C9）と WS114 p007 の decoration の wire の試験、boot-test。Linux の Keiland（WS105）でも同じ。
4. p023（P2）で各 client に足した明示の SSD の要求は残してよい。

## 所有 path

`userland/desktop/wayland/`（decoration・protocol）、試験、`plan/ws114/`。

## 結果（q618-i01、P2、2026-10-03）

worktree `/home/awe/zedBSD-worktrees/p2`（branch `agent/p2`、main `1a99df3aa` から開始）。QEMU だけで確かめた（Linux は WS105 の Debian 13 guest、zedBSD は Venus）。iGPU・5330 は使っていない。

### 実装（`userland/desktop/wayland/`）

- `org_kde_kwin_server_decoration_manager` v1 の global（name 24）。bind すると `default_mode(SERVER)` を送る。`create(id, surface)` は decoration object を作り、`mode(SERVER)` を送る。`request_mode(NONE/CLIENT/SERVER)` には `mode` で答える（範囲外の値は protocol error）。`release` で object を外す（`decoration.c`・`protocol.c`・`objects.c`・`zwl.h`・`extras.h`）。
- 決め方を `decoration_wanted()` の 1 か所にまとめた。上から順に:
  1. xdg-decoration の object があれば、`client_side` なら CSD、`server_side` か unset なら SSD。
  2. xdg-decoration の object を destroy した窓は CSD（`withdrawn`）。
  3. KDE の object があれば、SERVER なら SSD、CLIENT か NONE なら CSD。
  4. keiland_titlebar があれば SSD。
  5. KDE の manager を bind していて、その窓の object を作っていない client は CSD。
  6. どれにも当たらなければ SSD（既定）。
- mode の変化は、configure → ack → commit の既存の道で反映する（`decoration_propose`）。全画面の窓は、これまでどおり装飾なしで描く（`shell.c`、変更なし）。
- 5 の理由: GTK 4.18.6 は CSD の窓に KDE の object を作らない。`gdk_wayland_toplevel_set_decorated` は `decorated` が変わったときだけ create/request_mode を送り、初期値は FALSE。最初の実装（object の無い窓は SSD）では、GTK4 の窓に Keiland のタイトルバーと GTK の header bar が二重に出た（`wire` の後の 1 回目の GTK 試験）。KDE の protocol は object を通してしか装飾しないので、manager を bind した client の object の無い窓は CSD と読む。KWin と同じ読み方。どちらの protocol も使わない client は既定の SSD のまま。

### 確かめ

- Linux（Debian 13、`build/p2-p008/linux` を `install-guest.sh` で導入。guest の wayland の SHA256 は host と一致）:
  - decoration の wire（`plan/ws114/tests/decoration-wire.py`。新しい既定と KDE の場合を足した）: 10 PASS。各 checkpoint で compositor が適用した mode（`ZWL DECORATION applied`）を期待と比べる確認を 25 回行い、全て一致した（[summary](../evidence/p008/wire-summary.log)・[wire.log](../evidence/p008/wire.log)・[compositor の mode](../evidence/p008/wire-compositor-modes.log)）。
  - GTK4 4.18.6（`gtk4-baseline.py`）: `DECORATION applied mode=1`。header bar だけで、二重にならない（[PNG](../evidence/p008/gtk4-csd.png)、[modes](../evidence/p008/gtk4-modes.log)）。
  - wlshm `--csd`（宣言なし）は SSD（[PNG](../evidence/p008/linux-wlshm-csd-flag.png)）。Terminal は SSD（[modes](../evidence/p008/linux-terminal-modes.log)）。
  - 停止: `ZWL EXIT error=0`。`systemctl poweroff` の後 35 秒たっても QEMU が終わらなかったので、自分の QEMU に TERM を送った。port は空いた（[stop](../evidence/p008/linux-stop.log)）。
- zedBSD（`build/p2-p008-img`）: 宣言の無い wlshm `--csd` は SSD（[PNG](../evidence/p008/zedbsd-wlshm-csd-flag.png)）。wlshm（titlebar あり）・Terminal・zterm（X11、[PNG](../evidence/p008/zedbsd-zterm.png)）・Notes は全て `mode=2`。
- build: Linux の keiland は exit 0・warning 0。zedBSD の image は exit 0 で、compiler の warning は外部の openssh だけ。`style-check.py` の違反 0。
- 途中で見つけた回帰: p023 で wlshm に足した libkeiland が、Linux と FreeBSD の Makefile に入っていなかった（main の Linux の build が失敗していた）。独立した commit `0ce650b2a` で直した。FreeBSD 版は build していない。
- zedBSD の C 基準（`criteria.sh build/p2-p008-img/hdd-image.img … C2 C3 C4 C8 C9`、1 回）: 15/15 PASS（[results](../evidence/p008/zedbsd-criteria-results.txt)）。p072 の画面も目で確かめた。
- boot test: PASS（[PNG](../evidence/p008/boot-login.png)）。
- 全画面: 描画の側（`shell.c`）は全画面の窓を本体だけで描く。今回は変えていない。C3 の c3-swipe-back（Notes の全画面と解除）が PASS した。

### 未実施・残り

- zedBSD の GTK4（WS115）と Qt6（WS117）は未実施（まだ動く物が無い。その WS で確かめる）。
- Linux の Keiland の全 app、物理 GPU、5330 は未実施（範囲外、iGPU は使わない規則）。
- FreeBSD 版の wlshm の build は未実施。
- C の 5 回の反復はしていない（受け入れの条件は 1 回）。

### 判定の提案

範囲 1〜3 を、QEMU の Linux（GTK4 の実物）と zedBSD で確かめた。KDE の protocol の読み方（manager を bind して object を作らない窓は CSD）は、GTK4 の実際の動作に合わせた技術的な判断で、ユーザーの決定（宣言の無い窓は SSD、CSD を宣言した窓は CSD）の範囲に収まると考える。**cleared を提案する。**

2026-10-03 Q1: cleared（main 8140d36a9）。P2 の解釈「KDE の manager を bind したが窓の object を作らない client は CSD」（KWin と同じ。GTK 4.18.6 は CSD の窓で request を送らないため、無いと二重のタイトルバー）はユーザーの決定の文言の外なので、朝にユーザーに確認する。

2026-10-03 user（確認）:「「KDE のプロトコルを使える状態にしたが、その窓については何も頼まない」窓はCSDでOKです。」→ P2 の実装（KWin と同じ）を確定。何のプロトコルも使わない窓は SSD。
