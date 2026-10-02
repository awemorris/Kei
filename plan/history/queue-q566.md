<!-- awesome-plan project=zedbsd record=queue -->

# Queue

Active Queue: なし
Last finished Queue: q566
Status: finished
Cycle: q566
Approval: current user /2026-10-02 JST /this chat「WS108の完了後、WS109の実行をお願いします。」＋「drm-kmodを利用OKです。FreeBSDにも例外を適用します。FreeBSD実機は用意しておくので、作業を進めておいてください。」。WS109の既存目標内で実装先行、実機関門を保持。
Timebox: 最大60分 /1Phase
Focus: fg016 /WS109、WIP commit/pushなし、GitHub publication deferred。
Snapshot: [approved Phase](history/ws109/q566/approved-phase.md)、SHA256 9961f9d174bc5b9b212e6222d91f6bdc42ee6179a09114d2809971c32f4c8f69

| Attempt | Phase | Exact scope | Status | Dependencies |
| --- | --- | --- | --- | --- |
| q566-i01 | [ws109p005](ws109/phase005/phase.md) | p005 partial: full WS source conformance/retained wire+Places fixes/native operational docs/affected native+Linux+zedBSD regressions; physical GUI/GPU/WiFi gates retained。 | cleared | verified p001 software/ABI/environment output; further scope-specific prerequisites in Phase |

Dependency graph: verified p001 software/ABI/environment output; further scope-specific prerequisites in Phase → q566-i01。外部の実機準備はcontext、native実装先行はユーザー承認。未着実機はverified acceptanceでない。
Started UTC: 2026-10-01T23:54:42.769175+00:00

## Upcoming Work Outlook

本Queueの実outputから次の有限Phaseを選定。GPU/WiFiの実機関門、C全文review/affected3OS回帰を保持。WS106の保留は別。

## 2026-10-02 / ws109-20261002-qemu-venus-acceptance

Current user, this chat async reply: 「実機検証は不要です。qemuでVenusが使えればclearとします。」
This explicitly replaces the earlier user-provided-machine gate. Physical GPU/display/WiFi
acceptance is waived for WS109; do not request hardware or reintroduce those gates. Required
replacement evidence is actual FreeBSD QEMU Venus usage; mere host support, headless lavapipe or
Linux/zedBSD Venus does not establish that evidence. Native backend/build/standards and affected
regression obligations remain. p004 hardware radio operations become waived, not falsely tested;
native audio/wired and native radio ABI/refusal/WPA wire evidence remains classified accurately.
p003/F3 and p005/F5 replace physical display/main-app checks with owned FreeBSD QEMU Venus-backed
checks. If the native guest stack lacks a required Venus driver, investigate a bounded actual
capability chain and expose the remaining platform/scope choice; kernel/driver port is still outside
WS109's agreed scope. Current q566 standards/regression/docs subset stays authorized; Venus
configuration/implementation is selected separately after q566. No automatic WS/Phase clearance.
Origin user decision reconciled to WS/all changed Phase own criteria, Guardrail/scoped standard,
Queue supplement and docs; remote decision/structural events pending publication.

Outcome: q566-i01 cleared /whole Phase uncleared。Final changed-source standards, wire/Places fixes, native docs and affected native/Linux/zedBSD checks PASS. [result](/home/awe/zedBSD-claude1/plan/history/ws109/q566/result.md). Physical tests waived; whole p005/WS pending actual FreeBSD QEMU Venus.
Finished UTC: 2026-10-02T00:22:28.924164+00:00
