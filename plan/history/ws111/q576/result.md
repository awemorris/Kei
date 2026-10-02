# WS111 q576 / final launcher acceptance

L1/LinuxとFreeBSD実shのconsole/default runtime/argv/customprefix/exit/execSIGTERM/environmentとpath refusal全PASS。L2両native mkall/installメンバー・Linux stage0755/ELF/GDMcmp、FreeBSD~/zedBSD実make/sudo install、/opt/bin/keiland-desktop rootwheel0755・buildbytecmp成功。root elevated launchはscriptで自動化しない。GDMはwayland直接のまま（sameSHA/変更0）、既存logind環境/backendを変更しない。L3全変更source最終review/該当buildとprobeを確認、C差分0。launcher script以外の--session/testing/renderer/auth semantics不変。

FreeBSD runtime readiness: source41ef7553を非force push/実機pull後makeinstallしinstalledlauncherの引数拒否で前seatexec確認。GUI/sessionはagent再起動しない（ユーザーの合格済み環境を保つ）。最後のsource/test/docs/result commitもpush/実機pullしてsourcestateを合わせる。各CI release builderは既存native install loopからscriptをpackageに含める、GDM.desktopは直接entry。ログイン/PINは本人確認gateのuserdesign選択をlogin-designに保存、--login実装は無し。

[全source全文レビュー](source-review.md)、[実機install](freebsd-install.txt)、[Linux shell](linux-final-shell.txt)、[FreeBSD shell](freebsd-final-shell.txt)、[再利用probe](/home/awe/zedBSD-claude1/plan/tools/keiland-launcher/README.md)。WIP、Issue/Project公開保留。
