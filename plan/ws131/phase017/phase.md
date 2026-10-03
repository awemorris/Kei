<!-- awesome-plan project=zedbsd record=ws131-p017 -->

# ws131-p017: PDF Viewer・Image Viewer を新しい API へ

Status: planning（p002 第 2 版はユーザーのレビュー済み（2026-10-03、D7 は確認中）。開始はユーザーの承認と P2 の終了の後に Q1 が指示）
Disposition: normal
Parent: [WS131](../ws.md)、計画の正本 [design.md](../design.md)
Queue: none
依存: p016 cleared。D8 の単独走行で番号の順
目安: 3〜4h（1 Queue）。実行者: Q1 が割り当てる（high）
所有 path: `userland/desktop/pdfviewer/`・`imageview/`、API の不足の補い、`plan/ws131/`

## 目的と結果

2 app を新 API へ。Image Viewer の自前の present は `kl_window_vulkan_surface`。touch の結線は呼び方を変えず、共有の候補を記録。

## 範囲

1. 新名と `<keiland.h>`。menu（365・550）・titlebar（321・369）・glass（Image Viewer 191）を宣言的に。
2. PDF の password の card と inset を保つ。

## 受け入れ

- 3 OS の build（zedBSD の amd64 は exit 0・自前の warning 0、`make keiland-linux` の gcc と clang、FreeBSD の native build と `native-build-audit.py`）と `plan/tools/keiland-os-boundary/check.sh` PASS。
- 対象の app に旧名（`kui_`・`KUI_`・`keiland_`・`KEILAND_`）と自前の menu・titlebar・glass の結線・main loop が無い（grep）。結線の行数の減りを記録。
- zedBSD: `viewers-p008.sh`、`plan/tools/imageview/run-host.sh`・`imageview-guest.sh`・`touch-guest.sh`、`plan/ws079/tests/run-pdfviewer-host.sh`、`plan/ws081/tests/run-pdftouch.sh`、`demo-s8-s9.sh`、boot-test。Linux: 2 app の PNG。

## 検証の方法

[zedbsd-commands.md](../../tools/keiland-linux/zedbsd-commands.md)、[keiland-linux/README.md](../../tools/keiland-linux/README.md)、[keiland-freebsd/README.md](../../tools/keiland-freebsd/README.md)。QEMU の console・serial の log で判定しない（不具合は gdbstub・monitor・QMP）。build は自分の `BUILD=build/ws131-pNNN/`、共有の `build/` を消さない。FreeBSD の起動の確認は判断 D11 の範囲（passthrough なし）。QEMU と実機の証拠を分ける。

## 衝突・危険・rollback

- 衝突: WS128 と同じ file。 順は Q1 が決める（design.md §7.2）。
- rollback: 前へ直すのを基本にし、後ろに同じ file の変更が無い時だけ統合の commit を revert する（design.md §7.4）。

## Resume

依存の Phase の cleared と main への統合、関係する判断の決定の後に、Q1 が Queue を作る。
