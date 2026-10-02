<!-- awesome-plan project=zedbsd record=ws114-p007 -->

# ws114-p007: GTK4 CSDとKeiland SSDの選択を実装・検証する

Parent: [WS114](../ws.md)
Status: planned
Disposition: normal
Primary Milestone: MG006（WSから継承）
Queue / attempts: q587-i01
Purpose / goal: SSDを明示要求しないclientとCSD要求clientをKeilandが装飾しないモードにし、標準GTK4で検証する。
Investigation bound: 最大3時間 / 1 Phase。未達基準と再開条件を保存し、無制限再試験しない。

## Authority / prerequisites

2026-10-02 current user:「B1に伝えてください。GTK4のCSDに対応して、SSDが要求されないかCSDが要求された場合は、Keilandがデコレーションを行わないモードを実装し、それを使ってGTK4が動作するかを確認するPhaseを追加で実行してもらってください。」

G05の個別採用指示としてp002の残る行別判断より先に実行する。q587はAgent Aが予約。p001/q581の[調査結果](../phase001/q581-result.md)と停止済み専用guest資産を確認済み。p002全体のclearanceは前提にしない。

## Scope / design / affected components

xdg-decoration v1のpreferred/configured/committed modeを分け、configure → ack → surface commitに合わせて適用する。objectなし/初期unset/CSD要求はclient_side、明示SSD要求はserver_side。unsetとdestroyの次commit、重複object/不正modeをprotocol契約に従い扱う。native keiland_titlebar_v1は明示的Keiland SSD要求として既存アプリを維持し、明示xdg CSDを優先する。

対象はuserland/desktop/waylandのdecoration.c、zwl.h、extras.h、protocol.c、objects.c、shell.cと、必要なtitlebar.c/titlebar-shell.c/menu-shell.cの装飾状態・描画・hit/resize判定・shadow/rounding/geometry・teardown。描画だけを消して不可視titlebarが入力を奪う状態を残さない。WS114 tests/evidenceを追加できる。共通OS境界とlibvulkan経由のGPU契約を維持する。

portal、未採用G行、HAL API、toolchain、browser、Settingsの新機能はscope外。既存の有効な境界APIを使う。

## Clearance / verification

1. wire試験: objectなし、初期unset、CSD、SSD、unset、destroy-next-commit、mode変更のack前後、duplicate/invalid、native opt-inとCSD優先を確認。
2. 現在sourceから専用Linux compositorをbuild/linkし、runtime SHA/ELFとguest導入物を記録。Debian13標準GTK4でKeiland SSDなしの画面、click/key/menu/tooltip/modal、move/resize、maximize/fullscreen restore、別client clipboardをPNGとprotocol証拠で確認する。GL/Cairo/Vulkan各1回、実rendererと利用可能性を区別し、物理GPU/dmabuf等の未確認を明記する。
3. native Terminalのtabs/menu、Files/Textedit controls、maximize/dock/closeでSSDの回帰を確認。
4. 変更Cの全文規約/manual review、必要範囲format、style/boundary検査、専用zedBSD対象build（warning 0）と最終boot-testを行いPNGを確認。全体make checkは禁止。
5. owned guest/rendererを停止し、commands/versions/commits/results/skip/limitationsと成果を保存。失敗は具体的な残件と再開条件を残しuncleared。

## Standards / resource ownership

[Guardrail](../../guardrail.md)、[C全文規約](../../coding-style.md)、[automation](../../standards/automation.md)、[GTK/Qt方針](../../standards/ws114-gtk-qt-learning.md)を変更前に実内容で読む。追加・変更されたcodeは全文規約適用。全WS最終conformanceはp006に保持する。

B1専用worktree/build/overlay/SSHを使い、共有toolchain/sysrootは読取専用。QEMUはmainがB2終了後に割当。zedBSD bootはplan/tools/boot-test.shのみ、serial/console logをacceptance根拠にしない。Linux guestはloopback SSHとQMP PNGを使う。

## Evidence / event / resume

実装・試験未実施。q581は旧baseline測定であり本Phaseの成功証拠に流用しない。GitHub publicationはAgent Aによる投影待ち。

2026-10-02 / ws114-csd-user-selection-20261002: userがG05の具体実装とGTK4確認を追加指示。p007へ分離し、p002/p003の重複scopeを除き、p005にp007成果を追加。WS acceptanceは他行判断・引継ぎ・最終conformanceを含め未達のまま。[WS summary](../ws.md)。

2026-10-02 / q587-scope-amendment01: Current CSD runtimeでGTK4 move release欠落を再現。GTK4実操作の元基準を満たす最小[追加scope](q587-scope-amendment-01.md)をB mainが具体化し、seat/toplevelと元client operation state/lifecycleを所有へ追加。元snapshotと3h期限を保持、基準を緩和しない。
