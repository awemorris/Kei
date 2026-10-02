<!-- awesome-plan project=zedbsd record=ws109p007 -->

# WS109 p007: 実機build/install・最終全文規約

Parent: [WS109](/home/awe/zedBSD-claude1/plan/ws109/ws.md)
Status: cleared
Disposition: normal
Queue / Attempt: q574 / q574-i01

## Scope / criteria

awe@10.0.30.3 ~/zedBSDでpull、必要なnative packages/seatd/videoを準備、make keiland-freebsdとsudo make keiland-freebsd-installを実行し実prefix/header/ELF/依存をread-back。p006で変えた全sourceの最終全文規約レビュー。

## Prerequisites / design / open decisions

p006 cleared actual source。SSH鍵確認済み。GUI合格はユーザーに残す。
変更はroot make入口/native FreeBSD mk/直接起動docs、指定実機のnative package/service/installのみ。kernel/HAL/toolchainの変更やdriver移植は対象外。Linux installは既存別install-sessionを正しく説明。FreeBSD GDMはユーザー撤回によりscope外。

## Standards / verification / bounds

[Guardrail](/home/awe/zedBSD-claude1/plan/guardrail.md)、[全文C規約](/home/awe/zedBSD-claude1/plan/coding-style.md)、[native full rules](/home/awe/zedBSD-claude1/plan/standards/ws109-native.md)。既存C変更が必要になれば全文を適用。新make/shellは近傍形式を保ち、BSD/GNU parse、no-config/toolchain dry-run、実FreeBSD native build warning0、実install/ELF/readbackで検証。最終規約レビューはp007。最大60分/attempt、新driver修理は含めない。user GUIはp008別。Issue/Project同期保留。

## Event history

2026-10-02 / ws109-20261002-physical-reopen-p007: ユーザーの実機受け入れ指示とGDM撤回により追加、p006→p007→p008の依存。既存p001〜p005結果は当時の条件で保存。実機操作・WIP push/pull承認はQueueで具体化。

## Design refinement / q574

実native build/install/header auditはPASS。GNU root entryのdry-runで既存cross-toolchain parseがFreeBSD realpath -mを呼ぶと判明。toolchainは変更せず、GNUmakefileでFreeBSD-only goalsを独立native mkへ直接転送する。BSD make既存entry/外部目標/依存/WS受け入れは同じ。再pull後GNU/BSD入口とinstallを再確認。seatd onestatusはpidfile権限のためsudoでread-back、普通userの失敗をdaemon不在と取り違えない。

## Result / q574-i01 / 2026-10-02T02:15:32.400926+00:00

cleared。実機native build/sudo install warning0、334source/11library native auditと全14実行file loader/hash PASS、seatd/video/runtime準備済み。[結果](/home/awe/zedBSD-claude1/plan/history/ws109/q574/result.md)。p008ユーザーlocalGUI acceptanceは未実施。

Event ws109-q574-cleared: local result/close intent保存、Issue comment/closeは保留。
