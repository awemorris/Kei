<!-- awesome-plan project=zedbsd record=ws131-p014 -->

# ws131-p014: 旧 libkeiland の名前を kl_・KL_ に

Status: planning（p002 第 2 版はユーザーのレビュー済み（2026-10-03、D7 は確認中）。開始はユーザーの承認と P2 の終了の後に Q1 が指示）
Disposition: normal
Parent: [WS131](../ws.md)、計画の正本 [design.md](../design.md)
Queue: none
依存: p013 cleared、p011 cleared（旧 network・audio・preferences を除いた後）。判断 D16
目安: 3〜4h（1 Queue）。実行者: Q1 が割り当てる（high）
所有 path: `userland/desktop/libkeiland/`、`userland/desktop/keiland/keiland.h`、共有の source の内部の名前（`userland/desktop/picture/`・`artwork/` の `keiland_*`）、`plan/ws131/`

## 目的と結果

libkeiland の公開の名前（`keiland_*`・`KEILAND_*`、[rename-map.md](../rename-map.md) の 266、`paths.h` の install の path の macro を除く）を `kl_`・`KL_` にし、`keiland.h` の終わりに旧名の互換の macro の block を置く。共有の source の内部の `keiland_` も `kl_` に（D16）。ABI は変えてよく版は上げない（`KL_VERSION` は 21）。

## 範囲

1. rename-map の表から機械的に改名。`KL_EDIT_*`・`KL_KEYBOARD_INSET_*` を一本化。
2. 互換の macro の block（`KL_COMPAT` が無い時、p023 で除く）。
3. exports.map を header から作り直す。
4. 利用者（app・xserver・probe）は互換の macro で無変更。

## 受け入れ

- 3 OS の build（zedBSD の amd64 は exit 0・自前の warning 0、`make keiland-linux` の gcc と clang、FreeBSD の native build と `native-build-audit.py`）と `plan/tools/keiland-os-boundary/check.sh` PASS。
- `nm -D libkeiland.so` に `keiland_` が無い（protocol の識別子は D15 で p021）。B4 PASS。
- p012 と同じ回帰に、xserver・menu-probe・titlebar-probe の build と、titlebar の probe の手順（`plan/tools/titlebar/`）。

## 検証の方法

[zedbsd-commands.md](../../tools/keiland-linux/zedbsd-commands.md)、[keiland-linux/README.md](../../tools/keiland-linux/README.md)、[keiland-freebsd/README.md](../../tools/keiland-freebsd/README.md)。QEMU の console・serial の log で判定しない（不具合は gdbstub・monitor・QMP）。build は自分の `BUILD=build/ws131-pNNN/`、共有の `build/` を消さない。FreeBSD の起動の確認は判断 D11 の範囲（passthrough なし）。QEMU と実機の証拠を分ける。

## 衝突・危険・rollback

- 衝突: 全 app の compile に触れる（source は無変更）。D8 の単独走行の間は他の担当が動かない。 順は Q1 が決める（design.md §7.2）。
- rollback: 前へ直すのを基本にし、後ろに同じ file の変更が無い時だけ統合の commit を revert する（design.md §7.4）。

## Resume

依存の Phase の cleared と main への統合、関係する判断の決定の後に、Q1 が Queue を作る。
