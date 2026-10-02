<!-- awesome-plan project=zedbsd record=ws127-p007 -->

# ws127-p007: 5330 の実機での操作と速さ

Status: planning（2026-10-02 ベータ1の計画。要る判断・成果は「未決の判断」と「依存」）
Disposition: normal
Parent: [WS127](../ws.md)
Queue: なし
依存: p002〜p006 の選んだ物、5330 とユーザーの時間（WS099 p012・WS094 p012 と同じ回にまとめられる）
目安: 2h + ユーザー 20 分（1 Queue）。実行者の目安: phase-runner-mid
所有 path: `files/present.c`・`canvas.c`・`main.c`（F-037 を行うときだけ）、`plan/ws127/`

## 範囲

ユーザーが 5330（USB の素の起動、demo の image）で主な操作を行い、agent が SSH で Files の log を読む。F4 の数値を測る。遅ければ（> 目標）描き直しを damage の矩形に絞る（F-037）直しを同じ Phase で行う（2h の中で届かなければ別 Phase）。

## 受け入れ

F4: 主な操作がユーザーの目視で通る。1000 項目の folder の最初の frame ≤ 1000 ms、選択の frame ≤ 50 ms（3 回の中央値、5330）。QEMU と実機の証拠を分ける。
変えた source の style-check の違反 0、build の warning 0、`files-regress.sh` の 14 本と boot test PASS。

## 未決の判断

F4 の数値（p001 の基準値の後にユーザー）

## 検証の方法と範囲

QEMU の Venus（`plan/tools/files/files-guest.sh`、`files-regress.sh`）と host の試験。console・serial の log では判定しない。各 Phase の最後に `files-regress.sh` の 14 本と boot test。
Settings と共有の `canvas.c`・`text.c`・`icons.c` を変えたら `sh plan/ws089/tests/host-build.sh` と Settings の host 試験も流す。

## Event

2026-10-02 / ws127-beta1-plan: fg019 の計画で新設（planning）。p001 の結果とユーザーの選択で範囲を確定し planned にする。
