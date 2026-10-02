<!-- awesome-plan project=zedbsd record=ws129-p006 -->
# ws129-p006: release candidate の最終回帰

Status: planning（p003〜p005、凍結の日を待つ）
Disposition: normal
Parent: [WS129](../ws.md)
Focused goal: fg019（ベータ1）
Queue: none
目安: 3h

## 範囲

凍結した commit（RC）で release の config の image を作り、次を通す（集約の `make check` は使わない）:

1. `plan/tools/boot-test.sh`（framebuffer の login prompt、PNG をユーザーに見せる）。
2. 領域ごとの host の試験の選んだ組（p001 の一覧: network の story と managed-lan、Settings の `settings-regress.sh`、Files、インストーラの QEMU の通しなど）。
3. 5330 の passthrough の smoke（`plan/ws099/tests/c5-hw.sh`、`/tmp/i915-hw.lock`）。AX211 が入るなら ws005-p021 の短い版。
4. p002 の license の一覧の再生成と audit。

## 受け入れ

全部の PASS/FAIL と未実施を記録。FAIL は担当の WS へ返し、RC を作り直すかは main とユーザーの判断。QEMU・passthrough の証拠と書く。

## 所有 path

`plan/ws129/`。

## 依存

p003・p004・p005、凍結の日（ユーザー）。

## 未決の判断

なし。
