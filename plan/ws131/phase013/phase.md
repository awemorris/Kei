<!-- awesome-plan project=zedbsd record=ws131-p013 -->

# ws131-p013: 旧 libkeiui の名前を kl_・KL_ に

Status: planning（p002 第 2 版はユーザーのレビュー済み（2026-10-03、D7 は確認中）。開始はユーザーの承認と P2 の終了の後に Q1 が指示）
Disposition: normal
Parent: [WS131](../ws.md)、計画の正本 [design.md](../design.md)
Queue: none
依存: p012 cleared
目安: 3〜4h（1 Queue）。実行者: Q1 が割り当てる（high）
所有 path: `userland/desktop/libkeiland/ui/`、`userland/desktop/keiland/`（新しい `keiland-ui.h`、互換の `keiui.h`、`keiland.h` の include）、`plan/ws131/tools/rename-map.py`、`plan/ws131/`

## 目的と結果

旧 libkeiui の 341 の公開の名前を [rename-map.md](../rename-map.md) のとおり `kl_`・`KL_` にする（2026-10-03 user の最終の規則）。`keiui.h` は旧名 → 新名の macro だけの互換の header、新しい宣言は `keiland/keiland-ui.h`（`keiland.h` が include）。app の source は無変更で build・動作する。

## 範囲

1. `keiui.h` の中身を `keiland-ui.h` に移して改名。`ui/` の source の公開名も改名（内部の `keiui_*` 55 個は保つ）。改名は rename-map の表から機械的に行い、手で改名しない。
2. 例外: `kui_version`・`KUI_VERSION` を除く（`kl_version` は p014 で）、`kui_edit_fn` → `kl_window_edit_fn`、`kui_keyboard_inset_fn` → `kl_window_keyboard_inset_fn`、`KUI_EDIT_*`・`KUI_KEYBOARD_INSET_*` は p014 で `KEILAND_*` と一本化するまで `KL_*` の定義を一つ置き、既存の `KEILAND_*` は p014 まで残す。
3. 互換の `keiui.h`: `#include <keiland.h>` と旧名の macro の列（tool で生成）、`KUI_VERSION 12U`。
4. FreeBSD の公開の header の表に `keiland-ui.h` を足す。

## 受け入れ

- 3 OS の build（zedBSD の amd64 は exit 0・自前の warning 0、`make keiland-linux` の gcc と clang、FreeBSD の native build と `native-build-audit.py`）と `plan/tools/keiland-os-boundary/check.sh` PASS。
- 利用者の source の diff が 0。`nm -D libkeiland.so` に `kui_` が無く、旧 `kui_*` の全てに `kl_*` がある。
- p012 と同じ回帰。

## 検証の方法

[zedbsd-commands.md](../../tools/keiland-linux/zedbsd-commands.md)、[keiland-linux/README.md](../../tools/keiland-linux/README.md)、[keiland-freebsd/README.md](../../tools/keiland-freebsd/README.md)。QEMU の console・serial の log で判定しない（不具合は gdbstub・monitor・QMP）。build は自分の `BUILD=build/ws131-pNNN/`、共有の `build/` を消さない。FreeBSD の起動の確認は判断 D11 の範囲（passthrough なし）。QEMU と実機の証拠を分ける。

## 衝突・危険・rollback

- 衝突: p012 と同じ。 順は Q1 が決める（design.md §7.2）。
- rollback: 前へ直すのを基本にし、後ろに同じ file の変更が無い時だけ統合の commit を revert する（design.md §7.4）。

## Resume

依存の Phase の cleared と main への統合、関係する判断の決定の後に、Q1 が Queue を作る。
