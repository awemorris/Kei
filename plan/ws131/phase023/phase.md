<!-- awesome-plan project=zedbsd record=ws131-p023 -->

# ws131-p023: 互換の除去・header の一本化・PnP の接続

Status: planning（p002 第 2 版はユーザーのレビュー済み（2026-10-03、D7 は確認中）。開始はユーザーの承認と P2 の終了の後に Q1 が指示）
Disposition: normal
Parent: [WS131](../ws.md)、計画の正本 [design.md](../design.md)
Queue: none
依存: p016〜p020・p025・p022 cleared。PnP の部分は WS132 の kernel の通知が main にある時だけ
目安: 3〜4h（1 Queue）。実行者: Q1 が割り当てる（high）
所有 path: keiland-ime・kuidemo・xserver・probe・その他の残りの改名、`userland/desktop/keiland/`（`keiui.h` の削除、`keiland-ui.h` の `keiland.h` への統合、互換の block の除去）、backend の device 領域、`wayland/system.c` の devices、`libkeiland/system/`、`plan/ws131/`。（Q1 の委任が要る: Q1 が Phase の前に委任を記録する）: `keiui.h` を使う host の試験 13 file（design.md §2.4）、`plan/tools/keiland-os-boundary/`（B5）

## 目的と結果

旧名（`kui_`・`KUI_`・`keiland_`・`KEILAND_`（paths.h を除く））の使用を 0 にし、互換の `keiui.h` と互換の block を除き、`keiland-ui.h` を `keiland.h` に統合して公開の header を一つにする（2026-10-03 user）。WS132 の成果があれば PnP を backend と拡張に接続する。

## 範囲

1. 残りの利用者（keiland-ime 27 の名前ほか、grep で数える）と host の試験 13 file（review 19）を新名と `<keiland.h>` へ。
2. `keiui.h`・`keiland-ui.h` を除き、`keiland.h` に統合。FreeBSD の header の表を更新。checker B5 を FAIL の条件に。
3. PnP（WS132 があれば）: backend の device 領域、`kl_system_devices_v1`、`kl_system_devices_*`。無ければ残件として WS132 へ。

## 受け入れ

- 3 OS の build（zedBSD の amd64 は exit 0・自前の warning 0、`make keiland-linux` の gcc と clang、FreeBSD の native build と `native-build-audit.py`）と `plan/tools/keiland-os-boundary/check.sh` PASS。
- `grep -rE "\bkui_|\bKUI_|keiui\.h|keiland-ui\.h"` と旧 `keiland_`・`KEILAND_`（paths.h と D16 の対象外を除く）が userland と plan の試験で 0。B5 PASS。
- 全 app の起動（zedBSD と Linux の PNG）、§2.4 の host の試験、boot-test、C1・C2・C9。PnP を接続した時は WS132 の試験。

## 検証の方法

[zedbsd-commands.md](../../tools/keiland-linux/zedbsd-commands.md)、[keiland-linux/README.md](../../tools/keiland-linux/README.md)、[keiland-freebsd/README.md](../../tools/keiland-freebsd/README.md)。QEMU の console・serial の log で判定しない（不具合は gdbstub・monitor・QMP）。build は自分の `BUILD=build/ws131-pNNN/`、共有の `build/` を消さない。FreeBSD の起動の確認は判断 D11 の範囲（passthrough なし）。QEMU と実機の証拠を分ける。

## 衝突・危険・rollback

- 衝突: keiland-ime は WS095・WS102 の file。 順は Q1 が決める（design.md §7.2）。
- rollback: 前へ直すのを基本にし、後ろに同じ file の変更が無い時だけ統合の commit を revert する（design.md §7.4）。

## Resume

依存の Phase の cleared と main への統合、関係する判断の決定の後に、Q1 が Queue を作る。
