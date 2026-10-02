# ws094-p007 / q588-i01 partial source conformance

Approval: current user / 2026-10-02「サブエージェントは終了せずに、次々とqueueを送り込んで、長時間稼働させてください。」を既存B2 desktop担当の次の有限規約レビューへ適用。Aがq588をB2用に予約。
Timebox: 最大3時間 / p007のsource review・host/build部分。
Prerequisites: p011/q582 cleared、B main統合cea10fd7とlane ACK3bed3013。全Phaseの実機p012/最終guest依存は保持し、この部分source作業だけ先行する。

## Exact scope / ownership

全WS094 sourceのinventoryをPhase記録と実commitから作る。旧include/libc/keiland.hは現行userland/desktop/keiland/keiland.hへ対応し、WS094差分・後続変更・所有・現行SHAを分類する。inventory全体を全文C/Guardrailでreviewする。

編集はuserland/desktop/filesのdesktop-layout.c、ui-desktop.c、ui-desktop-drag.c、ui-desktop-actions.c、main.c/present.c/window.c/files.h/window.hのWS094関連部分、userland/desktop/picture/picture.c/h、userland/desktop/libkeiland/desktop.c、plan/ws094/tests/*.cのin-scope規約違反修正。既存挙動を維持し新機能/APIを追加しない。修正する機械findingが実違反かを全文/由来で判定する。

公開keiland.h、exports/Makefile、imageview/image.c、build/platform、共有tools、waylandのp002/p005差分は読取reviewとfindingsだけ。B1のwayland編集へ手を入れない。編集禁止部分の未解消を所有者/次の対応へ渡す。

## Verification / partial criteria

全WSsource inventory、変更由来、所有範囲、全文manual規約判定、既知例外/実違反/false-positive/未解消の表を耐久記録で完成する。選定された所有内の実違反を挙動不変で修正し、source style/diff、host desktop/thumb/model/imageview4本、専用files/wayland/imageview/libkeiland target build warning0、OS境界検査をPASSさせる。本文規約を機械checkのPASSだけで代替しない。既存main.c2件/picture setjmp1件等はp007の根拠を照合し、変更codeの例外へ自動拡張しない。

対象を必要範囲formatし、commands/versions/current commits/reviewed ranges/results/例外/未実施/限界を保存。完了可能ならpartial Queue item cleared、未解消の所有内実違反/対象gate失敗/時間切れならunclearedで再開条件を残す。

## Whole Phase / resources / resume

whole p007はこのpartial itemではclearしない。p012実機gate、全guest/probe2回/p010/files14本/C9/最終全体bootの受け入れを保持し、WS094はincomplete。後にcodeが変われば当該最終source review/buildを再validateする。

AGENTS/Guardrail/C全文/automation/境界を既読内容と変更差分から適用。共有toolchain/sysrootは読取専用、q582の私有sysroot/BUILDとdry-runを使い再生成・LLVM/Noct rebuildを避ける。fresh image生成、QEMU/実機はこの部分scopeに含めない。後続guest QueueにはB1 source統合後の現在runtimeとmain資源grantが必要。

WIP/no push。部分MRを順次mainへ送り、終了せず同じagentで次のdispatchを待つ。
