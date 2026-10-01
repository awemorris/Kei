<!-- awesome-plan project=zedbsd record=q537 -->

# Queue

Status: finished
Active Queue: なし
Last finished Queue: q537

## q537

- Purpose: libkeiland の Linux の backend（wpa_supplicant・Linux の interface・ALSA）
- Focus: fg012 / WS105 の既存 Linux の受け入れを満たす。
- Timebox: 60 分。既知の build・必須検証は最大 150 分、未知の問題の調査は 30 分で結果と再開条件を記録する。
- Approval: current user, 2026-10-01「ws105の完了をゴールにして、自走をお願いします。」。既存 WS105 p001〜p011 の範囲を依存順の 1 Phase Queue で実装・検証・記録する。commit は WIP、push / GitHub 公開は行わない。
- Exact approved scope: [ws105-p010](ws105/q537/phase.md) 全範囲、開始前 [snapshot](ws105/q537/scope.md) SHA256 `2a155662c91c3691c3a57512f3b4f4020bb7297c25b84397925d87302ebdf9f1`。
- Executor: Codex Q1。前 Queue finished、並行 executor なしを確認。
- Applicable rules: AGENTS.md、Guardrail、coding-style.md 全文、WS105 D1〜D25、design.md の指定節。
- Prerequisites: Phase の依存と既存出力を実装・証拠で確認。material scope / criteria の変更・取消なし。
- Started UTC: 2026-10-01T11:23:22.777729+00:00

| Attempt | Phase | Status | Dependency | Selection reason |
| --- | --- | --- | --- | --- |
| q537-i01 | [ws105-p010](ws105/q537/phase.md) | cleared | p008（Settings が Linux で動く）（context） | WS105 の既存依存順 |

Dependency graph: Phase の依存（context）→ q537-i01/ws105-p010。他の Phase は次の Queue で選定する。

## Upcoming Work Outlook

WS105 の依存を満たす次の Phase。新しい product / risk 判断はユーザーの決定が前提。既存 WS105 の完了までの自走指示で選定・実行する。
GitHub への publication は deferred、outbox に保持する。

## Outcome

ws105-p010 **cleared**。

cleared（q537-i01）。source b55b6bf0 + Unix cleanup91343cf2 WIP。p002 placeholdersをown wpa control client / Linux interface backend / ALSA controlへ置換、公開keiland.h / exports / 共通source / appは不変。private network-wpa.hをOS内共有、外部source複写なし。commandとATTACH event socketを別に所有し、request/updateはnonblockingのreply状態遷移、各deadline2秒 / service retry1秒。STATUS・scan・profile table検証、SSIDescape decode / strongest24 / exactly-one request completion、JOINはLIST→ENABLE→SELECT、saveはhexSSID / escapedkey / SAVE_CONFIG、ENABLEせずJOIN後SAVEせず。Linux radio up/downはkernel権限の実errorを返す。

gcc14.2 / clang19.1.7 warning0、28ELF（24本体+chain/display/network/audio fixtures4）、makefile-sync、331source header-check、全新C + probeのstyle-check0 / formatter19 / whitespace PASS。ALSA card/list/info/read/write/subscribeを直接使用（alsa-libなし）、Master→PCM→Speaker、min/max/channel、readback、eventのbounded drain、device再接続。feedbackはconnectedで0 / 無音。

Linux QEMU Debian13 / real mac80211_hwsim2radio + hostapd + wpa_supplicant、HDA Master raw0〜74、一般userkei audio/netdev権限。network-probe PASS（実secured AP scan、save直後に未接続、PROFILES→JOIN keiland-test / WIFI CONNECTED+kindWIFI+IPv4、disconnect、wlan0とenp0s5 / MAC / MTU / traffic、savedprofile、DNS10.0.2.3）。audio-probe PASS（available1/reachable1/device1、40%readback、amixer41%=許容±3、別process70%→fd readable / CHANGED_VOLUME /70、mute off/on、feedback0）。解析用cache値の偽装なし。fixture固定192.0.2.2はguest setupだけ、DHCPはsystemに任せる。fake wpa fallback不要。

Settings QMP操作: scanにsecured keiland-test、key欄へkeiland-pass→Join→wpa_stateCOMPLETED / Settings Connected / systembarWiFi icon。Sound slider→amixer74%、systembar slider→amixer23%（Settings rawfloor22%）、PNG目視と当チャット表示。最初の接続3秒PNGはSCAN中の一時状態、7秒後stablePNGとwpaCOMPLETEDを最終証拠とする。最後にSIGTERM frames124/error0/cleanup_failed0。再起動時openvt7がbusyで拒否された元setup logを保存し、未使用VT8で再実行（source障害ではない）。

終了時一時directory4個残留を発見（common compositorはwatch close呼出なし）。Linux/WPA内single event-loop watch registryとlibrary/process destructorで明示closeされないwatchを同じcloseへ渡す補完。新overlayでprobe2本再PASS、Settings正常closeとcompositor SIGTERM、frames58/error0/cleanup_failed0、owned Unix path0 / console復元を再確認。SIGKILL等destructorを実行しない異常終了と多thread共有は未検証・追加対応なし。

zedBSD: Linuxだけの変更なのでPhase指定どおりdisk-image build warning0 PASS、common libkeiland不変。共通回帰一式の再実行は本Phase不要、p011 finalsource全体回帰で実施する。host追加package0、host /opt未install、host画面/input未使用、toolchain変更なし。Linux guest停止 / overlay破棄。実機・realWiFi device未実施、QEMU証拠。GitHub未公開、event/intendedcloseはoutbox pending、pushなし。次は既存p011全文規約・境界L1〜L5・両OS最終回帰・install文書・WS全体acceptance。

証拠 [q537 manifest](../../history/ws105/q537/evidence/SHA256SUMS)。


実装 commit `91343cf27a4f6d8a5a34c238657227b30aafa03d`。Finished UTC: 2026-10-01T11:56:09.219178+00:00。次の Phase は WS105 の既存完了までの自走指示で選定。push / GitHub 公開は未実施。
