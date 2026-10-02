# WS110 起動modeの検討（実装未承認）

## ユーザー指示

2026-10-02、このchat「検討だけまずはお願いしたいこと」「--sessionがないと開発モードになるのを廃止。--testingをつけると開発モードにするように変更。関連するテストスクリプトも修正する。--sessionはもともとどういう意味？教えて」。今回は調査と計画のみ。source/test script/サービス/installed binaryは変更しない。

## 現在の意味と根拠

- `userland/desktop/wayland/main.c`: 初期session=0、timeout_ms=150000。--sessionはsession=1/timeout_ms=UINT64_MAX、指定がなければlock idle既定10分を設定。--greeterは別roleでauth-fdが必須。
- `home.c`: sessionならLog Outを追加、control-fdがあるsessionだけLock Screenを追加。`greeter.c`はcontrol-fd無しのlockを拒否。
- `desktop.c`: sessionならFilesのdesktop clientを既定で起動、明示の--desktop-client=noneや既存設定offは維持。
- `sessiond/session.sh`: 認証済みユーザーのHOME/runtime等を受けて--sessionで起動、sessiondは--control-fd=3を渡す。ログインscreenの--greeter/--auth-fdとは役割が異なる。
- FreeBSD nativeはsessiondを含めず、通常consolelogin後の直接session。--session自体が認証するわけではない。今回の提案はnative login/lock backendの移植を含めない。

## 推奨案（実装時に仕様を確定）

| Invocation | 推奨する意味 |
| --- | --- |
| wayland | ユーザーデスクトップ、期限なし、Log Out/既定desktop client有効 |
| wayland --testing | 明示的な試験用、既存150秒既定と通常session機能を持たない現在の有限動作 |
| wayland --testing --timeout=N --max-frames=M | 時間/完了frameの試験上限、従来の値範囲/cleanup/外部watchdogを維持 |
| wayland --session | 既定の通常sessionと同じ互換alias。現行sessiond/GDM/手順をすぐ壊さない |
| wayland --greeter --auth-fd=N | 既存ログイン画面、認証channelとroleを維持 |

--sessionの推奨扱いは互換aliasとして保持、通常起動で必要なくなる。--session/--testingの同時指定はエラーを推奨。--testingと--greeter/--control-fdの組合わせは既存auth testの実起動調査後に決め、greeter/auth試験の役割を壊さない。--timeout/--max-framesだけでtestingへ暗黙移行させず、--testingを要求する案を推奨。診断用--log-frames、--width/--height、--socket等は通常でも利用可とする。

parseの途中でmodeがtimeoutを上書きする現行形式を避け、引数を読んだ後にrole/期限を一度確定。--testing --timeout=N と逆順を同じ動作にする。既定を無期限にする際、全compositor test launchの終了/cleanupを確認し、有限試験に--testingを追加する。通常session/認証の統合試験は通常roleを試験し外部watchdogで上限を保つ。クライアントの--timeout-s、guest helperの--timeout、Vulkanprobeの--timeoutを誤って置換しない。

## 影響範囲・調査の限界

現役tracked script/config/sourceを限定した一行ヒューリスティックの候補179file。[候補一覧](launch-candidates.txt)。これは修正確定数ではない（usage/source・productionlauncherも含む）。複数行argv、変数/共通helperで構築するlaunch、引数なしlaunchは実装前に追加追跡が必要。history/過去承認snapshotは変更対象外。

- common: `wayland/main.c`、`zwl.h`のmode説明、home/desktop/greeterとの動作整合。
- shared tests: `plan/tools/{files,titlebar,imageview,x11,gpu-boundary,keiland-freebsd}/`等。
- WS固有tests: 現存`plan/ws*/tests/`のcompositor launchと共通wrapper。各WS acceptance/roleは変えない。
- distribution CI: `tools/release/keiland-linux-deb/run.py`のsession smokeは通常sessionを維持し既定動作も確認する。package起動説明/README、Linux.desktop/zedBSD session.shを互換性として点検。
- 3OS同じmain.cを使うためzedBSD/Linux/FreeBSD共通の変更。今回合格済み実機はまだ変更しない。

## 計画する検証

引数なし/互換--sessionが150秒で終わらないこと、--testing既定と指定timeout/frameで有限終了、option順序独立、矛盾/不正値拒否、Log Out/desktop client、auth channelを持つzedBSDのgreeter→session/lock/logoutを対象とする。auth channelの無いnativeへ認証成功を捏造しない。共通cleanup/VTlease回帰、3OS build warning0、最後のzedBSD boot-test.sh PNG、全文規約レビュー。試験はapproved finite Queue後のみ。
