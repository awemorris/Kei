<!-- awesome-plan project=zedbsd record=ws118-p005 -->
# ws118-p005: i915 を起動の後に手で初期化して 5320 の LCD を debug する

Status: in-progress（q634-i01、P4）
Disposition: normal
Parent: [WS118](../ws.md)
Focused goal: fg019（ベータ1）
Queue: q634
依存: p001 の image の道具（未実施の残りは q634 で仕上げる）。実機の起動・USB への書き込み・目視はユーザー
目安: 4〜6h ＋ ユーザーの立会い

## 由来

2026-10-03 user:「実機テストを加速したいので、11th gen Core i5のマシンで、i915のLCDが点灯しない問題を解決します。P4を立ててこれを割り当てます。sshが有効なディスクイメージを作成し、起動時にはi915を初期化しないことにします。その上で、SSHで接続できたあとで、i915を初期化し、デバッグメッセージを得ることで、LCDの初期化をデバッグします。これが完了すれば、利用できるi915実機が2台になり、1台が使用中でも私が実機確認できます。」

## 範囲

1. **変種 D の image**: p001 の A・B・C に並べて、i915 を kernel に入れたまま起動の時には bring-up しない image を作る（sshd、USB の LAN の DHCP、root の鍵、自動の login 無し、firmware の framebuffer）。
2. **起動の後の手での初期化**: 起動の option（例: `i915=defer`）で i915 の deferred start（`src/drivers/gpu/i915/i915.c` の attach が登録する遅延の bring-up）を止め、SSH の中から root が明示に始める手段を足す（sysctl・device の node・command のどれかを P4 が設計して Q1 に返す）。既定の起動の挙動は変えない。
3. **debug の message**: LCD の初期化の経路（VBT/OpRegion の panel の情報、eDP の AUX・DPCD・link training、panel の power sequence・backlight、PLL・DDI・pipe・plane、hotplug）の詳しい log を、option（例: `i915.debug=`）で出し、`dmesg` と SSH で取れるようにする。
4. **QEMU での確認**: D が起動して SSH で入れ、手での初期化の経路が i915 の無い QEMU でも安全に失敗する（panic しない）。5330 の passthrough は使わない（iGPU を使う i915 の作業の規則に従うなら Q1 に確かめる）。
5. **実機（ユーザーと）**: ユーザーが 5320 で D を起動 → SSH で入り → 手で初期化 → log を取り、LCD が点かない原因を分類して直す（修正の規模が大きければ p003 に分ける）。5330 の表示を壊さないことを確かめる（5330 の確認はユーザーと時期を合わせる）。

## 受け入れ

- D の build（warning 0）、boot-test、QEMU の SSH の login と手での初期化の安全な失敗。
- 実機: 5320 で SSH から i915 を初期化し、LCD の初期化の log を取れる（実機の証拠、ユーザーの立会い）。原因の分類と、修正で 5320 の LCD が点く、または残りの原因と次の Phase。
- 既定（option 無し）の起動で i915 の挙動が変わらない（5330 で回帰しない）。

## 規則の注意

- HAL の API（`include/hal/hal.h`）は変えない。変えるなら差分を plan に置いて Q1 経由でユーザーの承認。
- 実機の操作の前に、Q1 経由でユーザーに時期を確かめる。QEMU の証拠と実機の証拠を分ける。
