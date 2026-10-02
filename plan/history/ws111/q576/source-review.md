# WS111 final all changed source conformance

- `userland/desktop/wayland/keiland-desktop.in` SHA256 `df2bedceb9976777ce2f9766101527a0087b1b699b4953d7bcd3586cccd59a60`
- `userland/desktop/keiland-linux.mk` SHA256 `ed4dd375f974c4dc2a427e7a4738829a1b8ebc900929aa9f52e0121533ac8570`
- `userland/desktop/keiland-freebsd.mk` SHA256 `f7dde07f004b2f023350040a886da5da1f5116eb83436df23d8e7304f6d19114`
- `userland/desktop/README.freebsd.md` SHA256 `b5a85dac1441c6289701aa45213436fd50af21ccca9185c89926ef107ee6d6da`
- `tools/release/keiland-linux-deb/build.py` SHA256 `744fe31d7c4c7019298b54d8984f90d39d372696236a77f4a82646b59e118ec4`
- `tools/release/keiland-linux-deb/README.md` SHA256 `56ea0c6e8894c62807767415e05b6f53e03f4008970fd795d873eaa3f48a65bf`
- `plan/tools/keiland-launcher/check.py` SHA256 `0fe55b528fe6044479f732d5ea071d996b404802bce4454ce950c336a598f5f8`
- `plan/tools/keiland-launcher/README.md` SHA256 `d0d50a3b98839fcaf756406a43c1d8ff13995a6e94e35e248aaacb70692bd98b`

全変更sourceをAGENTS/Guardrail/full coding-styleの適用範囲と実codeで全文レビュー。C/header/source membership/renderer/権限/--testing/main semantics変更0なのでC clang-format/stylechecker/3OS C回帰は省略。POSIXsh、quoted argv/prefix/runtime、前段path rejection、private0700、既存XDG/seat/env不変、execによるsignal/exit、native mk生成/install0755、prefix/DESTDIR/packagingメンバー、Linux.desktop直接wayland維持を確認。テンプレートdefaultprefix以外に空白入りcustomprefixも実shで検証。パスワードチェックや--login/PINは未実装の設計として区別。常時running user GUIには干渉しない。

Tools/commands: /bin/sh -n template・FreeBSD installedscript、Python3 portable real-shell fixture Linux/FreeBSD（SIGTERM fixtureのready前にhandler登録）、py_compile packagebuild/probe、Linux native all/install/install-session stage warning0・ELF24PASS、GDM.desktop SHA25650e478a98e32bfef888fa928ed4ffa2c19e31ddb4b0b3f439a83f1cbfc222bb8原本/stage一致、FreeBSD actual make all/sudo install/cmp/stat rootwheel0755・installedscript--help expectedexit2前seat実exec成功、git diff --check PASS。shellcheckはhost未導入で省略、native両sh実行とmanualreviewを使用。zedBSD image/bootは新native launcherだけでzedBSD構成/C不変のため省略。Linux GDMの実ログインやfreshdpkg/physicaldisplay起動は未実施、既存GDMsource byte不変とscript payload installを確認。新OSlibs/dependency追加無し。
