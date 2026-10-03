<!-- awesome-plan project=zedbsd record=ws131-p004 -->

# ws131-p004: backend の音声の領域

Status: cleared（q650、2026-10-03、P1。T2-009 の再試験 PASS、Q1 判定）
Disposition: normal
Parent: [WS131](../ws.md)、計画の正本 [design.md](../design.md)
Queue: q650（2026-10-03 user「backend への移行はP1が今やりましょう。…」、Q1 の割り当て）
依存: p003 cleared（main に統合済み）
目安: 3〜4h（1 Queue）。実行者: Q1 が割り当てる（high）
所有 path: `userland/desktop/libkeiland-backend*/`、`userland/desktop/libkeiland/`（audio の OS の file と転送）、`userland/desktop/wayland/volume.c`・`Makefile*`、`plan/ws131/`。（Q1 の委任が要る: Q1 が Phase の前に委任を記録する）: `plan/ws100/tests/host-audio.sh`（path だけ）、`plan/tools/keiland-linux/audio-probe.c`（path だけ）

## 目的と結果

audio の OS の code を backend へ移す。終わると system bar の音量は `kl_backend_audio_*` を使い、libkeiland の `keiland_audio_*` は転送になる（Settings は無変更）。

## 範囲

1. interface に audio（今の 8 関数の改名）。
2. `git mv`: `libkeiland/zedbsd/audio-zedbsd.c` → backend-zedbsd、`linux/audio-linux.c` → backend-linux、`freebsd/audio-freebsd.c` → backend-freebsd（中の名前は機械的に）。`libkeiland/` の OS の dir はここで空になる。
3. libkeiland: `keiland_audio_*` を `system-compat.c` の転送に。compositor: `volume.c` を backend へ（記録は今の `keiland_preferences_set` のまま、p010 で store）。
4. checker: libkeiland に OS の dir が無いことを確かめる項目。

## 受け入れ

- 3 OS の build（zedBSD の amd64 は exit 0・自前の warning 0、`make keiland-linux` の gcc と clang、FreeBSD の native build と `native-build-audit.py`）と `plan/tools/keiland-os-boundary/check.sh` PASS。
- zedBSD: boot-test、`host-audio.sh`、`volume-p004.sh`・`volume-p005.sh`（`build/ws100-tests/audiod-feedback` の前提、zedbsd-commands.md §8）、`settings-regress.sh`。Linux: guest の `audio-probe`（ALSA）。

## 検証の方法

[zedbsd-commands.md](../../tools/keiland-linux/zedbsd-commands.md)、[keiland-linux/README.md](../../tools/keiland-linux/README.md)、[keiland-freebsd/README.md](../../tools/keiland-freebsd/README.md)。QEMU の console・serial の log で判定しない（不具合は gdbstub・monitor・QMP）。build は自分の `BUILD=build/ws131-pNNN/`、共有の `build/` を消さない。FreeBSD の起動の確認は判断 D11 の範囲（passthrough なし）。QEMU と実機の証拠を分ける。

## 衝突・危険・rollback

- 衝突: P2 と `volume.c`・`Makefile*` を取り合いうる。 順は Q1 が決める（design.md §7.2）。
- rollback: 前へ直すのを基本にし、後ろに同じ file の変更が無い時だけ統合の commit を revert する（design.md §7.4）。

## Resume

依存の Phase の cleared と main への統合、関係する判断の決定の後に、Q1 が Queue を作る。

## 実施の記録（q650、P1、2026-10-03）

base は main（`368385d4f` に merge した worktree `/home/awe/zedBSD-worktrees/p1`）。Q1 の q650 の指示で、checker（`plan/tools/keiland-os-boundary/check.sh`）と、移動で壊れる他の WS の試験（`plan/ws100/tests/host-audio.*`、`plan/ws089/tests/host-build.sh`）を直した（委任）。

### 変えたこと

- `git mv`: `libkeiland/zedbsd/audio-zedbsd.c` → `libkeiland-backend-zedbsd/`、`libkeiland/linux/audio-linux.c` → `libkeiland-backend-linux/`、`libkeiland/freebsd/audio-freebsd.c` → `libkeiland-backend-freebsd/`。中の名前は機械的に `keiland_audio_*` → `kl_backend_audio_*`、`KEILAND_AUDIO_*` → `KL_BACKEND_AUDIO_*`、`<keiland.h>` → `keiland-backend.h`（D10 の変えない移動）。`libkeiland/` の OS の dir（zedbsd・linux・freebsd）はこれで無くなった。
- `libkeiland-backend/keiland-backend.h`: audio の領域（`struct kl_backend_audio_state`、`KL_BACKEND_AUDIO_CHANGED_*`、8 関数）。
- `libkeiland/audio-compat.c`（新規）: 旧 `keiland_audio_*` 8 関数を backend へ転送（同じ名前・型・意味、change bit の一致は `#if` で確かめる）。design.md は転送を `system-compat.c` に置くとしていたが、別の file にした。理由: Settings の host 試験（`plan/ws089/tests/host-build.sh`）は network を `host-network.c` の作り物で置き換えて audio だけを要るので、network の転送と同じ file だと重複の定義になる。p011 で `system-compat.c` と一緒に除く。
- build: `libkeiland-backend-zedbsd/sources.mk`・`-linux/Makefile.linux`・`-freebsd/Makefile.freebsd` に audio、libkeiland の 3 つの Makefile から OS の audio を外し `audio-compat.c` を足した。
- compositor: `wayland/volume.c` を `kl_backend_audio_*` に（preferences は今の `keiland_preferences_*` のまま、p010 で store）。
- checker: L6（`libkeiland/` に zedbsd・linux・freebsd の dir が無い）を足し、B3 の例外に `audio-compat.c` を足した。
- 試験: `host-audio.c` は backend の API（`kl_backend_audio_*`）を直に試すように改名、`host-audio.sh` は新しい path。`host-build.sh` は `audio-zedbsd.c`（新しい path）と `audio-compat.c` を compile し、古い layout の object（`shared-audio.o`）が `host-wallpaper.sh` の glob に拾われないよう obj の `.o` を最初に消す。

### 確認（host）

| 確認 | 結果 |
| --- | --- |
| zedBSD の build（`ZEDBSD_CONFIG=plan/ws089/tests/config-amd64-settings.mk BUILD=build/p1-q640`、`dynamic/libkeiland.so`・`bin/wayland`・`bin/settings`） | exit 0、`warning:` 0 |
| zedBSD の公開の symbol | `libkeiland.so` の `keiland_audio_*` 8 と `keiland_network_*` 11 は前と同じ集合、`kl_backend_*` は出ない。compositor は `kl_backend_audio_*` を 8 個持つ |
| Linux の build（host の native、gcc・clang、`KEILAND_LINUX_BUILD=build/ws131-p004/linux`・`linux-clang`） | どちらも exit 0、`warning:` 0 |
| Linux の symbol | `libkeiland.so` の `keiland_audio_*`・`keiland_network_*` は 19、`kl_backend_*` は出ない。`libkeiland-backend.a` の未定義の symbol に `zwl_`・`kwl_`・`keiland_` が無い（B1）。compositor は `kl_backend_audio_*` を 8 個持つ |
| Linux の install と ELF | `keiland-linux-install` exit 0、`elf-check.sh` PASS（24 ELF）、`header-check.sh` PASS（346 sources） |
| `audio-probe.c` | 新しい `libkeiland.so` に link できる（実行は guest の mixer を変えるので host では流さない。Linux の guest は使わない方針） |
| makefile-sync | p003 と同じ既存の誤検出（`apps.conf` の中の `apps.c`）だけ |
| 境界の checker | `check.sh` PASS（C1〜C5・L1〜L6・B1・B3）。`libkeiland/linux` を一時に作ると `L6 FAIL`・exit 1、消して PASS |
| host 試験 | `host-audio.sh` 14/14、`host-build.sh` と `host-wallpaper.sh` PASS |
| FreeBSD | 書くだけ（2026-10-03 user。build・audit・guest は行わない） |

### QEMU の試験（T1・T2 に予約、結果待ち）

boot-test、`volume-p004.sh`・`volume-p005.sh`（volume の image、`build-volume-image.sh`）、`settings-regress.sh`（settings の image）。

## 結果（Q1、2026-10-03、T2-006、QEMU）

uncleared。boot-test PASS、settings-regress PASS 8/8。volume-p004 FAIL ×2: 1 回目は session の画面が出ず（HANDOFF go=1 MISSING 以下全て、fstrim で host の I/O が止まっていた時間）、再試行は `mute off: audiod unmuted (1) FAIL` の 1 項目だけ。volume-p005 は 1 回目 FAIL（desktop.conf の保存・release の音）、再試行 PASS。証拠 worktrees/t2/build/t2-006/。mute off は backend の移行（audio-compat の mute の引き渡し）の退行の疑い。再開: P1 が mute off を調べて直し、BUG-161（ws100-p012）の試験と合わせて T2 で volume-p004・p005 を再試験。desktop.conf の項目は BUG-161 の新しい項目に置き換わる。

## 再試験（Q1、2026-10-03）

cleared。T2-009（QEMU、efc5ea7fc、build warning 0）: volume-p004 PASS（136 s）、volume-p005 PASS（83 s）、volume-bug153 PASS（70 s）。証拠 worktrees/t2/build/t2-009/。実機は未実施（mute の確認を最長約 4 s 待つように直した試験で、mute off も ok）。
