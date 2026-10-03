<!-- awesome-plan project=zedbsd record=ws128-p001 -->

# ws128-p001: 標準アプリの棚卸し・回帰の取り直し・改善の候補

Status: cleared（q623-i01、P1 generation4、2026-10-03。QEMU の Venus と host。結果は [requirements.md](../requirements.md)）
Disposition: normal
Parent: [WS128](../ws.md)
Queue: q623 / q623-i01（Q1 の dispatch、2026-10-03。承認: 自走の指示（planned の Phase と試験の直し）。時限 3 時間。所有 path に `plan/ws079/tests/` を加えた（demo-s8-s9.sh の待ち））
依存: なし
目安: 3h（1 Queue）。実行者の目安: phase-runner-mid
所有 path: `plan/ws128/`（`requirements.md`・`tests/`）だけ。product の source は変えない

## 範囲

1. 基準: 現在の main でアプリごとの既存の回帰を流す（ws.md の A1 の一覧。Terminal の既存の試験は `plan/ws035/tests/`・`plan/ws081/tests/` から特定する）。結果を A1 の基準として記録する。
2. menu の全項目の通し（QEMU の Venus）: Text Editor・Image Viewer・Notes・PDF Viewer・Terminal・system bar の音量・スクリーンキーボード。項目ごとに働く／働かない／未完成の表示、と不具合の表（重さ: 重い＝データを失う・止まる／中＝操作ができない・崩れる／軽い＝見た目）。
3. 未完成の UI の一覧: `grep -rn -i "not available\|not yet\|unsupported" userland/desktop/{textedit,imageview,notes,pdfviewer,terminal}` と 2 の結果（A3）。
4. 改善の候補の一覧（価値・規模 h・危険・依存・触る file、計画エージェントの推奨）。少なくとも: Text Editor（Replace は p003、encoding・print は外の見込み）、Image Viewer（Move to Trash・Open With・slideshow・copy）、PDF Viewer（文字の検索・選択・copy、libpdf の ToUnicode の規模）、Terminal（scrollback の検索・色・font の設定の保存）、Notes（Open は p002、Export）。
5. ws.md の「既存の WS の残りとの照合」を、各 WS の最新の phase.md と照らして確かめる（違えば Q1 に報告）。

## 受け入れ

- `plan/ws128/requirements.md` に 1〜5 があり、回帰の結果は commands・image の hash・結果の行つき。
- 代表の画面（10 枚以内）を Q1 経由でユーザーに見せられる形で保存。重い・中の不具合は Bug Board の候補として Q1 に渡す。
- ユーザーの選択（Q1 が聞く）は p004〜p006 の Status に反映する（計画の更新）。回答待ちは cleared の妨げにしない。

## 検証の方法と範囲

QEMU の Venus と host。console・serial の log では判定しない。source を変えないので boot test は不要。 やっていない確認は「未実施」と書く。

## 未決の判断

候補の採否（ユーザー）。

## Event

2026-10-02 / ws128-beta1-plan: fg019 の計画で新設。

## q623-i01 の結果（P1 generation4、2026-10-03 06:32〜07:16、base main `6df847d54`）

- 1〜5 は [requirements.md](../requirements.md)（基準の回帰の command・image の hash・結果、menu の全項目と確かめの印、不具合の表、未完成の UI、
  候補 C1〜C15、既存 WS との照合）。source は変えていない。
- 基準の回帰（全て最終は PASS）: Text Editor host 53/53、Image Viewer host と guest（1 回目は build の負荷で install の put が timeout、流し直しで ok）、
  PDF Viewer host、Notes host、S8・S9（1 回目は host の負荷で 203 ms、2 回目 PASS）、Terminal の menu（menu-p003）、音量 p004・p005、
  スクリーンキーボード（history の file の読み出しの SSH timeout → `install tools history` で PASS）。
- 不具合: 重い・中は 0、新しい軽い物も 0。試験の基盤の 3 件（T1 は直した、T2・T3 は Q1 へ）。Bug Board の候補は無い。
- 試験の直し（F-062）: `plan/ws079/tests/demo-s8-s9.sh` の `sleep 20` を `guest.py wait --timeout 180` に（commit `0a5e8ae42`）。直した版で PASS。
- 照合: ws.md の「既存の WS の残りとの照合」は各 WS の最新の表と一致。
- 未実施: スクリーンキーボードの send・hand（ime-probe が image に無い）・roll・large、Image Viewer の touch、実機。
- 代表の画面 9 枚: worktree の `build/ws128-p001/reps/`（まとめ `reps-sheet.png`）。
