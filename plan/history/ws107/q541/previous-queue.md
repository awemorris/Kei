<!-- awesome-plan project=zedbsd record=queue -->

# Queue

Active Queue: なし
Last finished Queue: q540

## q540

Status: finished
Purpose / Focus: WS106 の承認された移動を段階的に実施する。
Timebox: 最大60分。調査の上限と結果/未達を残す。
Approval: current user, 2026-10-01「WS106を実行してください。」、既存 WS106（commit0a43f635）の p001〜p003 / T1〜T4 と対象30件。
追加scope: 同日回答「13 ファイルも追加して移動する」（base/tests直下の12C+versiontest.map）。この範囲の source/location/reference の移動に限る。
Approved Phase snapshot: [保存した定義](/home/awe/zedBSD-claude1/plan/history/ws106/q540/approved-phase.md)、SHA256 9da9c345789fe7a41a4fe467f4c49a0028eb1b3d401a079730e6a354ee33d401。
Executor: Codex Q1 / N=0。前 Queue finished、owner 停止を確認。commit WIP / pushなし / GitHub公開deferred。
Applicable rules: AGENTS.md、Guardrail、coding-style全文、WS106 inventory/design。実装の意味を変えない移動。

| Attempt | Phase | exact scope | Status | 依存 |
| --- | --- | --- | --- | --- |
| q540-i01 | [ws106p002](/home/awe/zedBSD-claude1/plan/ws106/phase002/phase.md) | p001確定手順のpartial scope: 所有確認待ちime-probeを除く29 package＋直下13files、grouping/Tests menu、source/build/runnerの参照を移動・照合。ime-probeは非競合回答後に明示的scope補完するまで移動/編集しない。 | cleared | ws106p001 cleared と実成果（context） |

Dependency graph: ws106p001（context） → q540-i01。context は別作業の許可ではない。

## Upcoming Work Outlook

WS106 内の後続を上の指示の同じ scope/criteria で1Phaseずつ選定。新 scope は自動で追加しない。
WS107〜109 と既存 fg010 は候補のみ、WS106 の実行指示から実装しない。

## Outcome

partial item cleared / whole Phase uncleared。Started UTC: 2026-10-01T14:21:05.773866+00:00。

Finished UTC: 2026-10-01T14:50:18.952284+00:00。29 package＋追加13filesの承認partial scopeはcleared。164filesのhash/mode/参照・registry保存、Linux GCC/Clang clean build/install warning0（ELF24/source331各）、zedBSD28app/POSIX/loader/image warning0、boot-test.sh login PNG確認。ime-probe2filesは非競合回答待ちで未変更。whole p002はuncleared、p003は未実行、WS106はincomplete。

[再開と検証](/home/awe/zedBSD-claude1/plan/ws106/verification-checkpoint.md)、[BUG-129](/home/awe/zedBSD-claude1/plan/bugs/BUG-129.md)。次Queueは未選定。ime-probe所有確認後に残りscopeの新attemptを選定する。p003依存はwhole p002のclearance、今回の部分達成を全体clearanceに読み替えない。GitHub publication pending、commit WIP / pushなし。
