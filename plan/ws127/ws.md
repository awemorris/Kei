<!-- awesome-plan project=zedbsd record=ws127 -->

# WS127: Files のベータ1 のブラッシュアップ（最重点）

<!-- awesome-plan-current:start -->
Status: planned
Primary Milestone: MG006
Objectives: O2
Parent: [Master](../master.md)
Focused goal: fg019（ベータ1、2026-10-17）
Queue: none
Resume point（2026-10-02 計画）: **p001（棚卸し・回帰の取り直し・候補の一覧、source は変えない）を最初の Queue に**。p001 の最後にユーザーが候補を選び、p002 以降の採否と順が決まる。p002〜p007 は p001 の結果で planned にする。
2026-10-02 user のベータ1 の採否（[requirements](requirements.md) §5 への回答）:
- 推奨の 5 つ（BUG-140/141 と試験の直し、BUG-142 の調査、PDF の thumbnail と disk cache（F-035）、DnD の自動 scroll と spring-loaded（F-039）、「Move To」「Open in New Window」）を入れる。
- 日本語 UI は入れない:「ローカライズの仕組みをあとで実装して、複数の言語で一斉に対応したいです。ベータ2以降です。」
- 取り出し（eject、F-036）を入れる:「マウントの制御をlibkeilandに入れる必要がありますね。工数が小さいので入れましょう。」（USB storage の hotplug の通知が前提なので依存を確かめる）
- ネットワーク・クラウドの drive は入れない:「私の方で設計どころか構想も終わっていません。」
- 検索の index は入れない:「私はこの技術があまり好きではないです。ドキュメントはクラウドに置く時代だと思うので、ローカルのインデックスはバッテリーの無駄と思います。」
- カラム表示はベータ1 に入れる（ユーザー）。詳細（list）表示は既に有る（Name・Kind・Size・Date Modified、並べ替え）ので、Finder のカラム（階層の pane）か詳細表示の改善かを確認中。
<!-- awesome-plan-current:end -->

## 目標（2026-10-02 ユーザー（ベータ1、リリース目標 10/17））

「標準アプリ：まんべんなくブラッシュアップします。Filesは最重点、Settingsも重点的にしましょう。」

- Files（`userland/desktop/files`、`/bin/files`。WS071・WS093 は completed なので新しい WS）をベータ1 の品質に仕上げる。改善の項目と基準は p001 でユーザーと決める。
  候補は WS071 の Future Work（F-033・F-035・F-036・F-037・F-039・F-041・F-050。F-032 network・F-034 indexer・F-040 system Quick Look は規模が大きくベータ1 の外の見込み）と、p001 の通しで見つかる不具合。
- desktop の icon（`files --desktop`）は WS094 が持つ。WS127 は窓の Files を扱う。

## ベータ1 の到達目標と受け入れ（測れる形）

| # | 受け入れ | 測り方 | Phase |
| --- | --- | --- | --- |
| F1 | 最終の image で Files の回帰（`plan/tools/files/files-regress.sh` の 14 本、host の `host-model.sh`・`host-png.sh`・`host-default.sh`）が全て PASS | QEMU の Venus | p001（基準）、p008（最後） |
| F2 | p001 の通しで見つけた不具合のうち重い・中の物が 0（直したか、ticket でユーザーが非阻害を決めた） | p001 の不具合の表 | p002 |
| F3 | p001 でユーザーが選んだ改善の項目が全て実装され、項目ごとの試験が PASS | 各 Phase の試験 | p003〜p006 |
| F4 | 5330 の実機で主な操作（開く・copy・move・衝突・Trash・検索・preview・Quick Look・DnD・tab）が通る（ユーザーの目視）。数値（仮、p001 で確定）: 1000 項目の folder を開いて最初の frame まで ≤ 1000 ms、click から選択の frame まで ≤ 50 ms | ユーザーの操作と SSH の log | p007 |
| F5 | 変えた source の全文規約、boot test | — | p008 |

## Phase

| Phase | 目的 | Status | 依存 | 目安 |
| --- | --- | --- | --- | --- |
| [ws127-p001](phase001/phase.md) | 棚卸し: 現在の main で回帰の取り直し、spec.md と実装の照合表、QEMU での実使用の通しと不具合の表、改善の候補の一覧（価値・規模・危険・依存）。最後にユーザーが選ぶ | planned | — | 3h |
| [ws127-p002](phase002/phase.md) | p001 で見つけた不具合の直し（重い・中） | planning（p001 の不具合の表が要る） | p001 | 3h |
| [ws127-p003](phase003/phase.md) | 名前の衝突と Trash の残り（F-050: Replace で消さずに Trash、cut の Esc で clipboard を戻す、folder の merge。F-041 の一部: `$topdir/.Trash-$uid`） | planning（p001 でユーザーの採否） | p001 | 3h |
| [ws127-p004](phase004/phase.md) | thumbnail の拡張（F-035: PDF の 1 頁目を libpdf で、disk の thumbnail の cache。動画は WS122 の後で外） | planning（p001 でユーザーの採否） | p001 | 3h |
| [ws127-p005](phase005/phase.md) | 日本語の UI の文言と、名前の変更での IME（F-041 の残り）。Settings と共通の翻訳の仕組み | planning（ユーザーの判断: 日本語の UI をベータ1 に入れるか。WS095 の IME の状態） | p001、WS095、WS089 p016 と仕組みを共有 | 4h |
| [ws127-p006](phase006/phase.md) | DnD の自動の scroll と spring-loaded（F-039） | planning（p001 でユーザーの採否） | p001 | 2h |
| [ws127-p007](phase007/phase.md) | 5330 の実機での操作と速さ。遅ければ描き直しを damage の矩形に絞る（F-037） | planning（p002〜p006 の後、実機とユーザーの時間） | p002〜p006 の選んだ物 | 2h + ユーザー 20 分 |
| [ws127-p008](phase008/phase.md) | 全文規約と回帰（WS の最後） | planning | 実装の Phase | 2h |

候補のうち Phase にしていない物（p001 で選ばれたら Phase を足す）: F-033（カラム・ギャラリーの表示、4h 以上）、F-036（装置の unmount・eject、USB の storage の hotplug の通知が要る）、
F-032・F-034・F-040（ベータ1 の外の見込み）。

## source の衝突（並列の注意）

- WS127 の実装の Phase は全て `userland/desktop/files/` を変える。互いに直列（同じ担当に続けて割り当てるのがよい）。
- WS094 p014（`files/thumb.c`・`ui-context.c`）と WS094 p007 の回帰と重ねない。WS094 を先に締める（WS094 の完了が WS090 p009 の前提でもある）。
- **WS090 p009・p010（Files を libkeiui へ移す）と WS127 は同じ file を大きく変える**。ベータ1 の間に両方を流さない（下の未決の判断）。
- Settings は Files の `canvas.c`・`text.c`・`icons.c`・`artwork/mark.c` を source で共有する（WS089 の ws.md）。これらを変える Phase は Settings の host 試験も流し、WS089 の Phase と重ねない。
- `picture/`（共有の decoder）を変える p004 は WS128 の Image Viewer の Phase と直列。
- WS104 p007（install の path の macro、files を含む）が残っていれば Q1 が順を決める。

## 未決の判断（ユーザー）

1. p001 の候補の一覧からベータ1 に入れる項目（p001 の最後）。
2. Files の libkeiui への移行（WS090 p009・p010）をベータ1 の前に行うか、後に回すか（計画エージェントの案: 後。ベータ1 の間は Files の今の部品のまま磨く）。
3. 日本語の UI をベータ1 に入れるか（p005、WS089 p016 と同じ判断）。
4. F4 の数値（1000 項目 ≤ 1000 ms、選択 ≤ 50 ms）を基準にするか（p001 の基準値の後）。

## Event

2026-10-02 / ws127-beta1-plan: fg019 の計画エージェントが到達目標 F1〜F5 と p001〜p008 を作成。p001 だけ planned、他は p001 のユーザーの選択待ちの planning。Queue は未投入。
