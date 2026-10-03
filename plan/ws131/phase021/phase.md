<!-- awesome-plan project=zedbsd record=ws131-p021 -->

# ws131-p021: compositor の内部の名前を kwl_・KWL_ に（log の文字列は変えない）

Status: planning（p002 第 2 版はユーザーのレビュー済み（2026-10-03、D7 は確認中）。開始はユーザーの承認と P2 の終了の後に Q1 が指示）
Disposition: normal
Parent: [WS131](../ws.md)、計画の正本 [design.md](../design.md)
Queue: none
依存: p009・p011 cleared。compositor に他の作業が無い時（P2・WS113・WS099 と同時に流さない）。判断 D15
目安: 3〜4h（1 Queue）。実行者: Q1 が割り当てる（high）
所有 path: `userland/desktop/wayland/`（全 file、`zwl.h`・`zwl-*.h` の名前）、D15 を採る時は libkeiland の protocol の wrapper・`userland/desktop/keiland/wayland/` と libwayland の protocol の header・keiland-ime、`plan/ws131/`。（Q1 の委任が要る: Q1 が Phase の前に委任を記録する）: compositor の source を compile する道具（`plan/tools/gpu-boundary/`・`plan/tools/titlebar/`・`plan/ws035/tests/`・`plan/ws102/tests/`・`plan/tools/keiland-freebsd/` の各 1 file）

## 目的と結果

compositor の内部の名前 `zwl_`（494）・`ZWL_`（271）を [rename-map-kwl.md](../rename-map-kwl.md) のとおり `kwl_`・`KWL_` にし、`zwl.h` を `kwl.h` にする。log の文字列 `"ZWL "` は変えない（p022）。D15 を採れば Keiland の Wayland の protocol の名前（`keiland_*_v1`）と生成の定数も `kl_*_v1`・`KL_*_V1_*` に（compositor の global の表、libkeiland、libwayland の header、ime を同時に）。

## 範囲

1. rename-map-kwl を作り直し（backend へ移った名前は既に無い）、機械的に改名。
2. D15: wire の名前の変更は compositor と全 client を同じ build で。

## 受け入れ

- 3 OS の build（zedBSD の amd64 は exit 0・自前の warning 0、`make keiland-linux` の gcc と clang、FreeBSD の native build と `native-build-audit.py`）と `plan/tools/keiland-os-boundary/check.sh` PASS。
- compositor に `zwl_`・`ZWL_` の識別子が 0（log の文字列を除く）。
- zedBSD: boot-test、C1・C2・C9、GPU の境界（§6）。Linux: compositor と app の PNG。log を読む試験は文字列が変わらないので今のまま PASS すること。

## 検証の方法

[zedbsd-commands.md](../../tools/keiland-linux/zedbsd-commands.md)、[keiland-linux/README.md](../../tools/keiland-linux/README.md)、[keiland-freebsd/README.md](../../tools/keiland-freebsd/README.md)。QEMU の console・serial の log で判定しない（不具合は gdbstub・monitor・QMP）。build は自分の `BUILD=build/ws131-pNNN/`、共有の `build/` を消さない。FreeBSD の起動の確認は判断 D11 の範囲（passthrough なし）。QEMU と実機の証拠を分ける。

## 衝突・危険・rollback

- 衝突: compositor の全 file に触れる。 順は Q1 が決める（design.md §7.2）。
- rollback: 前へ直すのを基本にし、後ろに同じ file の変更が無い時だけ統合の commit を revert する（design.md §7.4）。

## Resume

依存の Phase の cleared と main への統合、関係する判断の決定の後に、Q1 が Queue を作る。
