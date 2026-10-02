<!-- awesome-plan project=zedbsd record=ws127-p001 -->

# ws127-p001: Files の棚卸し・回帰の取り直し・改善の候補

Status: cleared（q595-i01、2026-10-02、P4。成果物は [requirements.md](../requirements.md)。未実施の通しの項目は下。判定と記録は Q1）
Disposition: normal
Parent: [WS127](../ws.md)
Queue: q595 / q595-i01（承認: 継続 dispatch、2026-10-02 user「N=4で週次利用制限に達するまで作業してください」、Files はユーザーの「最重点」。時限 3h）
依存: なし（WS071・WS093 completed、WS094 の desktop は対象の外）
目安: 3h（1 Queue）。実行者の目安: phase-runner-mid
所有 path: `plan/ws127/`（`requirements.md`・`tests/`）だけ。**product の source は変えない**

## 範囲

1. 基準: 現在の main で Files の image（`plan/tools/files/build-files-image.sh`）を作り、`files-regress.sh` の 14 本と host の試験（`host-model.sh`・`host-png.sh`・`host-default.sh`）を流す。結果を F1 の基準として記録する（FAIL は不具合の表へ）。
2. 照合表: [WS071 の spec.md](../../ws071/spec.md) の 1〜38 節ごとに「済み・部分・無し」と根拠（code の位置か試験）を `requirements.md` に書く。
3. 実使用の通し（QEMU の Venus、QMP の pointer と key、画面と Files の log）: folder の作成・名前の変更・copy・move・名前の衝突（Replace・Skip・Keep Both）・Trash と Put Back・Empty・undo/redo・検索・tag・preview と Quick Look・Open With と Always Open With・窓の中と窓の間の DnD・tab・New Window・大きい folder（`make-home.sh` で 1000 項目）・画像の多い folder（jpg・gif・png）・長い名前と日本語の名前。
   見つけた不具合を表にする（重さ: 重い＝データを失う・止まる／中＝操作ができない・表示が崩れる／軽い＝見た目）。重い・中は Bug Board の候補として Q1 に渡す。
4. 速さの基準値: 1000 項目の folder を開いて最初の frame まで、click から選択の frame まで（Files の log の時刻、3 回の中央値、QEMU の値として）。
5. 候補の一覧: Future Work（F-033・F-035・F-036・F-037・F-039・F-041・F-050、参考に F-032・F-034・F-040）と 3 の不具合・気づきを、価値・規模（h）・危険・依存・触る file で並べ、計画エージェントの推奨（ベータ1 に入れる／入れない）を付ける。

## 受け入れ

- `plan/ws127/requirements.md` に 1〜5 があり、回帰の結果は commands・image の hash・結果の行つき。
- 実使用の通しの画面（PNG）を Q1 経由でユーザーに見せられる形で保存（`plan/ws127/tests/` か worktree の build、代表 10 枚以内）。
- ユーザーが候補を選んだら（Q1 が聞く）、選択を ws.md と p002〜p007 に反映して planned にする（反映は Phase の外の計画の更新でよい）。ユーザーの回答待ちで Queue が終わるのは cleared の妨げにしない（成果物は 1〜5）。

## 検証の方法と範囲

QEMU の Venus だけ。console・serial の log では判定しない。source を変えないので boot test は不要。

## 未決の判断

WS127 の ws.md の「未決の判断」1〜4 の材料をここで作る。

## Event

2026-10-02 / ws127-beta1-plan-p001: fg019 の計画で新設。

## 結果（q595-i01、2026-10-02、P4、worktree `/home/awe/zedBSD-worktrees/p4`、main 08405114a に合わせて実行）

成果物: [requirements.md](../requirements.md)（1 回帰の基準、2 spec の照合、3 通しと不具合の表、4 速さ、5 候補の一覧）、`tests/walk-lib.sh`（通しの helper）、
`tests/latency.py`（host からの click の遅れの計測）、`tests/evidence/*.jpg`（代表 8 枚）。product の source は変えていない。

- 1: image `build/p4-files/hdd-image.img` SHA-256 `fd9ab556…34440`。host 3 本 PASS。guest 14 本は 1 回目 7 PASS、FAIL の再試験で 9 PASS。残る 5 本は p014・p017 が試験の座標の古さ（手で undock を確かめた）、
  p003・p005・p008 が負荷の時の timing の疑い（未確定）。
- 2: 済み 19・部分 16（設計で変更 6）・無し 3。FW の無い差 5 つ。
- 3: 重い 0、中 2（B1 Trash の 2 つ目の名前、B2 起動直後の shortcut の順序。B2 は QEMU だけ）、軽い 1、試験 2。F-050 は実装済みで p003 の (1)〜(3) は不要。
- 4: QEMU で 1000 項目を開く 576 ms・選択 273 ms（項目の数は効かず、present が支配的）。
- 5: 14 項目、推奨付き。

未実施: Open With・Always Open With の通し（`files-open.sh`）、日本語の名前への名前の変更（image に IME が無い）、検索・tag・tab 等は通しでなく回帰の結果で代えた、負荷の低い時の p003・p005・p008 の再試験、実機。
QEMU の証拠だけ。console・serial の log は使っていない。boot test は不要（source を変えない）。

次: ユーザーが 5 の表から選ぶ（Q1 が聞く）。B1・B2 は Bug Board の候補（Q1）。
