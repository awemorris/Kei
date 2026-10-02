<!-- awesome-plan project=zedbsd record=ws111p002 -->

# WS111 p002

Parent: [WS111](/home/awe/zedBSD-claude1/plan/ws111/ws.md)
Status: cleared
Disposition: normal
Queue / Attempt: q576 / q576-i01

## Scope / criteria

最終shell/make/docs全文規約確認、Linux/FreeBSD実sh/env/args/execとnative script build/install/mode/byte、GDM direct entryを検証。

## Design / dependencies / standards / bounds

Sharedscriptは既存native --session/glass/wallpaperをexecし、GDMのXDG runtime/envはそのまま利用、consoleで欠ければowned0700runtimeを作る。標準seat/defaultを利用し権限昇格なし、user追加引数quoteを保持。prefixは両native makeから生成。GDM desktop Execは直接waylandのまま。--login/--testingは対象外。

依存: p001 actual launcher output。Guardrail/全文C規約/automation参照はWSにリンク。C差分無し、sh syntax/両実OS環境とargv/execチェック、native script生成/0755install/diffcheck、最終reviewはp002。GUI操作自動再実施/driver/toolchain変更/remoteGDM改造は行わない。最大30分/attempt。

## Events

2026-10-02 ws111-user-console-launcher: current user実装依頼とGDM direct correctionで作成。Queueのみ実行境界、Issue/Project公開保留。

## Result / q576-i01 / 2026-10-02T02:51:00.577965+00:00

cleared。L1〜L3: 両OS shell/native build/install、FreeBSD/opt script0755、LinuxGDM direct entry unchanged、全source review PASS。[結果](/home/awe/zedBSD-claude1/plan/history/ws111/q576/result.md)。--loginは本人確認/PIN交換案のみ、WS110/testing未実装。

Event ws111-q576-cleared: result/clearance/close intent local保存、Issue comment/close pending。
