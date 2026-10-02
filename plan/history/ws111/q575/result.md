# q575 launcher implementation

共通POSIX shell.inを両native makeから生成しbin/keiland-desktopをinstall memberに追加。consoleHOME fallback0700runtime、既存XDG runtimeとseat/session env維持、display名とsocketの一致、session/glass/installedwallpaper、追加argv/space/customprefix、execによるPID/exit/signal保持。GDM.desktopはbyte変更無し、直接waylandのまま。

実Linux/FreeBSD shでownedfakecompositorを使ったenv/args/runtime/path refusal/execSIGTERM全PASS。Linux native all/install/session stage成功warning0、launcher0755、ELF24全PASS、GDM.desktopを元sourceとcmp一致。console配布READMEのみ短縮、packageの既存install loopがscriptを含む。C/--testing変更無し。scriptに--loginはまだ実装せず、最新の本人password確認→将来PIN provider案へ設計を更新。FreeBSD実native installと最終全文reviewはp002。
