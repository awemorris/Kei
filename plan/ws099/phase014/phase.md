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

## q578-i01 / P9 の途中結果（2026-10-02）

[証拠と再開記録](q578-checkpoint.md)。試験scriptを作成し、構文・差分whitespace・所有資源のcleanupと判定をreview。
既存C10の「窓の開閉」を確かめるため、元の10appを保ち、10周ごとに追加Terminalを開き`exit`で閉じる操作と前後PNGを追加。
毎周の回数・実経過秒・QEMU PIDをatomic checkpointへ保存する。compositor sourceは変更無し。

旧WS103イメージの道具確認は実行ツールの中断で未達。所有者・元のQEMU PIDを照合してログを救出し、自分のVMとlockを返却。
現行source `5ac9b753d` のfresh demo image（mainのbuild exit0/project warnings0、SHA256
`72003343313d85b5e0950f5659ca6af5a6183ef3c0776c68a1e7ecdfba0b3e7e`）をread-only copyで使用。
fresh 3分試走は **PASS**: 14周・実187秒、errors0/restarts0、disk receiptと実Terminal PNGを確認。
10appの維持・追加窓の実closeもPNGとdisk sessionで確認。lock/owner返却、vfio binding不変。

60分本番は独立session（PID203752、`build/ws099-p014/c10-full-fresh`）で実行中。全体Phaseのclearanceは未確定。
補助のframebuffer boot検証はmainが実施中、実機のPNG/受け入れと区別する。未実施をPASSにしない。
