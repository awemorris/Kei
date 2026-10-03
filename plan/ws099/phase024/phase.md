<!-- awesome-plan project=zedbsd record=ws099-p024 -->
# ws099-p024: BUG-147 — C 基準の試験の app の起動の検出を負荷に強くする

Status: cleared（2026-10-03 Q1 の判定: 起動の検出の失敗を直し、強制の遅い起動でも PASS、C3/C4/C8/C9 ×5 で FAIL 0。cursor-owner の喪失は未再現で、診断を criteria.sh に入れ BUG-147 は tracking のまま。C2 ×3 の残り（ws099-p021）は WS131 の後）
Disposition: normal
Parent: [WS099](../ws.md)
Bug: [BUG-147](../../bugs/BUG-147.md)

## 範囲（Q1、2026-10-03。p020・p021 の「C9 ×5 FAIL 0」「C2 ×3 FAIL 0」を BUG-147 の間欠が止めている）

- 原因の見当（p021）: `ZWL GLASS launch` は App Home の click から 5 秒以内に map した時だけ出る（`home.c` の `zwl_home_launched`）。guest の app の起動が数秒止まる（BUG-135 と同じ系統）と、5 秒を越えて line が出ず、C2・p128 が止まる。cursor-owner の session の消失も調べる。
- 直し: 試験が launch した client の `ZWL MAP` から窓を特定する（BUG-146 と同じ考え、`plan/ws099/tests/` の共有の helper）、または launch の待ちと log の扱いを見直す（compositor の変更が要るなら最小に）。
- 確かめ: C3/C4/C8/C9 ×5 で FAIL 0、p020・p021 の受け入れの残りを満たす。boot-test。

## 結果（q625-i01、P2、2026-10-03 07:19〜）

worktree `/home/awe/zedBSD-worktrees/p2`（main `ab09ab533`）。QEMU の Venus。console・serial の log は読んでいない。

### 直し

1. **起動の検出（BUG-147 の C2・p128）**: compositor の変更を最小にした。App Home から launch した app の窓が 5 秒（`HOME_LAUNCH_WAIT_MS`）を超えて map されたら、これまでは何も log に出なかった。これを、30 秒（`HOME_LAUNCH_FORGET_MS`）までは launch の窓として `ZWL HOME launched-late waited_ms=…` と `ZWL GLASS launch-late surface=… to=X,Y size=WxH` を出し、grow の animation は付けないようにした（`home.c` の `zwl_home_launched` は 1=間に合った、2=遅れた、0=無し を返す。`shell.c` の `zwl_glass_mapped`）。animation の有無と、5 秒以内の `HOME launched`・`GLASS launch` の行は変えていない（p071 の grow の試験はそのまま）。C2（`c2-geometry.sh`）と p128 は `GLASS launch(-late)?` を受け入れる。BUG-146 の `find_window`（`ZWL MAP` の client から探す）は採らなかった。session の log には desktop の Files など他の client の MAP も入るうえ、C2 と p128 は launch の行の `to=…size=…` を窓の初めの幾何に使うため。
2. **files-desktop-guest.sh の手順の順**: `fresh_desktop`（~/Desktop を install の後の 5 項目に戻す）を作り、install・saved・prune の始めに呼ぶ。期待の配置と判定は変えていない。
3. **cursor-owner の session の消失**: 該当の回の runtime は上書きされていて、原因は調べられなかった（その後の 20 回以上で再現しない）。今後のために `criteria.sh` が FAIL の試験ごとに、QEMU がまだ動いているか・QEMU 自身の stderr（emulator の log。guest の console ではない）・session と試験の compositor の log を `OUT/<name>.failure.txt`・`.qemu.log`・`.session.log`・`.zdesktop.log` に残すようにした。

### 確かめ

- 遅い起動の再現: guest の `/bin/files` を「7 秒眠ってから exec」に包み（desktop の instance は除く）、1920×1280 で `c2-geometry.sh` を流した。`ZWL HOME launched-late waited_ms=9056`・`ZWL GLASS launch-late … size=1120x720` が出て、C2 は 14/14 PASS（[lines](evidence/c2-slow-files-9s-session-lines.txt)、[log](evidence/c2-slow-files-9s.log)）。直す前の試験は、この遅れで必ず launch の行を待って止まる（p021 の 1 回目と同じ形）。
- build: image `build/p2-p024-img` は exit 0。compiler の warning は外部の openssh だけ。`style-check.py`（`home.c`・`shell.c`）は違反 0。test の script は `sh -n` を通る。
- C2・C3・C4・C8・C9 を 3 回、C3・C4・C8・C9 を 2 回（[summary](evidence/acc/summary.txt)）: C3・C4・C8・C9 は 5 回とも FAIL 0（14〜15 本ずつ）。C2 は 3 回中 2 回 PASS（14/14）。2 回目は unmaximize の後の `geometry` で、log を読む SSH が空を返し、`set -u` の script が中断した（[log](evidence/acc/run2-c2.log)）。session の log には該当の行が 11 本あった。`criteria.sh` が新しく残す diagnostics では `qemu alive=yes`（[failure](evidence/acc/run2-c2.failure.txt)）。c2-geometry.sh にはまだ SSH の retry が無かったので、`guest-retry.sh` を入れた（`fa53a6a37`）。retry を入れた後の 3 回目は PASS。5 回の合計で、SSH の retry は 3 回起きて、全て回復した。
- `files-desktop-guest.sh install show input menu saved prune`（直した image）: PASS（[log](evidence/files-guest-sequence.log)）。p014 では saved と prune が FAIL していた。
- boot test: PASS（[PNG](evidence/boot-login.png)）。

### p020・p021 の受け入れの残り

- p020（C9 ×5 FAIL 0）: この 5 回で満たした（p076 と p128 を含む）。
- p021（C2 ×3 FAIL 0）: 3 回中 1 回 FAIL。原因は c2-geometry.sh に SSH の retry が無かったことで、retry を入れた後の C2 は 1 回しか流していない（PASS）。厳密には満たしていない。geometry の不一致と BUG-127 は再現していない。

### 判定の提案

BUG-147 の 2 つの形のうち、起動の検出（5 秒の窓）は compositor の `launch-late` の log と試験の受け入れで直した。遅い起動を作った再現でも PASS した。cursor-owner の session の消失は再現せず、次に起きたときの diagnostics を `criteria.sh` に足した。BUG-147 は resolved を提案する（cursor-owner の分は diagnostics を足したうえで tracking に残す案もある。判断は Q1）。p024 は **cleared を提案する**。根の guest の停止（SSH・app の起動）は BUG-135（P1 の q624）。
