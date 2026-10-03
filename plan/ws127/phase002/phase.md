<!-- awesome-plan project=zedbsd record=ws127-p002 -->

# ws127-p002: p001 で見つけた不具合の直し

Status: uncleared（q616-i01、P1、2026-10-03。実装した項目は QEMU と host の試験で PASS。eject は依存が無く未着手、BUG-141/142 は再現せず。結果は末尾）
Disposition: normal
Parent: [WS127](../ws.md)
Queue: q616 / q616-i01（Q1 の dispatch、2026-10-03。時限 4 時間）
依存: p001
目安: 3h（1 Queue）。実行者の目安: phase-runner-mid
所有 path: `userland/desktop/files/`（不具合の所だけ）、`plan/ws127/`

## 範囲

p001 の不具合の表のうち重い・中の物を直す。軽い物は時間の中で直すか Future Work へ。

## 受け入れ

直した不具合ごとに、直す前に FAIL・直した後に PASS の試験（host か guest）がある。重い・中が 0（直したか ticket でユーザーが非阻害）。
変えた source の style-check の違反 0、build の warning 0、`files-regress.sh` の 14 本と boot test PASS。

## 未決の判断

p001 の不具合の表（重さの分け方の確認）

## 検証の方法と範囲

QEMU の Venus（`plan/tools/files/files-guest.sh`、`files-regress.sh`）と host の試験。console・serial の log では判定しない。各 Phase の最後に `files-regress.sh` の 14 本と boot test。
Settings と共有の `canvas.c`・`text.c`・`icons.c` を変えたら `sh plan/ws089/tests/host-build.sh` と Settings の host 試験も流す。

## Event

2026-10-02 / ws127-beta1-plan: fg019 の計画で新設（planning）。p001 の結果とユーザーの選択で範囲を確定し planned にする。

## q616-i01 の結果（P1 generation3、2026-10-03、base main `eeeda7341`）

範囲（Q1 の dispatch、ユーザーの WS127 の採否の回答に基づく）: BUG-140/141 と試験の直し（files-p011.sh の 7 app の期待を含む）、BUG-142 の調査、
PDF の thumbnail と disk cache（F-035）、DnD の自動 scroll と spring-loaded（F-039）、Move To・Open in New Window、eject（F-036、mount の制御を
libkeiland に）、macOS 風の重ね表示のスクロールバーを libkeiui の共通部品に。p003・p004・p006 の範囲を含めてこの 1 Queue で行った。

### 実装したこと（commit `b90364e74`〜`2f6c20a60`）

| 項目 | 内容 | file |
| --- | --- | --- |
| BUG-140 | Trash の一覧で、同名の 2 つ目（Trash の中では `NAME.2`）を trashinfo の元の名前と種類で出す。path は Trash の実体のまま | `files/dir.c` |
| BUG-141 | 元の手順では再現せず。scroll の後に hover が item の番号のまま残る近い原因を直した（scroll で hover を消す） | `files/ui-input.c` |
| Move To ▸ | 項目の context menu に「Move To」の submenu。sidebar の Favorites のフォルダー（Home を含む）と mount された volume。表示中のフォルダーは出さない。move の task（衝突の確認・undo は drop と同じ） | `files/ui-context.c`、`files.h` |
| Open in New Window | フォルダー 1 つの context menu に。そのフォルダーで新しい process の窓（New Window と同じ起動の仕方） | `files/ui-context.c`、`files/main.c` |
| 重ね表示のスクロールバー | libkeiui に共通の部品 `kui_scroll_bar`（`libkeiui/scroll-bar.c`、`keiui.h`、KUI_VERSION 12）: スクロール中は細く出て、右端に pointer が近づくと太く（薄い track 付き）、thumb の drag と track の click（1 頁）、約 1 秒の後に 0.4 秒で消える。描画は持たないので Files は自分の canvas で描く（`files/ui-scrollbar.c`）。libkeiui の canvas 用の `kui_scroll_bar_draw` も足した（他の app 用、WS128） | `libkeiui/scroll-bar.c`・`scroll.c`、`keiland/keiui.h`、`files/ui-scrollbar.c`・`ui.c`・`ui-input.c`・`main.c` |
| spring-loaded・端の自動 scroll（F-039） | drag が folder（項目・sidebar）の上で 750 ms 止まると開く。drag は開始時の path を保持するので、開いた先の空き・フォルダーへ落とせる。content の上下 44 px の帯で、端に近いほど速く scroll。窓の中の drag と外からの drop の両方 | `files/ui-drag.c`、`ui.c`、`files.h` |
| PDF の thumbnail と disk の cache（F-035） | PDF の 1 頁目を libpdf で描く（libpdf は dlopen。無い system では従来の icon）。作った thumbnail を `$XDG_CACHE_HOME`（無ければ `~/.cache`）`/keiland/thumbnails/` に、path の SHA-256 の名前の PPM で保存し、file の mtime・size が同じなら次から読む | `files/thumb-cache.c`・`thumb.c`・`ui-grid.c`・`ui-preview.c`・`ui-desktop.c` |
| 試験の直し | p014・p017 の undock の座標（T1）。p012 の Budget.csv の既定（Text Editor に変わっていた）。p010 は spring-loaded で docs が開くので次の drag の前に開き直す。Files の試験 image から IME を除く（zdesktop が keiland-ime を先に client 1 として起動し、全ての guest 試験の `client=1` が外れていた） | `plan/tools/files/` |

### 確認（QEMU の Venus と host。実機は未実施）

- host: `sh plan/tools/files/host-model.sh` → PASS（BUG-140 の check は直す前 FAIL・後 PASS、thumbnail の cache の check を追加）。`host-p009.sh`（Move To の
  case 5 を追加）・`host-p010.sh`・`host-p013.sh`・`host-p014.sh` PASS。`sh plan/ws127/tests/scroll-bar-test.sh`（libkeiui の部品、20 check）PASS。
  `sh plan/ws127/tests/host-spring.sh`（spring で docs が開き、開いた先へ落とせる・素通りでは開かない・下端の帯で scroll）PASS。
- image: `sh plan/tools/files/build-files-image.sh build/p1-files` rc=0、変えた file の warning 0（`-Werror`）。guest は P1 の port（10100〜）で
  [files-guest-p1.sh](../tests/files-guest-p1.sh)。
- guest の新しい試験 [files-p002.sh](../tests/files-p002.sh) → PASS: Move To で Budget.csv が Downloads へ（`moveto.png`）、PDF の thumbnail が
  作られ（`cached=0`）次の起動で cache から（`cached=1`、`pdf.png`）、hover の確認（`hover-on.png`・`hover.png`）、Open in New Window で 2 つ目の
  窓（`newwindow.png`）、300 項目の folder で細い bar → 端で太い bar → thumb の drag で scroll（`SCROLLBAR press ... dragging=1`・`release scroll=…`）
  → 消える（`scroll-thin.png`・`scroll-thick.png`・`scroll-faded.png`）。画面は worktree の `build/ws127-p002/`。
- **回帰**: `files-regress.sh`（14 本、最終の image）→ `files-regress: PASS`（p002〜p017・p010 全て）。`files-p011.sh`（App Home から Files、組み込みの
  一覧が 11 になっていたので「7 以上」に直した）→ PASS。boot test `plan/tools/boot-test.sh build/p1-files/hdd-image.img` → PASS
  （`build/p1-files-boot/login.png`、GPU の無い QEMU なので console の login）。

### 未実施・止めたこと

- **eject（F-036）は未着手**: zedBSD に USB の storage の hotplug の通知と自動の mount が無く（userland の automount・通知の仕組みが見当たらない）、
  利用者が unmount してよいかの方針（権限）も決まっていない。WS127 の ws.md が前提に挙げた依存が満たされていないので、計画に無い依存として
  止める（Q1 へ）。
- BUG-142: 調べのみ（source の上で順序は保たれる、詳細は ticket）。実機で再試行（ws127-p007）。
- Linux・FreeBSD の Keiland の build（Files の `Makefile.linux`・`Makefile.freebsd` に `ui-scrollbar.c`・`thumb-cache.c`・`libkeiui/scroll-bar.c` を足した、
  libkeiui の両 Makefile にも `scroll-bar.c`）は未実施。Linux の古い glibc では dlopen に `-ldl` が要る可能性。
- Settings の host 試験: Settings と共有の `canvas.c`・`text.c`・`icons.c` は変えていないので流していない。
- 試験 image から IME を除いた点: guest の試験は IME の無い状態を見ている。試験を client の番号に依らない形に直すのは残り。
- 実機（5330）での操作と速さは ws127-p007。
