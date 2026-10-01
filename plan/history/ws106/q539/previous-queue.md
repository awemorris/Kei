<!-- awesome-plan project=zedbsd record=queue -->

# Queue

Status: finished
Active Queue: なし
Last finished Queue: q538

## q538

- Purpose: 規約の全文の見直し、境界の確かめの拡張、回帰、install の文書
- Focus: fg012 / WS105 の既存 Linux の受け入れを満たす。
- Timebox: 60 分。既知の build・必須検証は最大 150 分、未知の問題の調査は 30 分で結果と再開条件を記録する。
- Approval: current user, 2026-10-01「ws105の完了をゴールにして、自走をお願いします。」。既存 WS105 p001〜p011 の範囲を依存順の 1 Phase Queue で実装・検証・記録する。commit は WIP、push / GitHub 公開は行わない。
- Exact approved scope: [ws105-p011](history/ws105/q538/phase.md) 全範囲、開始前 [snapshot](history/ws105/q538/scope.md) SHA256 `cc64af7e3f6a6f579e373225a171ba23b6020be49b8b3484c00d73ec1f642c35`。
- Executor: Codex Q1。前 Queue finished、並行 executor なしを確認。
- Applicable rules: AGENTS.md、Guardrail、coding-style.md 全文、WS105 D1〜D25、design.md の指定節。
- Prerequisites: Phase の依存と既存出力を実装・証拠で確認。material scope / criteria の変更・取消なし。
- Started UTC: 2026-10-01T11:56:09.585238+00:00

| Attempt | Phase | Status | Dependency | Selection reason |
| --- | --- | --- | --- | --- |
| q538-i01 | [ws105-p011](history/ws105/q538/phase.md) | cleared | p001〜p010（context） | WS105 の既存依存順 |

Dependency graph: Phase の依存（context）→ q538-i01/ws105-p011。必要な前提出力は全て verified、未選定のPhaseなし。

## Upcoming Work Outlook

WS105の既存Phaseは全てcleared、L1〜L9を照合してWS completed。Linux残りの実装なし。次の候補はMasterの既存fg010/デモ優先順、BUG-125/127のbounded window調査、F-065のFreeBSD/design§8（指定時）。予測のみ、次Queueは未選定・未承認で開始しない。GitHub publication/Issue close/Projectはdeferred、outboxに保持。

### 2026-10-01 レビュー後の Outlook 追記

予測のみ。新しい Queue は無し、q538 の承認/結果は保持。

| 候補 | 理由 | readiness / 依存 |
| --- | --- | --- |
| [WS106 p001](ws106/phase001/phase.md) | test/probe/demo の配置を整理 | 対象30件、ime-probe所有/参照/契約の確定が必要 |
| [WS107 p001](ws107/phase001/phase.md) | engine の source 所有と component 品質 | 新境界全文あり、配置/API/lifetime点検から |
| [WS108 p001](ws108/phase001/phase.md) | Debian13/Ubuntu26.04 deb/CI | WS105 completed出力を利用、manifest/環境設計から |
| [WS109 p001](ws109/phase001/phase.md) | Linux版の native FreeBSD15 移植 | F-065 FreeBSDをpromote、graphics/seat/ABI/license/環境調査から |

相対順位は未指定。既存デモの priority を保持。F-065 の FreeBSD は新 WS へ、その他 design§8 の未指定分は deferred。
Source/decision: [レビュー記録](reviews/2026-10-01-review.md)。GitHub/Project の反映は outbox pending。

## Outcome

ws105-p011 **cleared**。

cleared（q538-i01）。WS105の全変更をcoding-style.md全文・Guardrailで見直し、公開コメント・ANSI宣言/整数の形式・意味のあるcall結果/Boolean/void returnを補完。clean buildでlibrary/program同名waylandのobject変数の衝突を再現し、内部変数を分離した。final source `c7e8a35a`、全source manifest / full-review / AST32unit0 / style-check0 / declared-interpreter syntax / diff-check PASS。`LINUX.md`にbuild/install/gdm/direct/環境・権限・制限を記録、OS境界C1〜C5+L1〜L5 PASS。継続fixture dbus-wire.c/.py・seat-fd.cをplan/toolsへ移し、foreign p005/p009・WS・Masterの参照/eventを更新。

Linux: clean GCC14.2/clang19.1.7 warning0、24production ELF各 / RUNPATH / source-sync /331header PASS。host chain1MiB/262144word・bindings0/NO_DEEPBIND・vulkaninfo・WSI90frame×4・D-Bus5case ordinary+ASan/UBSan PASS。own fresh Debian13 gdm imageでKMS両経路6色×4点/oldSwapchain/caller fd、vkdemo、root session/shm/pointer、Home9apps/Terminal echo/Textedit日本語変換・確定、画像/PDF2page、real hwsim/wpa WiFi scan/save/join/disconnect、Settings/bar ALSA readback PASS。root LogOut/SIGTERM/defaultsocket error0/cleanup_failed0・paths0。Notesは既存の手書きUIで、keyboard入力未実装の既知範囲を保持。

最終整数形式補完後のaffected Linux guest経路とclean/hostを再検証。Vulkan window600frame+5×20frame、18import/700acquirefence、実画素RGB、fd29→29、forge out_of_bounds拒否・compositor継続、SIGTERM frames716/error0/cleanup_failed0 PASS。gdmのchooserでKeilandを選択→手動login userkei PID5733、Wayland/runtime1000、SwitchTo/chvtの両方でsamePID/all5leaseを維持、pause10/resume10、復帰echo、HomeLogOut→greeter PASS。実通知はforce、cooperative ACKはsource確認のみ。own guest停止/overlay破棄。

zedBSD: final共通sourceでdisk-image warning0・boot loginPNG・V1/dedicated18/decoder17 ordinary+sanitize・forge拒否後120frame・fence600/generation1・glass p059・Settings8subtest・host audio14/14・volume p004/p005 PASS。C1/C2/C9の元13項目は11PASS/2FAIL。C2/p076のresize不一致をBUG-125へ追加し、ユーザー「バグリストに記載…先に進みましょう。clear判定に進んでいいです」の具体的許可を適用。BUG-125/BUG-127とも未修正tracking、全13PASS・修理済み・Linuxとの因果は主張しない。その他の条件は確認済み。追加の長いresize調査なし。

元のclean-build障害・harness誤期待（ALSA丸め20、VT log spelling、Texteditのunsaved確認、uppercase IME、already-connected network precondition、Filesを含むGPU aggregate count）を保存し、正しいsource/UI/対象別の結果を添付。旧attemptのFAILは改変しない。base-before hashは未採取、overlay isolation/停止後overlay無しを確認。hardware GPU/実機、cooperative pause/hotplug/systemd restart、musl/ARM/FreeBSD・design§8は未実施/範囲外。全標準に新しい例外なし。

証拠: [q538 manifest](history/ws105/q538/evidence/SHA256SUMS)、[全文規約の照合](history/ws105/q538/conformance.md)、source SHA256、実行script。PNG目視・代表画像を当チャットに提示。console/serial/kernel log判定なし、host package追加0、host /opt変更なし、toolchain変更なし。GitHub未公開・event/intended close/Projectはoutbox保留、pushなし。全Phase clearedだけでWSを自動判定せず、この後L1〜L9を照合してWS完了と記録整理を行う。


実装 commit `c7e8a35a30f138b766385f2b48183540821c325b`。Finished UTC: 2026-10-01T13:23:49.194444+00:00。Phase Queue を完了。WS105 自身の受け入れを照合して完了を記録する。次の Queue は自動で開始しない。push / GitHub 公開は未実施。


WS acceptance reconciliation: L1〜L9/full-standard conformance verified、WS105 completed。[WS105](ws105/ws.md)、[履歴索引](history/ws105/index.md)。BUG-125/127未修正tracking。
