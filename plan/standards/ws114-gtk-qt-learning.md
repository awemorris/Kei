# WS114–116: GTK4/Qt6 upstream移植から独自実装への学習順（2026-10-02）

User指示: Linux Keiland上の標準ビルドGTK4を先に実測し、XDG-shell/portal等の機能表をユーザーが行別にレビューして採用範囲を決める。その範囲でLinux compositorを改善してから、zedBSDへupstream GTK4、次にQt6を移植して学ぶ。後で既存の完全書き下ろしWS097/WS096を行う。GNOMEの全機能を要求しない。既存の書き下ろし計画は取り消さない。

- [WS114機能表](../ws114/gtk4-compat-matrix.md)はソース実装とguest実測を分け、未検証を合格としない。portalはsession D-Busのfrontend/backend統合として扱い、XDG-shellとは別に判断する。FileChooserの通常dialog経路があるため、portal全種類を一律必須にはしない。
- GTK4/Qt6の外部sourceは[Guardrail](../guardrail.md)の `userland/packages/` 境界に従い、公式tarballの版/hash、個別patch、license/provenanceを保存。baseや独自実装へ外部sourceを転用しない。Linux distro packageの実証をzedBSD targetの実証に代えない。
- WS034 p029/p030の旧実装枠はWS115/WS116へ移管する。p028/p034/p038の独立目的は保ち、必要な成果を依存として参照する。WS096/WS097のAPI interfaceだけ参照するzlib書き下ろし方針を保持する。
- codeを作るPhaseはQueue選定/承認後に開始。全WSに最終sourceの全文規約/formatter/static検査/build/runtime Phaseを持つ。Qt6のmodule/代表操作/portal範囲はGTK4の移植後にユーザーが決める。

This is a scoped design/ownership rule, not a C coding-style exception. [Full C style](../coding-style.md) and checked-in formatting remain authoritative.
