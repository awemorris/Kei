
## Terminal 起動の bounded 補完（2026-10-01、Q1）

App HomeのTerminal childがstatus139、直接起動も同じ。source/objdumpでmain_start→main_menu_stateが最初のmain_tab_newより先、main_screenはNULLのままselection/rangeを読むと確認。OS分岐の問題ではない。D21のTerminal起動・入力という既存受け入れに必要な普通の技術修正として、common terminal/main.c:main_menu_stateを「screenが無ければ選択なし」にする。起動順、menu/tabs/shellの所有、product、依存、受け入れは不変。広いTerminal改修はしない。Linuxの修正前139→修正後Home起動・10秒生存・echo入力、zedBSDのTerminal起動/文字/終了と必須回帰で検証。ユーザーのWS105完了まで自走指示の委任を適用し、move/resizeのbug移管判断とは分ける。
