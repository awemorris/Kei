<!-- awesome-plan project=zedbsd record=ws109p008 -->

# WS109 p008: ユーザー実機GUI受け入れ

Parent: [WS109](/home/awe/zedBSD-claude1/plan/ws109/ws.md)
Status: cleared
Disposition: normal
Queue / Attempt: none

## Scope / criteria

ユーザーが実機でKeilandを操作し動くと確認した時のみclearしF6/WS acceptanceを再判定。

## Prerequisites / design / open decisions

p007 build/install。ユーザーの実操作報告が必要、agentが代替しない。
変更はroot make入口/native FreeBSD mk/直接起動docs、指定実機のnative package/service/installのみ。kernel/HAL/toolchainの変更やdriver移植は対象外。Linux installは既存別install-sessionを正しく説明。FreeBSD GDMはユーザー撤回によりscope外。

## Standards / verification / bounds

[Guardrail](/home/awe/zedBSD-claude1/plan/guardrail.md)、[全文C規約](/home/awe/zedBSD-claude1/plan/coding-style.md)、[native full rules](/home/awe/zedBSD-claude1/plan/standards/ws109-native.md)。既存C変更が必要になれば全文を適用。新make/shellは近傍形式を保ち、BSD/GNU parse、no-config/toolchain dry-run、実FreeBSD native build warning0、実install/ELF/readbackで検証。最終規約レビューはp007。最大60分/attempt、新driver修理は含めない。user GUIはp008別。Issue/Project同期保留。

## Event history

2026-10-02 / ws109-20261002-physical-reopen-p008: ユーザーの実機受け入れ指示とGDM撤回により追加、p006→p007→p008の依存。既存p001〜p005結果は当時の条件で保存。実機操作・WIP push/pull承認はQueueで具体化。

## Resume / 2026-10-02

p007/q574 cleared、実機/opt・seatd/video/runtime準備済み。ユーザーへlocalconsoleからの--session手順を案内。GUIのuser確認報告待ち、実装Queue無し。実機で動いたとの報告が受け入れ判断の入力。ログインscreen/sessiond/nativepasswordlockの未移植をdocs/ユーザーへ明示した。

## Human acceptance / 2026-10-02 JST

Current user、このchat「完璧に動作しました。」。p007で準備した awe@10.0.30.3 /FreeBSD15.1-RELEASE /TigerLake IrisXe /b49ffa9bのlocalconsole直接sessionについて、最後のユーザー実操作条件F6を受け入れ合格と判定。Phase cleared、WS自身のF1〜F6を照合してcompleted。ユーザーの手動acceptance記録であり実装Queueは無し、q574はp007の当時の終了結果を保つ。FreeBSD GDMは撤回済み、native sessiond/graphical authenticationの移植は未実施。ユーザーの一括動作報告を個別WiFi/PCM検査の測定証拠には拡張しない。

Event ws109-physical-user-accepted-20261002: cleared/close intent local保存、Issue comment/closeは公開保留。
