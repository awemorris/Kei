<!-- awesome-plan project=zedbsd record=ws116-p004 -->

# ws116-p004: host の Qt の道具

Parent: [WS116](../ws.md)
Status: planning
Disposition: normal
Primary Milestone: MG002（WSから継承）
Queue / attempts: none
Purpose / goal: Qt6 の cross build に要る、target と同じ版の host の Qt の道具（moc・rcc・uic・qtwaylandscanner・syncqt 等）を host の compiler で build する。
Prerequisites: p001 の版と module の決定。
Investigation bound: timebox 3〜4h（host の qtbase の build は 16 並列で数十分の見込み）。

## 範囲

公式 tarball（p001 の版）を検証して展開し、host 用に tools だけを build する規則を `userland/packages/desktop/qt6` に置く（成果物は自分の worktree の `build/` の下の host tools、host に install しない）。host の CMake 3.31 と ninja を使う。

## 受け入れ

host の moc・rcc・uic（と qtwayland の scanner）が build され、`QT_HOST_PATH` として p002 が使える。provenance と license を記録。

## 所有 path

`userland/packages/desktop/qt6/`、`plan/ws116/`

## Standards / evidence / resume

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[scoped方針](../../standards/ws114-gtk-qt-learning.md)、[automation](../../standards/automation.md)、license audit を実装前に確認する。外部 package は公式 tarball を取得・検証して patch し、source tree へ取り込まない。toolchain は変更・build しない。共有 `build/` は読取専用。全体の `make check` は禁止。具体コマンド/版/結果/commit/環境/成果物/skip/制限は未実施（計画のみ）。各PhaseはQueueの個別scope承認後に実行する。

2026-10-02 / ws116-beta1-plan-20261002: 計画担当が作成。
