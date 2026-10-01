<!-- awesome-plan project=zedbsd record=queue -->

# Queue

Active Queue: なし
Last finished Queue: q539

## q539

Status: finished
Purpose / Focus: WS106 の承認された移動を段階的に実施する。
Timebox: 最大60分。調査の上限と結果/未達を残す。
Approval: current user, 2026-10-01「WS106を実行してください。」、既存 WS106（commit0a43f635）の p001〜p003 / T1〜T4 と対象30件。
追加scope: 同日回答「13 ファイルも追加して移動する」（base/tests直下の12C+versiontest.map）。この範囲の source/location/reference の移動に限る。
Approved Phase snapshot: [保存した定義](/home/awe/zedBSD-claude1/plan/history/ws106/q539/approved-phase.md)、SHA256 67b7514d750527e7151084b55bbfe07e30c74711b118feb5ad7abffa6e48ed14。
Executor: Codex Q1 / N=0。前 Queue finished、owner 停止を確認。commit WIP / pushなし / GitHub公開deferred。
Applicable rules: AGENTS.md、Guardrail、coding-style全文、WS106 inventory/design。実装の意味を変えない移動。

| Attempt | Phase | exact scope | Status | 依存 |
| --- | --- | --- | --- | --- |
| q539-i01 | [ws106p001](/home/awe/zedBSD-claude1/plan/ws106/phase001/phase.md) | 30 package＋承認追加13ファイルの棚卸し、source/reference/menu/build/install契約と移動手順の確定。source移動なし。 | cleared | なし |

Dependency graph: user approval → q539-i01。context は別作業の許可ではない。

## Upcoming Work Outlook

WS106 内の後続を上の指示の同じ scope/criteria で1Phaseずつ選定。新 scope は自動で追加しない。
WS107〜109 と既存 fg010 は候補のみ、WS106 の実行指示から実装しない。

## Outcome

未確定。Started UTC: 2026-10-01T14:12:10.698083+00:00。

Outcome: q539-i01 cleared。30 package＋承認追加13files、全166 tracked fileのhash/mode/source→destinationと参照217fileを棚卸し。menu/package/config/install互換とbuild/boot手順をdesign.mdへ確定。既存styleはユーザーの限定例外を記録。ime-probeは人間作業との非競合回答まで選定から外す条件で、他29件と13filesは実行可能。証拠: plan/ws106/survey.json、programs-before.txt、style-before.txt、design.md。

Finished UTC: 2026-10-01T14:19:02.676446+00:00。同じWS106の既存承認範囲を除いて次Queueは自動実行しない。
