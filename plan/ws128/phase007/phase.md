<!-- awesome-plan project=zedbsd record=ws128-p007 -->

# ws128-p007: 5330 の実機で標準アプリの通し

Status: planning（p002〜p006 の選んだ物の後、実機とユーザーの時間）
Disposition: normal
Parent: [WS128](../ws.md)
Queue: なし
依存: 実装の Phase。WS099 p012・WS094 p012・WS089 p011・WS127 p007 と同じ demo の image・同じ回にまとめる（Q1 が声をかける）
目安: agent 1h（image と手順の用意、SSH の log）+ ユーザー 30 分（1 Queue）。実行者の目安: phase-runner-mid
所有 path: `plan/ws128/`

## 範囲

demo の image（`plan/ws075/demo/build-demo-image.sh`）を作り、boot test。ユーザーが 5330 で Text Editor・Image Viewer・Notes・PDF Viewer・Terminal・音量（WS100 A7: 確かめの音が鳴るか）・スクリーンキーボード（mouse で）を開く・編集・保存・閉じる。WS079 の S8・S9 も同じ回（`plan/ws079/demo-s8-s9-manual.md`）。agent は SSH で session の log の `ERROR` と各 app の log を読む（ユーザーの許可を得てから）。

## 受け入れ

ユーザーの報告で全アプリの通しが崩れ・止まりなし。SSH の log の ERROR 0（SSH が使えなければ未実施と書く）。WS079・WS100 の結果はその WS へ Q1 が投影。QEMU と実機の証拠を分ける。

## 検証の方法と範囲

5330 の素の起動（USB）。 やっていない確認は「未実施」と書く。

## 未決の判断

なし。

## Event

2026-10-02 / ws128-beta1-plan: fg019 の計画で新設。
