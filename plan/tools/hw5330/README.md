# Dell Latitude 5330 の実機の操作の手引き（OSC のデモの機械）

2026-10-01 に script と plan の記録を読んで書いた。根拠は `file:行` で示す。script を読んで確かめられなかった事柄と
command には **未確認** と書いた。この文書と script だけで操作できるように書いてあるが、規則は [AGENTS.md](../../../AGENTS.md) が優先する。
script が変わったら、この文書の行の番号も直す。

## 0. 先に読む規則（要約）

| 規則 | 根拠 |
| --- | --- |
| 実機を使う試験は `/tmp/i915-hw.lock`（centris の `/tmp`）を `flock` で取ってから、1 つずつ。2 つ同時に走らせない | `plan/ws031/tests/vkloop-hw.sh:41`、`plan/ws075/ws.md:39` |
| `hdmi-h4-hw.sh start` とそれを呼ぶ script（`c5-hw.sh`・`resize-hw.sh`・`measure-apps.sh`・`s13-hw.sh`）を `timeout` で括らない。長いので background で走らせて待つ | `plan/ws075/tests/hdmi-h4-hw.sh:16-20`、`plan/ws075/phase018/phase.md`「実機の harness の事故と対策」 |
| QEMU の console log・serial log を読んで判定しない（解析も含む）。例外はユーザーがその場で許したときだけ | AGENTS.md「検証」、`plan/guardrail.md:53` |
| QEMU（passthrough を含む）の証拠と、5330 を USB から単独で起動した証拠を分けて書く。やっていない確認は「未実施」 | AGENTS.md「検証」、`plan/guardrail.md:59-60` |
| subagent は toolchain（`build/llvm`・`build/NoctLang` 等）を build・変更しない。共有の `build/` の成果物を消さない | AGENTS.md「禁止と承認」 |
| push しない。commit は `git commit -m WIP` だけ | AGENTS.md「git」 |

## 1. 実機の構成

### 1.1 機械と host

| 役割 | 名前・場所 | 根拠 |
| --- | --- | --- |
| 作業・build の host | centris（`10.0.10.2`）。この checkout は `/home/awe/zedBSD-claude1`（以前は `/home/awe/zedBSD-rpi4`） | `hostname`・`ip addr` で確認（2026-10-01） |
| 試験機 | Dell Latitude 5330。CPU i5-1245U（ADL-P）、iGPU `8086:46a8`（subsystem `1028:0b02`）。普段は Linux（hostname `chaos`）が動き、KVM の host を兼ねる | `plan/ws031/ws.md` の「ホスト」の表 |
| ssh の名前 | centris の `~/.ssh/config` の alias `solaris10-man`（2026-10-01 は `HostName 10.0.10.25`）。全ての試験の script の既定の `I915_HOST` | `~/.ssh/config`、`plan/ws075/tests/hdmi-h4-hw.sh:24` |
| 5330 の側の作業の directory | `~/bigbang/`（`run-parity-vk.sh`・`igpu-mode.sh`・`ufs-cat.py`）と `~/bigbang/h4/`（H4 の harness） | `plan/ws031/tests/host/README.md`、`plan/ws075/tests/hdmi/h4-qemu.sh:15` |

5330 の IP は電源の入れ直しで変わったことがある（`10.0.10.25` → `10.0.30.3` → `10.0.10.25`。`plan/ws075/hdmi-main-output.md:76`、
`plan/ws084/ws.md:132`）。届かないときは `~/.ssh/config` を直すのは main の仕事であり、subagent は main に報告する。

### 1.2 二つの使い方

| 使い方 | 何が起きるか | 誰が操作するか |
| --- | --- | --- |
| **passthrough**（日常の試験） | 5330 の Linux の上の QEMU（KVM、q35、4 GiB、4 vCPU、OVMF、NVMe）に、5330 自身の iGPU（`0000:00:02.0`）を vfio-pci で渡して Kei の image を起動する。i915 driver は本物の GPU と本物の内蔵 LCD を駆動する（LCD に実際に映る） | エージェント（centris から ssh） |
| **単独の起動**（bare metal、デモの本番の形） | ユーザーが USB に書いた image で 5330 を UEFI から起動する。Linux は止まっている | ユーザー |

- passthrough の QEMU の引数: `plan/ws075/tests/hdmi/h4-qemu.sh:19-34`（`-device vfio-pci,host=0000:00:02.0,x-igd-opregion=on,rombar=0`、
  `-vga std`、`-serial file:...serial.log`、`-debugcon file:...run.log`、QMP の socket 2 つ、`usb-tablet`・`usb-kbd`）。
- passthrough の guest には OpRegion が見えないので、試験の image は kernel に 5330 の VBT を入れる（`I915_TEST_VBT=y`）。
  `build-demo-image.sh` の `passthrough` の語がこれ（`plan/ws075/demo/build-demo-image.sh:15-17,26-29`）。**USB の単独起動の image には付けない**。
- passthrough には firmware の画面（GOP の点けた pipe）が無いので、firmware の画面の引き継ぎ（takeover）の経路は passthrough では確かめられない
  （`plan/ws084/ws.md:44`）。takeover は単独の起動でだけ確かめられる。
- 単独の起動の間は 5330 の Linux が動いていないので、passthrough の試験はできない（`ssh solaris10-man` が届かない）。

### 1.3 iGPU の割当て

5330 の iGPU は起動時の既定で vfio-pci（`/etc/modprobe.d/vfio-igd.conf`）。Venus の試験だけ host の i915 に付け替える。
切替は `~/bigbang/igpu-mode.sh host|vfio|show`（写し `plan/ws031/tests/host/igpu-mode.sh`）で、QEMU が動いている間は拒む。
passthrough の試験の script は開始時に `ssh "$host" bigbang/igpu-mode.sh vfio` を呼ぶので、手で戻す必要は無い
（`plan/ws075/tests/hdmi-h4-hw.sh:53`、`plan/ws101/tests/hw/gles-hw.sh:39`、`plan/ws031/tests/vkloop-hw.sh` の 108 行付近）。

状態を見るだけなら（**未確認**: 5330 の上の実物の script は写しと違うかもしれない）:

```sh
ssh solaris10-man bigbang/igpu-mode.sh show        # vfio-pci と出れば passthrough の用意ができている
ssh solaris10-man hostname                          # chaos と出れば 5330 の Linux に届いている（未確認）
ssh solaris10-man 'pgrep -af qemu-system-x86_64'   # 動いている QEMU（未確認）
```

### 1.4 画面の見方（passthrough）

| 何を | どうやって | 出る場所 |
| --- | --- | --- |
| firmware の画面（loader の splash、std VGA） | QMP の `screendump`。起動の直後から 250 ms ごとに、resident の buffer ができるまで（最大 180 秒） | `OUTDIR/shots/splash-*.ppm`・`.png`、`OUTDIR/splash.log`（`h4-qemu.sh:42`、`h4-ctl.py:16-17`） |
| i915 が LCD に出している画面 | `ctl shot NAME`: std VGA と resident の 2 つの buffer（A・B）を monitor の `memsave` で読み、`PLANE_SURFLIVE` の register（passthrough の BAR を `xp` で読む）で今 scanout 中の方を決める | `OUTDIR/shots/NAME-vga.png`・`NAME-A.png`・`NAME-B.png`・**`NAME-live.png`（画面に出ている方。判定はこれで見る）**・`NAME.json`（`h4-ctl.py:10-15`、`h4-png.py:2-4,39-42`） |
| 実物の LCD | passthrough でも 5330 の LCD に映る。目で見るのはユーザー（例: `plan/history` の q509 の追い「i915実機で表示されています。」） | — |
| LCD の写真（古い道具） | Windows の workstation のカメラ `C:\Work\qemu-work\tools\capture_lcd.ps1`（写し `plan/ws031/tests/host/capture_lcd.ps1`）。エージェントからは使えない（**未確認**） | — |

`shot` の resident の buffer の場所は kernel の log（debugcon、`run.log`）の `resident display: buffers ...` の行から読む（`h4-ctl.py:62-65,115-124`）。
これは道具が座標を得るためであり、判定には使わない。

### 1.5 入力の入れ方（passthrough）

`plan/ws075/tests/hdmi-h4-hw.sh ctl ARGS...` が 5330 の上で `sudo -n python3 bigbang/h4/h4-ctl.py ARGS` を走らせる（`hdmi-h4-hw.sh:61-64`）。

| command | 意味 | 根拠 |
| --- | --- | --- |
| `pointer move X Y` / `down` / `up` / `sleep MS` / `drag X0 Y0 X1 Y1` | USB tablet。座標は出力の画素（resident の buffer の大きさ。buffer が無い間は 1920x1280 と仮定） | `h4-ctl.py:18,233-260` |
| `keys TEXT` | US 配列の key。`\n` は Enter。使える記号は `h4-ctl.py:72-74` の表だけ（例: `>` `<` `|` `&` `_` `:` `*` `"` `-` `=` `.` `/` `;` と空白） | `h4-ctl.py:19,263-278` |
| `hmp COMMAND...` | QEMU の monitor の command（例: `sendkey meta_l-tab`、`sendkey esc`、`gdbserver tcp:127.0.0.1:1234`） | `h4-ctl.py:20`、`c5-hw.sh:36-43`、`engine-gdb.sh:39` |
| `shot NAME`、`splash S MS`、`load`、`watch`、`latency`、`c6`、`rate`、`freq` | 撮影と計測 | `h4-ctl.py:10-49` |
| `quit` | QEMU を終える | `h4-ctl.py:50` |

1920x1080 の内蔵 LCD（`display=edp`）での App Home の位置: Kei の button は `22 16`、tile は `files:600:386 notes:743:386 settings:887:386
terminal:1031:386 pdf:1175:386 images:1319:386 browser:600:538 mview:743:538 gears:887:538 xterm:1031:538`（`plan/ws075/tests/hdmi/apps8.sh:15`）。
App Home の tile を開く 1 回の操作（`c5-hw.sh:29`）:

```sh
plan/ws075/tests/hdmi-h4-hw.sh ctl pointer move 22 16 sleep 100 down up sleep 1500 move 1031 386 sleep 150 down up sleep 8000
```

`plan/ws075/tests/hdmi/h4-cycle.sh` の座標は以前の 1920x1280 の HDMI の出力のもので（`h4-cycle.sh:6`）、今の eDP の構成では合わない（**未確認**）。

### 1.6 結果の読み戻し（passthrough）

- QEMU の終わりの後、guest の disk の image（5330 の `~/bigbang/h4/guest.img`）から UFS の file を `plan/ws031/tests/ufs-cat.py` で読む。
  `stop` は `/var/log/sessiond.log /var/log/greeter.log /var/log/messages /run/user/0/session.log /run/user/1000/session.log` を
  `OUTDIR/guest-logs.txt` に入れる（`hdmi-h4-hw.sh:90-92`）。
- `/run` は disk に無い（tmpfs）。session の log が要る試験は、Terminal で `cp /run/user/1000/session.log /home/kei/X.log; sync` を打ってから
  QEMU を止め、image から `/home/kei/X.log` を読む（`plan/ws099/tests/c5-hw.sh:48-56`、`resize-hw.sh:102-110`）。
- `fetch` が写すもの: `kernel.log`（debugcon）、`serial.log`、`qemu.log`、`load.log`・`splash.log`・`watch.log`・`c6.log`（あれば）、`shots/`、
  PPM と raw を PNG に（`hdmi-h4-hw.sh:65-79`）。PNG を作るのに host の python3 の PIL が要る（centris には PIL 11.1.0 がある）。
- `kernel.log`・`serial.log` は保存されるが、判定には使わない（0 節）。判定は画面（`*-live.png`）と guest の disk の log で行う。

### 1.7 lock（`/tmp/i915-hw.lock`）

| 項目 | 内容 | 根拠 |
| --- | --- | --- |
| 場所 | centris の `/tmp/i915-hw.lock`。centris の上の全ての checkout と worktree が共有する。centris の外の agent には効かない（advisory） | `h4-lock.sh:8` |
| 取り方（一括の script） | `exec 9>/tmp/i915-hw.lock; flock 9` … `flock -u 9`。空くまで待つ | `test-hw.sh:24-31`、`gles-hw.sh:35-45`、`g3-hw.sh:41-51` |
| 取り方（H4 の harness） | `start` が `setsid nohup hdmi/h4-lock.sh OUTDIR &` を起こす。それが lock を取ると `OUTDIR/.locked` を作り、`OUTDIR/.running` が消えるまで持つ。`start` は lock を得ると `/tmp/i915-h4-owner` に OUTDIR の絶対 path を書く | `hdmi-h4-hw.sh:46-52`、`h4-lock.sh:7-15` |
| H4 の守り | `ctl`・`fetch` は、この tree の run が lock を持つとき（owner の path がこの tree の下で `.locked` がある）だけ動く。持たない `stop` は自分の待ちだけを終える | `hdmi-h4-hw.sh:30-39,62,67,82-87` |
| 返し方 | `stop OUTDIR` が `.running` を消し、`.locked` が消えるのを待ち、owner を消す | `hdmi-h4-hw.sh:93-96` |
| image の build | lock の外で先に済ませる。lock の中で make する script（`vkloop-hw.sh` 系）は、待つ間の tree の変更を拾う | `plan/ws101/tests/hw/run-hw.sh:2-6`、`plan/ws101/phase005/phase.md` の注意 |

空いているかを見る（待たない。空いていれば一瞬だけ取って返す。`plan/ws099/phase016/phase.md:67` が使った形）:

```sh
flock -n /tmp/i915-hw.lock true && echo free || echo busy
cat /tmp/i915-h4-owner 2>/dev/null                      # H4 の run の OUTDIR（あれば）
ps -eo pid,etime,args | grep -E 'h4-lock|test-hw|gles-hw|g3-hw|capture-hw|menu-hw|hdmi-h[12]-hw|bug085-hw|vkloop-hw' | grep -v grep
```

busy のとき: 待つ（`start` は空くまで待つので background で走らせる）か、その Phase では「5330 は lock が使用中で未実施」と記録して先へ進む
（`plan/ws101/phase016/phase.md:5`、`plan/ws099/phase016/phase.md:5` の前例）。他の agent の lock を消さない。

### 1.8 時間の上限

| 物 | 上限 | 根拠 |
| --- | --- | --- |
| H4 の QEMU | `H4_MINUTES` 分（既定 60）。5330 の上で `sudo -n timeout` が QEMU を止める | `hdmi-h4-hw.sh:15,57`、`h4-qemu.sh:35` |
| `c5-hw.sh` | `H4_MINUTES` 既定 20 | `c5-hw.sh:25` |
| `s13-hw.sh` | `H4_MINUTES` 既定 12 | `plan/ws101/tests/demo/s13-hw.sh:18` |
| 1 つの `ctl` | script の中では `timeout 120`（`c6` は 300、`stop` は 400） | `c5-hw.sh:21`、`measure-apps.sh:18,30,37` |
| `run-parity-vk.sh`（gles・g3・vkloop 系） | QEMU は `timeout 360`、呼ぶ側の ssh は `timeout 600` | `plan/ws031/tests/host/run-parity-vk.sh:53`、`gles-hw.sh:41` |
| `run-hw.sh` | 事前の build 3000 秒、run は lock の待ちを含めて 3600 秒 | `run-hw.sh:10,32,41` |

### 1.9 止まったとき・壊れたとき

| 症状 | すること | 誰が |
| --- | --- | --- |
| guest が止まった（画面が変わらない、`ZWL FIRST_FRAME` が出ない） | script はそのまま進んで FAIL を出す。`stop OUTDIR` まで終われば lock は返る。原因の解析は gdbstub・monitor・QMP（下）。kernel.log・serial.log を読んで判定しない | エージェント |
| guest の起動が splash のまま | まず image の boot の行（3.4 節の重複）を疑う。`BUILD/zedbsd-native-uefi-*.cfg` を読む | エージェント |
| script を途中で殺した（H4 の run） | 同じ tree で `plan/ws075/tests/hdmi-h4-hw.sh stop OUTDIR` を走らせる。QEMU を `quit` し、log を写し、lock を返す | エージェント |
| `stop` も動かない（owner が違う等） | 自分の `OUTDIR/.running` を消すと、`h4-lock.sh` が 2 秒以内に lock を返す（`h4-lock.sh:11-15`）。5330 の QEMU は `H4_MINUTES` で止まる | エージェント（自分の run だけ） |
| 一括の script（`test-hw.sh` 等）を殺した | `flock` の fd は process と共に閉じるので lock は返る。5330 の QEMU は `timeout 360` で止まる（**未確認**: 殺した時点による） | エージェント |
| 5330 に ssh が届かない | IP が変わったか、ユーザーが単独の起動をしている。main に報告する。`~/.ssh/config` は main が直す | main・ユーザー |
| 5330 の Linux そのものが固まった | 電源の入れ直しはユーザーだけができる。止めて報告する | ユーザー |
| 他の agent の QEMU が残っている | 消さない。main に報告する | main |

止まった guest を調べる（passthrough、自分の run が lock を持つ間。`plan/ws075/tests/hdmi/engine-gdb.sh:39-40` と同じ形）:

```sh
plan/ws075/tests/hdmi-h4-hw.sh ctl hmp gdbserver tcp:127.0.0.1:1234
ssh -f -N -o ExitOnForwardFailure=yes -L 1234:127.0.0.1:1234 solaris10-man
gdb BUILD/vmunix -ex 'target remote :1234'          # BUILD はその image の build の directory（未確認: 対話の手順）
plan/ws075/tests/hdmi-h4-hw.sh ctl hmp info registers
```

終わったら ssh の転送を止める（`engine-gdb.sh` は `pgrep -f '^ssh -f -N -o ExitOnForwardFailure=yes -L 1234'` を kill する）。

## 2. 単独の起動（USB、ユーザーが行う）

### 2.1 流れ

| 段 | 誰が | 内容 |
| --- | --- | --- |
| 1 | エージェント | USB 用の image を build する（3 節。`passthrough` を付けない、新しい BUILD の名前） |
| 2 | エージェント | QEMU の boot test（`plan/tools/boot-test.sh`）で login prompt まで届くことを確かめ、`login.png` をユーザーに見せる |
| 3 | エージェント（任意） | 同じ source の passthrough の image を**別の BUILD** で作り、`c5-hw.sh` で 5330 の passthrough の smoke を通す（4.2 節） |
| 4 | エージェント | ユーザーに渡す: image の絶対 path、大きさ、`sha256sum`、boot の行（`BUILD/zedbsd-native-uefi-*.cfg` の中身）、確かめてほしい点 |
| 5 | ユーザー | USB に書き、5330 を UEFI で USB から起動し、目で見る。結果を伝える |
| 6 | エージェント | ユーザーの報告を phase.md に「実機（素の 5330、ユーザー）」として、QEMU の証拠と分けて書く |

- image の名前の前例: `build/demo-lcd1` 〜 `build/demo-lcd9`、build の出力を `build/demo-lcd9.log`、boot test を `build/demo-lcd9-boot/`
  （`ls build` で確認）。次は使われていない番号にする。
- USB への書き方と、image をユーザーの機械へ運ぶ方法は plan に記録が無い（**未確認**）。build の script の説明は「write it to a USB stick and boot
  the machine from it (UEFI)」だけ（`build-demo-image.sh:13-14`）。ユーザーには path を伝え、書き方はユーザーに任せる。
- `tools/release/kei-nightly.mk` の `Kei-nightly.zip` は Windows の QEMU（Venus）の配布物であり、5330 の USB 用ではない（`kei-nightly.mk:1-11`）。
- 起動の後、boot の行は image の ESP の `/zedbsd.cfg` を書き換えて変えられる（`build-demo-image.sh:13-14`、
  `docs/reference/kernel-boot-parameters.md:338-343`）。同じ名前の行を 2 回書くと起動が止まる（同 :56、3.4 節）。

### 2.2 ユーザーに頼む確かめ（前例）

| 点 | 前例 |
| --- | --- |
| firmware の画面から Kei の LCD への引き継ぎ（黒い画面・固まりが無い）、greeter または自動の login のデスクトップ | `plan/ws084/ws.md:23,157-165` |
| 操作の軽さ（「1fps」「描画が1秒後」のような報告） | `plan/ws084/ws.md:126,132` |
| USB マウス（logi M650、Logi Bolt の受信機）、タッチパッド、keyboard | `plan/bugs/BUG-105.md` |
| C1: 起動・login・Log Out・Shut Down を 1 回通す（Shut Down で電源が切れるか） | `plan/ws099/ws.md:93`。S5 の電源断は QEMU だけで確認（BUG-119）、実機は未確認 |
| `plan/master.md` の Outlook の「実機の image の確認」の一覧（LCD の takeover、10 app、Files → Image Viewer・Text Editor、Settings、icon ほか） | `plan/master.md:263` |

### 2.3 ユーザーから返るもの

- 言葉の報告（「起動しました」「フリーズしています」等）と、画面の写真（`plan/ws084/ws.md:18-19`）。
- 素の 5330 の Kei に ssh で入った `dmesg` の行（例: `takeover`・`preflight`・`perf:`。`plan/ws084/ws.md:55,157`）。demo の image は sshd を持ち
  （`config-demo-hdmi.mk:16-19`）、root は password `root` と guest の harness の鍵（`plan/tmp/guest/id_ed25519.pub` があれば入る、
  `build-demo-image.sh:41-43`）で入れる。kei の password は `kei`（`plan/ws035/demo/demo-accounts.sh:2-5`）。
- 素の 5330 の Kei の IP は DHCP で決まり、前例は `10.0.30.5`（`plan/ws084/ws.md:142`）。centris から届かなかったこともある
  （`plan/ws084/ws.md:128`「No route to host」）。届くならエージェントが直接入ってよいか、ユーザーに確かめる。入る command（**未確認**、
  `plan/tools/guest/guest.py:128-138` の option に合わせた形）:

```sh
ssh -i plan/tmp/guest/id_ed25519 -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null root@IP dmesg
```

- 素の 5330 の COM1 の有無と、そこからの log の取り方は記録が無い（**未確認**）。

## 3. デモの image

### 3.1 build

```sh
plan/ws075/demo/build-demo-image.sh [BUILD] [passthrough] [make の引数...]     # 既定の BUILD は build/demo-hdmi
```

| 引数 | 意味 | 根拠 |
| --- | --- | --- |
| `BUILD`（1 つ目） | build の directory。出力は `BUILD/hdd-image.img` | `build-demo-image.sh:11,23,47` |
| `passthrough`（2 つ目、任意） | `I915_TEST_VBT=y`（5330 の VBT を kernel に）。passthrough の試験の image だけ。USB 用には付けない | `build-demo-image.sh:15-17,25-29` |
| 残り | そのまま make へ（例: `ZEDBSD_NOCT_ACCEL=n`、`ZEDBSD_BOOT_EXTRA_LINES=...`、`CONFIG_PCAT_SERIAL_MIRROR=y`） | `build-demo-image.sh:17-18,45-46` |

script がすること: App Home の一覧 `plan/ws035/demo/apps.conf`、壁紙（`build/ws035-wallpaper/wallpaper-1080.ppm` があれば。2026-10-01 は無いので入らない）、
Settings の壁紙 5 枚（`userland/desktop/wallpapers/generate.py` で BUILD に生成）、demo の利用者（root/root、kei/kei、kei の自動の login）、
guest の harness の ssh の鍵（あれば）を入れ、`make ZEDBSD_CONFIG=plan/ws075/demo/config-demo-hdmi.mk ... disk-image`（`build-demo-image.sh:30-46`）。

構成 `plan/ws075/demo/config-demo-hdmi.mk` の要点: zdesktop の i915 の構成（`plan/ws031/tests/config-zdesktop-hw.mk`）、Notes・PDF Viewer・Image Viewer、
`ZEDBSD_GRAPHICAL_BOOT := y`（:14）、`ZEDBSD_BOOT_EXTRA_LINES ?= display=edp`（:15、command line で上書きすると `display=edp` も消える）、
openssh（:19）、Settings・audiod（:23）、libkeiui（:26）、`ZEDBSD_NOCT_ACCEL := y`（:29）、gpudemo（:31）。

| 出力 | 値 | 根拠 |
| --- | --- | --- |
| image | `BUILD/hdd-image.img`、2216689664 byte（約 2.1 GiB。root 1 GiB・swap 1 GiB） | `ls -la build/demo-lcd9/hdd-image.img`、`platform/amd64/vmunix.mk:1869-1870` |
| boot の行 | `BUILD/zedbsd-native-uefi-graphical-<y/n>-kmsg-<y/n>[-<追加の行>].cfg`（ESP の `/zedbsd.cfg` になる） | `platform/amd64/vmunix.mk:1879-1880,1900-1907` |
| 最後の行 | `demo image: BUILD/hdd-image.img` | `build-demo-image.sh:47` |
| 時間 | 既存の directory の差分の build で約 2 分（`build/demo-lcd9`: accounts 08:27:18 → image 08:29:00）。新しい directory からの全体の build の時間は記録が無い（**未確認**） | file の時刻 |

規則:
- 版ごとに新しい BUILD の名前にする。C の flag の変更は make に見えない（`build-demo-image.sh:19`）。passthrough の image と USB の image は別の BUILD。
- `build/amd64` は main の checkout の既定の build であり共有の成果物。main は passthrough の smoke に
  `plan/ws075/demo/build-demo-image.sh build/amd64 passthrough ZEDBSD_NOCT_ACCEL=n` を使い、出来た image を `build/ws103/p002-pt2.img` のように
  名前を付けて複写してから試験に渡した（git の履歴の `plan/ws103/phase002/phase.md`、`a9df23e1^`）。subagent は自分の worktree の `build/` の中の名前を使う。
- `ZEDBSD_NOCT_ACCEL=n` は、2026-09-30 に Noct の CMake の cache（`userland/base/noct/noct/build-zedbsd-amd64`）が別の checkout で作られていて
  新しい build directory で Noct の build が失敗したための回避（同上）。2026-10-01 の cache は `/home/awe/zedBSD-claude1` を指すので、今も要るかは
  **未確認**。n にすると `/bin/noct --gpu`（デモの場面 S13）は使えない。USB のデモの本番の image では外す。Noct は toolchain の範囲なので、失敗したら
  直さずに main に報告する。
- 生成された image を渡す前に boot test を通す:

```sh
OUTPUT=build/demo-lcdN-boot plan/tools/boot-test.sh build/demo-lcdN/hdd-image.img     # 成功で 0、OUTPUT/login.png（boot-test.sh:10-16,29-31）
```

GPU の無い QEMU では greeter が終わって console の login に落ちる（`build/demo-lcd9-boot/login.txt`）。それで正常。

### 3.2 boot の行の作られ方

`platform/amd64/vmunix.mk:1900-1907` が `platform/amd64/zedbsd-native-uefi.cfg`（`kernel=vmunix video=640x480 rootpart=... swap0=...`）から作る:

1. `ZEDBSD_GRAPHICAL_BOOT=y`（既定、`Makefile:898`）なら `video=` を落とし `logo=logo.ppm` と `login=graphical` を足す（:1881,1903-1904）。
2. `ZEDBSD_BOOT_KERNEL_MESSAGES=n`（graphical の既定、`Makefile:904`）なら `kmsg=quiet`（:1905）。
3. `ZEDBSD_BOOT_EXTRA_LINES` の語を 1 行ずつ足す（:1906）。

`display=` の値: 無い・`auto`・`hdmi` は HDMI を先に探し、`edp`・`panel` は内蔵の LCD（`plan/master.md:368`）。デモは `display=edp`
（HDMI の touch LCD は 2026-09-29 に外した、`plan/master.md:54`）。

### 3.3 二つの正しい形

| 目的 | command | `/zedbsd.cfg` |
| --- | --- | --- |
| デモ・実機の既定（logo、message を隠す） | `plan/ws075/demo/build-demo-image.sh BUILD` | `logo=logo.ppm` `login=graphical` `kmsg=quiet` `display=edp`（`build/amd64/zedbsd-native-uefi-graphical-y-kmsg-n-display=edp.cfg`） |
| GPU の driver を直す Phase（logo を消し kernel の message を画面に残す） | `plan/ws075/demo/build-demo-image.sh BUILD ZEDBSD_GRAPHICAL_BOOT=n "ZEDBSD_BOOT_EXTRA_LINES=display=edp login=graphical"` | `video=640x480` `display=edp` `login=graphical`（`build/demo-lcd9/zedbsd-native-uefi-graphical-n-kmsg-y-display=edp+login=graphical.cfg`） |

根拠: `plan/guardrail.md:57-58`、`plan/master.md:370`。どちらも passthrough の試験の image なら BUILD の後に `passthrough` を置く。

### 3.4 よく踏む誤り: `login=graphical` の重複で起動が止まる

`ZEDBSD_GRAPHICAL_BOOT=y`（既定）のまま `ZEDBSD_BOOT_EXTRA_LINES` に `login=graphical` を足すと、cfg に `login=graphical` が 2 行入る。
kernel は同じ名前の parameter を誤りとし（`docs/reference/kernel-boot-parameters.md:56`）、`boot: parameter parsing failed (16); entering idle.` で止まる。
画面は splash のままで、`c5-hw.sh` は `ZWL FIRST_FRAME` 0 で FAIL する。2026-09-30 の ws103-p002 で main の regression と見誤られた（`plan/history/queue-q509.md:30`）。
実物の悪い例が `build/amd64/zedbsd-native-uefi-graphical-y-kmsg-n-display=edp+login=graphical.cfg` に残っている（`login=graphical` が 5 行目と 8 行目）。

確かめ方（build の後、渡す前に毎回）:

```sh
cat BUILD/zedbsd-native-uefi-*.cfg
sort BUILD/zedbsd-native-uefi-*.cfg | cut -d= -f1 | uniq -d      # 何か出たら重複（同じ BUILD に古い cfg が残ることもあるので、使った名前の file を見る）
```

`ZEDBSD_BOOT_EXTRA_LINES=display=edp login=graphical` は `ZEDBSD_GRAPHICAL_BOOT=n` と組でだけ使う。

### 3.5 COM1 の serial の mirror（`CONFIG_PCAT_SERIAL_MIRROR`）

- kernel の message を COM1 にも写す option。既定 n（`Makefile:217`）、y で `-DPCAT_SERIAL_MIRROR`（`Makefile:514-515`）。
  `plan/ws075/tests/config-test-hw.mk:5` は y（`vkloop-hw.sh test` の場面の verdict を serial から読むため）。デモの構成は n。
- passthrough では COM1 は 5330 の `~/bigbang/h4/serial.log` になり、`fetch` が `OUTDIR/serial.log` に写す（`h4-qemu.sh:29`、`hdmi-h4-hw.sh:71`）。
- AGENTS.md は serial の log を判定にも解析にも使うことを禁じる。2026-09-30 にユーザーが「シリアルCOM1にdmesgをコピー出力するコンフィグがあります。
  それを使えば、エラーをつかめると思います。」と言い、その切り分けに限り解析に使った（判定には使っていない）。前例の image は
  `CONFIG_PCAT_SERIAL_MIRROR=y`・`ZEDBSD_BOOT_KERNEL_MESSAGES=y`（git の履歴の `plan/ws103/phase002/phase.md`）。使うときは毎回ユーザーの許可を得て、
  phase.md に例外として書く。その前例の build の command の全文は記録が無い（**未確認**。例:
  `plan/ws075/demo/build-demo-image.sh BUILD passthrough CONFIG_PCAT_SERIAL_MIRROR=y ZEDBSD_BOOT_KERNEL_MESSAGES=y`）。

## 4. 実機の試験の script

全て centris の repo の root から走らせる（各 script は自分で `cd` する）。「lock」は 1.7 節の取り方。所要は記録からの目安。

### 4.1 H4 の harness（段ごとに操作する土台）

```sh
plan/ws075/tests/hdmi-h4-hw.sh start IMAGE OUTDIR      # lock を待って取り、image と helper を 5330 の ~/bigbang/h4/ へ、QEMU を起動
plan/ws075/tests/hdmi-h4-hw.sh ctl ARGS...             # 1.5 節
plan/ws075/tests/hdmi-h4-hw.sh fetch OUTDIR            # log と shot を写し PNG に
plan/ws075/tests/hdmi-h4-hw.sh stop OUTDIR             # QEMU を quit、fetch、guest の disk の log、lock を返す
```

- IMAGE は `build-demo-image.sh BUILD passthrough` の `BUILD/hdd-image.img`（の複写）。`start` は lock を得た後に scp するので、待つ間に
  BUILD を作り直すと新しい方が送られる。名前を付けて複写してから渡す。
- `start` から session（kei の自動の login）まで: script は 75〜150 秒待つ（`c5-hw.sh:26`、`measure-apps.sh:21`、`s13-hw.sh:16`）。
- 出力: 1.4 節・1.6 節、`OUTDIR/start.epoch`。`start` は `OUTDIR/.running` があると拒む（`hdmi-h4-hw.sh:46`）。
- 終わりは必ず `stop`。「hdmi-h4-hw: machine given back」が出れば lock は返った（:96）。

### 4.2 一覧

| script | 目的 | 呼び方 | 出力と PASS の印 | 所要 | lock |
| --- | --- | --- | --- | --- | --- |
| `plan/ws099/tests/c5-hw.sh` | **passthrough の smoke の標準**（起動・login・10 app・App Home と Wiseview の開閉の最初の frame、C5） | `plan/ws099/tests/c5-hw.sh IMAGE OUTDIR [ROUNDS]`（既定 3、`LIMIT_MS` 既定 100） | `OUTDIR/session.log`・`first-frames.txt`・`shots/{opened,wiseview,home,saved}-live.png`。`C5-HW RESULT count=N max_ms=M over=K` と **`c5-hw: PASS`**（`count` > 0 かつ全て ≤ LIMIT_MS）。3 rounds で count 34 が前例 | 約 4 分（`build/ws103/p002-hw2`: 21:39:46 → 21:43:54） | H4（`c5-hw.sh:25,57`） |
| `plan/ws099/tests/resize-hw.sh` | BUG-121: Model viewer の角の drag で窓が消えないか | `plan/ws099/tests/resize-hw.sh IMAGE OUTDIR [COUNT]`（既定 20、`ORDER=top` で Model viewer を最後に開く） | `drag-N-live.png` ごとの判定、`events.txt`。`RESIZE-HW RESULT drags=N vanished=V` と **`resize-hw: PASS`**（vanished 0） | 記録なし（**未確認**） | H4（`resize-hw.sh:56,111`） |
| `plan/ws075/tests/hdmi/measure-apps.sh` | 1 run の計測: desktop と 10 app の flip の率・latency、C6、engine の時間、shot 2 枚 | `plan/ws075/tests/hdmi/measure-apps.sh IMAGE VMUNIX OUTDIR`（VMUNIX は `BUILD/vmunix`、`C6_TRIALS` 既定 40）。**OUTDIR を `rm -rf` する**（:15） | `OUTDIR/measure.txt`（`c6 samples:` と `c6:` の行）。PASS の印は無い（計測） | 約 6 分（`build/ws103/p007-measure-now`: 00:45:35 → 00:51:20） | H4（:17,37） |
| `plan/ws075/tests/hdmi/c6.py` | C6 の判定: measure-apps の 5 run 以上をまとめる | `python3 plan/ws075/tests/hdmi/c6.py OUTDIR1 OUTDIR2 ...` | `C6 (pooled median <= 50 ms): met` / `not met`。5 run 未満は「(fewer than 5 runs: not a verdict)」（:18,60-62）。基準の扱いは `plan/master.md` の決定の表の 6 | 数秒（5330 は使わない） | 不要 |
| `plan/ws075/tests/hdmi/apps8.sh` | 走っている H4 の run で App Home の 10 app を順に開き、それぞれ撮る | `plan/ws075/tests/hdmi/apps8.sh [WAIT_MS]`（既定 10000、`APPS=p018` で古い 8 app） | `shots/a-NAME-*.png` | 約 2 分（**未確認**） | 走っている H4 の run の中 |
| `plan/ws075/tests/hdmi/stress-117.sh` | BUG-117: Model viewer の起動と終了の繰り返しで描画が止まらないか | `plan/ws075/tests/hdmi/stress-117.sh [ROUNDS]`（既定 20、apps8 の後に） | round ごとに 1 行。flip 0 で止まる | **未確認** | 走っている H4 の run の中 |
| `plan/ws075/tests/hdmi/engine-gdb.sh` | session ごとの GPU の engine の時間（gdbstub） | `plan/ws075/tests/hdmi/engine-gdb.sh VMUNIX SECONDS`（`RATE_XY="X Y"`） | session ごとの行と rate の行 | SECONDS + 数十秒 | 走っている H4 の run の中 |
| `plan/ws075/tests/hdmi/perf-gdb.sh` | i915 の frame の時間の内訳（gdbstub） | `plan/ws075/tests/hdmi/perf-gdb.sh VMUNIX` | 3 つの窓の合計 | 約 15 秒＋ | 走っている H4 の run の中 |
| `plan/ws075/tests/hdmi/h4-cycle.sh` | logout と login の繰り返し | `plan/ws075/tests/hdmi/h4-cycle.sh OUTDIR COUNT [FIRST]` | `actions.log`、`logoutN`・`loginN` の shot | — | 走っている H4 の run の中。座標は 1920x1280 の HDMI 用（1.5 節） |
| `plan/ws101/tests/demo/s13-hw.sh` | デモの場面 S13（Terminal から `sh /usr/share/gpudemo/s13.sh`） | `plan/ws101/tests/demo/s13-hw.sh [IMAGE] [OUTDIR]`（`BOOT_S` 既定 150、`RUN_S` 既定 60）。image は `plan/ws101/tests/demo/build-s13-image.sh [BUILD]` | `shots/{session,terminal,s13}-live.png`。画面で「The results are the same」を読む（自動の PASS は無い） | 約 5 分（**未確認**） | H4 |
| `plan/ws101/tests/hw/gles-hw.sh` | GLES 3.1 の compute と egltest の feedback・queries | `plan/ws101/tests/hw/gles-hw.sh [OUTDIR]`（`BUILD` 既定 `build/ws101-p010-hw`、`GLES_HW_BUILD_ONLY=1` で build だけ） | `OUTDIR/guest-logs.txt`・`build.log`。**`gles-hw: PASS`**（`GLESCOMPUTE DONE failures=0` が 2 回、`EGLTEST CHECK run=feedback/queries failures=0 glerror=0x0`、`GLES HW DONE`、:50-54） | build ＋ lock 約 6 分（`plan/ws101/phase010/phase.md:32`） | 一括（:35-45、build は lock の外） |
| `plan/ws101/tests/hw/g3-hw.sh` | Noct の GPU の自動並列化（S13 の G3）: CPU と GPU の結果の一致 | `NOCT=PATH/bin/noct plan/ws101/tests/hw/g3-hw.sh [OUTDIR]`（`BUILD` 既定 `build/ws101-p011-hw`、`G3_HW_BUILD_ONLY=1`） | `OUTDIR/s13.txt`・`cpu.txt`・`gpu.txt`。**`g3-hw: PASS`**（同じ結果、`MIX check=1001 wrong=0` が 2 つ、checksum 一致、dispatch ≥ 8、`G3 HW DONE`、:66-72） | **未確認** | 一括（:41-51） |
| `plan/ws101/tests/hw/run-hw.sh` | i915 の試験の場面（vkx・vke1・vke2・vkc・vkcs）を順に | `plan/ws101/tests/hw/run-hw.sh TAG SCENARIO...`（`OUT` 既定 `build/ws101-hw`） | `OUT/hw-SCENARIO-TAG/run.log`（verdict の行）。場面ごとの PASS 数は `plan/ws031/ws.md` の表（vkx 8/8 等） | 1 場面 約 5 分（`plan/ws101/phase005/phase.md` の注意） | `test-hw.sh` が一括で |
| `plan/ws075/tests/test-hw.sh` | 1 つの試験の場面を lock の下で（`vkloop-hw.sh test SCENARIO`） | `I915_HOST=solaris10-man plan/ws075/tests/test-hw.sh SCENARIO OUTDIR`（`BUILD` 既定 `build/resident-vkx`）。**OUTDIR を `rm -rf` する**（:16） | `OUTDIR/run.log`・`serial.log`・`build.log`、`verdict` の行 | 3〜5 分（`plan/ws031/ws.md`「日常のコマンド」） | 一括（:24-31） |
| `plan/ws075/tests/capture-hw.sh` | capture の場面（`vkloop-hw.sh MODE` + `CAPTURE`） | `I915_HOST=solaris10-man plan/ws075/tests/capture-hw.sh SCENARIO MODE OUTDIR`。**OUTDIR を `rm -rf`**（:18） | `OUTDIR/capture/result.json`・PPM・`sheet.png`、`guest-logs.txt` | 3〜5 分 | 一括（:31-38） |
| `plan/tools/titlebar/menu-hw.sh` | System Menu の capture | `I915_HOST=solaris10-man plan/tools/titlebar/menu-hw.sh [OUTDIR]`。**OUTDIR を `rm -rf`**（:13） | `OUTDIR/capture/result.json` | 3〜5 分 | 一括（:17-24） |
| `plan/ws075/tests/hdmi-h1-hw.sh` | HDMI の試験の場面と 5330 の USB の記録（HDMI の LCD 用、今は使わない） | `plan/ws075/tests/hdmi-h1-hw.sh SCENARIO OUTDIR ["-DFLAG=1 ..."]` | `run.log`・`usb-*.txt`・`new-devices/` | — | 一括（:72-73） |
| `plan/ws075/tests/hdmi-h2-hw.sh` | 任意の boot の行の zdesktop の image を起動し scanout の buffer を撮る | `plan/ws075/tests/hdmi-h2-hw.sh OUTDIR "BOOT LINES" [SECONDS...]` | `shots/`（raw・PNG・`sheet.png`） | — | 一括（:40-41） |
| `plan/ws075/tests/bug085-hw.sh` | BUG-085 の capture の run を gdbstub 付きで。止まったら QEMU と lock を残して待つ | `plan/ws075/tests/bug085-hw.sh IMAGE OUTDIR [SCENARIO]`。残ったら `OUTDIR/release` を作ると終わる（:9-16） | `kernel.log`・`watch.log`・`capture/`・`guest-logs.txt` | 最大 `B085_END_SECONDS`（既定 420）＋ | 一括（:33-34）。**lock を長く持ち得る** |
| `plan/ws031/tests/vkloop-hw.sh` | WS031 の土台（上の一括の script が呼ぶ）。自分では lock を取らない | `flock /tmp/i915-hw.lock plan/ws031/tests/vkloop-hw.sh ...`（:41） | `/tmp/vkloop-last.log` 等（共有の `/tmp`） | 3〜5 分 | 呼ぶ側が取る |

- `vkloop-hw.sh` 系（`test-hw.sh`・`run-hw.sh`・`capture-hw.sh`・`menu-hw.sh`・`hdmi-h1/h2-hw.sh`）の verdict の多くは serial の log（kernel の console の写し）から
  読む作りで、AGENTS.md の今の規則より古い。新しい判定に使う前に main に確かめる。デモの image の確かめは H4 の harness（画面と guest の disk の log）で行う。
- 終わった WS の試験は削除されている（例: `plan/ws084/tests/reboot-loop.sh` は計画だけで、存在しない）。

### 4.3 標準の手順: passthrough の smoke（例）

```sh
# 1. image（lock の外）。N は新しい番号
plan/ws075/demo/build-demo-image.sh build/wsNNN/pNNN-pt passthrough > build/wsNNN/pNNN-pt-build.log 2>&1
tail -1 build/wsNNN/pNNN-pt-build.log                                    # demo image: build/wsNNN/pNNN-pt/hdd-image.img
cat build/wsNNN/pNNN-pt/zedbsd-native-uefi-*.cfg                          # 3.4 節: 重複が無いこと
cp build/wsNNN/pNNN-pt/hdd-image.img build/wsNNN/pNNN-pt.img
# 2. lock の確認（1.7 節）
flock -n /tmp/i915-hw.lock true && echo free || echo busy
# 3. 実行（timeout で括らない。Bash の background で走らせ、終わりを待つ）
plan/ws099/tests/c5-hw.sh build/wsNNN/pNNN-pt.img build/wsNNN/pNNN-hw 3 > build/wsNNN/pNNN-hw.log 2>&1
tail -2 build/wsNNN/pNNN-hw.log                                          # C5-HW RESULT ... と c5-hw: PASS
# 4. 画面を見る（判定の補助。ユーザーに見せる）
ls build/wsNNN/pNNN-hw/shots/*-live.png
```

手順 1 の `build/wsNNN/pNNN-pt` という BUILD の名前で全体の build が通るかは **未確認**（新しい directory での Noct の build の失敗の前例、3.1 節）。
失敗したら `ZEDBSD_NOCT_ACCEL=n` を足して作り直し、phase.md にそう書く。

## 5. 運用の規則

1. **同時に 2 つ走らせない。** H4 の run の中の `ctl`・`apps8.sh`・`engine-gdb.sh` 等は、同じ tree の、自分の `start` の後・`stop` の前だけに使う。
2. **QEMU の証拠と実機の証拠を分ける。** phase.md には「5330 の QEMU passthrough（`c5-hw.sh`、OUTDIR）」と「素の 5330（USB、ユーザー、日付、image の名前）」を
   別の行に書き、やっていない方は「未実施」。passthrough の PASS を「実機で動いた」と書かない（`plan/ws103/ws.md:37,54` の書き方が前例）。
3. **OUTDIR の名前:** run ごとに新しい名前。前例は `build/wsNNN/pNNN-hw`・`pNNN-hw2`（再試行に番号）、`build/wsNNN-pNNN/...`、
   `build/ws101-p010-gles-hw`。image の複写は `build/wsNNN/pNNN-pt.img`。`test-hw.sh`・`capture-hw.sh`・`menu-hw.sh`・`measure-apps.sh` は OUTDIR を
   消してから書くので、既にある証拠の directory を渡さない。stdout は `OUTDIR.log` に残す（例: `build/ws103/p002-hw2.log`）。
4. **image は lock の外で先に作り、名前を付けて複写して渡す。** run の間は tree を編集しない（`run-hw.sh:6`）。
5. **busy のとき:** 待つか「lock が使用中で未実施」と記録して進む。他の run の lock・QEMU・`/tmp/i915-h4-owner` を消さない。
6. **画面の PNG はユーザーに見せる**（AGENTS.md「検証」の boot test の規則と同じ扱い）。
7. **ユーザーが要る段:**

| 段 | 理由 |
| --- | --- |
| USB への書き込み、素の 5330 の起動、目視と写真、BIOS の boot の順（USB を先に） | エージェントは機械に触れない（`plan/ws084/ws.md:173`） |
| 素の 5330 の起動の後に 5330 を Linux へ戻す、電源の入れ直し | 同上 |
| 実物の LCD の目視（C1 の黒い画面の有無など） | `plan/ws099/ws.md:27,93` |
| serial の log を解析に使うこと | AGENTS.md の例外（3.5 節） |
| 素の 5330 の Kei に ssh で入ること（IP と可否） | 2.3 節 |
| `~/.ssh/config` の書き換え、toolchain の変更 | main の仕事（AGENTS.md） |

## 6. よくある失敗

| 失敗 | 何が起きたか | 避け方 | 根拠 |
| --- | --- | --- | --- |
| `login=graphical` の重複 | `boot: parameter parsing failed (16)`、splash のまま。main の不具合と誤認 | 3.4 節。cfg を読む | `plan/history/queue-q509.md:30`、git の `plan/ws103/phase002/phase.md` |
| `start` を `timeout` で括った | lock の待ちの間に start が殺され、後の `ctl` が**別の agent の QEMU** に click を送った | 括らない。今の `hdmi-h4-hw.sh` は owner の確認で拒むが、待ちの間に殺すと lock だけ後で取られる | `hdmi-h4-hw.sh:16-20`、`plan/ws075/phase018/phase.md` |
| lock の中の make が途中の tree を拾った | 回帰の実行中に source を書き、`vke1` が BUILD FAILED | image は lock の外で先に（`VKLOOP_BUILD_ONLY=1`・`*_BUILD_ONLY=1`）、run の間は編集しない | `plan/ws101/phase005/phase.md`、`run-hw.sh:2-6` |
| 古い IP・古い既定の host | `test-hw.sh`・`capture-hw.sh`・`menu-hw.sh` の既定は `awe@10.0.30.3`（2026-10-01 の 5330 は `10.0.10.25`） | `I915_HOST=solaris10-man` を明示 | `test-hw.sh:9,20`、`capture-hw.sh:10,20`、`menu-hw.sh:8,15`、`~/.ssh/config` |
| checkout の移動で path が切れた | `gles-hw.sh`・`g3-hw.sh`・`build-s13-image.sh`・`g3-venus.sh` が `/home/awe/zedBSD-rpi4/build/...`（font・`demo-lcd9/bin/noct`・壁紙）を指すが、その directory は無い | `NOCT=/home/awe/zedBSD-claude1/build/demo-lcd9/bin/noct` を渡す（2026-10-01 に存在を確認。最新の source で作った noct かは別）。font の path は script の中で固定なので font は入らない（`[ -f ]` で飛ばす）。直すのは script の持ち主の WS | `gles-hw.sh:19`、`g3-hw.sh:20,24`、`plan/ws101/tests/demo/build-s13-image.sh:12,15` |
| C の flag の違う image を同じ BUILD で作った | make が flag の変更を見ず、古い object のまま | 版ごとに新しい BUILD | `build-demo-image.sh:19` |
| USB の image に `passthrough` を付けた・付けなかった | 素の 5330 は OpRegion の VBT を使い、passthrough は試験の VBT が要る | USB は付けない、passthrough は付ける。別の BUILD | `build-demo-image.sh:15-17` |
| `ZEDBSD_BOOT_EXTRA_LINES` を上書きして `display=edp` が消えた | 構成の既定は `?=` なので command line の値で丸ごと置き換わる | 上書きするときは `display=edp` も書く | `config-demo-hdmi.mk:15` |
| 黒い greeter の画像 | 表示していない buffer A を撮った | `*-live.png` を見る | `plan/ws075/ws.md:66`（ws075-p014） |
| host の電源の入れ直しの直後の最初の run | i915 の start が `time_base_anomaly` で止まり compositor が起動しなかった（BUG-094、2026-09-29 に修正） | 再び起きたら同じ image で 1 回やり直し、BUG-094 を見る | `plan/bugs/BUG-085.md:40`、`plan/known-bugs.md` の BUG-094 |
| capture の image の power-off が終わらない | oneshot の service の `/sbin/poweroff` と init が待ち合う（BUG-095、tracking） | `run-parity-vk.sh` の `timeout 360` で終わる。待ち時間を見込む | `plan/bugs/BUG-095.md:14` |
| passthrough では takeover を確かめられない | QEMU には firmware の画面が無い。素の 5330 で 1 fps・1 秒の遅れ・フリーズが出たが passthrough では出なかった | takeover と単独の起動の性能はユーザーの実機で確かめる | `plan/ws084/ws.md:44,95-100,126-140` |
| 素の 5330 の GPU の周波数が最低のまま | RPS の割込みが未移植で GT が RP0 の 1/12（2026-09-29 に RP0 を要求する形で回避、F-054） | 素の 5330 で遅いときは `dmesg` の `perf:` の行をユーザーに頼む | `plan/ws084/ws.md:149-159` |
| 素の 5330 で USB マウスが効かない | Logi Bolt の HID の 3 interface を parser が拒んだ（BUG-105、host 試験で修正、実機の確認待ち） | ユーザーの確かめの項目に入れる | `plan/bugs/BUG-105.md` |
| Shut Down で電源が切れない | amd64 の Shut Down は以前は halt（ACPI S5 未実装）。BUG-119 で S5 を足したが QEMU だけで確認 | 素の 5330 での電源断はユーザーの確かめ（C1） | `plan/ws075/hdmi-main-output.md:91`、`plan/known-bugs.md` の BUG-119 |
| compositor が落ちた後に greeter が文字の console に落ちる | BUG-122（passthrough でも起きた。sessiond の起こし直しを延ばして解決は QEMU） | 5330 で再び起きたら sessiond の log の `SESSIOND GREETER failed reason=` を読む（guest の disk から、1.6 節） | `plan/ws099/ws.md:76-77` |
| 窓の多いときの描画の停止・窓の消失 | BUG-117（executor の object 表）・BUG-121（角の drag） | 再発の確かめは `stress-117.sh`・`resize-hw.sh` | `plan/known-bugs.md` |
| Notes の起動の後に kernel が止まった | BUG-091（i915 の timer thread の spin lock、ws075-p014 で修正） | 再発したら gdbstub（1.9 節） | `plan/known-bugs.md` の BUG-091 |
| lock を見たら使用中だった | 他の WS（WS075・WS099・WS101）と共有で、長く使われることがある | 1.7 節。未実施と書いて進むか、background で待つ | `plan/ws101/phase016/phase.md:5`、`plan/ws099/phase016/phase.md:67` |
| HDMI 用の座標・道具を使った | `h4-cycle.sh`・`h4-ctl.py watch`（pipe B）・`hdmi-h1/h2-hw.sh` は HDMI の LCD の頃（2026-09-28）のもの。今は eDP（pipe A） | eDP では pipe `A`（`ctl rate A`・`latency A`・`c6 A`、`measure-apps.sh:23-30`） | `h4-ctl.py:68-69`、`plan/master.md:54` |

**注意（2026-10-03）**: iGPU と AX211 を同じ QEMU に同時に passthrough しない（host の hard hang の危険、[Guardrail](../../guardrail.md)）。

**注意（2026-10-03）**: iGPU の passthrough は i915 の driver の改善の Phase だけ（描画の性能の改善は 5330 の Venus）（[Guardrail](../../guardrail.md)）。
