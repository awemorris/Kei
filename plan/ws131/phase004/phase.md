<!-- awesome-plan project=zedbsd record=ws131-p004 -->

# ws131-p004: backend の音声の領域

Status: planning（p002 第 2 版はユーザーのレビュー済み（2026-10-03、D7 は確認中）。開始はユーザーの承認と P2 の終了の後に Q1 が指示）
Disposition: normal
Parent: [WS131](../ws.md)、計画の正本 [design.md](../design.md)
Queue: none
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
