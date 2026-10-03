<!-- awesome-plan project=zedbsd record=ws131-p022 -->

# ws131-p022: compositor の log の接頭辞を KWL に（試験と同時に）

Status: planning（p002 第 2 版はユーザーのレビュー済み（2026-10-03、D7 は確認中）。開始はユーザーの承認と P2 の終了の後に Q1 が指示）
Disposition: normal
Parent: [WS131](../ws.md)、計画の正本 [design.md](../design.md)
Queue: none
依存: p021 cleared。compositor と試験の他の作業が無い時
目安: 3〜4h（1 Queue）。実行者: Q1 が割り当てる（high）
所有 path: `userland/desktop/wayland/` の log の文字列（44 file、63 種類の tag）、`plan/ws131/`。（Q1 の委任が要る: Q1 が Phase の前に委任を記録する）: `"ZWL "` を読む試験の script 201 本（sh・py、`plan/history` を除く。他の WS の試験を含む）

## 目的と結果

compositor の log の行の接頭辞 `"ZWL "` を `"KWL "` にし、それを読む試験 201 本を同じ commit で直す（2026-10-03 user の注意: log の文字列の変更は symbol の改名と別の段に）。

## 範囲

1. 最初に `grep -rlE "ZWL [A-Z]" plan` で全ての試験を列挙して phase.md に表で残し、機械的に置き換える（`ZWL_` の識別子や関係の無い文字列を巻き込まない pattern）。
2. BUG の ticket や history の文書の中の log の引用は変えない。

## 受け入れ

- 3 OS の build（zedBSD の amd64 は exit 0・自前の warning 0、`make keiland-linux` の gcc と clang、FreeBSD の native build と `native-build-audit.py`）と `plan/tools/keiland-os-boundary/check.sh` PASS。
- 列挙した試験の全てが新しい接頭辞を読む。compositor に `"ZWL "` の文字列が 0。
- zedBSD: C1・C2・C9（criteria.sh）を前後で、各 WS の代表の試験（`zdesktop-p101.sh`・`settings-regress.sh`・`volume-p005.sh`・`textinput-p013.sh`・`demo-s8-s9.sh`）を前後で流し、結果が同じ。

## 検証の方法

[zedbsd-commands.md](../../tools/keiland-linux/zedbsd-commands.md)、[keiland-linux/README.md](../../tools/keiland-linux/README.md)、[keiland-freebsd/README.md](../../tools/keiland-freebsd/README.md)。QEMU の console・serial の log で判定しない（不具合は gdbstub・monitor・QMP）。build は自分の `BUILD=build/ws131-pNNN/`、共有の `build/` を消さない。FreeBSD の起動の確認は判断 D11 の範囲（passthrough なし）。QEMU と実機の証拠を分ける。

## 衝突・危険・rollback

- 衝突: 他の全ての WS の試験の script に触れる。Q1 が投入の時期を決める。 順は Q1 が決める（design.md §7.2）。
- rollback: 前へ直すのを基本にし、後ろに同じ file の変更が無い時だけ統合の commit を revert する（design.md §7.4）。

## Resume

依存の Phase の cleared と main への統合、関係する判断の決定の後に、Q1 が Queue を作る。
