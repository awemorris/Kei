<!-- awesome-plan project=zedbsd record=ws129-p010 -->
# ws129-p010: desktop の全 app を CI と試験の config に入れる

Status: cleared 候補（q606-i01、2026-10-02、P4。下の未確認の点があるので判定は Q1）
Disposition: normal
Parent: [WS129](../ws.md)

## 範囲（2026-10-02 user「イメージにSettingsアプリが入っていませんでした。CIもテスト用も、desktopのアプリはすべてコンフィグを追加しておいてください。ただしCIではテストは含みません。」）

1. `userland/desktop/` の app（Settings・audiod など、App Home の apps.conf に載る全 app とその daemon）を一覧にし、`config/ci/config-amd64.mk`（と他の CI の desktop を持つ config）の `ZEDBSD_USER_PROGRAMS` に足す。CI には `userland/tests/` の試験は入れない。
2. 試験用の config（`plan/ws035/tests/config-amd64-userland.mk`、各 WS の試験 config、デモの config）にも全 app を足す（試験は従来どおり）。
3. 足した後の image の build（warning 0）、root partition（1 GiB）・inode に収まるか、`boot-test.sh`。App Home の全 tile が起動できるかを QEMU の Venus で確かめる。
4. `config/ci/` の変更はこの Phase で main が許可する（ユーザーの指示）。

## 所有 path

`config/ci/`、`config.mk`（desktop の app の行だけ）、試験とデモの config、`plan/ws129/phase010/`。


## 2026-10-02 user の追加

「/bin/testを入れてください。というか、テストでないbaseはすべて入れてください。」→ 範囲に追加: `userland/base/` の試験でない program（`/bin/test` を含む）をすべて CI と試験用の config に入れる。`userland/tests/` の試験は CI に入れない。追加した program の一覧と、root partition・inode に収まるかを記録する。

## 結果（q606-i01、2026-10-02、P4、worktree `/home/awe/zedBSD-worktrees/p4`、main 808be6155 に合わせて実行）

承認: user「CIもテスト用も、desktopのアプリはすべてコンフィグを追加しておいてください。ただしCIではテストは含みません。」「/bin/testを入れてください。というか、テストでないbaseはすべて入れてください。」、
Q1 の判断（2026-10-02）: 既定の apps.conf を wayland の package の data に（主）、`home.c` の内蔵の予備の一覧に Settings を足し wltest・wlshm を外す（従）。

### 足した物（package の一覧は `make list-user-programs` 相当の metadata から、menu が base・desktop の選べる物を照合）

| config | 足した program |
| --- | --- |
| `config/ci/config-amd64.mk`（Keiland の CI） | settings、keiland-ime、ime-dict-ja、test、zterm（App Home の X terminal）、zedinst |
| `config/ci/config-intelmac.mk` | audiod、diskpart、libgif-compat、libjpeg-compat、libpdf、libpng-compat、libz-compat、lspci、lsusb、mkfs、mkswap、sync、zedinst |
| `config/ci/config-pcat.mk`（i386） | audiod、diskpart、libgif-compat、libjpeg-compat、libpdf、libpng-compat、libz-compat、mkfs、mkswap、zedinst |
| `config/ci/config-pc98.mk` | audiod、libgif-compat、libjpeg-compat、libpdf、libpng-compat、libz-compat、lspci、lsusb |
| `plan/ws035/tests/config-amd64-userland.mk`（試験の image の土台。menu・files・ime・guest・zdesktop-hw 等が include） | base: audiod base64 install mktemp pwd zedinst libgif-compat libjpeg-compat libpdf libpng-compat libz-compat、desktop: libvulkan libwayland-client libwayland-egl libegl libglesv2 libgl libtruetype libkeiui wayland sessiond terminal files notes pdfviewer imageview textedit settings browser libbrowser xserver zgears keiland-ime ime-dict-ja |

- 足さなかった物: 試験の client（ime-probe・venus-frame・glxtest、`userland/tests/` の全て）は CI に入れない。試験の config は従来どおり各 config が足す。
- **CI に既にある試験**: `config-amd64.mk` に `mview`・`vkdemo`（menu が tests）が以前から入っている。「CI ではテストは含みません」に照らして外すかは判断待ち（mview は App Home の Model viewer の tile）。
- デモの config（`plan/ws075/demo/config-demo-hdmi.mk`）は CI の config を include するので自動で増える。
- 試験の config で `config-amd64-userland.mk` を土台にしない古い物（ws004・ws014・ws029・ws031 の mview・ws035 の pc98/pcat/sun4u/x68k・ws045 の base 等、約 20）は変えていない（多くは完了した WS の目的別の config）。

### App Home（Settings が CI の image で出る）

- `userland/desktop/wayland/apps.conf`（新）: 既定の一覧。中身はデモの `plan/ws035/demo/apps.conf` と同じ 11 の tile（Files・Notes・Settings・Terminal・PDF Viewer・Image Viewer・Text Editor・Browser・Model viewer・Gears・X terminal）で、差は先頭の注釈だけ。
- `userland/desktop/wayland/Makefile`: wayland の package が `/etc/keiland/apps.conf`（0644）に入れる。ただし image が extra files で自分の apps.conf を持ってくる時（デモ・試験の build）は入れない（同じ行き先が 2 つになると image の作成が止まるため）。
- `userland/desktop/wayland/home.c`: apps.conf が無い時の内蔵の一覧に Settings を足し、試験の client（Vulkan test = wltest、Shared memory = wlshm）を外した。

### 確認

- CI の image: `make -j40 -o build/NoctLang/.zedbsd-source-2.0.1-zedbsd12 ZEDBSD_CONFIG=config/ci/config-amd64.mk BUILD=build/p4-ci disk-image` → exit 0、`build/p4-ci/hdd-image.img` SHA-256 `f201142d8e360355a82a6b527c69bcd456f740580c307805f0e38723c005fc53`。
  （`-o` は、worktree の checkout で noct の patch の mtime が共有の `build/NoctLang` の stamp より新しくなり、make が共有の tree を作り直そうとして拒まれるのを避けるため。toolchain は変えていない。）
  zedBSD の source の warning 0（warning は openssl・openssh・llvm の外部 source と host の noct の interpreter.c の既存の 1 件だけ）。
- 入った file（UFS の image を `tools/build/check-ufs-image.py` の reader で照合）: `/bin/settings`、`/bin/test`、`/usr/libexec/keiland-ime`、`/usr/share/kei/ime/ja/SKK-JISYO.{X,kei}`、`/bin/zterm`、`/sbin/zedinst`、`/etc/keiland/apps.conf`。
- **root partition**（`plan/ws129/phase010/rootfs-usage.py`）: zedBSD-root 1024 MiB のうち使用 339.3 MiB・空き 684.7 MiB（66.9%）、inode 65,536 のうち使用 1,023（空き 98.4%）。収まる。
- intelmac の CI の image: build 成功（`build/p4-ci-intelmac`、payload の partition の形で zedBSD-root を持たない）。
- **pcat・pc98（i386）の CI の image は未確認**: i386 の sysroot（`build/i386/sysroot`）が main の build の整理で無く、作るのは toolchain の作業なので行っていない。
- `plan/tools/boot-test.sh build/p4-ci-plain.img`（CI の image そのもの）→ PASS（`build/p4-boot-ci/login.png`）。
- **App Home の全 tile**（CI の config ＋ guest の harness の extra files だけの image `build/p4-ci-harness.img`、SHA-256 `6d1ef175…6d41d`、QEMU の Venus、`plan/ws129/phase010/apphome.sh`）: image の apps.conf が既定と同じ。Home に 11 の tile（Settings を含む）。
  Files・Notes・Settings・Terminal・PDF Viewer・Image Viewer・Text Editor・Browser・Gears・X terminal が窓を出した（Settings は 1 回目は試験の数え方で NO WINDOW と出たが log に `ZWL MAP client=4`、画面 `evidence/03-settings.jpg` で起動を確認）。
  **Model viewer は Home から 8 つ目の app として起動すると `MVIEW FAILED api=vkAllocateMemory result=-4`** で終わる（単独で起動すると動く）。QEMU の Venus の hostmem（256 MiB）を前の 7 つの app が使った後の資源の不足と見る（実機では未確認）。PNG: `evidence/00-home.jpg`・`03-settings.jpg`・`99-all.jpg`。

### 残り

- CI の `mview`・`vkdemo`（試験）を外すかの判断。
- i386 の CI の image（pcat・pc98）の build の確認（i386 の sysroot が要る）。
- `plan/tools/files/files-p011.sh` は内蔵の一覧の数（7）を前提にしており、内蔵の一覧の変更（Settings を足し wltest・wlshm を外した）で数が変わる見込み（未実行）。ws127-p002 の「試験の直し」で扱える。


## 2026-10-02 user の判断と Q1 の反映

- 「Model viewer（mview）と vkdemo を CI の image から外します。これらはGPUドライバやVulkan実装のテストでのみ使用していきましょう。」→ Q1 が `config/ci/config-amd64.mk` から mview・vkdemo を外し、既定の apps.conf とデモの apps.conf から Model viewer の tile を消した（試験の config には残す）。BUG-144 は CI の image では起きなくなるが、GPU の試験の image での扱いとして tracking に残す。
- 「i386はしばらくテストもビルドもしなくていいです。」→ pcat・pc98 の CI の config の追加は build で確かめない（i386 の確認は免除）。amd64 の範囲で受け入れを満たすので、この Phase は cleared（Q1、2026-10-02）。
