<!-- awesome-plan project=zedbsd record=ws129-p009 -->
# ws129-p009: デモの image を CI の設定を土台に作る

Status: uncleared（q594-i01、P1。1〜3 と、clang・libcxx・remacs を除く変種での 4 は済み。CI と同じ完全な image の build と boot test は toolchain の package のため main の作業で、それが通れば clear。5330 の実機の単独起動はユーザーの確認で未実施）
Disposition: normal
Parent: [WS129](../ws.md)
Queue: q594 / q594-i01（P1）

## 範囲（2026-10-02 user「デモのイメージはCI設定をベースに変更しましょう。」）

1. デモの image の config（`plan/ws075/demo/build-demo-image.sh`、`config-demo-hdmi.mk` → `config-zdesktop-hw.mk` → `plan/ws035/tests/config-amd64-userland.mk`）を、CI の `config/ci/config-amd64.mk` を土台にして、デモに要る差分（display の boot line など）だけを重ねる形に変える。AX211 の driver と `intelax211-firmware` が CI と同じく揃う（BUG-134 の q590 の発見）。passthrough 用の VBT の選択（`passthrough` の語）は今の挙動を保つ。
2. 差分を表にする（旧デモ config と CI の config の package・driver・boot line の違い、残す差分の理由）。
3. q590 で見つかった `boot-test.sh` の QMP `screendump` の 30 秒 timeout（CI 土台の image、AX211=n でも同じ）を調べ、原因を特定して道具か image 側を直す（`plan/tools/boot-test.sh`・`boot-test.py` の修正は可）。
4. 新しいデモ image を build（warning 0）し、`boot-test.sh` で login prompt を確認。5330 の実機の単独起動はユーザーの確認（未実施と書く）。

## 所有 path

`plan/ws075/demo/`、`plan/ws035/tests/config-amd64-userland.mk` と demo が使う config、`plan/tools/boot-test.*`、`plan/ws129/phase009/`。`config/ci/` の CI の設定そのものは変えない（変えるなら Q1 に返す）。

## 受け入れ

上の 1〜4。古い demo の build directory は消さない。

## 結果（q594-i01、P1、2026-10-02、base `c4ed68f12`）

### 1. デモの config を CI の設定の土台に

`plan/ws075/demo/config-demo-hdmi.mk` を `include config/ci/config-amd64.mk` に変え、デモに要る差分だけを重ねた（旧: `plan/ws031/tests/config-zdesktop-hw.mk` →
`plan/ws035/tests/config-amd64-userland.mk` の連鎖）。`config-zdesktop-hw.mk`・`config-amd64-userland.mk` は i915 の試験など他の WS の試験も使うので変えていない
（デモはもう使わない）。`build-demo-image.sh` は変えていない（`passthrough` の語で `I915_TEST_VBT=y` を渡す今の挙動のまま）。

実効の値は config を読む小さな makefile（`$(info)` で `ZEDBSD_USER_PROGRAMS` と `CONFIG_*`・`ZEDBSD_*` を出す）で比べた。

| 項目 | 旧デモ | CI（`config/ci/config-amd64.mk`） | 新デモ | 理由 |
| --- | --- | --- | --- | --- |
| driver（AX211・i915・HDA・ACPI・Venus・USB hub ほか） | AX211=y（旧デモは HDA・ACPI・Venus・USB hub を書かず Makefile の既定の y） | 全て明示で y | CI と同じ | 実効は旧も同じ。CI の明示に揃う |
| graphical boot・kmsg・Noct accel・rootfs development | y・quiet・y・既定 y | y・既定 quiet・y・y | CI と同じ | — |
| boot の行 `display=edp` | あり | なし | **あり**（`ZEDBSD_BOOT_EXTRA_LINES ?=`） | 5330 の内蔵 LCD だけを出力にする（2026-09-29） |
| `intelax211-firmware`・`rtl8822b-firmware` | **なし** | あり | あり | BUG-134 の q590 の発見（driver=y で firmware 無し）が解消 |
| `textedit` | なし（App Home の Text Editor の tile が空振り） | あり | あり | — |
| `curl`・`expat`・`zlib`・`ca-certificates`・`vkdemo`・`base64`・`install`・`mktemp`・`pwd`・`sessiond`・`libbrowser` | なし（一部は依存で入っていた） | あり | あり | CI に揃う |
| `clang`・`libcxx`・`remacs` | なし | あり | あり | CI に揃う（image が大きくなる。build は toolchain の package、下） |
| `settings` | あり | なし | **あり** | App Home の Settings の tile |
| `zterm` | あり | なし | **あり** | App Home の「X terminal」の tile |
| `gpudemo` | あり | なし | **あり** | デモの場面 S13（`/usr/share/gpudemo`） |
| `Xzed`・`zshell`・`zwm` | あり | なし | なし | 旧 X の desktop。App Home は使わない |
| `egltest`・`glxtest`・`wlshm`・`wltest` | あり | なし | なし | 試験の program。デモに要らない |
| `test`（`/bin/test`） | あり | **なし** | なし | CI の一覧に無い（shell の builtin で足りる前提）。CI の設定の問題なら Q1 の判断 |

新デモの実効の値と CI の差は `ZEDBSD_BOOT_EXTRA_LINES=display=edp` と program の `settings`・`zterm`・`gpudemo` の 3 つだけ（比べた結果）。

この config を include する他の config: `plan/ws101/tests/demo/config.mk`（noct を除く）は新しい土台をそのまま継ぐ。`plan/ws004/tests/config-ax211-desktop.mk`（P1 の q590 の道具）も同じ。
`plan/ws081/tests/config-amd64-demo-win.mk`・`plan/ws035/tests/config-amd64-demo-venus.mk` は include していない（影響なし）。

### 2. `boot-test.sh` の QMP screendump の 30 秒の timeout

- 再現: q590 の CI 土台の image（`build/p1-usb`、AX211=n の対照 `build/p1-usb-noax` も）で `boot-test.py` が `screendump` の応答を 30 秒待って `TimeoutError`。
- 計測（boot-test.py の写しに時間の print を足して）: OUTPUT が `build/`（host の `/`、**使用率 98%、空き 27G**、他の agent の build で忙しい）のとき、screendump が
  1.5〜4 秒、続いて 30 秒超。OUTPUT を tmpfs の `/tmp` にすると全ての screendump が 0.02 秒台で PASS。
- 切り分け: frame（PPM）だけを tmpfs にして stick の写しを `build/` に残しても 30 秒超の停止が出た。stick の写しも tmpfs にすると出ない。
  → **QEMU の main loop が、満杯で忙しい host の disk の上の guest の disk の写し（と frame の書き出し）で数十秒止まり、QMP が応答しない**のが原因。
  image の内容や AX211 とは無関係（同じ image が tmpfs では PASS）。QEMU の中のどの呼び出しで止まるか（flush など）までは調べていない。
- 修正（`plan/tools/boot-test.sh`・`boot-test.py`）: QEMU の作業の file（stick の写し、UEFI の変数、QMP の socket、毎回の frame）を `BOOT_TEST_WORK`
  （既定は `TMPDIR`、無ければ `/tmp`）の下の `mktemp -d` の directory に置き、終わりに消す。OUTPUT には `login.png`・`login.txt`・`qemu.log` だけを残す。
  `boot-test.py` に `--frame` を足した（省略時は従来どおり screenshot の隣）。
- 確認: 修正後の `boot-test.sh` で、以前 timeout した `build/p1-usb`・`build/p1-usb-noax` と、`build/p1-ax211`・新デモ `build/p1-demo` の 4 つとも PASS。
  作業の directory は終わりに消えた（`/tmp/boot-test.*` が残らない）。
- 付記: host の `/` の空きが 27G（98%）。共有の機械の問題なので Q1 に報告する。

### 3. 新しいデモの image の build と boot test

- `clang`・`libcxx` は AGENTS.md の toolchain の範囲（subagent は build しない）、`remacs` は新しい tree で数時間かかるため、検証の build はこの 3 つだけを除く
  一時の config（`build/p1-demo-notc.mk`、git に入れない: `include plan/ws075/demo/config-demo-hdmi.mk` と `filter-out clang libcxx remacs`）で行った:
  `plan/ws075/demo/build-demo-image.sh build/p1-demo ZEDBSD_CONFIG=build/p1-demo-notc.mk` → rc=0、`demo image: build/p1-demo/hdd-image.img`。
  zedBSD の source の warning 0（warning は OpenSSH の package の deprecated・perl の locale・jobserver だけ）。
- 中身: boot の行 `kernel=vmunix`・`rootpart=PARTLABEL=zedBSD-root`・`swap0=PARTLABEL=zedBSD-swap`・`logo=logo.ppm`・`login=graphical`・`kmsg=quiet`・`display=edp`。
  vmunix に AX211 の symbol 90、`/lib/firmware/{i915,intel/iwlwifi,rtw88}`、`/bin/{settings,zterm,textedit,noct,files,browser}`、`/usr/share/gpudemo/{mix.nct,s13.sh}`。
  VBT 無し（`passthrough` の語を付けない build）。
- `OUTPUT=build/p1-demo-boot plan/tools/boot-test.sh build/p1-demo/hdd-image.img` → **PASS**（`build/p1-demo-boot/login.png`、console の `login:`。QEMU には
  i915 が無いので greeter は終わり getty へ、期待どおり）。

### 未実施・残り

- CI と同じ clang・libcxx・remacs を含む完全なデモの image の build（toolchain の package。main の checkout の package の cache で
  `plan/ws075/demo/build-demo-image.sh BUILD [passthrough]` を走らせる）。root の 1 GiB に収まるかもその build で確かめる。
- 5330 の実機の単独起動（ユーザー）。5330 の passthrough の smoke（この Phase の範囲外）。
- 古い demo の build directory は消していない（main の `build/demo-*` に触れていない）。
