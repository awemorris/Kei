# WS106 確定する移動手順（q539）

Source: 0a43f635。対象30 package + ユーザー追加指定の直下13 files。
[全 tracked file/hash/mode/移動先と参照](survey.json)、[移動前 menu rows](programs-before.txt)、[既存 style 指摘候補](style-before.txt)。

## 1. source 所有と registry

base/tests を userland/tests に移し、desktop の9 app を同 root の各 app directory へ移す。
base/tests の grouping Makefile（ID tests、選択不可、実sourceなし）も新 root に置き、二重登録しない。
userland/tests/package.mk は desktop/package.mk と同じ薄い wrapper で ../base/package.mk を再利用する。
subapp の relative package.mk は ../package.mk に揃える。top Makefile の既存 wildcard で全30 package と grouping を発見可能。
全30 app の MENU を tests、PACKAGE path を tests/<app> とし、menuconfig に Tests の category を追加する。
package名、label、platform、default、class、selectable、require のlibrary名、mode/type、install directory と runtime/data先は保存する。
require 内に実際に app の旧 PACKAGE path がある場合だけ tests の新 path に正規化する。config の ZEDBSD_USER_PROGRAMS は ID ベースなので変更不要。

## 2. source・data の参照

compositor の vkdemo/display.c、retro/glxtest の egltest/scene.c を含む Makefile と include を新経路へ更新する。
acquire-fence は移動後の ../wltest/wltest.h を include。vkdemo の ../../base/common/sha256.h は移動後も同じ深さなのでそのまま。
platform/{amd64,pcat,pc98,arm64} の syscall/POSIX/TLS/dynamic-loader の直接object/rule/map参照を全て更新する。
Linux aggregator と5 app の Makefile.linux を更新。makefile-sync は userland/*/*/Makefile.linux を既に走査し、tests/ も対象なので範囲変更不要。
現役 runner/source/tool の参照を更新、retired と history の結果は当時のまま。変更は fileごとの一覧と diffで確認する。
モデルの元 provenance は当時のcommand/path/hashを保存し、新しい所在を別の dated locator に記録。converterの今後の生成先表記/手順は新 path。
shader/header/model/texture を再生成しない。付属binary/dataのhashと実install destination/contentを照合する。

## 3. 所有と規約

ime-probe の現在treeは clean、既知WS095 worktreeは不存在。人間作業との競合回答を待ち、許可/非競合確認までその移動を行わない。
WS095 の production ime を編集/移植しない。移動前hashを再照合して同時変更を保護する。
59 C source を style-check で走査し1300指摘候補。既存codeを維持する例外の要否をユーザーへ提示。
ユーザーが既存style維持を承認。[限定例外](../standards/ws106-relocation.md)をp002/p003へ適用。新実装のコードをここへ追加しない。

## 4. 検証 command と上限

- registry: make --no-print-directory list-user-programs。旧snapshotのID/label/platform/default/requireと新rowsを正規化して比較、30appがTestsへ移ったことを確認。
- target: make -j16 disk-image と、全29 executable test packageの build/amd64/bin/<ID>（必要なoptionalも明示build）。保存済みtoolchain/共有buildは消さない。
- loose test: amd64のsyscall/POSIX/SUSv4/SMP、dynamic loader fixtureの既存targetを明示しbuild。platformの未実行arm64/pcat/pc98も展開したsource参照を検査する。
- Linux clean: 新しい build/ws106-linux-gcc と build/ws106-linux-clang をKEILAND_LINUX_BUILDで指定、make -j16 keiland-linux CC=gcc/clang。
- Linux install: keiland-linux-install DESTDIR=$PWD/build/ws106-<compiler>-stage。既存elf/header/makefile checks、model/texture/app manifestとhash確認。
- scripts: 変更したsh/bashを宣言interpreterでsyntax確認、Pythonをcompile（syntaxのみ）。Cはinclude locatorの変更だけを個別review。
- full-standard: C全文、Guardrail、scoped exception（決定時）で全差分と全moveのhash/ownership/意味の維持を確認。git diff --check。
- final boot: OUTPUT=build/ws106-p003/boot plan/tools/boot-test.sh build/amd64/hdd-image.img。PNGを確認してユーザーへ表示。

p001最大60分、p002移動/参照最大60分、p003 build/install/最終review最大120分の実行案。
純粋な移動なのでGPU/実機や大量の機能回帰を足さない。warningはproject/externalを区別して記録。
失敗/未達は結果と再開条件を残す。WS106以外の製品scopeは追加しない。
