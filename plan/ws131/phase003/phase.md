<!-- awesome-plan project=zedbsd record=ws131-p003 -->

# ws131-p003: libkeiland-backend の土台と network の領域

Status: cleared（Q1 判定 2026-10-03、統合 bfeb2faf8。T1（QEMU、main d169912dd の image）: zdesktop-p013 PASS、settings-regress 8/8 PASS、C1・C2・C9 13/13 PASS。Linux の guest の hwsim の PNG は user「Linuxでのビルドはネイティブで行いましょう」で行わず、FreeBSD は実機の native build と audit）。元の記載: planning（p002 第 2 版はユーザーのレビュー済み（2026-10-03、D7 は確認中）。開始はユーザーの承認と P2 の終了の後に Q1 が指示）
Disposition: normal
Parent: [WS131](../ws.md)、計画の正本 [design.md](../design.md)
Queue: none
依存: p002 の承認（D1・D10・D11 は 2026-10-03 承認済み）。P1 の `libkeiland/zedbsd/network-zedbsd.c` の変更が main に merge 済み — **満たされた**（2026-10-03 確認: P1 の q627 は main `48237bb2a` に統合、q627 は `network-zedbsd.c` に触れず最後の変更は `6efb4f2bb`（2026-10-01）、P1 の worktree に libkeiland・`settings/network.c`・`wayland/network.c` の未 commit の変更は無い。開始の時に Q1 が再確認）。開始はユーザーの承認と P2 の終了の後に Q1 が指示（D8、単独走行 N=1）
目安: 4〜5h（1 Queue）。実行者: Q1 が割り当てる（high）
所有 path: `userland/desktop/libkeiland-backend/`・`libkeiland-backend-zedbsd/`・`-linux/`・`-freebsd/`（新規）、`userland/desktop/libkeiland/`（network の OS の file の移動と転送）、`userland/desktop/wayland/network.c`・`Makefile*`、`keiland-linux.mk`・`keiland-freebsd.mk` の package の一覧、`plan/ws131/`。（Q1 の委任が要る: Q1 が Phase の前に委任を記録する）: `plan/tools/keiland-os-boundary/`・`plan/tools/keiland-linux/makefile-sync.sh`・`network-probe.c`（path だけ）・`plan/ws089/tests/host-network.c`（path だけ）、`platform/amd64/vmunix.mk`（libkeiland.so の source の変数だけ）

## 目的と結果

backend の 3 OS の tree・build・interface の中心を作り、最初の領域として network を移す。終わると compositor の system bar の WiFi は `kl_backend_network_*` を使い、libkeiland の `keiland_network_*` は backend の code への転送（同じ名前・型・意味）になって Settings は無変更で動く。

## 範囲

1. `libkeiland-backend/keiland-backend.h`: 中心（`kl_backend_open`・`close`・`poll_*`・`tick`、`struct kl_backend_host` の枠、`KL_BACKEND_INTERFACE`）と network（今の 11 関数の改名、型も `kl_backend_network_*`）。design.md §3.3。
2. `git mv`: `libkeiland/zedbsd/network-zedbsd.c`・`network-link-zedbsd.c` → `libkeiland-backend-zedbsd/`、`libkeiland/linux/network-link-linux.c` → `-linux/`、`libkeiland/freebsd/network-link-freebsd.c` → `-freebsd/`、`libkeiland/wpa/*` → `libkeiland-backend/wpa/`。zedBSD の `userland/base/net/{protocol,wifi-conf,wifi-store}.c` は backend-zedbsd の source 一覧へ。中の名前は rename の表の `kl_backend_` に機械的に（D10）。
3. build: zedBSD は `libkeiland-backend-zedbsd/sources.mk`（`Makefile` の名前にしない、design.md §3.1）を compositor と（移行の間は）libkeiland の source 一覧が include。Linux・FreeBSD は `KEILAND_LINUX_STATIC`・`KEILAND_FREEBSD_STATIC` で `libkeiland-backend.a`。
4. libkeiland: `keiland_network_*` を backend への転送（`libkeiland/system-compat.c`、p011 で除く）。exports.map は `kl_backend_*` を出さない。
5. compositor: `wayland/network.c` を `kl_backend_network_*` に。backend を `kl_backend_open()` で開く（host の callback は空）。
6. checker（委任）: B1・B3、L2・L3 の path、`makefile-sync.sh` の backend の規則。Guardrail の「移行中」の文を Q1 に渡す（design.md §3.7）。

## 受け入れ

- 3 OS の build（zedBSD の amd64 は exit 0・自前の warning 0、`make keiland-linux` の gcc と clang、FreeBSD の native build と `native-build-audit.py`）と `plan/tools/keiland-os-boundary/check.sh` PASS。
- `libkeiland/` に network の OS の file が無い。`nm -D libkeiland.so` の `keiland_network_*` の集合が前と同じ、`kl_backend_*` が無い。Linux の `libkeiland-backend.a` の未定義の symbol に `zwl_` が無い（B1）。
- zedBSD: boot-test（login の PNG をユーザーに見せる）、C1・C2・C9、`settings-regress.sh`、system bar の WiFi の menu の手順。Linux: guest の `network-probe`、`wifi-setup.sh` の hwsim で system bar の WiFi の PNG。

## 検証の方法

[zedbsd-commands.md](../../tools/keiland-linux/zedbsd-commands.md)、[keiland-linux/README.md](../../tools/keiland-linux/README.md)、[keiland-freebsd/README.md](../../tools/keiland-freebsd/README.md)。QEMU の console・serial の log で判定しない（不具合は gdbstub・monitor・QMP）。build は自分の `BUILD=build/ws131-pNNN/`、共有の `build/` を消さない。FreeBSD の起動の確認は判断 D11 の範囲（passthrough なし）。QEMU と実機の証拠を分ける。

## 衝突・危険・rollback

- 衝突: D8 の単独走行（N=1）の間は他の担当が動かない。P1・P2 が後で再開する時は、Q1 が移した path（`network-zedbsd.c` → `libkeiland-backend-zedbsd/`）を伝える。 順は Q1 が決める（design.md §7.2）。
- rollback: 前へ直すのを基本にし、後ろに同じ file の変更が無い時だけ統合の commit を revert する（design.md §7.4）。

## Resume

依存の Phase の cleared と main への統合、関係する判断の決定の後に、Q1 が Queue を作る。

## 実施の記録（q632、P3 generation3、2026-10-03）— ラップアップ時点、未完

Status は planning のまま（Queue の attempt の結果は Q1 が記録する）。user の指示（2026-10-03「ホストの仮想メモリの状態のせいで、テストが実行できなくなっていると判断しました。すべてのエージェントをラップアップさせて、再開可能なように記録させてください。」）で、zedBSD の criteria の試験の途中でラップアップした。

### Q1 の委任（2026-10-03）

- `plan/tools/keiland-os-boundary/check.sh` の更新（L1 の prune に backend の tree、L2・L3 の対象の path を backend の tree に、B1: backend が compositor の header と `<keiland.h>`・`<keiui.h>` を include しない、B3: app と libkeiland が `keiland-backend.h` を include しない、移行中の例外は `libkeiland/system-compat.c` だけ）。
- 他の WS の試験と道具の path と include の修正（p003 の移動で壊れる所だけ）。調べた結果、壊れる物は無かった（`plan/ws089/tests/host-network.c` は旧 API の stub で移動した file を compile しない。`plan/ws100/tests/host-audio.sh`・`plan/ws089/tests/host-build.sh` は audio と preferences だけ）。直した file は無い。P1 の `plan/ws005/` の文書（phase018・019 の flow.md など）に旧 path の言及が残る（文書だけ、試験ではない）。

### 実装（commit、base は main `5ee632781` を merge した worktree）

| commit | 中身 |
| --- | --- |
| `04e36b4ed` | D7 の計画（phase025 の新設、design.md・ws.md） |
| `381b403b8` | 本体: `git mv` で network の OS の file を backend の tree へ（`libkeiland-backend-zedbsd/network-zedbsd.c`・`network-link-zedbsd.c`、`-linux/network-link-linux.c`、`-freebsd/network-link-freebsd.c`、`libkeiland-backend/wpa/*`）、中の名前を `kl_backend_network_*`・`KL_BACKEND_NETWORK_*`・`KL_BACKEND_WIFI_*` に機械的に改名（D10 の変えない移動）。新規: `libkeiland-backend/keiland-backend.h`（中心と network）、`libkeiland-backend/backend.c`（中心の object）、`libkeiland/system-compat.c`（旧 `keiland_network_*` 11 関数の転送、p011 で除く）、`libkeiland-backend-zedbsd/sources.mk`、`-linux/Makefile.linux`、`-freebsd/Makefile.freebsd`。libkeiland・compositor の Makefile 3 本ずつ、`keiland-linux.mk`・`keiland-freebsd.mk` の package の一覧。compositor の `wayland/network.c` を backend へ、`main.c` で `kl_backend_open`・`kl_backend_close`、`zwl.h` に `server->backend`。`check.sh`（委任） |
| `881f87555` | `main.c` の段落の分け（comment と空行だけ） |
| `43c9fb90c` | **P1 の発見の修正（D10 の例外ではない、意味を変える修正）**: `network-zedbsd.c` の `network_readable` が poll の後に `recv(fd, &byte, 1, MSG_PEEK | MSG_DONTWAIT)` を見る。EAGAIN・EINTR（EWOULDBLOCK が別の値の OS ではそれも）は「まだ」で 0、byte・EOF・他の error は 1。症状: Settings と system bar の join が必ず EIO（2 秒を超える request の全て）。原因: request の後の `shutdown(SHUT_WR)` を zedBSD の kernel の poll が POLLERR で返す（`src/kern/net/unix-socket.c:2523`・`socket.c:1054`）。P1 の試験 client（P1 の worktree の `build/p1-q631-tools/jointest`）で err=5 を再現済み、確かめは統合の後に P1 が新しい path で行う |
| `153f3278b` | `plan/ws131/tools/freebsd-guest.sh`（FreeBSD 15.1 の guest の作り直しの道具、下の注） |

### 済んだ確認（host）

| 確認 | 結果 |
| --- | --- |
| Linux の build（Debian 13 の host の native、gcc） | `make -j32 keiland-linux KEILAND_LINUX_BUILD=build/ws131-p003/linux` exit 0 |
| Linux の build（clang） | `make keiland-linux CC=clang KEILAND_LINUX_BUILD=build/ws131-p003/linux-clang` exit 0 |
| Linux の install と ELF | `make keiland-linux-install … DESTDIR=build/ws131-p003/stage` exit 0、`elf-check.sh` PASS（24 ELF） |
| header | `header-check.sh` PASS（343 sources） |
| makefile-sync | backend の tree は `Makefile.linux` の `keiland-linux-sync: only` の注で対象外にした。残る FAIL「`userland/desktop/wayland` missing `userland/desktop/wayland/apps.c`」は **main（`784c47aba`）でも同じ既存の誤検出**（tool の正規表現が `apps.conf` の中の `apps.c` を source と読む）で、p003 の変更ではない |
| 公開の symbol | Linux の `libkeiland.so` の `keiland_network_*` の集合（11）は main の build と同じ、`kl_backend_*` は出ない。`libkeiland-backend.a` の未定義の symbol に `zwl_`・`keiland_` が無い。compositor は `kl_backend_*` を 17 個持つ |
| 境界の checker | `check.sh` C1〜C5・L1〜L5・B1・B3 PASS（委任の更新の後）。故意の違反（backend に `<keiland.h>`、libkeiland に `keiland-backend.h`）での FAIL の確認は、並行の image の build が `build/packages` を書いていて `make -pn disk-image`（L5）が落ちたため **未完**（再開の時に流す） |
| host 試験 | `plan/ws100/tests/host-audio.sh` 14/14、`plan/ws089/tests/host-build.sh`（settings-render）PASS |
| zedBSD の build | criteria の image（`build-criteria-image.sh build/ws131-p003/criteria`）の build exit 0、自前の warning 0（3 回目の build は `43c9fb90c` を含む）。`network-zedbsd.c` の修正は zedBSD の cross clang の `-Wall -Wextra -Werror -fsyntax-only` も通った |

### 未実施・未完の確認

| 確認 | 状態 |
| --- | --- |
| zedBSD の C1・C2・C9 | **未完**。1 回目は worktree に Venus の strict-queue の renderer（`build/ws035-sq-venus`）が無く greeter の compositor が ZWL EXIT error=5 で起動できず C1 p126 FAIL（環境の誤り、guest の sessiond.log で判定）。main の `build/ws035-sq-venus`・`ws035-fonts`・`ws035-wallpaper` を読み取り専用の symlink にして作り直した 2 回目は C1 p126 FAIL（144 s）と c1-boot-shutdown FAIL（shutdown が `?`）の後、user の指示で中断。host の仮想メモリの状態が試験を妨げていたと user が判断しており、この FAIL は p003 の変更によるものかは**切り分けていない**。中断の時、自分の QEMU（pid 2590524）と image の複写の `cp` が D 状態で kill -9 を受け付けなかった（host の再起動で消える） |
| zedBSD の boot-test | 未実施（予定: lean な network の image（`config-amd64-network.mk`、text の login）で） |
| system bar の WiFi | 未実施（予定: `plan/ws035/tests/build-network-image.sh` と `zdesktop-p013.sh`） |
| `settings-regress.sh` | 未実施（予定: `build-settings-image.sh`） |
| Linux の guest | 使わない（2026-10-03 user「Linuxでのビルドはネイティブで行いましょう」）。`network-probe`・`audio-probe`・compositor の PNG は行わない |
| FreeBSD | **書くだけ**（2026-10-03 user「FreeBSDでのビルド確認は後回しで、書くだけにします」。WS131 の全 Phase で、user の再開の指示まで build・audit・guest を行わない）。FreeBSD の backend の source（`network-link-freebsd.c` の機械的な改名と include の path）と `Makefile.freebsd` は書いた |

### FreeBSD の guest について

- `plan/ws109/guest-plan.json` が指す `/home/awe/zedBSD-claude1/build/ws109-control/`（`disk.qcow2`・`freebsd15.1.qcow2`）は、user の許可で古い `build/` を消した時に一緒に消えていた（Q1）。WS109 の記録の更新は Q1。
- user の「guest を作り直す（推奨）」で、自分の worktree の `build/ws109-control/` に作り直した: 公式の `FreeBSD-15.1-RELEASE-amd64-BASIC-CLOUDINIT-ufs.qcow2.xz`（SHA256 `e4ca4db8…e9e1f` 一致）、展開した base（2.5 GB）、20G の overlay、NoCloud の seed（root は鍵だけ）。道具は `plan/ws131/tools/freebsd-guest.sh`。
- 1 回目の起動は kernel の message の後で 8 分以上止まり、2 回目（同じ overlay）は SeaBIOS の loader が「Loading kernel...」のまま 5 分以上（いずれも QMP の screendump で確認）。次に試すのは `-cpu host`（道具に入れた）と `FIRMWARE=uefi`（OVMF）。その途中で user が FreeBSD の試験を後回しにし、guest を止めた（QMP の quit、process の終了を確認）。image と overlay は残してある。user の再開の指示まで起動しない。

### 再開の手順（host の再起動の後）

1. worktree `/home/awe/zedBSD-worktrees/p3`（branch `agent/p3`、先頭は `153f3278b` の後の記録の commit）に main を merge する。`config.mk` は main からの複写（git に入らない）、`build/sources/remacs`・`build/ws035-sq-venus`・`build/ws035-fonts`・`build/ws035-wallpaper` は main の `build/` への読み取り専用の symlink（無ければ作り直す）。
2. 境界の checker の故意の違反の確認: `backend.c` に `#include <keiland.h>` と `libkeiland/glass.c` に `keiland-backend.h` を一時に足して `sh plan/tools/keiland-os-boundary/check.sh` が B1・B3 で FAIL・exit 1、戻して PASS（image の build と同時に流さない）。
3. zedBSD: `sh plan/ws099/tests/criteria.sh build/ws131-p003/criteria/hdd-image.img build/ws131-p003/criteria-results C1 C2 C9`（image は `43c9fb90c` を含む。source が変わっていれば `build-criteria-image.sh build/ws131-p003/criteria` で作り直す）。FAIL なら main の同じ source で同じ試験を流して p003 の変更との関係を切り分ける。試験の guest は同時に一つだけ、終わったらすぐ止める。
4. network の image（`sh plan/ws035/tests/build-network-image.sh build/ws131-p003/network`）で boot-test（`OUTPUT=build/ws131-p003/boot plan/tools/boot-test.sh build/ws131-p003/network/hdd-image.img`、PNG をユーザーに）と `zdesktop-p013.sh`（system bar の WiFi）。
5. settings の image で `settings-regress.sh`（zedbsd-commands.md §8）。
6. 全て PASS なら Q1 に merge を依頼する（SHA の範囲、直した file の一覧、FreeBSD は書くだけ、P1 の修正 `43c9fb90c` の確かめは統合の後に P1）。

## 実施の記録（q632-i03、P3 generation4・5、2026-10-03、host の再起動の後）

user の承認（2026-10-03「承認します。N=2でP1,P3を起動して再開してください。」）で再開。generation4 は user の操作の副作用で途中で止まり（killed）、generation5 が続けた。

### generation4 が済ませたこと

- 手順 1: main（`de0817643`）の merge = `66eb36e59`（衝突なし）。`config.mk` の複写と `build/ws035-*` の読み取り専用の symlink。
- 手順 2: 境界の checker の故意の違反（`libkeiland-backend/backend.c` に `#include <keiland.h>`、`libkeiland/glass.c` に `#include "keiland-backend.h"`）で B1・B3 FAIL・`keiland-os-boundary: FAIL`（`build/ws131-p003/check-violate.log`）、戻して PASS。generation5 が clean な tree で再実行して PASS（`check-pass5.log`、exit 0）。
- criteria の image を merge 後の source で作り直し（`build-criteria-image.sh build/ws131-p003/criteria`、exit 0、warning 0、`criteria-build4.log`）。
- vgem（`/dev/dri/renderD128`）を host に準備（12:34）。その後の C1・C2・C9（`criteria-results4`）は途中で止まった（判定に使わない）。

### FreeBSD 実機（5320）の native build（generation5）

user（2026-10-03）「…FreeBSDのビルドやテストにSSH経由で利用してOKです。awe@10.0.30.3 です。」「5320にはFreeBSD 15.1がインストールされており、起動しています。」で「書くだけ」を解除。**FreeBSD 実機（Latitude 5320、FreeBSD 15.1-RELEASE amd64、base clang、python3.12）**の上で、専用の `~/zedbsd-p3/` だけを使った。pkg・sudo・system の設定は使っていない（必要な package は既に入っていた）。

| 確認 | 結果 |
| --- | --- |
| source | worktree の `66eb36e59` を `git archive` で `~/zedbsd-p3/src` に複写。distfile（Noto emoji・辞書）は main の `build/distfiles` から複写、seatd は 5320 が fetch |
| build | `gmake -j8 keiland-freebsd KEILAND_FREEBSD_BUILD=build/native-p3` exit 0、`warning:` 0・`error:` 0（`-Wall -Wextra -Werror`） |
| install | `gmake -f userland/desktop/keiland-freebsd.mk install … DESTDIR=~/zedbsd-p3/stage` exit 0、`header-dependencies` exit 0 |
| audit | `python3 plan/tools/keiland-freebsd/native-build-audit.py build/native-p3 ~/zedbsd-p3/stage/opt/keiland` **PASS**（source の所属 343・一意 332、header 640、library 11） |
| 公開の symbol | `libkeiland.so` の `keiland_network_*` は 11 個で Linux の build と同じ集合、`kl_backend_*` は出ない。`libkeiland-backend.a` の未定義の symbol に `zwl_`・`keiland_` が無い（B1）。`wayland` は `kl_backend_*` を 17 個持つ |
| 起動・GUI | 未実施（graphical な確認は Q1 に案を出してから） |

### zedBSD の確認（generation5、QEMU）

| 確認 | 結果 |
| --- | --- |
| 境界の checker | clean な tree で `check.sh` PASS（exit 0、`build/ws131-p003/check-pass5.log`） |
| C1・C2・C9（`criteria.sh build/ws131-p003/criteria/hdd-image.img build/ws131-p003/criteria-results5 C1 C2 C9`） | C1 p126 PASS、c1-boot-shutdown PASS。**C2 c2-geometry FAIL**（13/14: 最初の右下の resize の `ZWL RESIZE settled` が 10 試行の中に見えず。compositor の log では resize は起き settled も後に出ており、以後の 12 項目は ok。試験の SSH が banner exchange で timeout し、348 s かかった）。C9 p052・p053・p072・p076・p126 PASS、**p128 FAIL**（888 s、SSH が全く通らず `ZWL RESIZE settled MISSING`）。このとき host の disk I/O が飽和（13:39 `/proc/pressure/io` full avg10=83.6、image の `cp` が D 状態。Q1 の S1 の image の build と P2 の image の build）で、判定にならないとして run を止め guest を止めた（13:41）。C9 の p134・p137・p138・cursor-owner は未実施。C2・p128 の再試行と C9 の残りは**未実施**（2026-10-03 user「C2 と C9は今はテストしなくていいです。実機テスト後にします。」）。p003 との関係は未切り分け |
| network の image | `build-network-image.sh build/ws131-p003/network` exit 0、compiler の warning 0 |
| boot-test | **PASS**: `OUTPUT=build/ws131-p003/boot plan/tools/boot-test.sh build/ws131-p003/network/hdd-image.img`（道具の既定の /tmp の作業 directory）、`build/ws131-p003/boot/login.png`（networkd・audiod・sshd・greeter が起動し login:）。1 回目は `BOOT_TEST_WORK` を worktree の build に置き I/O の飽和で QMP の screendump が timeout、2 回目は scratchpad の長い path が UNIX socket の 108 byte を超えて QEMU が起動しなかった（条件を変えた再試行） |
| zdesktop-p013 | **FAIL**（`build/ws131-p003/p013/`）。part 1（実の networkd、有線の icon、menu の「No Wi-Fi hardware」と「Wired (em9): connected」、外の click で閉じる）と part 2 の scan（3 件の list）は ok。「Kei Lab」の click で join せず key の入力欄が開き（`ZWL NETWORK key open ssid=Kei Lab`）、以後の join・失敗・off の項目が MISSING。見立て: key の欄は `c26fb2c78`（2026-10-02、ws005-p019・BUG-138）の機能で、user の store に保存の無い secured の network は key 欄にする（`kl_backend_network_get_saved` が store を読む）。p013 の試験は 2026-09-30 の `ab04f7dfd` から変わらず、stand-in の daemon 側の profile だけを用意するので key 欄になる。p003 の source を main（`de0817643`）と改名を除いて比べると `wayland/network.c` は comment・include・型名だけ、`network-zedbsd.c` の差は `43c9fb90c` の `network_readable` だけで、key の判断の経路は同じ → **p003 起因ではなく、既存の機能と試験の不整合**と判断（main の image での実行の切り分けは未実施）。試験の直しは別件（Q1 が Bug にする） |
| settings-regress | **未実施**（2026-10-03 user の新しい試験の方針: QEMU の試験は WS の最後に試験の担当 T1 にまとめる）。settings の image（`build-settings-image.sh build/ws131-p003-settings`、exit 0）は作ってある |

### 再開点（generation5 の終了時）

- p003 の source は `66eb36e59`（main `de0817643` の merge を含む）。FreeBSD 実機の build・audit、Linux の build、zedBSD の build・boot-test は済み。
- 残り: C2・p128 の再試行と C9 の残り（S1 の実機試験の後、user の指示）、settings-regress、p013 の試験の直し（別件）の後の再確認、Linux の hwsim の WiFi の PNG（Linux の guest は使わない方針）、FreeBSD の起動・GUI（Q1 に案を出してから）。QEMU の試験は WS の最後に T1 へまとめて依頼する方針。

## q637: zdesktop-p013 の試験を key 欄の機能に合わせる（P3 generation5、2026-10-03）

user（2026-10-03）「古い試験の修正は、P3が今行って、後回しにしないでください。…」「試験の修正は行うが、試験実行はT1に依頼することにします。」→ 試験の script だけを直し、QEMU では流していない（実行は T1）。base は main `6a60cf49b`（worktree を fast-forward）。

- 原因: `c26fb2c78`（ws005-p019、BUG-138）で system bar は「鍵を要し（secured）user の store に保存の無い network は click で key 欄を開く」ようになった。2026-09-30 の `plan/ws035/tests/zdesktop-p013.sh` は stand-in（`userland/tests/network-probe`）の側の profile だけを前提に「Kei Lab」の click で join を、「Neighbor 5G」で「Could not join」を待っていた。
- 直し方の判断: 機能の意図（ws005-p019 の記録・`network.c` の冒頭の comment）は「保存済みの network は click で join、未保存で鍵の要る network は key 欄 → Enter で store に保存 → PROFILES → JOIN の 3 段（Settings と同じ）」。stand-in は既に `PROFILES_CHANGED`（37）の後に「Neighbor 5G」の join を受ける作りなので、両方の経路を試験にした。
  1. part 2 の前（実の networkd が答える間）に root の `/etc/wifi.conf` を `/tmp/wifi.conf.p013` へ退避し、`net wifi add "Kei Lab" --password p013-test-key`（試験用の架空の鍵）。zdesktop は root で動くので store は `/etc/wifi.conf`。
  2. 「Kei Lab」の click → key 欄なしで join（op=35）、接続、「Disconnect from Kei Lab」（joined.png）。`key open ssid=Kei Lab` が無いことも見る。
  3. 「Neighbor 5G」の click → `ZWL NETWORK key open ssid=Neighbor 5G`・「Key for Neighbor 5G」の行、stand-in に join が届いていない（key.png）。
  4. `p013` + Enter → 「The key must be 8 to 63 characters」（failed.png、文字は残る）。
  5. `-fake-key` + Enter（合わせて 13 文字の架空の鍵）→ `key saved ssid=Neighbor 5G`、stand-in に op=37 と op=35 ssid=Neighbor 5G、state が `wifi=connected ssid=Neighbor 5G`、「Disconnect from Neighbor 5G」（key-joined.png）。
  6. switch で off（off.png、今までどおり）。最後に `/etc/wifi.conf` を元に戻す（退避が無ければ消す）。
- 確認: `sh -n` PASS、qmp-keys.py の `'\n'` が Enter になることを python で確認、`net wifi add` の dispatch・`--password`・networkd への通知の失敗が exit 0 のままであることを source で確認。**QEMU での実行は未実施（T1 に依頼）**。
