<!-- awesome-plan project=zedbsd record=ws095 -->

# WS095: IME（Wayland の標準の方法、まず日本語）

<!-- awesome-plan-current:start -->
Status: incomplete（p001〜p004 cleared、p005 uncleared で中断）
Primary Milestone: MG006
Related Milestones: MG006
Objectives: O2
Parent: [Master](../master.md)
Queue: なし
Resume point: 2026-09-29 ユーザーの指示でブラッシュアップ（p005 の残り・p012 の辞書の拡張・p006〜p011）は後回し、Keiland を優先。IME は p004 で変換（kanji → 漢字、確定）まで guest で動く。再開は p005（phase005 の Resume point、書きかけは plan/ws095/p005-wip.patch）
2026-10-02 user:「IMEは人間の作業を完了したので、あなたが担当します。」人間の作業中の制限を解除し、優先度を上げる。人間の commit（2026-10-01〜02、`userland/desktop/ime` ほか）で p005 の状態と `p005-wip.patch` が古い可能性があるため、最初の Queue で現行 source と Phase の照合から始める。
<!-- awesome-plan-current:end -->

## 目標（2026-09-29 ユーザー）

「IMEの追加（Wayland標準の方法にて。単一のIMEがシステムにデフォルトで入っており、言語は切り替えでき、1つのIMEが複数の言語に対応している。まずは日本語のみの変換を可能にする。日本語の形態素解析は非常に簡単でよく、名詞＋助詞、動詞＋送り仮名、くらいの分解でよい。REmacsに入っている日本語辞書をベースにする。足りない分は要相談。）」

- Wayland の標準: compositor が `zwp_input_method_v2`（IME の process）と `zwp_text_input_v3`（app の側）を仲介する。IME は 1 つの program が
  system の既定として入り、言語（まず日本語、切り替えの仕組みは複数言語の前提）を持つ。
- 日本語: ローマ字 → かな、変換（名詞＋助詞、動詞＋送り仮名 程度の簡単な分割）、候補の窓、確定・取り消し。辞書は REmacs の辞書
  （`dict/SKK-JISYO.remacs`・`SKK-JISYO.X`、SKK の形式）を土台にする。
- 辞書の license（2026-09-29）: REmacs の辞書の header は「remacs と同じ license（GPL）」とあるが、ユーザー「REmacsは私が著作権者なので、気にしなくていいです。」→ 著作権者の許可として Kei の IME の辞書に使う。
- 使う app の側: Terminal・text editor（WS092）・ブラウザの text field・Notes・Settings の検索（text-input-v3 の対応）。

## Phase（案）

| Phase | 目的 | Status | 依存 |
| --- | --- | --- | --- |
| [ws095-p001](phase001/phase.md) | 設計（[design.md](design.md)） | cleared（2026-09-29） | — |
| [ws095-p002](phase002/phase.md) | 日本語の engine（Wayland 無し）: ローマ字・辞書・活用の規則・分割・候補・利用者の辞書、固定の辞書で host の試験 | cleared（2026-09-29） | p001 |
| [ws095-p003](phase003/phase.md) | 辞書の package（pin した tarball の取得・検証）、100 文での品質の計測、補いの辞書の案（ユーザーと相談） | cleared（2026-09-29） | p002、D1・D3 |
| [ws095-p004](phase004/phase.md) | protocol の記述、zdesktop の仲介（起動と信頼・key の経路・Alt+Space・watchdog）、IME の program（日本語の engine の結線を含む）、ime-probe の guest の試験 | cleared（2026-09-29） | p002 |
| [ws095-p005](phase005/phase.md) | 候補の窓の合成と IME の描画、indicator、IME の中の key の repeat、guest の画面の確認 | uncleared（2026-09-29、ユーザーの指示で中断。書きかけは `p005-wip.patch`） | p003・p004 |
| ws095-p006 | libkeiland の text-input の helper と Terminal（password の検出） | planning | p004・p005、Terminal の CJK の font（D14） |
| ws095-p007 | Text Editor の対応（WS092 の口） | planning | p006、WS092 |
| ws095-p008 | zdesktop の自前の field（titlebar の検索）と Files の field | planning | p005・p006 |
| ws095-p009 | Browser の text field | planning | p006 |
| ws095-p010 | PS/2 の日本語の key の写し（条件付き: JIS の PS/2 keyboard の利用者が出た時、F-058 と一緒に。5330 は PS/2 だが US 配列で日本語の key が無い。main 2026-09-29） | planning | JIS の PS/2 の利用者（F-058） |
| ws095-p011 | 全体の規約の適合、guest の回帰（実機の確認は別に記録） | planning | p002〜p009・p012 |
| ws095-p012 | 補いの辞書を千語へ広げ、補いの辞書の候補に活用の種類の注釈（SKK の `;…`）を足して engine が読む。held-out の文 100 以上を書き下ろし、拡張の前後で測る（ユーザーの答え 2026-09-29 夜） | planning | p003・p004 |

## 再開のときに直すこと（2026-09-30 main、ws035-p137 の調べから）

- `plan/ws095/tests/ime-p004.sh`（139 行目付近）の kill は `grep "[i]me-probe"` に当たる全ての ime-probe を閉じ、残すはずの窓も閉じる。
  BUG-113（閉じた後に focus が戻らない）の観測はこれによる見込み（zdesktop は次の窓へ focus を移している、ws035-p137）。
  guest の `ps` は引数を出さないので、閉じる process は起動の時の pid（`$!`）で kill する。

## 再開のときに直すこと（2026-09-30 main、ws035-p137 の調べから）

- `plan/ws095/tests/ime-p004.sh`（139 行目付近）の kill は `grep "[i]me-probe"` に当たる全ての ime-probe を閉じ、残すはずの窓も閉じる。
  BUG-113（閉じた後に focus が戻らない）の観測はこれによる見込み（zdesktop は次の窓へ focus を移している、ws035-p137）。
  guest の `ps` は引数を出さないので、閉じる process は起動の時の pid（`$!`）で kill する。
