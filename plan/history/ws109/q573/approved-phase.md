<!-- awesome-plan project=zedbsd record=ws109p006 -->

# WS109 p006: native make入口/直接起動/docs

Parent: [WS109](../ws.md)
Status: planned
Disposition: normal
Queue / Attempt: none

## Scope / criteria

FreeBSD base makeとGNU makeの両方でkeiland-freebsd/keiland-freebsd-installが使え、config/toolchain不要。直接起動手順とLinuxの別session登録を正確に文書化。FreeBSD GDMはユーザー撤回により対象外。

## Prerequisites / design / open decisions

q572 native output verified。BSD make/GNU makeのfile探索差を確認。
変更はroot make入口/native FreeBSD mk/直接起動docs、指定実機のnative package/service/installのみ。kernel/HAL/toolchainの変更やdriver移植は対象外。Linux installは既存別install-sessionを正しく説明。FreeBSD GDMはユーザー撤回によりscope外。

## Standards / verification / bounds

[Guardrail](../../guardrail.md)、[全文C規約](../../coding-style.md)、[native full rules](../../standards/ws109-native.md)。既存C変更が必要になれば全文を適用。新make/shellは近傍形式を保ち、BSD/GNU parse、no-config/toolchain dry-run、実FreeBSD native build warning0、実install/ELF/readbackで検証。最終規約レビューはp007。最大60分/attempt、新driver修理は含めない。user GUIはp008別。Issue/Project同期保留。

## Event history

2026-10-02 / ws109-20261002-physical-reopen-p006: ユーザーの実機受け入れ指示とGDM撤回により追加、p006→p007→p008の依存。既存p001〜p005結果は当時の条件で保存。実機操作・WIP push/pull承認はQueueで具体化。
