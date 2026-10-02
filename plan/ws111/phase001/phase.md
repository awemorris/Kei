<!-- awesome-plan project=zedbsd record=ws111p001 -->

# WS111 p001

Parent: [WS111](../ws.md)
Status: cleared
Disposition: normal
Queue / Attempt: q575 / q575-i01

## Scope / criteria

共通keiland-desktop shell/script生成と両native build/installメンバー、consoleREADMEを実装・確認。GDM direct entryを保持。

## Design / dependencies / standards / bounds

Sharedscriptは既存native --session/glass/wallpaperをexecし、GDMのXDG runtime/envはそのまま利用、consoleで欠ければowned0700runtimeを作る。標準seat/defaultを利用し権限昇格なし、user追加引数quoteを保持。prefixは両native makeから生成。GDM desktop Execは直接waylandのまま。--login/--testingは対象外。

依存: WS105/WS109 verified native output。Guardrail/全文C規約/automation参照はWSにリンク。C差分無し、sh syntax/両実OS環境とargv/execチェック、native script生成/0755install/diffcheck、最終reviewはp002。GUI操作自動再実施/driver/toolchain変更/remoteGDM改造は行わない。最大30分/attempt。

## Events

2026-10-02 ws111-user-console-launcher: current user実装依頼とGDM direct correctionで作成。Queueのみ実行境界、Issue/Project公開保留。

## Result / q575-i01 / 2026-10-02T02:47:22.731083+00:00

cleared。共通scriptと両native install membership/console docsを実装。Linux native/stage/ELF/GDMdirect cmpと両OS shell env/args/exec PASS。[結果](/home/awe/zedBSD-claude1/plan/history/ws111/q575/result.md)。最終sourcereview/実機installはp002。

Event ws111-q575-cleared: result/clearance/close intent local保存、Issue comment/close pending。
