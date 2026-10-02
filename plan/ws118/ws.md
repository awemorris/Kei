<!-- awesome-plan project=zedbsd record=ws118 -->

# WS118: Dell Latitude 5320 で Kei を動かす

<!-- awesome-plan-current:start -->
Status: planning
Primary Milestone: MG003
Related Milestones: MG006
Objectives: O2
Parent: [Master](../master.md)
Focused goal: fg019（ベータ1、2026-10-02 user: ネットワークの次）
Queue: none
Resume point: [p001](phase001/phase.md)（sshd を起動する遠隔の実機 log 用 image と手順、planned、実機は使わない）。p002 以降の実機の作業は Q1 がユーザーに時期を聞いてから。
2026-10-02 user:「使うネットワークの手段はUSB の LAN の RTL8156。これは明日の朝以降にやります。」→ 5320 は RTL8156 の USB の LAN で遠隔の log を取る。実機の作業は 2026-10-03 の朝以降、時期はユーザーに確かめる。
<!-- awesome-plan-current:end -->

## 目標（2026-10-02 ユーザー）

「対象 platformですが、Latitude 5330のほか、Latitude 5320でも動かしたいのですが、5320ではLCDの制御がうまくいっていないので、SSHDを起動してリモート実機でログを取れるようなイメージを作成して、ユーザと一緒に進めましょう。タイミングはユーザに聞いてください。」

- ベータ1 の対象 platform に Latitude 5320 を加える（5330 と並ぶ）。
- 5320 では内蔵 LCD の制御がうまくいっていない。画面に頼らず、起動時に sshd を立て、network から実機の log を取れる image を作る。
- 実機の操作はユーザーと一緒に行う。Q1 は実機の作業の前に時期をユーザーに確かめる。QEMU の証拠と実機の証拠を分けて書く。

## 既知の事実（2026-10-02、plan と source を読んで）

- 5320 は Tiger Lake（CPU・iGPU の正確な ID は未記録、ws003-p001 の inventory は uncleared で終了）。i915 は `INTEL_TGL_IDS` を attach する（`src/drivers/gpu/i915/i915.c:60`）が、
  i915 の開発と受け入れは 5330（ADL-P `8086:46a8`）で行われ、TGL の表示の経路は実機で試されていない。5320 には passthrough の環境が無い。
- 5320 の network: RJ45 は無い前提。USB の RTL8156（`0bda:8156`、CDC NCM）は 5320 の実機で DHCP・ping・外部の fetch が通った（[ws005-p001](../ws005/phase001/phase.md)、q029）。
  内蔵の無線の chip は未調査（Tiger Lake の機種は多くが AX201 の CNVi。zedBSD の Intel の driver は AX211 `8086:51f0` だけ）。USB の Archer T3U（RTL8822BU）は使える。
- sshd と root の鍵: `plan/ws075/demo/build-demo-image.sh` は openssh と `plan/tmp/guest/id_ed25519.pub` を root の `authorized_keys` に入れる。`build-demo-image.sh` は他の WS の道具なので呼ぶだけにする。
- `/etc/net.conf` の既定は lo0 だけ。`ue0` の DHCP を image に設定として入れる要否は p001 で確かめる。

## ベータ1 の到達目標と受け入れ（測れる形）

| # | 条件 | 証拠 |
| --- | --- | --- |
| T1 | 5320 を USB から単独で起動し、host（centris）から SSH で入って `dmesg` と i915 の診断を取れる。画面に頼らない | ユーザーの立会いの下で取った log（実機の証拠） |
| T2 | LCD の制御の失敗の原因を log で分類し、修正が 5330 の表示を壊さない（5330 の passthrough の smoke `plan/ws099/tests/c5-hw.sh` が PASS） | 修正の Phase の記録 |
| T3 | 5320 の内蔵 LCD で graphical login から Keiland のデスクトップが出て、keyboard・touchpad で App Home から app を開ける | ユーザーの目視（実機） |
| T4 | 5320 の network: USB の LAN の DHCP と `fetch`（ws005-p023 と共通）。無線は p002 の inventory で決める | ユーザーの報告 |

T3 がベータ1 に間に合わないとき（LCD の修正が大きいとき）の扱い（5320 を「HDMI の外部 display だけ」などの制限つきで載せるか、既知の問題とするか）はユーザーの判断。

## 完了の条件（案、p001 の後に具体化）

1. 5320 で sshd が起動し、host から SSH で kernel/driver の log を取れる image（手順を含む）。
2. LCD の制御の原因を実機の log で特定し、修正する（i915 の Phase は原因を見て分ける）。
3. 5320 の内蔵 LCD で graphical login から Keiland のデスクトップが出る（ユーザーの目視）。

## Phase

| Phase | 目的 | Status | 依存 | 目安 |
| --- | --- | --- | --- | --- |
| [p001](phase001/phase.md) | sshd を起動する遠隔の実機 log 用 image（i915 あり・i915 の log を画面に・i915 無しの 3 種）と手順書（USB の LAN の DHCP と固定 IP、host の鍵、log の回収、hang の時の USB の読み戻し）。QEMU で SSH まで確かめる | planned | なし（実機は使わない） | 2〜3h |
| [p002](phase002/phase.md) | ユーザーと実機で: 3 種の image を順に起動、inventory（PCI・USB・無線・panel・VBT/OpRegion）と i915 の log を取り、LCD の制御の失敗を分類する | planning | p001、ユーザーの時期、5320 の network の手段の確認 | 2h（立会い）＋解析 1h |
| [p003](phase003/phase.md) | p002 の分類に基づく i915 の修正（分類の後に分割する。範囲・依存は p002 の結果で書き直す） | planning | p002 | 未定（2〜4h ×n） |
| [p004](phase004/phase.md) | 5320 のベータ1 の受け入れ T3・T4（ユーザーと一緒に、WS129 の実機の確認と同じ日にまとめられる） | planning | p003、ユーザーの時期 | 1〜2h（立会い） |
