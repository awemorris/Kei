<!-- awesome-plan project=zedbsd record=ws118-p003 -->
# ws118-p003: 5320 の LCD の制御の修正（p002 の分類の後に分割）

Status: planning（p002 の分類を待つ）
Disposition: normal
Parent: [WS118](../ws.md)
Focused goal: fg019（ベータ1）
Queue: none
目安: 未定（分類ごとに 2〜4h の Phase に分ける）

## 範囲

p002 の分類に基づき `src/drivers/gpu/i915/`（主に `display/`）を直す。分類が出たらこのPhaseを分類ごとの Phase（p003・p005…）に書き直す。
修正のたびに 5330 を壊さないことを passthrough の smoke（`plan/ws099/tests/c5-hw.sh`、`/tmp/i915-hw.lock` を取る）で確かめ、5320 での確認は p001 の image でユーザーと行う（時期はユーザーに聞く）。

## 受け入れ

p002 の後に分類ごとに書く。共通: build warning 0、全文規約、5330 の c5-hw の PASS、QEMU の `plan/tools/boot-test.sh`、5320 の実機の結果（ユーザーの目視）を分けて記録。

## 所有 path

`src/drivers/gpu/i915/` の該当の部分（他の WS の i915 の作業と重なるので main が順を調整する）、`plan/ws118/`。

## 依存

p002。HAL の API の変更が要るなら差分を plan に置いて止める。

## 未決の判断

ベータ1 に間に合わないときの扱い（ws.md の T3 の注）。
