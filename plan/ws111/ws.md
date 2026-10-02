<!-- awesome-plan project=zedbsd record=ws111 -->

# WS111: Linux/FreeBSD共通 keiland-desktop launcher

Status: incomplete
Primary Milestone: MG006
Related Milestones: MG007
Parent: [Master](../master.md)
Queue: none（q575 finished）
Resume point: userの共通console launcher実装をp001→p002で進める。GDMはwayland直接起動。

## Objective / acceptance

/opt/keiland/bin/keiland-desktopだけで既存--session/glass/wallpaper/runtime設定をまとめて起動、Linux/FreeBSD native installに含める。GDMのExecは既存waylandを直接実行、認証/seat環境を保つ。customprefix/追加引数にも対応。L1 共通scriptのenv/args/runtime/execが実Linux/FreeBSD shで成立。L2 native build/installに0755で含まれ既存GDM direct entry unchanged。L3 final全変更source全文規約/該当build/差分/実installを検証。

Scope: 共通shell.in・両native mk・FreeBSD/配布READMEとconsole docs、必要なfocused probes。main.c/--testing変更/ネイティブlogin実装/renderer/HAL/toolchain変更は含めない。LinuxGDMはscriptを通さないというuser correctionが優先。--loginは[設計検討](login-design.md)のみ。

## Standards / prerequisites

[Guardrail](../guardrail.md)、[全文C規約](../coding-style.md)、[automation](../standards/automation.md)。今回はC生成なし、shell/makeは近傍形式、POSIXsh+nativehost確認。WS105/WS109のactualnative `/opt`/GDM/seat outputsを利用。WIP auto commit、既存承認の指定FreeBSDへのpush/pull/installを利用し検証。Issue/Projectは保留。

| ID | Purpose | Goal | Status | Dependencies |
| --- | --- | --- | --- | --- |
| ws111p001 | [共通console launcher実装](phase001/phase.md) | L1/L2 script/build/install/docs | cleared / q575 | WS105/WS109 actual native outputs |
| ws111p002 | [最終全文規約/両OS確認](phase002/phase.md) | L3 final all changed source/native install | planned | p001 actual source |

2026-10-02 / ws111-user-console-launcher: currentuser共通起動script実装を承認。直後の「GDMはスクリプトを通さない方がいいです」でLinux.desktop変更をscopeから除外、currentExecに変更無し。WS110 testing提案は別の未承認検討として維持。新login質問は検討のみ。

2026-10-02 / ws111-login-own-user-decision: userが将来PIN・現在はログイン済み本人password確認を選択。OSlogin/ユーザー切替案を撤回しscreen/provider分離のgateへ更新。現launcher p001/p002外部scope/依存は変更無し、--login実装は別の有限Queueが必要。

2026-10-02T02:47:22.733590+00:00 / ws111-q575-cleared: p001 cleared。共通scriptと両native install membership/console docsを実装。Linux native/stage/ELF/GDMdirect cmpと両OS shell env/args/exec PASS。[結果](/home/awe/zedBSD-claude1/plan/history/ws111/q575/result.md)。最終sourcereview/実機installはp002。
