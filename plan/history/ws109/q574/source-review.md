# q574 final changed source / full-standard review

- `GNUmakefile` SHA256 `92eb2af88051a63e954a0722db3afcc41f7161186e4489f482da88b2ca792a36`
- `makefile` SHA256 `ed67593b397b06da5130946cb40132b1e3f21932149d52379e3018c0d675752a`
- `Makefile` SHA256 `51155ec70127cfc82d078999a1ab3e509f58993b2821f711960c1ca5ab08d13c`
- `userland/desktop/keiland-freebsd.mk` SHA256 `46593fc7e8f57495c88d5c2e7fd71f99c886c7e1b5d7caca56b116560a30c0e2`
- `userland/desktop/README.freebsd.md` SHA256 `322ae31021027372b7cb028684646e2af7406e81e6807a7c7b52e66c344d6aee`

全5fileの最終全文をAGENTS/Guardrail/native full standard/C full standardの適用範囲と照合。変更はmake入口・defaultPython・運用文書のみ、C source差分0。C formatter/linter/3OSのC再buildは該当source変更なしで省略。既存Cのq572全source conformanceを保つ。root GNU default disk-image/project rulesはincludeで維持、FreeBSD-only goalsはcross rules parseを回避。BSD overrideをquote、BSD MAKEFLAGSをGNUへ混入せず-j/CC/DESTDIRを転送。外部packageのpin/license/取得規則変更なし、toolchain/HAL/renderer/source membership変更なし。新interpreter commandはnative Python packageの標準aliasを使用しMeson module実行成功。git diff --check PASS。

Commands: GNU make no-config help / FreeBSD-only native entry dry-run / actual GNU native no-config incremental build / actual BSD make -j8 native full build / sudo make native install / native header-dependencies / native-build-audit / all 14 installed executable byte equality + ldd / pre-seat argument-rejection executable smoke。実機user画面は未実施、GDMは撤回。root defaultの意味を変えない入口変更なのでzedBSD image再build/bootは省略（native entryの該当build/read-backのみ）。
