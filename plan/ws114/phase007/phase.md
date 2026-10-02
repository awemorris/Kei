<!-- awesome-plan project=zedbsd record=ws114-p007 -->

# ws114-p007: GTK4 CSDとKeiland SSDの選択を実装・検証する

Parent: [WS114](../ws.md)
Status: in-progress（q592-i01、P3。zedBSD の boot PNG だけ未達、修正版 boot-test の再試行待ち）
Disposition: normal
Primary Milestone: MG006（WSから継承）
Queue / attempts: q587-i01（uncleared）、q592-i01（P3、in-progress）
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

q587-i01はユーザーの全agent終了指示により安全な区切りで終端し、未達基準を残してuncleared。CSD/明示SSDとclient move/resize releaseを実装し、wire・GTK4の3 renderer・native Terminal/Filesを部分検証した。最終sourceのguest導入、正しい空targetへの別client clipboard、Textedit全controls、最終zedBSD install/boot PNGは未達。[結果](q587-result.md) / [再開条件と資産](q587-resume.md)。q581は旧baseline測定であり本Phaseの成功証拠に流用しない。Queue/共有Board/remote publicationはmainへ引継ぐ。

2026-10-02 / ws114-csd-user-selection-20261002: userがG05の具体実装とGTK4確認を追加指示。p007へ分離し、p002/p003の重複scopeを除き、p005にp007成果を追加。WS acceptanceは他行判断・引継ぎ・最終conformanceを含め未達のまま。[WS summary](../ws.md)。

2026-10-02 / A-q587-start-projection: current userがBでのp007開始を共有。df66db5e内のexact scope/承認原文/snapshotを保持し、A canonicalでin-progressへ投影。実装・runtime結果のclearanceはBの後続checkpointで別に確認する。

2026-10-02 07:41 UTC / q587 B1 start: approved snapshot verified and B main322d127a fast-forwarded. Implementation/build starts; QEMU grant pending.

2026-10-02 / q587-scope-amendment01: Current CSD runtimeでGTK4 move release欠落を再現。GTK4実操作の元基準を満たす最小[追加scope](q587-scope-amendment-01.md)をB mainが具体化し、seat/toplevelと元client operation state/lifecycleを所有へ追加。元snapshotと3h期限を保持、基準を緩和しない。

2026-10-02 / q587 checkpoint02: amendment01のclient move/resize release修正と有限Linux/native検証を提出。[checkpoint](q587-checkpoint-02.md)。未達確認を保持し、Phaseはin-progress。

2026-10-02 08:50:10 UTC / q587-user-wrap-uncleared: user「すべてのエージェントを終了に向かわせます」「きりのいいところで作業をきりあげてもらいます」に従い、専用compositor error=0/cleanup_failed=0、QEMU PID327367消滅、SSH2249 listenerなしを保存。3時間deadlineより前の安全停止であり、残基準を免除しない。Phase disposition normal、WS114 incomplete、p006最終conformanceを保持。MR01/MR02の実装と採取済み証拠を残し、追加guest/反復を開始せず結果・再開資料を最終MRへ提出する。[停止](../evidence/q587/stop-proof.json) / [WS event](../ws.md)。

## 次の attempt（2026-10-02 計画担当、Queue 承認ではない）

Scope は新しく広げず、[q587-result](q587-result.md) の未達 1〜5 だけに限る。製品 source の追加変更は、未達の確認で欠陥が出た場合の最小修正だけ。

1. main の現行 source（19452fe8 以降の main の `userland/desktop/wayland/`）から Linux compositor を build し、guest へ導入して SHA/ELF を照合。限定 wire・GTK4 GL/Cairo/Vulkan の表示と入力 smoke。
2. 別 client の Unicode paste: receiver の空 entry の座標を PNG で確かめてから paste し、完全一致を確認。
3. native Textedit の edit/save/controls/max/dock/restore/close と、native SSD 操作で client への phantom release が無いこと（`plan/ws114/tests/interactive-wire.py` を使う）。
4. 最終 source の zedBSD target build（warning 0）、専用 image への install、`plan/tools/boot-test.sh` の PNG を確認しユーザーに見せる。
5. 変更 C の全文規約の再照合（q587 の review 以降の差分だけ）、OS/GPU 境界 checker、guest/QEMU の正常停止。

資源: q587 の停止済み overlay（`/home/awe/zedBSD-worktrees/b1/build/b1-q587/guest/overlay.qcow2`）は読取 input として新 Queue 所有の overlay へ clone する（[再開資料](q587-resume.md) の 2・3）。P 担当の worktree で行い、b1 の worktree と build は変更しない。QEMU と SSH port は main が割り当てる。目安 3h（timebox 3h、超えたら残件を保存して uncleared）。

所有 path: `userland/desktop/wayland/{decoration.c,protocol.c,shell.c,titlebar.c,seat.c,toplevel.c,objects.c,zwl.h,extras.h}`（修正が要る場合だけ）、`plan/ws114/`。衝突: WS095 p005（compose.c・protocol.c・display.c・shell.c・input-method.c）、WS099/WS094/WS113 の compositor の Queue と同時に shell.c/protocol.c を変える場合は main が merge 順を決める。

未決の判断: なし（G05 はユーザー採用済み）。cleared になれば WS117 p001 を開始できる。

2026-10-02 / ws114-beta1-plan-20261002: 次 attempt の範囲・資源・目安を記録。Status は uncleared のまま（新 attempt は main の Queue 承認後）。

2026-10-02 / q592 P3 checkpoint: 新 attempt q592-i01（承認: ユーザー「作業を開始しましょう。」、Q1 が投入）を base 901037f9f で実行。製品 source の変更なし。未達 1〜3 と 5 は達成: 最終 source の Linux 実物を guest に導入して SHA を照合、wire の 6 PASS、GTK4 の GL/Cairo/Vulkan の smoke、空 entry への別 client の Unicode paste の完全一致、Textedit の全 control、SSD の phantom release 0、Terminal/Files の回帰なし、style-check 0、OS/GPU 境界 PASS、guest の正常停止。未達 4 は target build の warning 0 と image まで済み、`boot-test.sh` は QMP の 30 秒 timeout で 3 回 FAIL（host I/O による QEMU main loop の停止。P1 の q594 で修正済みの boot-test で再試行待ち）。[q592 結果](q592-result.md)。
