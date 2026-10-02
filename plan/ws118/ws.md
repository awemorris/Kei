<!-- awesome-plan project=zedbsd record=ws118 -->

# WS118: Dell Latitude 5320 で Kei を動かす

<!-- awesome-plan-current:start -->
Status: planning
Primary Milestone: MG003
Related Milestones: MG006
Objectives: O2
Parent: [Master](../master.md)
Queue: none
Resume point: p001（sshd を起動する遠隔の実機ログ用 image）。実機の作業の時期はユーザーに聞く。
<!-- awesome-plan-current:end -->

## 目標（2026-10-02 ユーザー）

「対象 platformですが、Latitude 5330のほか、Latitude 5320でも動かしたいのですが、5320ではLCDの制御がうまくいっていないので、SSHDを起動してリモート実機でログを取れるようなイメージを作成して、ユーザと一緒に進めましょう。タイミングはユーザに聞いてください。」

- ベータ1 の対象 platform に Latitude 5320 を加える（5330 と並ぶ）。
- 5320 では内蔵 LCD の制御がうまくいっていない。画面に頼らず、起動時に sshd を立て、network から実機の log を取れる image を作る。
- 実機の操作はユーザーと一緒に行う。Q1 は実機の作業の前に時期をユーザーに確かめる。QEMU の証拠と実機の証拠を分けて書く。

## 完了の条件（案、p001 の後に具体化）

1. 5320 で sshd が起動し、host から SSH で kernel/driver の log を取れる image（手順を含む）。
2. LCD の制御の原因を実機の log で特定し、修正する（i915 の Phase は原因を見て分ける）。
3. 5320 の内蔵 LCD で graphical login から Keiland のデスクトップが出る（ユーザーの目視）。

## Phase

| Phase | 目的 | Status | 依存 |
| --- | --- | --- | --- |
| p001 | sshd を起動する遠隔の実機 log 用 image と手順（network の接続方法を含む） | planning | 5320 の network の手段（有線/USB の LAN）をユーザーと確認 |
| p002 | ユーザーと実機で log を取り、LCD の制御の失敗を分類 | planning | p001、ユーザーの時期 |
| p003〜 | 原因の修正（分類の後に分ける）と実機の確認 | planning | p002 |
