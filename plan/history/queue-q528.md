<!-- awesome-plan project=zedbsd record=q528 -->

# Queue

Status: finished
Active Queue: なし
Last finished Queue: q528

## q528

- Purpose: libvulkan-compat (3): 画面の WSI（KMS、VK_KHR_display、VK_EXT_acquire_drm_display）
- Focus: fg012 / WS105 の既存 Linux の受け入れを満たす。
- Timebox: 60 分。既知の build・必須検証は最大 150 分、未知の問題の調査は 30 分で結果と再開条件を記録する。
- Approval: current user, 2026-10-01「ws105の完了をゴールにして、自走をお願いします。」。既存 WS105 p001〜p011 の範囲を依存順の 1 Phase Queue で実装・検証・記録する。commit は WIP、push / GitHub 公開は行わない。
- Exact approved scope: [ws105-p005](ws105/q528/phase.md) 全範囲、開始前 [snapshot](ws105/q528/scope.md) SHA256 `074a22e365fd5c052ac5e0c66a879112dfcb2601083b4995a384bad3627356df`。
- Executor: Codex Q1。前 Queue finished、並行 executor なしを確認。
- Applicable rules: AGENTS.md、Guardrail、coding-style.md 全文、WS105 D1〜D25、design.md の指定節。
- Prerequisites: Phase の依存と既存出力を実装・証拠で確認。material scope / criteria の変更・取消なし。
- Started UTC: 2026-10-01T07:36:09.599874+00:00

| Attempt | Phase | Status | Dependency | Selection reason |
| --- | --- | --- | --- | --- |
| q528-i01 | [ws105-p005](ws105/q528/phase.md) | cleared | p004、p001（guest）（context） | WS105 の既存依存順 |

Dependency graph: Phase の依存（context）→ q528-i01/ws105-p005。他の Phase は次の Queue で選定する。

## Upcoming Work Outlook

WS105 の依存を満たす次の Phase。新しい product / risk 判断はユーザーの決定が前提。既存 WS105 の完了までの自走指示で選定・実行する。
GitHub への publication は deferred、outbox に保持する。

## Outcome

ws105-p005 **cleared**。

cleared。VK_KHR_display・direct-mode・DRM acquisition の O 10 entry、KMS inquiry/dup master/saved CRTC/double dumb FIFO copy、Linux vkdemo を実装した。

- gcc 14.2 / clang 19.1.7: final build exit0、warning0。elf-check PASS（10 ELF、probe を含む）、makefile-sync PASS、header-check PASS（70 sources）。clang-format19＋定義の改行、scoped style-check（実装・probe）0件。新規 KMS source と変更部分を全文規約で手動点検した。最終全 WS conformance は p011。
- host は DRM=none、display 環境無し。vk-chain-test PASS（API1.0・llvmpipe・1MiB全word一致）、interpose PASS（backend→compat binding0・opt-out）、Wayland FIFO/fallback/resize/MAILBOX 各90 frame、360色/extent/import/private wait PASS。
- guest kernel6.12.107+deb13-amd64、Mesa25.0.7、lavapipe。seat fd duplicate（元fdをclose）とdirectの2経路で Virtual-1 / 1280×800 / 74994mHz。赤・緑・青全6PNG各4点一致、probe exit0/PASS。赤→緑で live oldSwapchain を更新・破棄し、master所有権の継承を確認した。
- vkdemo --time-ms=1000 --hold=10: 320×240、中心 #20c5b0、描画PNGを表示、VKDEMO DONE frames=1。各経路の終了後と最終chvt1はconsole文字のPNGを確認・表示。
- 途中chvt1→5秒→chvt7: direct/root はmasterを失わず、3色を完走してPASS。実際のlogind revoke/OUT_OF_DATE はp009で確認する。ioctlのEACCES/EPERM→OUT_OF_DATEと100ms上限はsourceで確認。
- 完了後 guest stop、overlay廃棄。host package追加0、target toolchain/common zedBSD source変更0。実機GPU・物理monitorのcustom mode・Valgrindは未実施。問合せ/display/mode handleはprocess-lifetime、実機hotplugの動的再列挙は範囲外。
- [ログ・PNG・sha256 manifest](../../history/ws105/q528/evidence/)。不具合残件なし。次はp006（root compositor・wl_shm・入力・VT）、前提WS104とp005を確認。


実装 commit `18a983dd30b2586f56700113f2a82add518652a5`。Finished UTC: 2026-10-01T08:13:06.450181+00:00。次の Phase は WS105 の既存完了までの自走指示で選定。push / GitHub 公開は未実施。
