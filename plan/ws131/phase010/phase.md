<!-- awesome-plan project=zedbsd record=ws131-p010 -->

# ws131-p010: compositor の拡張の protocol と設定の記録

Status: planning（p002 第 2 版はユーザーのレビュー済み（2026-10-03、D7 は確認中）。開始はユーザーの承認と P2 の終了の後に Q1 が指示）
Disposition: normal
Parent: [WS131](../ws.md)、計画の正本 [design.md](../design.md)
Queue: none
依存: p004 cleared（network と音声が backend にある）。p005 の後なら電源も出す。P2 の BUG-125 の作業の merge 済み（D8 で P2 の終了の後に始めるので満たされる）。判断 D4（決定: compositor は touch IME の UI のために libkeiland を link してよい）・D5・D15
目安: 4〜5h（1 Queue）。実行者: Q1 が割り当てる（high）
所有 path: `userland/desktop/wayland/`（新しい `system.c`・`settings-store.c`・`worker.c`、`protocol.c` の global、`network.c`・`volume.c` の記録と worker）、`userland/desktop/libkeiland/`（新しい `system/`、`keiland.h` の `kl_system_*`、exports.map）、`plan/ws131/tests/`、`plan/ws131/`

## 目的と結果

design.md §4 の拡張 `kl_system_manager_v1`（settings・network・audio・power・devices の枠）と libkeiland の `kl_system_*` を作る。compositor の store（`desktop.conf` の書式・lock・rename）と worker の thread（disk と network の同期の待ちを event loop から外す、review 7）を作る。**毎秒の監視の thread はまだ残す**（Settings が p011 まで直接書くため、review 1）。

## 範囲

1. backend: `kl_backend_peer_uid`（zedBSD は `getpeereid`、`src/libc/openbsd.c:261`。compositor の socket で動くかを最初に確かめる）。device の枠は unsupported。
2. compositor: `system.c`（global の 25 番、registry で同じ uid の client にだけ見せる（`zwl_ime_global_visible` の仕組み）、snapshot・`done`・`request_id`・`result(applied, saved)`・error の列挙、network の要求は一度に一つ・PROFILES は `save_key` の中、design.md §4.1・§4.2）。`settings-store.c`（`libkeiland/preferences.c` の書式を移す）、`worker.c`（store の書き、`save_key`・`get_saved`・`query_details`、250 ms のまとめ、終了・log out の前の flush）。`volume.c` の記録と `network.c:1164`・`:1283` の同期の読み書きを worker へ。
3. compositor と libkeiland（D4 の決定）: compositor は libkeiland の link を続けてよい（touch IME の UI と motion のため）。循環を作らない条件（design.md §9 の D4）: compositor は libkeiland の Wayland の client の部分（`kl_system_*`・`kl_app_*`・`kl_window_*`・file chooser・protocol の wrapper）を使わず、拡張の protocol の定数は `userland/desktop/keiland/` の共有の header から取る。`desktop.conf` の store は compositor の中だけ。
4. libkeiland: `system/`（`kl_system_*`、protocol の `wl_interface` の表は static）、exports.map。
5. 試験（`plan/ws131/tests/`）: host の試験（store の書式・lock・まとめ・flush、protocol の encode と decode、snapshot の途中を見せない、busy）と guest の probe（`kl_system_*` で設定・音量を変える → compositor が記録・別の client に届く・範囲外は invalid・uid の違う client には global が見えない。probe は小さな Wayland の client で、SSH かシリアルで起動し、結果を client の終了の値と guest の file で判定する）。

## 受け入れ

- 3 OS の build（zedBSD の amd64 は exit 0・自前の warning 0、`make keiland-linux` の gcc と clang、FreeBSD の native build と `native-build-audit.py`）と `plan/tools/keiland-os-boundary/check.sh` PASS。
- 新しい host の試験と guest の probe PASS。Settings は旧経路のまま `settings-regress.sh`・`settings-p007.sh` PASS（監視が残るので）。`volume-p004.sh`・`volume-p005.sh` PASS（記録が store を通る）。boot-test、C1・C2・C9。Linux の guest で probe。

## 検証の方法

[zedbsd-commands.md](../../tools/keiland-linux/zedbsd-commands.md)、[keiland-linux/README.md](../../tools/keiland-linux/README.md)、[keiland-freebsd/README.md](../../tools/keiland-freebsd/README.md)。QEMU の console・serial の log で判定しない（不具合は gdbstub・monitor・QMP）。build は自分の `BUILD=build/ws131-pNNN/`、共有の `build/` を消さない。FreeBSD の起動の確認は判断 D11 の範囲（passthrough なし）。QEMU と実機の証拠を分ける。

## 衝突・危険・rollback

- 衝突: WS113 p005 は同じ manager の version 2（D13）で、この Phase の後。 順は Q1 が決める（design.md §7.2）。
- rollback: 前へ直すのを基本にし、後ろに同じ file の変更が無い時だけ統合の commit を revert する（design.md §7.4）。

## Resume

依存の Phase の cleared と main への統合、関係する判断の決定の後に、Q1 が Queue を作る。
