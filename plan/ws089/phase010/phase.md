<!-- awesome-plan project=zedbsd record=ws089-p010 -->

# ws089-p010: 現在の main での回帰・S7（QEMU）・棚卸しと候補

Status: planned（2026-10-02。Queue なし）
Disposition: normal
Parent: [WS089](../ws.md)
Queue: なし
依存: なし
目安: 3h（1 Queue）。実行者の目安: phase-runner-mid
所有 path: `plan/ws089/`（試験の期待が古いときの試験の直しと `beta1-candidates.md`）。product の source は変えない

## 範囲

1. [guide.md](../guide.md) §3.1 のとおり: §5.1 の host、§5.2 の `settings-regress.sh`（8 本）、§5.3 の `volume-p005.sh`、boot test。FAIL が 1 回なら流し直し、2 回続けば原因を調べ、試験の期待の古さなら試験を直す（WS089 の file）。
2. 全 23 頁の通し（QEMU の Venus、1280x800 と 1920x1080）: 頁ごとの画面（PNG）と、崩れ・読めない文字・働かない操作の不具合の表（重さ: 重い／中／軽い）。
3. ブラッシュアップの候補の一覧 `plan/ws089/beta1-candidates.md`: ws.md の「後回しの候補」の表と 2 の気づきを、価値・規模（h）・危険・依存・触る file で並べ、計画エージェントの推奨（ベータ1 に入れる／入れない）を付ける。準備中の頁のうち 5330 で意味のある物（Battery の残量など）の backend の有無も調べる。

## 受け入れ

- 1 が全て PASS（S-B1 の基準）。画面（`p004`・`p008`・`p009` の PNG）を Q1 経由でユーザーに見せる。
- 2 の不具合の表と 3 の候補の一覧がある。重い・中の不具合は Bug Board の候補として Q1 に渡す。
- ユーザーの選択（Q1 が聞く）は p013〜p017 の Status に反映する（計画の更新）。回答待ちは cleared の妨げにしない。

## 検証の方法と範囲

QEMU の Venus。console・serial の log では判定しない。 やっていない確認は「未実施」と書く。

## 未決の判断

候補の採否（ユーザー）。

## Event

2026-10-02 / ws089-beta1-plan: fg019 の計画で新設。
