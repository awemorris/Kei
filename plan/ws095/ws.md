<!-- awesome-plan project=zedbsd record=ws095 -->

# WS095: IME（Wayland の標準の方法、まず日本語）

<!-- awesome-plan-current:start -->
Status: incomplete（p001〜p004 cleared、p005 uncleared。2026-10-02 user の担当の変更で再開）
Primary Milestone: MG006
Related Milestones: MG006
Objectives: O2
Parent: [Master](../master.md)
Queue: なし
Resume point（2026-10-02 計画詳細化）: **次は p005 の新 attempt**（候補の窓・indicator・key の repeat・ime-p004.sh の kill の修正、目安 4h、Queue 投入可）。その後 p006（Terminal）→ p007（Text Editor の確認）→ p008（Files・titlebar の検索）と p012（辞書の拡張）を並行、最後に p011。
2026-10-02 user:「IMEは人間の作業を完了したので、あなたが担当します。」人間の作業中の制限を解除し、優先度を上げる。
**source の照合（2026-10-02 計画担当）**: 2026-09-29 の p004 以降、`userland/desktop/ime/`・`wayland/input-method.c`・`wayland/text-input.c` の変更は path の共通化（cec34d3e1、`paths.h`）と Linux/FreeBSD の Makefile の追加（ba46edf89・1ed1a4b59）だけで、IME の機能の変更は無い（commit の author は全て同じ名前で、人間とエージェントの区別は git では付かない）。`p005-wip.patch` は現行の `ime.h`・`input-method.c` に `git apply --check` で当たる（offset 2 行）。旧 worktree `.claude/worktrees/ws095-ime` は gitdir が `/home/awe/zedBSD-rpi4` を指して壊れているので使わず、P 担当の新しい worktree で再開する。**「人間の作業」の内容（repository の外の作業か、未 commit の差分か）をユーザーに確認する**。libkeiui（WS090 p013）に text-input-v3 の client（`kui_window_text_input`）があり、Text Editor は既に呼んでいる。Terminal・Notes も libkeiui の window を使う。
2026-10-02 / q593: ws095-p012 cleared（main c7bbbf06a）。辞書 337→1,478 見出し（計画の目安「千語」を約 4 割超え、ユーザーに報告）。held-out A 64→109/125、B 47→81/110、既存 100 文 92→97。
2026-10-02 user: 日本語入力が実機でできることを確認。「IMEのステータスを画面右上の通知領域に追加してください」→ p005 の indicator（system bar の通知領域、A／あ）を先に。「入力文字のサイズが小さくておかしかった…テキスト本文と同じ文字の大きさにしたい。UI/UXの完成度の要」→ 新 p013（BUG-139）。P4 の次の Queue として p013 → p005 を Files の棚卸しの後に投入する。
2026-10-02 user（辞書の大きさ）:「IME の辞書の大きさはこのままで大丈夫です。もっと増やしてもいいくらいですが、キリがないので、ひとまずこれでいいです。もし増やしても問題ないとだけ伝えておきます。」→ 1,478 見出しで確定。今後の Phase で増やすのは可。
<!-- awesome-plan-current:end -->

## 目標（2026-09-29 ユーザー）

「IMEの追加（Wayland標準の方法にて。単一のIMEがシステムにデフォルトで入っており、言語は切り替えでき、1つのIMEが複数の言語に対応している。まずは日本語のみの変換を可能にする。日本語の形態素解析は非常に簡単でよく、名詞＋助詞、動詞＋送り仮名、くらいの分解でよい。REmacsに入っている日本語辞書をベースにする。足りない分は要相談。）」

- Wayland の標準: compositor が `zwp_input_method_v2`（IME の process）と `zwp_text_input_v3`（app の側）を仲介する。IME は 1 つの program が
  system の既定として入り、言語（まず日本語、切り替えの仕組みは複数言語の前提）を持つ。
- 日本語: ローマ字 → かな、変換（名詞＋助詞、動詞＋送り仮名 程度の簡単な分割）、候補の窓、確定・取り消し。辞書は REmacs の辞書
  （`dict/SKK-JISYO.remacs`・`SKK-JISYO.X`、SKK の形式）を土台にする。
- 辞書の license（2026-09-29）: REmacs の辞書の header は「remacs と同じ license（GPL）」とあるが、ユーザー「REmacsは私が著作権者なので、気にしなくていいです。」→ 著作権者の許可として Kei の IME の辞書に使う。
- 使う app の側: Terminal・text editor（WS092）・ブラウザの text field・Notes・Settings の検索（text-input-v3 の対応）。

## ベータ1（fg019、2026-10-17）までの到達目標（2026-10-02 計画、ユーザー確認待ち）

| # | 受け入れ（測れる形） | Phase |
| --- | --- | --- |
| I-B1 | zedBSD QEMU の Keiland で、Alt+Space（と 変換/無変換）で日本語に切替え、ローマ字→かな→変換の候補の窓が cursor の下に出て、選んで確定できる。top bar の indicator（A／あ）が切替えに追従し、click で切替わる。全画面の窓の上でも候補が出る。PNG で確認 | p005 |
| I-B2 | Terminal・Text Editor・Notes で日本語を入力・確定でき、Terminal の password の入力では IME が無効。Terminal で CJK の文字が表示される（D14） | p006・p007 |
| I-B3 | Files の field と titlebar の検索で日本語の入力 | p008 |
| I-B4 | 補いの辞書の拡張と held-out の文 100 以上で変換の正解率を拡張の前後で測り、下がらない | p012 |
| I-B5 | 全変更の全文規約、guest の回帰。実機（5330）の切替えの確認はユーザーに依頼し、別に記録 | p011 |

最低線は I-B1・I-B2。ブラウザ（p009）は WS074 がこの session の対象外（2026-10-02 user、Codex の担当）なので、ベータ1では WS074 の担当に引継ぐか保留するかをユーザーが決める。

## Phase

| Phase | 目的 | Status | 依存 | 目安 |
| --- | --- | --- | --- | --- |
| [ws095-p001](phase001/phase.md) | 設計（[design.md](design.md)） | cleared（2026-09-29） | — | — |
| [ws095-p002](phase002/phase.md) | 日本語の engine（Wayland 無し）: ローマ字・辞書・活用の規則・分割・候補・利用者の辞書、固定の辞書で host の試験 | cleared（2026-09-29） | p001 | — |
| [ws095-p003](phase003/phase.md) | 辞書の package（pin した tarball の取得・検証）、100 文での品質の計測、補いの辞書の案（ユーザーと相談） | cleared（2026-09-29） | p002、D1・D3 | — |
| [ws095-p004](phase004/phase.md) | protocol の記述、zdesktop の仲介（起動と信頼・key の経路・Alt+Space・watchdog）、IME の program（日本語の engine の結線を含む）、ime-probe の guest の試験 | cleared（2026-09-29） | p002 | — |
| [ws095-p005](phase005/phase.md) | 候補の窓の合成と IME の描画、indicator、IME の中の key の repeat、guest の画面の確認 | cleared（q604-i01、2026-10-02、P4。QEMU で候補の窓・system bar の A／あ と click・repeat を確認、実機の目視はユーザー） | p003・p004 | 4h |
| [ws095-p006](phase006/phase.md) | Terminal の text-input（libkeiui の `kui_window_text_input` を使う、password の検出）と Terminal の CJK の fallback の font（D14） | planned（p005 の後） | p005 | 3〜4h |
| [ws095-p007](phase007/phase.md) | Text Editor と Notes の確認と不足の修正（preedit の表示・cursor の矩形） | planned（p005 の後） | p005 | 2〜3h |
| [ws095-p008](phase008/phase.md) | zdesktop の自前の field（titlebar の検索）と Files の field | planning（Files の担当 WS127 と file の調整） | p005・p006、WS127 との調整 | 3〜4h |
| ws095-p009 | Browser の text field | planning（WS074 はこの session の対象外。引継ぎか保留をユーザーが決める） | p006、WS074 の担当 | — |
| ws095-p010 | PS/2 の日本語の key の写し（条件付き: JIS の PS/2 keyboard の利用者が出た時、F-058 と一緒に。5330 は PS/2 だが US 配列で日本語の key が無い。main 2026-09-29） | planning | JIS の PS/2 の利用者（F-058） | — |
| [ws095-p011](phase011/phase.md) | 全体の規約の適合、guest の回帰（実機の確認は別に記録） | planning | p005〜p008・p012（p009/p010 は行った時だけ） | 2〜3h |
| [ws095-p012](phase012/phase.md) | 補いの辞書を千語へ広げ、活用の種類の注釈（SKK の `;…`）を engine が読む。held-out の文 100 以上で拡張の前後を測る（ユーザーの答え 2026-09-29 夜） | cleared（q593-i01、2026-10-02、P4。1,478 見出し、100 文 92→97、held-out A 64→102（盲検）・109、B 47→81。merge は Q1） | p003・p004 | 4h |

**source の衝突**: p005 は `userland/desktop/wayland/` の compose.c・protocol.c・display.c・shell.c・input-method.c・ime.h を触る。WS114 p007 の再 attempt（修正が要る時の shell.c・protocol.c）、WS117 p003、WS099（BUG-125）・WS094（desktop surface）・WS113（display.c）の compositor の Queue と同時に実行すると merge の衝突が出る。main が順を決める。p008 は Files（WS127 最重点）と titlebar-shell.c を触る。p006 は Terminal、p007 は textedit・notes。p012 は `userland/desktop/ime/` の中だけで衝突しない。

## 再開のときに直すこと（2026-09-30 main、ws035-p137 の調べから）

- `plan/ws095/tests/ime-p004.sh`（139 行目付近）の kill は `grep "[i]me-probe"` に当たる全ての ime-probe を閉じ、残すはずの窓も閉じる。
  BUG-113（閉じた後に focus が戻らない）の観測はこれによる見込み（zdesktop は次の窓へ focus を移している、ws035-p137）。
  guest の `ps` は引数を出さないので、閉じる process は起動の時の pid（`$!`）で kill する。

## Event

2026-10-02 / ws095-beta1-plan-20261002: 計画担当が source と plan を照合（上の current block）し、ベータ1の到達目標 I-B1〜I-B5、p005 の新 attempt の範囲、p006・p007・p008・p011・p012 の Phase file を作成。p006 は libkeiland の新しい helper ではなく既存の libkeiui の text-input を使う形に改め、p007 に Notes を加えた。p009（Browser）は WS074 がこの session の対象外のため判断待ち。重複していた「再開のときに直すこと」の節を 1 つにした（内容は p005 の新 attempt の範囲にも入れた）。実装・guest なし。
2026-10-02 / ws095-p012-q593: P4 が p012 を実行（q593-i01）。補いの辞書を 337 → 1,478 見出しに広げ、活用の種類の注釈（五段・一段・形容詞）を engine が読み、
分割の費用に日常の語の段、する・来る の語尾の規則を直した。host の試験 203 通過、100 文 92→97、held-out A（125 文）64→102（盲検）・109、
B（110 文）47→81。語の数がユーザーの「千語まで」を約 4 割超えた点は判断待ち（[phase012](phase012/phase.md)）。I-B4 の受け入れを満たす。
