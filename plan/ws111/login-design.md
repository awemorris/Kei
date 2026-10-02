# --login の検討

user「ログイン画面から開始する --login も追加するのはどうでしょうか？」。今回は候補interfaceの検討のみ。

推奨入口は `keiland-desktop --login`。既存compositor --greeterは画面とauthchannel、--sessionは認証後のdesktopという役割を保つ。単に--greeterへ置換してもauth-fd/ユーザー切替が無いためloginは成立しない。

native Linux/FreeBSDで必要な範囲: 既存sessiondのpolicy/protocolを分離し、OS側のPAM等による認証、account制限、initgroups/setgid/setuid・session/env/runtime作成、greeter→userseatの解放/取得、login後のchild起動/Logout/終了cleanupを実装する。FreeBSDはseatd、Linuxはlogind/直接consoleとsession登録を整合。unprivileged scriptが自動sudoする形やcompositor本体がrootへ昇格する形は避け、権限を持つmanagerの起動を明確にする。認証規約/API/privilege modelは別WSで設計/受け入れが必要。

LinuxGDM経由はGDMが認証してwaylandを直接起動する現行ルート。--loginをGDMから使って認証を二重にしない。standalone loginはconsole入口、GDMは別入口として持てる。GDM無しのFreeBSDにも独自loginを提供できる価値がある。現在のscriptに未実装optionや偽の認証channelを追加せず、この検討を次の具体的WS候補にする。

## 最新のユーザー判断 / 2026-10-02

user「ログイン画面は、そのうちPINチェックだけにするので、きちんとしたログイン認証でなくてよいです。現状では、ログイン済みユーザのパスワードチェックになっていると思いますし、それでいいです。」。

上の新しいOSlogin/session作成/ユーザー切替案を撤回し、既存ログイン済みユーザーの本人確認gateとして再設計する。入力対象は実UIDから取得する本人に固定、OSへの新login/initgroups/setuid/session/account選択は行わない。既存zedBSD greeterはユーザー選択を持つため、その動作を「現状native本人確認」とは説明しない。共通screenを再利用しgate通過後同じUID/環境でdesktopへ進む。GDMは既存認証後の直接waylandで二重gateを置かない。

推奨API: keiland-desktop --loginが共通本人確認screenを開始。確認providerを画面/desktop起動から分離し、当初は本人のOSpassword確認、将来PINへ交換する。取消/失敗/サービス不在はdesktopへ進まず、確認値をargv/env/log/fileへ置かない。今回launcherには未実装--loginを使えるように見せかけるstubを追加しない。

passwordを確認するOSproviderは別途必要。FreeBSD pam_unixはgetpwnamのpassword hashとcryptを比較する（[公式実装](https://github.com/freebsd/freebsd-src/blob/main/lib/libpam/modules/pam_unix/pam_unix.c)）。普通userからhashを読めない環境では、OSが提供する確認helperの利用可否、または本人確認だけの小さなhelperの必要性を調べる。ユーザー切替用sessionmanagerの全移植は不要でも、password照合まで画面だけで実現できるとは扱わない。PINはユーザーの将来計画で、現時点の架空PIN/無条件成功に置換しない。

本決定は本人確認gateという設計scopeの選択。現active q575/p001のsharedscript/install成果物とは独立、--login実装Queueはまだ無い。起動scriptを具体化してから必要なprovider/画面modeの小scopeを別途計画する。
