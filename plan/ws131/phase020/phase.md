<!-- awesome-plan project=zedbsd record=ws131-p020 -->

# ws131-p020: Files を新しい API へ（toplevel と desktop surface、DnD）

Status: planning（p002 第 2 版はユーザーのレビュー済み（2026-10-03、D7 は確認中）。開始はユーザーの承認と P2 の終了の後に Q1 が指示）
Disposition: normal
Parent: [WS131](../ws.md)、計画の正本 [design.md](../design.md)
Queue: none
依存: p018 cleared。旧 ws090-p010 の窓の部分はこの Phase（D9 の決定）
目安: 4〜5h（1 Queue）。実行者: Q1 が割り当てる（high）
所有 path: `userland/desktop/files/`、`libkeiland/app/`（desktop surface の役、DnD の source）、`plan/ws131/`

## 目的と結果

Files の自前の窓（`window.c` 1,340 行、toplevel と desktop surface の 2 役）と present（1,243 行）を除き、窓の役に desktop surface と DnD の source を足して移す。Files の xattr は app 側の OS の依存として残す（D14、2026-10-03 user: タグ（`tags.c`）とコピー（`task.c`）は今のまま、FreeBSD の extattr の読み替えは Files の中）。情報の panel（`info.c`）は xattr の名前だけを出す簡単な表示にする。

## 範囲

1. libkeiland: `KL_WINDOW_DESKTOP` の役、`kl_window_start_drag`。
2. Files の窓・present・入力・clipboard・DnD を libkeiland へ、menu（897）・titlebar（453）・glass（152）を宣言的に。CPU の canvas・text の写しの置き換え（旧 ws090-p009）は WS131 の完了の後に WS090 で扱う（D9 の決定）。
3. 情報の panel（`info.c`）の xattr の表示を名前だけの一覧にする（D14）。

## 受け入れ

- 3 OS の build（zedBSD の amd64 は exit 0・自前の warning 0、`make keiland-linux` の gcc と clang、FreeBSD の native build と `native-build-audit.py`）と `plan/tools/keiland-os-boundary/check.sh` PASS。
- 対象の app に旧名（`kui_`・`KUI_`・`keiland_`・`KEILAND_`）と自前の menu・titlebar・glass の結線・main loop が無い（grep）。結線の行数の減りを記録。
- zedBSD: `plan/tools/files/files-regress.sh`・`host-build.sh`・`files-open.sh`、`plan/ws094/tests/desktop-guest.sh`・`files-desktop-guest.sh`、`plan/ws081/tests/run-filestouch.sh`、`plan/ws127/tests/files-p002.sh`、boot-test、C9（desktop の icon を含む時）。Linux: Files の窓と desktop の PNG。

## 検証の方法

[zedbsd-commands.md](../../tools/keiland-linux/zedbsd-commands.md)、[keiland-linux/README.md](../../tools/keiland-linux/README.md)、[keiland-freebsd/README.md](../../tools/keiland-freebsd/README.md)。QEMU の console・serial の log で判定しない（不具合は gdbstub・monitor・QMP）。build は自分の `BUILD=build/ws131-pNNN/`、共有の `build/` を消さない。FreeBSD の起動の確認は判断 D11 の範囲（passthrough なし）。QEMU と実機の証拠を分ける。

## 衝突・危険・rollback

- 衝突: WS127・WS094 と同じ file。 順は Q1 が決める（design.md §7.2）。
- rollback: 前へ直すのを基本にし、後ろに同じ file の変更が無い時だけ統合の commit を revert する（design.md §7.4）。

## Resume

依存の Phase の cleared と main への統合、関係する判断の決定の後に、Q1 が Queue を作る。
