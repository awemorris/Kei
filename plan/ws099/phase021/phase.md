<!-- awesome-plan project=zedbsd record=ws099-p021 -->

# ws099-p021: C2 の geometry の不一致と BUG-127（最小化の直後の画像）

Status: uncleared（q620-i01、2026-10-03 P2。C2 の geometry の不一致と BUG-127 は再現しなかった。C2 の FAIL 0 は、BUG-147 の起動の検出の失敗 1 回で満たさない。判断は下の「判定の提案」）
Disposition: normal
Parent: [WS099](../ws.md)
Queue: q620 / q620-i01（自走の指示による Q1 の dispatch、時限 3h）
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

## 結果（q620-i01、P2、2026-10-03 04:24〜06:35）

worktree `/home/awe/zedBSD-worktrees/p2`（main `a53205d3b`）。image は `build/p2-p021-img`（build は exit 0。compiler の warning は外部の openssh だけ）。QEMU の Venus。console・serial の log は読んでいない。**製品 source は変えていない。**

### 観測

| 試験 | 結果 |
| --- | --- |
| `criteria.sh … C2 C3 C4 C8 C9` を 3 回（[summary](evidence/summary.txt)） | 1 回目は 13/15。C2 は App Home から Files を開いた後の `ZWL GLASS launch` が出ず、geometry に入る前に止まった（[log](evidence/crit-1-c2.log)）。p138 は Notes の解除の log が出ず、SSH の banner timeout が続いた（[log](evidence/crit-1-p138.log)）。2 回目・3 回目は 15/15 PASS |
| C2 の geometry（q538 の top-right の増分 20、bottom-left・left の settle の不一致） | 2 回目・3 回目は `pass=14 fail=0`（[2 回目](evidence/crit-2-c2.log)）。p020・p023・p008 の 7 回でも C2 は PASS。**再現しない。** q538 の 20 の増分（2 段の drag の後半だけ）は、p020 で直した event loop の stall で press が遅れた形と合う。ただし q538 の log でそれを証明してはいない |
| p072（BUG-127） | criteria の 3 回で PASS。単独で 10 回流して 10/10 PASS（[summary](evidence/p072-x10-summary.txt)、[1 回目の最小化の PNG](evidence/p072-run1-minimized.png)）。p023・p008 の 6 回でも PASS。**再現しない** |
| boot test | PASS（[PNG](evidence/boot-login.png)） |

### C2 の 1 回目の FAIL（BUG-147 と同じ形）の見当

`ZWL GLASS launch` は、App Home の icon を押してから `HOME_LAUNCH_WAIT_MS`（5 秒）の内に窓が map されたときだけ出る（`home.c` の `zwl_home_launched`、`shell.c` の `zwl_glass_mapped`）。新しい guest で同じ操作を 3 回流すと、待ちは 2.14〜2.24 秒だった（[1](evidence/launch-1.txt)・[2](evidence/launch-2.txt)・[3](evidence/launch-3.txt)）。同じ 1 回目の起動で、desktop の Files の `app=4680` ms のように、guest の中で数秒の遅れも出ている。この遅れ（BUG-135 の系統）が Files の窓の起動に重なると、map が 5 秒を超え、launch の log が出ない。C2 と p128 は、この log が出ることを前提にしている。証明はしていない（FAIL した回の session.log は残っていない）。

### 判定の提案

- q538 の C2 の geometry の不一致は再現しない（9 回）。BUG-127 は criteria 3 回と単独 10 回で再現しない。reproduced/tracking のまま証拠を足した。WS099 のベータ1 で非阻害にしてよいかは、ユーザーの確認が要る（phase.md の未決の判断）。
- 受け入れの「C2 の FAIL 0」は、1 回目の BUG-147 の形の失敗で厳密には満たさない。製品 source を変えていないので、C3・C4・C8・p076 の追加の回帰はしていない（criteria の中では C3・C4・C8・p076 も各回 PASS）。
- 次の手の案（別の Phase）: BUG-147 として、(a) 試験が `GLASS launch` だけでなく、起動した client の `ZWL MAP` からも窓を見つけられるようにする、または (b) launch の待ちと log の出し方を見直す。どちらも BUG-135（guest の stall）を直すまでの回避策。
