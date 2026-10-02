# ws099-p017 / q589-i01 low-overhead partial diagnosis

Whole Phase: uncleared; BUG-125: reproduced / tracking.
Approval: current user / 2026-10-02「サブエージェントは終了せずに、次々とqueueを送り込んで、長時間稼働させてください。」を既存B3 bug切り分け担当の次の有限診断へ適用。AがB3用q589を予約。
Timebox: 最大45分、original stages1–4最大5回、各実行240秒。guest資源待ちは実測開始前にmainへ報告。
Purpose: q583のcapture前SSH snapshotsによるsettle時間の影響だけを減らし、q577の2 popup症状を再び分類する。

## Exact scope / constraints

B3所有の独立helperをplan/ws099/testsに追加/変更し、元p076のpointer/check/期待値/negative条件は維持。capture前SSH snapshot8回を除き、入力/capture前後のhost時刻はshell builtinで/proc/uptimeを読む（約10ms分解能、guestとの原点共有を主張しない）。--log-framesを維持し、application logは最初のcapture判定後/終了時だけ回収。最初のFAILは保存、最大6後続captureは分類だけでPASSへ書換えない。並列guest/load注入、shared p076、製品compositor修正は含めない。新 helperとこのPhase結果/evidenceのみ所有。

## Prerequisites / resource / standards

q583 helper/195 rawassetsはB main統合3ed1834c、terminal/projection df66db5e。q577の元FAILとimage/明示renderer hashを保持する。自身のread-only入力imageから独立copyと専用runtimeを使う。QEMUはB1 q587停止後にmainがgrant。共有build/toolchain/sysrootは読取のみ。

既読AGENTS/Guardrail/C全文/automation/agent protocolを適用し、変更部分とhelperに必要な内容だけ追加確認。Python/shell構文、有限deadline/owned process停止、first-verdict保持のhost負例を確認。console/serial logで判定せず、plan/tools/boot-test.shのPNGとguest loopback SSH/QMPで証拠を残す。

## Partial clearance criteria / resume

最大5回のfirst PNGとframe/input順でmapped-but-invisibleとmap前clickの各症状を独立分類する、または5回非再現/期限到達を証拠・計測限界とともに記録する。syntax/manual規約・host負例、owned guest/renderer停止readbackを保存。修正効果を断定しない。結果/MR/commands/versions/hashes/skipを耐久記録し、Queue partial outcomeを報告。

全p076/C9・p128/C2・物理・修正の基準はこの部分scope外。partial clearはwhole p017のclearanceやBUG resolutionを意味しない。追加反復/負荷は別の有限Queue。完了後は同一agentでmainの後続dispatchを待つ。
