<!-- awesome-plan project=zedbsd record=queue -->

# Queue

Status: finished
Active Queue: なし
Last finished Queue: q532

## q532

- Purpose: compositor の Linux の module (2): `zwp_linux_dmabuf_v1` の server と implicit sync
- Focus: fg012 / WS105 の既存 Linux の受け入れを満たす。
- Timebox: 60 分。既知の build・必須検証は最大 150 分、未知の問題の調査は 30 分で結果と再開条件を記録する。
- Approval: current user, 2026-10-01「ws105の完了をゴールにして、自走をお願いします。」。既存 WS105 p001〜p011 の範囲を依存順の 1 Phase Queue で実装・検証・記録する。commit は WIP、push / GitHub 公開は行わない。
- Exact approved scope: [ws105-p007](ws105/phase007/phase.md) 全範囲、開始前 [snapshot](history/ws105/q532/scope.md) SHA256 `02384be9a813be9c97b9166df0d73c0bd8581aed176d719fbb492950cb0ae7fb`。
- Executor: Codex Q1。前 Queue finished、並行 executor なしを確認。
- Applicable rules: AGENTS.md、Guardrail、coding-style.md 全文、WS105 D1〜D25、design.md の指定節。
- Prerequisites: Phase の依存と既存出力を実装・証拠で確認。material scope / criteria の変更・取消なし。
- Started UTC: 2026-10-01T08:50:34.668497+00:00

| Attempt | Phase | Status | Dependency | Selection reason |
| --- | --- | --- | --- | --- |
| q532-i01 | [ws105-p007](ws105/phase007/phase.md) | cleared | p006（context） | WS105 の既存依存順 |

Dependency graph: Phase の依存（context）→ q532-i01/ws105-p007。他の Phase は次の Queue で選定する。

## Upcoming Work Outlook

WS105 の依存を満たす次の Phase。新しい product / risk 判断はユーザーの決定が前提。既存 WS105 の完了までの自走指示で選定・実行する。
GitHub への publication は deferred、outbox に保持する。

### 2026-10-01 window操作のユーザー判断

ユーザー（当チャット）: 「move/resizeは、Linux移植と関係ないバグの可能性があるので、いったんバグリストに記載するか、既存バグチケットに追記して、先に進みましょう。clear判定に進んでいいです。また、直せそうなら直してもいいですが、時間がかかりそうなら直さなくていいです。」

p076はBUG-125へ追加、p072はBUG-127へ移管。両方のFAILは保持し、修正済みとはしない。準備した追加diagnosticは未実施。その他の必須確認と新しいcursor-ownerの失敗は別途確認する。全13件PASSとは記録しない。GitHub公開は保留。

## Outcome

ws105-p007 **cleared**。

cleared（q532）。source `2d4abde1`（WIP）。Linuxのstandard zwp_linux_dmabuf_v1 v3 server / 1-plane validation / SCM_RIGHTSの所有 / Vulkan import / commitごとのimplicit acquire syncを実装。bind・object-free hookを既存OS境界へ追加、zedBSDは空実装。wltestとmviewの独立Linux buildとmodel data、test-only dmabuf-forgeを追加。gcc14.2 / clang19.1.7 exit0・warning0、15ELF / source-sync / 135headers PASS。formatter19・新moduleのstyle-check0、該当全文規約のmanual review。Linux guest: wltest600frame exit0・3import・600acquirefence、窓内部RGB(32,96,208)、mview model（25861vertex / 37000triangle / 13texture）描画。両PNGを目視・ユーザーに提示。forge out_of_bounds6 / IMPORT_ERROR / compositor継続 PASS、5窓fd20→20、SIGTERM error0 / cleanup_failed0、guest停止済み。V5の別process dma-buf importを確認。

zedBSD: disk-image exit0・自前warning0、OS境界C1〜C5 / V1（54source）、dedicated18 / decoder17 ×ordinary/sanitize、login PNG、forge拒否後の120frame / 3import、fence600（generation1全600、62秒）PASS。C1/C2/C9の元13件は10PASS・3FAILを保持。p076のresize416（期待200）は既存[BUG-125](../../bugs/BUG-125.md)、p072の最小化直後PNGは[BUG-127](../../bugs/BUG-127.md)へ未修正trackingとして移管。ユーザーは当チャットで「move/resizeは、Linux移植と関係ないバグの可能性があるので、いったんバグリストに記載するか、既存バグチケットに追記して、先に進みましょう。clear判定に進んでいいです。また、直せそうなら直してもいいですが、時間がかかりそうなら直さなくていいです。」と具体的なclear判断を許可。両件の長い追加調査は実施しない。修理・13/13PASSとは主張しない。cursor-ownerはtitle画像だけFAIL、同じsource・imageの単独1回で全条件PASS（title57 / desktop118 / body0 / body-again0）。[BUG-118](../../bugs/BUG-118.md)に元と追試の証拠を追記、原因と発生率は未調査で、既存修正の無効化は未証明。

[証拠とSHA256manifest](../../history/ws105/q532/evidence/SHA256SUMS)。未実施:実機GPU、非同期hardware wait / FOREIGN queue ownership（design既存V3/V8/V9の制限）、tracked2件の修正。その他のp007必須条件は確認済み。GitHub publication / closeは未実施、eventはoutboxに保持。WS105はincomplete、次はp008。

実装 commit `2d4abde19fa2837bb9b7014d1bf931542d557c21`。Finished UTC: 2026-10-01T09:45:04.063506+00:00。次の Phase は WS105 の既存完了までの自走指示で選定。push / GitHub 公開は未実施。
