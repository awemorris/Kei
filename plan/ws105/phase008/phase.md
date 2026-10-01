<!-- awesome-plan project=zedbsd record=ws105-p008 -->

# ws105-p008: app の Linux の build と install の data

Status: planned
Disposition: normal
Parent: [WS105](../ws.md)
Queue: なし
依存: p007
実行者: phase-runner（high）か phase-runner-mid（機械的な部分が多い）

## 目的

決定 D21 の app を Linux で build・install し、guest の compositor の App Home から起動して使えるようにする。

## 作る・変える file

`Makefile.linux` を作る package（依存と system の library は [design.md](../design.md) §3.3 の表。source の一覧は各 package の zedBSD の `Makefile` の source の変数を書き写す）:

| package | 種類 | 備考 |
| --- | --- | --- |
| `userland/desktop/libkeiui` | library `libkeiui.so` | |
| `userland/base/libpdf` | library `libpdf.so` | `libkeiland-compat.a` を link（`sha2.h`・`md5.h`） |
| `userland/desktop/terminal` | program | `forkpty` は glibc の libc（2.34 から） |
| `userland/desktop/files` | program | `libkeiland-compat.a`（`sha2.h`） |
| `userland/desktop/settings` | program | network・音の頁は p010 まで「無い」と出る（仮の backend） |
| `userland/desktop/notes` | program | `libkeiland-compat.a` |
| `userland/desktop/textedit`・`imageview`・`pdfviewer`・`kuidemo` | program | |
| `userland/desktop/ime` | program `libexec/keiland-ime` | 辞書（下） |

共通の file の変更（design §5.6）:

- `userland/base/libpdf/font.c` の `convert_contours()`: loop の前に `control[0] = 0.0; control[1] = 0.0;`（gcc の誤検出）。
- `libkeiui/text.c:595` の emoji の font: `KUI_TEXT_EMOJI`（公開の header の macro）の代わりに `KEILAND_DATADIR "/fonts/keiland-emoji.ttf"` を使う（`#include "userland/desktop/paths.h"`）。
  header の macro は残す（他の利用者のため）。zedBSD では同じ文字列（WS104 p007 の確かめの方法で `strings` が同じこと）。
- `files/apps.c:111` の `apps_program_folders[]` の先頭に `KEILAND_BINDIR` を足す（zedBSD では `"/bin"` が 2 回になるので、重複を避けるなら「`KEILAND_BINDIR` が `"/bin"` でない時だけ足す」を
  実行時の `strcmp` で書く。振る舞いは zedBSD で同じ）。

install の data（`KEILAND_LINUX_DATA` か、生成する物は `keiland-linux.mk` の規則）:

- App Home の一覧: `wayland/linux/apps.conf.in` と `keiland-linux.mk` の生成規則を追加。既存 demo の `plan/ws035/demo/apps.conf` と同じ wire/config 書式で D21 の利用者向け app と試験の window client を載せ、command の prefix を build 時の `KEILAND_PREFIX` に置換。Settings・kuidemo は共通の built-in 一覧にないため、この既存 config の入口で提供する（common home.c は不変）。`etc/keiland/apps.conf` に install。
- wallpaper: zedBSD の image の作り方（`plan/ws099/tests/build-criteria-image.sh` の wallpaper の行）と同じ: `python3 userland/desktop/wallpapers/generate.py <dir>` で `share/keiland/wallpapers/`、
  既定の `share/keiland/wallpaper.ppm`（criteria の image が入れる物と同じ元）。script の引数は `generate.py` の先頭の注釈を読む。
- IME の辞書（`share/kei/ime/ja/`）: `SKK-JISYO.kei` は `userland/desktop/ime/dict/SKK-JISYO.kei` を写す。`SKK-JISYO.X` は外部の取得物:
  `build/distfiles/remacs-1a724393053e.tar.gz`（zedBSD の build の取得物）があれば使い、無ければ
  `curl -L -o build/distfiles/remacs-1a724393053e.tar.gz https://github.com/awemorris/remacs/archive/1a724393053e18c4e1f502ecc5ca8ce07d99287a.tar.gz`。
  archive の SHA-256 が `419d03a195e18875e4761f4d81992905d4130697b72a5fc87849f0228a2f1507`（`userland/desktop/ime/dict/Makefile` の値）であることを確かめ、
  `REmacs-1a724393053e18c4e1f502ecc5ca8ce07d99287a/dict/SKK-JISYO.X` を取り出し、その SHA-256 が `73819384159330a0c822915d0fd211c3e21dea77f2cfd2083273ac1ffd121ab9` であることを確かめる
  （値は Makefile から読む規則にする。直書きしない）。license は zedBSD と同じく WS095 D1 の著作権者による zlib 再許諾と project の license の範囲（`ime/dict/Makefile`）。別の license file は不要。
- 各 app の data: 各 package の zedBSD の `Makefile` の 13 番目の引数（`DEST=SRC`）を見て、`/usr/share/X` → `share/X`、`/etc/X` → `etc/X`、`/bin/X` → `bin/X`、
  `/usr/libexec/X` → `libexec/X` に写す。
- `keiland.desktop` の `Exec`（p009）に `--wallpaper=/opt/keiland/share/keiland/wallpaper.ppm` を足す（zedBSD の `sessiond/session.sh` と同じ）。

source が Linux で compile できない所が新しく見つかったら:

1. glibc に無い小さな関数なら `userland/desktop/linux-compat/` に足す（src/libc の source を compile する形が先）。
2. app の code の中の OS の違いなら、止めて main に報告する（その app の OS の部分を `linux/` の file に分けるかを決める）。macro の block で逃げない。

## 手順

```
make -j64 keiland-linux && make keiland-linux-install DESTDIR=$PWD/build/keiland-linux/stage
make -j64 keiland-linux CC=clang KEILAND_LINUX_BUILD=build/keiland-linux-clang
sh plan/tools/keiland-linux/elf-check.sh build/keiland-linux/stage && sh plan/tools/keiland-linux/makefile-sync.sh && sh plan/tools/keiland-linux/header-check.sh
G=plan/tools/keiland-linux/guest.sh
sh $G start && sh plan/tools/keiland-linux/install-guest.sh
sh $G ssh 'openvt -c 7 -s -- sh -c "KEILAND_SEAT=direct /opt/keiland/bin/wayland --session --glass --wallpaper=/opt/keiland/share/keiland/wallpaper.ppm --socket=/run/keiland-0 > /tmp/wayland.log 2>&1"'
sh $G ssh 'for i in $(seq 30); do grep -q "ZWL READY" /tmp/wayland.log && break; sleep 1; done'
sh $G click 23 17; sleep 2; sh $G screenshot $PWD/build/keiland-linux/p008-home.png
# App Home の tile の位置を p008-home.png で読み、app ごとに: click → 10 秒 → screenshot → pidof → esc か窓を閉じる
```

app ごとの確かめ（screenshot は `build/keiland-linux/p008-<app>.png`、PNG をユーザーに見せる）:

| app | 確かめ |
| --- | --- |
| Terminal | `guest.sh type 'echo keiland-linux-ok'`・`guest.sh key ret` の後の screenshot に文字が出る（目で確かめ、結果に書く） |
| Files | home の directory の一覧が出る |
| Settings | 頁を移れる（network・音は p010） |
| Notes・Text Editor | 文字を打てる |
| Image Viewer | `share/keiland/wallpapers/` の画像を開ける |
| PDF Viewer | 小さな PDF（`plan/ws079/tests/` の試験の PDF があれば。無ければ「未実施」） |
| IME | 日本語の入力の切り替え（zedBSD と同じ key）が効くか（効かなければ記録） |
| kuidemo・mview | 窓が出る |

## 完了の条件

1. build（gcc・clang）、`elf-check: PASS`、`makefile-sync: PASS`、`header-check: PASS`。
2. guest: 上の表の各 app の窓が出て、10 秒後に process が生きている（`guest.sh ssh pidof <program>`）。表の確かめが PASS か理由つきの未実施。
3. 各 app を閉じた後に compositor が動き続ける。
4. zedBSD の回帰（共通の file を変えたので design §9.2。`libkeiui/text.c`・`files/apps.c`・`libpdf/font.c` の `strings` が zedBSD で同じこと: WS104 p007 の手順 1・4 の方法）。

## 結果

（実行の後に書く）

## 開始前の検証手順補正（2026-10-01、Q1）

p006 / p007 の実際の共通 shell を確認し、App Home は top-left launcher の click（23,17）で開くよう手順を修正。Super+Tab は Wiseview。既存 app の受け入れ・依存・実装範囲は不変。

## 開始前の data / 検証設計補完（2026-10-01、Q1）

- 現行 Home の built-in list に Settings・kuidemo がない。既存 apps.conf の config 入口を Linux の install data として使い、既存 D21 の app を Home から起動可能にする（product の追加なし、common source の変更なし）。design §2 の config 配置を具体化。
- ユーザーの default wallpaper は WS035 p061 の決定どおり git 外に保持する。現在の `build/ws035-wallpaper/wallpaper.ppm` を、zedBSD criteria と同じ元として install。cache のない checkout でも build できるよう、既存 generator の Aurora を fallback とし、`KEILAND_LINUX_WALLPAPER` で元の画像を指定できるようにする。画像を git に取り込まない。
- Image Viewer は既存 PNG / JPEG / GIF の reader で PPM reader を持たない。wallpapers directory の検証には同じ wallpaper の既存 PNG を guest の試験用 file として配置し、対応形式の画像表示を確認する。PPM 対応の追加はこの Phase の product scope に含めない。
