# WS114–117: GTK4/Qt6 upstream移植から独自実装への学習順（2026-10-02）

User指示: Linux Keiland上の標準ビルドGTK4を先に実測し、XDG-shell/portal等の機能表をユーザーが行別にレビューして採用範囲を決める。その範囲でLinux compositorを改善してから、zedBSDへupstream GTK4、次にQt6を移植して学ぶ。後で既存の完全書き下ろしWS097/WS096を行う。GNOMEの全機能を要求しない。既存の書き下ろし計画は取り消さない。

**順序の更新（2026-10-02 user）**: 「GTK4は、Linuxでの本物のGTK4によるCSDの動作を完成させましょう。そのあと、本物のGTK4のzedBSDへの移植に進む前に、Linuxで本物のQt6の動作を調査しましょう。ここで、素のQt6アプリを動かせるように、コンポジタの改良を行います。そのあとで、素のGTK4と素のQt6を移植します。」→ 順序は WS114 p007（Linux 本物 GTK4 の CSD 完成）→ [WS117](../ws117/ws.md)（Linux 本物 Qt6 の調査と素の Qt6 アプリのための compositor 改良）→ WS115（zedBSD へ GTK4 移植）→ WS116（zedBSD へ Qt6 移植）→ 後の WS097/WS096。上の「GTK4移植後にQt6」と下の「Qt6の範囲はGTK4の移植後に決める」はこの順序で置き換え、Qt6 の範囲は WS117 の機能表で決める。

**順序の再更新（2026-10-02 user（作業開始の指示））**: 「まずは素のGTK4を移植してください。移植できないところがないか、ノウハウを蓄積します。そのあとで独自実装を作ります。Qt6はこれらの作業のあとにします。」→ WS114 p007 → WS115（素の GTK4 の zedBSD 移植）→ WS097（独自実装の互換 GTK4）→ WS117（Linux 本物 Qt6 調査と compositor 改良）→ WS116（Qt6 移植）→ WS096。上の順序の更新を置き換える。

- [WS114機能表](../ws114/gtk4-compat-matrix.md)はソース実装とguest実測を分け、未検証を合格としない。portalはsession D-Busのfrontend/backend統合として扱い、XDG-shellとは別に判断する。FileChooserの通常dialog経路があるため、portal全種類を一律必須にはしない。
- GTK4/Qt6の外部sourceは[Guardrail](../guardrail.md)の `userland/packages/` 境界に従い、公式tarballの版/hash、個別patch、license/provenanceを保存。baseや独自実装へ外部sourceを転用しない。Linux distro packageの実証をzedBSD targetの実証に代えない。
- WS034 p029/p030の旧実装枠はWS115/WS116へ移管する。p028/p034/p038の独立目的は保ち、必要な成果を依存として参照する。WS096/WS097のAPI interfaceだけ参照するzlib書き下ろし方針を保持する。
- codeを作るPhaseはQueue選定/承認後に開始。全WSに最終sourceの全文規約/formatter/static検査/build/runtime Phaseを持つ。Qt6のmodule/代表操作/portal範囲はGTK4の移植後にユーザーが決める。

This is a scoped design/ownership rule, not a C coding-style exception. [Full C style](../coding-style.md) and checked-in formatting remain authoritative.
