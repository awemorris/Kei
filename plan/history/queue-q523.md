<!-- awesome-plan project=zedbsd record=q523 -->

# Queue

Status: finished
Active Queue: なし
Last finished Queue: q523

## q523

- Purpose: Linux の試験の guest（QEMU の Debian 13）と操作の道具
- Focus: fg012 / WS105 の既存 Linux の受け入れを満たす。
- Timebox: 60 分。既知の build・必須検証は最大 150 分、未知の問題の調査は 30 分で結果と再開条件を記録する。
- Approval: current user, 2026-10-01「ws105の完了をゴールにして、自走をお願いします。」。既存 WS105 p001〜p011 の範囲を依存順の 1 Phase Queue で実装・検証・記録する。commit は WIP、push / GitHub 公開は行わない。
- Exact approved scope: [ws105-p001](ws105/q523/phase.md) 全範囲、開始前 [snapshot](ws105/q523/scope.md) SHA256 `2af6a84c4f217180d3f1db1e5448cd9ac3be6928569f640587fad5dae6fb65a4`。
- Executor: Codex Q1。前 Queue finished、並行 executor なしを確認。
- Applicable rules: AGENTS.md、Guardrail、coding-style.md 全文、WS105 D1〜D25、design.md の指定節。
- Prerequisites: Phase の依存と既存出力を実装・証拠で確認。material scope / criteria の変更・取消なし。
- Started UTC: 2026-10-01T05:55:57.928854+00:00

| Attempt | Phase | Status | Dependency | Selection reason |
| --- | --- | --- | --- | --- |
| q523-i01 | [ws105-p001](ws105/q523/phase.md) | cleared | なし（context） | WS105 の既存依存順 |

Dependency graph: Phase の依存（context）→ q523-i01/ws105-p001。他の Phase は次の Queue で選定する。

## Upcoming Work Outlook

WS105 の依存を満たす次の Phase。新しい product / risk 判断はユーザーの決定が前提。既存 WS105 の完了までの自走指示で選定・実行する。
GitHub への publication は deferred、outbox に保持する。

## Outcome

ws105-p001 **cleared**。

Debian 13 の base guest と操作の道具を作成。`guest.sh` は小さな sh 入口から Python の controller を呼ぶ（QMP JSON と座標変換を shell escaping 無しで扱う）。依存 package は既存 host にあり、host package 追加なし。

- `timeout 600 sh .../build-guest.sh`: exit 0。mmdebstrap 121.9975 秒、raw ext4 8 GiB。mesa-vulkan-drivers 25.0.7-2+deb13u1 / libvulkan1 1.4.309.0-1 / weston 14.0.2-1 / linux-image-amd64 6.12.107-1。guest kernel 6.12.107+deb13-amd64。
- `timeout 200 .../guest.sh start`: 180 秒以内に `guest: ready`。root と kei の loopback SSH 成功。kei の audio/video/input/kvm/render/netdev、`/run/user/1000` を確認。
- DRM card0 / ALSA controlC0 / event0〜5、Vulkan llvmpipe、mac80211_hwsim wlan0/wlan1、ALSA `'Master'` を確認。
- QMP PNG の login prompt を目視・ユーザーに提示。png-probe の size `1280 800`、2 点の色 `#000000`。key / click / type が QMP error 無し。
- run2 / port2226 の同時起動・SSH・停止 PASS。両 guest 停止後 overlay 無し、base image の size / mtime は一致。build の再実行は既存 image を保持。
- 追加の file 転送 / install 確認: 専用 stage の probe.txt を guest に install し、get 後 cmp 一致。guest 停止済み。host の `/opt` は変更していない。
- sh syntax、Python compile、`git diff --check` PASS。Master Tools 登録済み。共通 product code の変更無し、zedBSD 回帰対象無し。

証拠: `build/ws105-p001/`（build.log、start.log、verify.log、devices.txt、user.txt、image-before/after.txt、received.txt）。永続 PNG・版・試験 summary は `plan/history/ws105/q523/`。gdm variant の実行は p009、compositor / app の動作は後続 Phase。console / serial log は読んでいない。


実装 commit `39a0941c272ae3da5091efcba14907610ba5683b`。Finished UTC: 2026-10-01T06:04:15.180248+00:00。次の Phase は WS105 の既存完了までの自走指示で選定。push / GitHub 公開は未実施。
