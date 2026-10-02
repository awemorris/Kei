<!-- awesome-plan project=zedbsd record=q578 -->
# Queue q578 / finished

# P9 Queue q578
Status: finished
Attempt: q578-i01 / cleared
Owner: Q1 main（canonical記録） / P9 generation 1（isolated executor）
Approval: current user / 2026-10-02 chat「では、N=3でしばらく実行を続けてください」、直前の専任3枠と最初の候補に基づく。
Started UTC: 2026-10-02T04:50:43.686853+00:00
Timebox: 最大3時間 / 1Phase
Phase: [phase](../../ws099/phase014/phase.md)
Snapshot: [q578-approved-phase.md](q578-approved-phase.md) / SHA256 `6b7a2e619fe882e5e9a3feee6dc6445206c81680cbd800374fe958a6ee6008d8`
Exact scope: C10 hardware試験script、3分試走、60分soak。host占有は所有lock確認後。compositor修正なし。
Dependencies: approved Phaseにある実出力を開始時に検証。未検証依存は実行せずmainへ返す。
Worktree: /home/awe/zedBSD-worktrees/p9
Branch: codex/p9
Checks/criteria: snapshotのwhole-Phase基準を保持。部分commit/Queue結果とPhase clearanceを区別。
Ordered next Queues: 未投入。mainが結果/依存確認後に明示dispatch。
Merge requests / ACK: none
Outcome: cleared。実3602秒/278周/errors0/restarts0、main画面・receipt・cleanup確認。
Sync: local-only records pending publication（configured github、公開保留）。push禁止、全commit -m WIP。

Preflight: fixture host solaris10-man（chaos）SSH可、他QEMU/owner/lock無し、GPU既にvfio-pci。mainが所有lockを取得した専用fixtureの使用を許可。rebind/reboot/他VM停止はしない。source freshnessを確認してimageを選ぶ。

MR P9-q578-01: requested4bdd9224f/base41aac4fc7。main review: sh-n/diff-check PASS、exact owner/QEMU PID cleanup、有限elapsed、fresh receipt、error/restart判定を確認。試験script統合checkpointのみ、短試走/60分/10窓/実open-closeの画面確認は未実施。古いimageで現行clearanceを主張しない。

MR P9-q578-02: requested21042cf4ebc25cb529f30b3ccb51959a45fe3962、ACK済4bdd9224f以降、script atomic checkpoint/5round進捗追加のみ。sh-n/diff-check/main review PASS。旧short exec session外部中断143、owner/PID照合してfixture/log救出し正常返却。旧shortはPASSなし、fresh fixtureをsetsid nohupで継続。

MR P9-q578-03: requested992b6de86、ACK済21042cf4e以後のPhase checkpoint/旧short3PNG/disk events。main provenance/中断の限界/ownercleanup/PNG証拠の範囲/diff-check reviewPASS。旧short結果でclearanceなし、現行freshshort進行中。

MR P9-q578-04: requested7d82da63a、lastACK992b6de86。freshshort14round/187秒/errors0/restarts0/exitstatus0、session receiptとlive Terminal/実open-close PNGの耐久証拠をmain review。whole60minは継続中、short結果でclearanceなし。script source変更なし、diff-check PASS。

2026-10-02 / q578-terminal: P9 model usage limitで停止後、mainが最終判定を引継ぎ。whole-Phase cleared、[result](../../ws099/phase014/q578-result.md)。60分criteriaを満たす。次Queueは未投入。GitHub close/comment未公開。

## Approved snapshot

<!-- awesome-plan project=zedbsd record=ws099-p014 -->

# ws099-p014: C10 の 5330 の 1 時間（L3）

Status: in-progress
Disposition: normal
Parent: [WS099](../ws.md)
Queue: q578 / q578-i01 / P9
依存: p001（C10 の QEMU の試験 `c10-soak.sh`）、p002（`c5-hw.sh` の passthrough の形）、WS075 の `hdmi-h4-hw.sh`

## 範囲と受け入れ

- C10 の実機: 5330 の i915 の passthrough で、窓 10 個を開いた session の上で 60 分、窓の開閉・移動・Wiseview・App Home の開閉を繰り返し、
  zdesktop が落ちず（sessiond が greeter に戻らず）、session の log の `ZWL ERROR`・`ZWL FAILED` が 0。
- 新しい試験 `plan/ws099/tests/c10-hw.sh` を作る（試験の script だけ。compositor の source は変えない）。
- 範囲外: 落ちた時の直し（別の Phase。原因が i915 なら WS075）。

## 手順（2026-10-01 追記）

1. `plan/ws099/tests/c10-hw.sh IMAGE OUTDIR [MINUTES]` を `c5-hw.sh` を元に書く（`c5-hw.sh:15-57` の形をそのまま使う）:
   - `H4_MINUTES=$((MINUTES + 20)) plan/ws075/tests/hdmi-h4-hw.sh start IMAGE OUTDIR`、`sleep 75`。
   - App Home の 10 個の app を開く（`c5-hw.sh:27-30` の tiles の loop）。`ctl shot opened`。
   - MINUTES（既定 60）の間、1 周: `ctl hmp "sendkey meta_l-tab"`・2 秒・`ctl hmp "sendkey esc"`・2 秒（Wiseview）、
     `ctl pointer move 22 16 sleep 100 down up sleep 2000`・`ctl hmp "sendkey esc"`・2 秒（App Home）、
     窓の title bar の drag（`ctl pointer move 960 300 sleep 100 down sleep 100 move 1060 340 sleep 300 up sleep 500` と戻し）。
     20 周ごとに `ctl shot round-N`。周の数を数える。
   - 終わりに `c5-hw.sh:49-57` と同じく Terminal を開いて `cp /run/user/1000/session.log /home/kei/c10-hw.log; sync`、
     `h4-ctl.py quit`、`ufs-cat.py` で `/home/kei/c10-hw.log` と `/var/log/sessiond.log` を読み、`hdmi-h4-hw.sh stop OUTDIR`。
   - 判定: session.log の `ZWL ERROR|ZWL FAILED` の数 0、sessiond.log に `SESSIOND GREETER` の起こし直し（`SESSIOND GREETER retry|failed`）が 0、
     最後の shot `saved-live.png` に Terminal がある（session が生きている）。`C10-HW RESULT rounds=N minutes=M errors=E restarts=R` と `c10-hw: PASS|FAIL`。
   - 中で zdesktop が落ちて greeter に戻ると Terminal の手順が greeter に打たれる。その場合は session.log が残らないので、sessiond.log の
     `SESSIOND GREETER` の行（起こし直し）で FAIL にする。
2. 構文の確かめ: `sh -n plan/ws099/tests/c10-hw.sh`。まず `MINUTES=3` で短く 1 回流して道具を確かめる。
3. image（passthrough）を作る:

```
mkdir -p build/ws099-p014
sh plan/ws075/demo/build-demo-image.sh build/ws099-p014-pt passthrough > build/ws099-p014/pt-build.log 2>&1; echo "exit=$?"
```

4. 本番:

```
sh plan/ws099/tests/c10-hw.sh build/ws099-p014-pt/hdd-image.img build/ws099-p014/c10-hw-short 3
sh plan/ws099/tests/c10-hw.sh build/ws099-p014-pt/hdd-image.img build/ws099-p014/c10-hw 60
tail -3 build/ws099-p014/c10-hw/c10-hw.out 2>/dev/null
```

- `c10-hw.sh` の出力は `| tee OUTDIR/c10-hw.out` で残す形にする。lock（`/tmp/i915-hw.lock`）は `start` が待って取る。約 80 分 lock を持つので、
  他の WS の passthrough の予定（WS075・WS094・WS102）を main に確かめてから。

## 完了の条件

- `c10-hw.sh … 60` が `c10-hw: PASS`（rounds ≥ 300 の目安、errors=0、restarts=0）。画面 `shots/round-*-live.png` と `saved-live.png` を残す。
- ws.md の段の表の L3 の C10 に値を記録。素の 5330（USB）での 1 時間は範囲外（passthrough で代える。ws.md に「passthrough で確かめた」と明記）。
- FAIL なら uncleared。落ちた周、session.log の最後の `ZWL` の行、`SESSIOND GREETER failed reason=` を記録し、直しの Phase を main に提案する。

2026-10-02 / n3-start-P9: current user「では、N=3でしばらく実行を続けてください」により最初の有限Queue q578を承認・開始。上限3時間、既存whole-Phase基準を保持。部分commitはclearanceではない。GitHub publication保留。
