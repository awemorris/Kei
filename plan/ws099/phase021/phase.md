<!-- awesome-plan project=zedbsd record=ws099-p021 -->

# ws099-p021: C2 の geometry の不一致と BUG-127（最小化の直後の画像）

Status: planned（2026-10-02 ベータ1の計画。Queue なし）
Disposition: normal
Parent: [WS099](../ws.md)
Queue: なし
依存: [p020](../phase020/phase.md) cleared（同じ shell.c・toplevel.c の周りを変えるため直列。p020 の直しで消える可能性もある）
目安: 3h（1 Queue）。実行者の目安: phase-runner（high）
所有 path: `userland/desktop/wayland/` の shell.c・toplevel.c・compose.c・damage.c の該当の所、`plan/ws099/tests/c2-geometry.sh`（判定は弱めない）、`plan/ws099/phase021/`、[BUG-127](../../bugs/BUG-127.md) と Bug Board の該当の行

## 背景

- WS105 q538（zedBSD の Venus、target source `8fb18105`）の C2: top-right の drag で 40 の増分を期待して 20（1220×820）、bottom-left・left の settle と settled の log が不一致（[証拠](../../history/ws105/q538/evidence/criteria/c2-geometry.log)）。C2 は WS099 の基準の 1 つ。
- [BUG-127](../../bugs/BUG-127.md): C9 p072 の最小化の log の後の PNG に青い窓が残る（1 回の観測、後の回は PASS）。

## 範囲

1. p020 の後の main で `criteria.sh … C2 C9` を 3 回流し、C2 の不一致と BUG-127 が残るかを確かめる。残らなければ、その証拠で ticket を更新し（BUG-127 は再現せずなら reproduced/tracking のまま証拠を足す）、Phase を clear にする。
2. 残るなら、resize の増分（pointer の delta と configure の大きさ、client の ack の大きさ）と最小化の後の最初の compose の時刻を log で比べ、compositor の誤りを直す。
3. 試験の期待が誤り（例えば app の size の刻み）と証明できたときだけ試験を直す。

## 受け入れ

- `criteria.sh … C2 C9` を同じ image で 3 回、C2 と p072 の FAIL 0。
- BUG-127: 原因を直して resolved、または 3 回 + p072 の単独 10 回で再現せず、証拠を ticket に足して tracking のままにする（後者はユーザーに非阻害の確認を求める）。
- 変えた source の全文規約、wayland の build の warning 0、C3・C4・C8 と p076 の単独 5 回 PASS、boot test PASS。

## 検証の方法と範囲

QEMU の Venus。console・serial の log では判定しない。実機は範囲の外。

## 未決の判断

- BUG-127 が再現しないときに tracking のまま WS099 のベータ1 の受け入れを通してよいか（ユーザー。2026-10-01 の WS105 での非阻害の許可は WS105 の clear の判断で、WS099 には別に確かめる）。

## Event

2026-10-02 / ws099-beta1-plan-p021: fg019 の計画で新設。C1〜C10 は不変。
