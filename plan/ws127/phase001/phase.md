<!-- awesome-plan project=zedbsd record=ws127-p001 -->

# ws127-p001: Files の棚卸し・回帰の取り直し・改善の候補

Status: planned（2026-10-02。Queue なし）
Disposition: normal
Parent: [WS127](../ws.md)
Queue: なし
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
