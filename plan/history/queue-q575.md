<!-- awesome-plan project=zedbsd record=queue -->

# Queue

Active Queue: なし
Last finished Queue: q575
Status: finished
Cycle: q575
Approval: current user /2026-10-02 chat「起動コマンドが長いので、/opt/keiland/bin/keiland-desktop でシェルスクリプトにまとめておいてもらえますか。LinuxでもFreeBSDでも使えるように。LinuxではGDMも引き続き対応。」＋correction「GDMはスクリプトを通さない方がいいです」。共通launcher実装/install/verificationを承認、GDMは既存wayland直接起動を保つ。--loginは「追加するのはどうでしょうか」の検討のみ。WS110 --testing/main.c変更は未承認。WIP commit、git push/実機pullは既存の開発host→FreeBSD検証承認を本launcherの同じ環境導入に適用、force pushなし。Issue/Project公開は保留。
Timebox: 最大30分 /1Phase
Focus: WS111 console launcher。既存fg010とdemo順保持。
Snapshot: [Phase](history/ws111/q575/approved-phase.md)、SHA256 d18874381ccd533e07a705a9df1c0d10ec00e5723673abc0b462bd8eca6a6dab

| Attempt | Phase | Exact scope | Status | Dependencies |
| --- | --- | --- | --- | --- |
| q575-i01 | [ws111p001](ws111/phase001/phase.md) | 共通keiland-desktop shell/script生成と両native build/installメンバー、consoleREADMEを実装・確認。GDM direct entryを保持。 | cleared | WS105/WS109 native build/install output |

Dependency graph: WS105/WS109 context → p001 → p002。WS110/--login implementationは本Queue外。
Started UTC: 2026-10-02T02:42:11.142975+00:00

## Upcoming Work Outlook

p001 launcher/install/README → p002最終全文規約/Linux+FreeBSD確認。WS110/testingとnative loginは検討のみ。

Outcome: q575-i01 /Phase cleared。共通scriptと両native install membership/console docsを実装。Linux native/stage/ELF/GDMdirect cmpと両OS shell env/args/exec PASS。[結果](/home/awe/zedBSD-claude1/plan/history/ws111/q575/result.md)。最終sourcereview/実機installはp002。
Finished UTC: 2026-10-02T02:47:22.732457+00:00
