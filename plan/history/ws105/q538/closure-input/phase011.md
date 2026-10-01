<!-- awesome-plan project=zedbsd record=ws105-p011 -->

# ws105-p011: 規約の全文の見直し、境界の確かめの拡張、回帰、install の文書

Status: cleared
Disposition: normal
Parent: [WS105](../ws.md)
Queue: q538 / q538-i01
依存: p001〜p010
実行者: Q1 / main（N=0、単独実行）。`plan/tools/`、Master、WS105 完了も同じ executor が扱う。

## 目的

WS105 の完了の前の、規約の全文による見直し（Awesome Plan の code を作る WS の必須の Phase）、OS の境界の確かめを Linux の側に広げること、Linux と zedBSD の回帰、install の文書。

## 手順

1. **見直し**: WS105 の全ての source の変更（`userland/desktop/libvulkan-compat/`・`*/linux/`・`*/wpa/`・`linux-compat/`・`keiland-linux.mk`・全ての `Makefile.linux`・共通の file の変更・
   `plan/tools/keiland-linux/`）を [coding-style.md](../../coding-style.md) の全文に照らす。差分は `git diff <WS105 の最初の実装の commit の前>..HEAD -- userland/ Makefile`。
   機械の確かめは `python3 plan/tools/style-check.py <file>...`。違反を直す。理由のある例外は記録する。
2. **境界の確かめ**（`plan/tools/keiland-os-boundary/check.sh`、WS104 p008 の物を main が直す）に足す:
   - L1: 共通の source（`zedbsd/`・`linux/`・`wpa/` の外）に `#if defined(__linux__)`・`#ifdef __linux__` が無い（`wayland/zwl-evdev.h` を除く）。
   - L2: `linux/`・`wpa/` の source が `<uapi/`・`"userland/base/net/`・`"userland/base/audiod/` を include しない（zedBSD の物を Linux に持ち込まない）。
   - L3: `zedbsd/` の source が `<linux/`・`<drm/`・`<sound/` を include しない。
   - L4: libvulkan-compat が zedBSD の `userland/desktop/libvulkan/` の file を include・compile しない（D7）。
   - L5: `Makefile.linux` という名前の file が top-level の `Makefile` の include の wildcard に当たらない（`Makefile:260-264`、`userland/*/*/Makefile` だけ）。
3. **install の文書** `userland/desktop/LINUX.md`: 要る host の package（Debian 13 の名前: `build-essential`・`clang`（任意）・`libvulkan-dev`・`linux-libc-dev`・`python3`・`curl`、
   試験には `libwayland-dev`・`libwayland-bin`・`wayland-protocols`・`mesa-vulkan-drivers`・`vulkan-tools`・`qemu-system-x86`・`qemu-utils`・`mmdebstrap`・`e2fsprogs`）、
   build（`make keiland-linux`）、install（`sudo make keiland-linux-install`、`sudo make keiland-linux-install-session`）、起動（text console から root で
   `KEILAND_SEAT=direct /opt/keiland/bin/wayland --session --glass --wallpaper=...`、gdm）、環境変数（`KEILAND_VULKAN_BACKEND`・`KEILAND_VULKAN_NO_DEEPBIND`・
   `KEILAND_DRM_DEVICE`・`KEILAND_SEAT`）、利用者の group（`video`・`input`・`render`・`kvm`（lavapipe の時）・`netdev`・`audio`）、範囲の外（design §8）、仕組みへの link（`plan/ws105/design.md`）。
4. **回帰（Linux）**: `make keiland-linux-clean` の後に gcc と clang で build、`elf-check.sh`・`makefile-sync.sh`・`header-check.sh`、host の p003・p004 の試験、guest の p005〜p010 の確かめを
   **自分の新しい guest の image で通しで**: `sh plan/tools/keiland-linux/build-guest.sh $PWD/build/ws105-p011/guest` と `GUEST_DIR=$PWD/build/ws105-p011/guest`
   （共有の `build/keiland-linux/guest` を `--force` で作り直さない）。
5. **回帰（zedBSD）**: design §9.2 の全部（[WS104 の commands.md](../../ws104/commands.md) の §1・§4・§5・§6・§7・§8、`keiland-os-boundary/check.sh`）。
6. WS105 を完了の形にする（main）: ws.md を書き直す（結果・制限・移管）、Phase の directory と `survey/` を削除し（git の履歴に残る）、続けて使う試験は `plan/tools/keiland-linux/` に残して
   master の Tools 節に登録、[F-065](../../future/F-065-keiland-portable.md) と Future Work の表を更新（Linux は WS105 で済んだ、FreeBSD と design §8 の範囲の外は残る）、Master の registry・Past Log。

## 完了の条件

- 見直しの違反が 0（または理由つきの例外）。check.sh が PASS（L1〜L5 を含む）。Linux と zedBSD の回帰が全て PASS（未実施の物は理由つき）。`LINUX.md` がある。

## 結果

cleared（q538-i01）。WS105の全変更をcoding-style.md全文・Guardrailで見直し、公開コメント・ANSI宣言/整数の形式・意味のあるcall結果/Boolean/void returnを補完。clean buildでlibrary/program同名waylandのobject変数の衝突を再現し、内部変数を分離した。final source `c7e8a35a`、全source manifest / full-review / AST32unit0 / style-check0 / declared-interpreter syntax / diff-check PASS。`LINUX.md`にbuild/install/gdm/direct/環境・権限・制限を記録、OS境界C1〜C5+L1〜L5 PASS。継続fixture dbus-wire.c/.py・seat-fd.cをplan/toolsへ移し、foreign p005/p009・WS・Masterの参照/eventを更新。

Linux: clean GCC14.2/clang19.1.7 warning0、24production ELF各 / RUNPATH / source-sync /331header PASS。host chain1MiB/262144word・bindings0/NO_DEEPBIND・vulkaninfo・WSI90frame×4・D-Bus5case ordinary+ASan/UBSan PASS。own fresh Debian13 gdm imageでKMS両経路6色×4点/oldSwapchain/caller fd、vkdemo、root session/shm/pointer、Home9apps/Terminal echo/Textedit日本語変換・確定、画像/PDF2page、real hwsim/wpa WiFi scan/save/join/disconnect、Settings/bar ALSA readback PASS。root LogOut/SIGTERM/defaultsocket error0/cleanup_failed0・paths0。Notesは既存の手書きUIで、keyboard入力未実装の既知範囲を保持。

最終整数形式補完後のaffected Linux guest経路とclean/hostを再検証。Vulkan window600frame+5×20frame、18import/700acquirefence、実画素RGB、fd29→29、forge out_of_bounds拒否・compositor継続、SIGTERM frames716/error0/cleanup_failed0 PASS。gdmのchooserでKeilandを選択→手動login userkei PID5733、Wayland/runtime1000、SwitchTo/chvtの両方でsamePID/all5leaseを維持、pause10/resume10、復帰echo、HomeLogOut→greeter PASS。実通知はforce、cooperative ACKはsource確認のみ。own guest停止/overlay破棄。

zedBSD: final共通sourceでdisk-image warning0・boot loginPNG・V1/dedicated18/decoder17 ordinary+sanitize・forge拒否後120frame・fence600/generation1・glass p059・Settings8subtest・host audio14/14・volume p004/p005 PASS。C1/C2/C9の元13項目は11PASS/2FAIL。C2/p076のresize不一致をBUG-125へ追加し、ユーザー「バグリストに記載…先に進みましょう。clear判定に進んでいいです」の具体的許可を適用。BUG-125/BUG-127とも未修正tracking、全13PASS・修理済み・Linuxとの因果は主張しない。その他の条件は確認済み。追加の長いresize調査なし。

元のclean-build障害・harness誤期待（ALSA丸め20、VT log spelling、Texteditのunsaved確認、uppercase IME、already-connected network precondition、Filesを含むGPU aggregate count）を保存し、正しいsource/UI/対象別の結果を添付。旧attemptのFAILは改変しない。base-before hashは未採取、overlay isolation/停止後overlay無しを確認。hardware GPU/実機、cooperative pause/hotplug/systemd restart、musl/ARM/FreeBSD・design§8は未実施/範囲外。全標準に新しい例外なし。

証拠: [q538 manifest](../../history/ws105/q538/evidence/SHA256SUMS)、[全文規約の照合](../../history/ws105/q538/conformance.md)、source SHA256、実行script。PNG目視・代表画像を当チャットに提示。console/serial/kernel log判定なし、host package追加0、host /opt変更なし、toolchain変更なし。GitHub未公開・event/intended close/Projectはoutbox保留、pushなし。全Phase clearedだけでWSを自動判定せず、この後L1〜L9を照合してWS完了と記録整理を行う。


実装 commit: `c7e8a35a30f138b766385f2b48183540821c325b`（WIP）。終了 UTC: 2026-10-01T13:23:49.194444+00:00。GitHub は未公開、Phase / WS event と intended close は outbox に保持。

## 文書の参照修正（2026-10-01、p004 の規約照合）

全文規約 §12 により production の試験専用 NO_IMPLICIT_SYNC は実装しないため、install 文書の環境変数一覧から除いた。CPU fallback は能力不足/ioctl の実際の失敗で選ぶ。p011 はこの production の規則と p004 の改訂した検証をそのまま確認する。


## 2026-10-01 最終確認の補完（q538、Q1）

clean gcc build で libwayland と compositor の同名 `wayland` が `KEILAND_LINUX_OBJS_wayland` を上書きし、library の link recipe が program の object 一覧を使うことを再現（`build/ws105-p011/initial-build-failure.md` の tool 観測抜粋、missing object）。library / program / static archive の内部変数を分けた。増分 build の残存 object に依存しない clean gcc / clang、ELF と header/source checks、host と own fresh guest の全受け入れで確かめる。外部の build command・配置・公開 API・依存・既存 WS105 の受け入れは不変。p002 の過去の attempt outcome は保持し、最終 source の conformance で補完する。

全文規約 §3/§11 の公開関数コメントと直接 call return を補完し、§6 の Boolean expression を分解する。評価順、short circuit、error convention、所有権は保持する。errno は libc macro の `__errno_location()` 展開であり、値の取得として手動分類する。機械 AST は semantic review の補助で、代替ではない。

継続利用する `dbus-wire.c` / `.py` と `seat-fd.c` を `plan/tools/keiland-linux/` に移した。試験内容は保持、README と Master に所在地を記録する。Phase archive の過去結果を改変しない。


## q538 の window 操作の結果の扱い

ユーザーの当チャットの move/resize の未修正 tracking と clear 許可を p011 の最終回帰にも適用する。C2 geometry と C9 p076 の resize 失敗は元の FAIL を保持して BUG-125 に追加し、原因や Linux 移植との因果を断定しない。BUG-127 は tracking を保持。これら以外の必須条件は通常どおり検証し、全13件PASSや修復済みとは記録しない。長い resize 調査はしない。

[p005](../phase005/phase.md) と [p009](../phase009/phase.md) へ再利用 fixture の移動の記録と参照先を配布した。内容と過去の結果は不変。origin / foreign Phase / WS の event はそれぞれ outbox で保留する。
