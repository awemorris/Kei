<!-- awesome-plan project=zedbsd record=ws089-p011 -->

# ws089-p011: 5330 の passthrough で S7

Status: planned（2026-10-02。Queue なし）
Disposition: normal
Parent: [WS089](../ws.md)
Queue: なし
依存: p010（現在の main の回帰が PASS）、`/tmp/i915-hw.lock` が空くこと（WS075 の lock）
目安: 2h（1 Queue）。実行者の目安: phase-runner-mid
所有 path: `plan/ws089/tests/settings-s7-hw.sh`（新しい）、`plan/ws089/phase011/`

## 範囲

[guide.md](../guide.md) §3.2 のとおり。新しい `settings-s7-hw.sh IMAGE OUTDIR` で App Home → Settings、Ctrl+F の検索、Wallpaper の差し替え（Aurora と既定）、Appearance の透明度 85%、session の log を image から読む（`/run` は disk に無い）。透明度 85% と 100% の frame の率を `h4-ctl.py rate` で比べる（U3）。

## 受け入れ

- `settings-s7-hw: PASS`（log: `SEARCH open page=wallpaper`、`LOOK set key=wallpaper`、`ZWL PREFERENCES key=wallpaper applied`、`LOOK set key=window.opacity value=85`、`ERROR` 0）。
- 画面（`OUTDIR/shots/*-live.png`）を Q1 経由でユーザーに見せる。証拠は「5330 の passthrough」と書き、素の実機と分ける。
- frame の率（85% と 100%）を記録（目標の数値は無い）。

## 検証の方法と範囲

5330 の passthrough（WS075 の `hdmi-h4-hw.sh`）。i915 の lock の下。QEMU の console・serial の log では判定しない。 やっていない確認は「未実施」と書く。

## 未決の判断

なし。

## Event

2026-10-02 / ws089-beta1-plan: fg019 の計画で新設。
