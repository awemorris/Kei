<!-- awesome-plan project=zedbsd record=ws116-p005 -->

# ws116-p005: qtwayland と zedBSD QEMU での代表 app

Parent: [WS116](../ws.md)
Status: planning
Disposition: normal
Primary Milestone: MG002（WSから継承）
Queue / attempts: none
Purpose / goal: qtwayland（Wayland の QPA）を cross build し、zedBSD amd64 QEMU の Keiland で p001 の代表 app を起動・操作する。
Prerequisites: p002、WS115 p009（libwayland 互換）、WS117 p003（compositor の改良）。
Investigation bound: timebox 4h。起動しない場合は原因の切り分けと残件を返す。

## 範囲

qtwayland の client の QPA plugin（shm の buffer integration を最初、EGL は p001 の判断）、代表 app（Qt Widgets の小さな app。Quick は p001 で採用時のみ）を `QT_QPA_PLATFORM=wayland` で起動、window・装飾・click・key・menu・dialog・resize・clipboard を QMP PNG で確認。Linux（WS117）との差を表にする。

## 受け入れ

代表 app が zedBSD QEMU で window を表示し p001 の代表操作が通る PNG と操作の証拠、`boot-test.sh` の起動確認。実機は未実施と書く。

## 所有 path

`userland/packages/desktop/qt6/`、`plan/ws116/`。compositor の修正が要る場合は WS117 か main に依頼する。

## Standards / evidence / resume

[Guardrail](../../guardrail.md)、[C全文](../../coding-style.md)、[scoped方針](../../standards/ws114-gtk-qt-learning.md)、[automation](../../standards/automation.md)、license audit を実装前に確認する。外部 package は公式 tarball を取得・検証して patch し、source tree へ取り込まない。toolchain は変更・build しない。共有 `build/` は読取専用。全体の `make check` は禁止。具体コマンド/版/結果/commit/環境/成果物/skip/制限は未実施（計画のみ）。各PhaseはQueueの個別scope承認後に実行する。

2026-10-02 / ws116-beta1-plan-20261002: 計画担当が作成（旧 p002 の実行部分を分割）。
